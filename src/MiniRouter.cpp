#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <condition_variable>
#include <algorithm>
#include <fstream>
#include <atomic>
#include <chrono>

#include "TenantManager.h"

#pragma comment(lib, "Ws2_32.lib")
using namespace std;

// Global tenant manager
TenantManager tenantMgr;

struct ForwardTask { 
    SOCKET clientSock; 
    string tenantId;
    string command; 
};

queue<ForwardTask> forwardQ;
mutex forwardMutex;
condition_variable forwardCv;
int FORWARD_WORKERS = 8;
atomic<bool> shuttingDown(false);

// Connection pools per port
unordered_map<int, queue<SOCKET>> connPools;
unordered_map<int, unique_ptr<mutex>> poolLocks;
mutex poolMapMutex;
mutex clientWriteMutex;

void sendStr(SOCKET s, const string &msg) {
    const char *buf = msg.c_str();
    int len = (int)msg.size();
    int sent = 0;
    while (sent < len) {
        int r = send(s, buf + sent, len - sent, 0);
        if (r == SOCKET_ERROR || r == 0) break;
        sent += r;
    }
}

SOCKET getConn(int port) {
    {
        lock_guard<mutex> gmap(poolMapMutex);
        if (poolLocks.find(port) == poolLocks.end()) {
            poolLocks[port] = make_unique<mutex>();
        }
    }
    
    {
        lock_guard<mutex> lk(*poolLocks[port]);
        if (!connPools[port].empty()) { 
            SOCKET s = connPools[port].front(); 
            connPools[port].pop(); 
            return s; 
        }
    }
    
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;
    
    sockaddr_in serv{}; 
    serv.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);
    serv.sin_port = htons(port);
    
    if (connect(s, (sockaddr*)&serv, sizeof(serv)) == SOCKET_ERROR) { 
        closesocket(s); 
        return INVALID_SOCKET; 
    }
    
    return s;
}

void returnConn(int port, SOCKET s) {
    lock_guard<mutex> gmap(poolMapMutex);
    if (poolLocks.find(port) == poolLocks.end()) {
        poolLocks[port] = make_unique<mutex>();
    }
    lock_guard<mutex> lk(*poolLocks[port]);
    connPools[port].push(s);
}

void forwardWorker() {
    while (!shuttingDown.load()) {
        ForwardTask task;
        {
            unique_lock<mutex> lk(forwardMutex);
            forwardCv.wait(lk, []{ return !forwardQ.empty() || shuttingDown.load(); });
            if (shuttingDown.load() && forwardQ.empty()) return;
            task = move(forwardQ.front()); 
            forwardQ.pop();
        }
        
        // Get node port for this tenant
        int nodePort = tenantMgr.getNodePort(task.tenantId);
        if (nodePort < 0) {
            string err = "-ERR tenant not configured\r\n";
            lock_guard<mutex> lk(clientWriteMutex);
            sendStr(task.clientSock, err);
            continue;
        }
        
        SOCKET nodeSock = getConn(nodePort);
        if (nodeSock == INVALID_SOCKET) {
            string err = "-ERR node unavailable\r\n";
            lock_guard<mutex> lk(clientWriteMutex);
            sendStr(task.clientSock, err);
            continue;
        }
        
        // Prefix command with tenant ID for node
        string cmdWithTenant = task.tenantId + " " + task.command + "\r\n";
        send(nodeSock, cmdWithTenant.c_str(), (int)cmdWithTenant.size(), 0);
        
        // Read response
        char buf[8192];
        string response;
        while (true) {
            int r = recv(nodeSock, buf, sizeof(buf)-1, 0);
            if (r <= 0) break;
            buf[r] = '\0';
            response += buf;
            
            // Check if complete RESP response received
            if (response.size() >= 2) {
                char type = response[0];
                if (type == '+' || type == '-' || type == ':') {
                    if (response.find("\r\n") != string::npos) break;
                } else if (type == '$') {
                    size_t firstCRLF = response.find("\r\n");
                    if (firstCRLF != string::npos) {
                        string lenStr = response.substr(1, firstCRLF - 1);
                        int len = stoi(lenStr);
                        if (len == -1) break;
                        if (response.size() >= firstCRLF + 2 + len + 2) break;
                    }
                }
            }
        }
        
        returnConn(nodePort, nodeSock);
        
        {
            lock_guard<mutex> lk(clientWriteMutex);
            sendStr(task.clientSock, response);
        }
    }
}

void clientHandler(SOCKET sock, string ip) {
    cout << "[Router] Client connected: " << ip << "\n";
    string currentTenant;
    bool authenticated = false;
    char buf[4096];
    string buffer;
    
    while (true) {
        int r = recv(sock, buf, sizeof(buf)-1, 0);
        if (r <= 0) { 
            cout << "[Router] Client disconnected: " << ip << "\n"; 
            closesocket(sock); 
            break; 
        }
        buf[r] = '\0'; 
        buffer += string(buf);
        
        size_t pos;
        while ((pos = buffer.find('\n')) != string::npos) {
            string line = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            
            if (line.empty()) continue;
            
            if (!authenticated) {
                istringstream iss(line);
                string cmd, apiKey;
                iss >> cmd >> apiKey;
                transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
                
                if (cmd != "AUTH") { 
                    string err = "-ERR must AUTH first\r\n"; 
                    sendStr(sock, err);
                    continue; 
                }
                
                currentTenant = tenantMgr.authenticate(apiKey);
                
                if (!currentTenant.empty()) { 
                    authenticated = true;
                    cout << "[Router] Client authenticated as tenant: " << currentTenant << "\n";
                    string okmsg = "+OK authenticated as " + currentTenant + "\r\n"; 
                    sendStr(sock, okmsg);
                } else { 
                    string err = "-ERR invalid API key\r\n"; 
                    sendStr(sock, err);
                }
                continue;
            }
            
            // Special commands
            istringstream iss(line);
            string cmd;
            iss >> cmd;
            transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
            
            if (cmd == "QUIT") {
                string bye = "+BYE\r\n";
                sendStr(sock, bye);
                closesocket(sock);
                cout << "[Router] Client " << ip << " quit\n";
                return;
            } else if (cmd == "STATS") {
                string stats = tenantMgr.getTenantStats(currentTenant);
                sendStr(sock, "$" + to_string(stats.size()) + "\r\n" + stats + "\r\n");
                continue;
            }
            
            // Enqueue forward task with tenant context
            { 
                lock_guard<mutex> lg(forwardMutex); 
                forwardQ.push({sock, currentTenant, line}); 
            }
            forwardCv.notify_one();
        }
    }
}

int main(int argc, char *argv[]) {
    int PORT = 7000; 
    if (argc >= 2) PORT = stoi(argv[1]);
    
    cout << "=================================\n";
    cout << "  MiniRedis Router v2.0\n";
    cout << "  Multi-Tenant Architecture\n";
    cout << "=================================\n\n";
    
    // Initialize tenant manager with default 40MB per tenant
    tenantMgr.addTenant("tenant1", "Customer A", 6379);
    tenantMgr.addTenant("tenant2", "Customer B", 6380);
    tenantMgr.addTenant("tenant3", "Customer C", 6381);
    
    if (!tenantMgr.loadAPIKeys()) {
        cerr << "[Router] WARNING: Could not load API keys, authentication disabled\n";
    }
    
    cout << "\n";
    
    // Winsock init
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { 
        cerr << "WSAStartup failed\n"; 
        return 1; 
    }
    
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) { 
        cerr << "socket() failed\n"; 
        WSACleanup(); 
        return 1; 
    }
    
    sockaddr_in serv{}; 
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(PORT);
    
    if (bind(listenSock, (sockaddr*)&serv, sizeof(serv)) == SOCKET_ERROR) {
        cerr << "bind() failed\n"; 
        closesocket(listenSock); 
        WSACleanup(); 
        return 1;
    }
    
    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "listen() failed\n"; 
        closesocket(listenSock); 
        WSACleanup(); 
        return 1;
    }
    
    cout << "[Router] Listening on port " << PORT << "\n";
    cout << "[Router] Waiting for connections...\n\n";
    
    // Start forward workers
    vector<thread> workers;
    for (int i = 0; i < FORWARD_WORKERS; ++i) 
        workers.emplace_back(forwardWorker);
    
    while (true) {
        sockaddr_in client{}; 
        int clientLen = sizeof(client);
        SOCKET clientSock = accept(listenSock, (sockaddr*)&client, &clientLen);
        if (clientSock == INVALID_SOCKET) { 
            cerr << "accept() failed\n"; 
            continue; 
        }
        string clientIP = inet_ntoa(client.sin_addr);
        thread(clientHandler, clientSock, clientIP).detach();
    }
    
    closesocket(listenSock);
    WSACleanup();
    return 0;
}
