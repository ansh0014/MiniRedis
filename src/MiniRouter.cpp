// MiniRouter.cpp — Router with API Key auth + live-reload of apikeys.txt
// Winsock version. Compile with: g++ MiniRouter.cpp -o MiniRouter.exe -lws2_32 -pthread -O2 -s

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
#include <filesystem>

#pragma comment(lib, "Ws2_32.lib")
using namespace std;
namespace fs = std::filesystem;

struct Node { string host; int port; int slotStart; int slotEnd; };
vector<Node> cluster = {
    {"127.0.0.1", 6379, 0, 5460},
    {"127.0.0.1", 6380, 5461, 10922},
    {"127.0.0.1", 6381, 10923, 16383}
};


unordered_map<string,string> apiKeys; 
mutex apiKeysMutex;
string APIKEYS_FILE = "apikeys.txt";
chrono::system_clock::time_point apikeys_mtime = chrono::system_clock::from_time_t(0);


void reloadAPIKeysFromFile() {
    lock_guard<mutex> g(apiKeysMutex);
    ifstream f(APIKEYS_FILE);
    if (!f.is_open()) {
        cerr << "[Router] Warning: cannot open " << APIKEYS_FILE << "\n";
        apiKeys.clear();
        return;
    }
    unordered_map<string,string> tmp;
    string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        size_t pos = line.find(':');
        if (pos == string::npos) continue;
        string tenant = line.substr(0,pos);
        string key = line.substr(pos+1);
        // trim whitespace
        tenant.erase(0, tenant.find_first_not_of(" \t\r\n"));
        tenant.erase(tenant.find_last_not_of(" \t\r\n")+1);
        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n")+1);
        if (!tenant.empty() && !key.empty()) tmp[key] = tenant;
    }
    apiKeys.swap(tmp);
    cout << "[Router] Reloaded " << apiKeys.size() << " API keys from " << APIKEYS_FILE << "\n";
}


void watchAPIKeys() {
    try {
        if (!fs::exists(APIKEYS_FILE)) {
            // no file yet
            apikeys_mtime = chrono::system_clock::from_time_t(0);
        } else {
            apikeys_mtime = fs::last_write_time(APIKEYS_FILE);
            reloadAPIKeysFromFile();
        }
    } catch (...) {
        apikeys_mtime = chrono::system_clock::from_time_t(0);
    }

    while (true) {
        this_thread::sleep_for(chrono::seconds(5));
        try {
            if (!fs::exists(APIKEYS_FILE)) {
                // file removed
                bool need = false;
                {
                    lock_guard<mutex> g(apiKeysMutex);
                    need = !apiKeys.empty();
                }
                if (need) {
                    // clear map
                    {
                        lock_guard<mutex> g(apiKeysMutex);
                        apiKeys.clear();
                    }
                    cout << "[Router] apikeys.txt removed; cleared API keys\n";
                }
                apikeys_mtime = chrono::system_clock::from_time_t(0);
                continue;
            }
            auto cur = fs::last_write_time(APIKEYS_FILE);
            if (cur != apikeys_mtime) {
                apikeys_mtime = cur;
                reloadAPIKeysFromFile();
            }
        } catch (const std::exception &e) {
            cerr << "[Router] watchAPIKeys error: " << e.what() << "\n";
        } catch (...) {
            cerr << "[Router] watchAPIKeys unknown error\n";
        }
    }
}


static const unsigned short crc16tab[256] = {
 0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,
 0x8108,0x9129,0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF,
 0x1231,0x0210,0x3273,0x2252,0x52B5,0x4294,0x72F7,0x62D6,
 0x9339,0x8318,0xB37B,0xA35A,0xD3BD,0xC39C,0xF3FF,0xE3DE,
 0x2462,0x3443,0x0420,0x1401,0x64E6,0x74C7,0x44A4,0x5485,
 0xA56A,0xB54B,0x8528,0x9509,0xE5EE,0xF5CF,0xC5AC,0xD58D,
 0x3653,0x2672,0x1611,0x0630,0x76D7,0x66F6,0x5695,0x46B4,
 0xB75B,0xA77A,0x9719,0x8738,0xF7DF,0xE7FE,0xD79D,0xC7BC,
 0x48C4,0x58E5,0x6886,0x78A7,0x0840,0x1861,0x2802,0x3823,
 0xC9CC,0xD9ED,0xE98E,0xF9AF,0x8948,0x9969,0xA90A,0xB92B,
 0x5AF5,0x4AD4,0x7AB7,0x6A96,0x1A71,0x0A50,0x3A33,0x2A12,
 0xDBFD,0xCBDC,0xFBBF,0xEB9E,0x9B79,0x8B58,0xBB3B,0xAB1A,
 0x6CA6,0x7C87,0x4CE4,0x5CC5,0x2C22,0x3C03,0x0C60,0x1C41,
 0xEDAE,0xFD8F,0xCDEC,0xDDCD,0xAD2A,0xBD0B,0x8D68,0x9D49,
 0x7E97,0x6EB6,0x5ED5,0x4EF4,0x3E13,0x2E32,0x1E51,0x0E70,
 0xFF9F,0xEFBE,0xDFDD,0xCFFC,0xBF1B,0xAF3A,0x9F59,0x8F78,
 0x9188,0x81A9,0xB1CA,0xA1EB,0xD10C,0xC12D,0xF14E,0xE16F,
 0x1080,0x00A1,0x30C2,0x20E3,0x5004,0x4025,0x7046,0x6067,
 0x83B9,0x9398,0xA3FB,0xB3DA,0xC33D,0xD31C,0xE37F,0xF35E,
 0x02B1,0x1290,0x22F3,0x32D2,0x4235,0x5214,0x6277,0x7256,
 0xB5EA,0xA5CB,0x95A8,0x8589,0xF56E,0xE54F,0xD52C,0xC50D,
 0x34E2,0x24C3,0x14A0,0x0481,0x7466,0x6447,0x5424,0x4405,
 0xA7DB,0xB7FA,0x8799,0x97B8,0xE75F,0xF77E,0xC71D,0xD73C,
 0x26D3,0x36F2,0x0691,0x16B0,0x6657,0x7676,0x4615,0x5634,
 0xD94C,0xC96D,0xF90E,0xE92F,0x99C8,0x89E9,0xB98A,0xA9AB,
 0x5844,0x4865,0x7806,0x6827,0x18C0,0x08E1,0x3882,0x28A3,
 0xCB7D,0xDB5C,0xEB3F,0xFB1E,0x8BF9,0x9BD8,0xABBB,0xBB9A,
 0x4A75,0x5A54,0x6A37,0x7A16,0x0AF1,0x1AD0,0x2AB3,0x3A92,
 0xFD2E,0xED0F,0xDD6C,0xCD4D,0xBDAA,0xAD8B,0x9DE8,0x8DC9,
 0x7C26,0x6C07,0x5C64,0x4C45,0x3CA2,0x2C83,0x1CE0,0x0CC1,
 0xEF1F,0xFF3E,0xCF5D,0xDF7C,0xAF9B,0xBFBA,0x8FD9,0x9FF8,
 0x6E17,0x7E36,0x4E55,0x5E74,0x2E93,0x3EB2,0x0ED1,0x1EF0
};

unsigned short crc16(const unsigned char *buf, int len) {
    unsigned short crc = 0;
    for (int i=0;i<len;++i) crc = (unsigned short)((crc<<8) ^ crc16tab[((crc>>8)^buf[i]) & 0x00FF]);
    return crc;
}
int keySlot(const string &key) { return crc16((const unsigned char*)key.data(), (int)key.size()) % 16384; }


struct ForwardTask { SOCKET clientSock; string command; };
queue<ForwardTask> forwardQ;
mutex forwardMutex;
condition_variable forwardCv;
int FORWARD_WORKERS = 8;
atomic<bool> shuttingDown(false);
vector<queue<SOCKET>> connPools;
vector<mutex> poolLocks;
mutex clientWriteMutex;



SOCKET getConn(int idx) {
    {
        lock_guard<mutex> lk(poolLocks[idx]);
        if (!connPools[idx].empty()) { SOCKET s = connPools[idx].front(); connPools[idx].pop(); return s; }
    }
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;
    sockaddr_in serv{}; serv.sin_family = AF_INET;
    inet_pton(AF_INET, cluster[idx].host.c_str(), &serv.sin_addr);
    serv.sin_port = htons(cluster[idx].port);
    if (connect(s, (sockaddr*)&serv, sizeof(serv)) == SOCKET_ERROR) { closesocket(s); return INVALID_SOCKET; }
    return s;
}
void returnConn(int idx, SOCKET s) {
    lock_guard<mutex> lk(poolLocks[idx]); connPools[idx].push(s);
}
string forwardToNode(int idx, const string &cmd) {
    SOCKET s = getConn(idx);
    if (s == INVALID_SOCKET) return "-ERR cannot connect to node\r\n";
    send(s, cmd.c_str(), (int)cmd.size(), 0);
    char buf[4096]; int r = recv(s, buf, sizeof(buf)-1, 0);
    if (r <= 0) { closesocket(s); return "-ERR node no response\r\n"; }
    buf[r] = '\0'; string resp(buf);
    returnConn(idx, s);
    return resp;
}
int findNode(int slot) {
    for (int i=0;i<(int)cluster.size();++i) if (slot>=cluster[i].slotStart && slot<=cluster[i].slotEnd) return i;
    return 0;
}


void forwardWorker() {
    while (!shuttingDown.load()) {
        ForwardTask t;
        {
            unique_lock<mutex> lk(forwardMutex);
            forwardCv.wait(lk, []{ return !forwardQ.empty() || shuttingDown.load(); });
            if (shuttingDown.load()) break;
            t = forwardQ.front(); forwardQ.pop();
        }
        string cmd = t.command; istringstream iss(cmd); string command, key; iss >> command >> key;
        if (key.empty()) {
            lock_guard<mutex> lg(clientWriteMutex); string err = "-ERR no key given\r\n"; send(t.clientSock, err.c_str(), (int)err.size(), 0); continue;
        }
        int slot = keySlot(key); int nodeIdx = findNode(slot);
        cout << "[Router] Forwarding key="<<key<<" slot="<<slot<<" -> node:"<<cluster[nodeIdx].port<<"\n";
        string resp = forwardToNode(nodeIdx, cmd + "\r\n");
        lock_guard<mutex> lg(clientWriteMutex); send(t.clientSock, resp.c_str(), (int)resp.size(), 0);
    }
}


void clientHandler(SOCKET sock, string ip) {
    cout << "[Router] Client connected: " << ip << "\n";
    bool authenticated = false;
    char buf[4096];
    while (true) {
        int r = recv(sock, buf, sizeof(buf)-1, 0);
        if (r <= 0) { cout << "[Router] Client disconnected: " << ip << "\n"; closesocket(sock); break; }
        buf[r] = '\0'; string input(buf);
        size_t pos = 0;
        while (pos < input.size()) {
            size_t nl = input.find_first_of("\r\n", pos);
            string cmd = (nl==string::npos) ? input.substr(pos) : input.substr(pos, nl-pos);
            pos = (nl==string::npos) ? input.size() : nl+1;
            // trim
            cmd.erase(0, cmd.find_first_not_of(" \t\r\n")); if (cmd.empty()) continue;
            cmd.erase(cmd.find_last_not_of(" \t\r\n")+1);
            if (!authenticated) {
                string c1, c2; istringstream iss(cmd); iss >> c1 >> c2; transform(c1.begin(), c1.end(), c1.begin(), ::toupper);
                if (c1 != "AUTH") { string err = "-ERR must AUTH first\r\n"; send(sock, err.c_str(), (int)err.size(), 0); continue; }
                // validate key
                bool ok = false;
                { lock_guard<mutex> g(apiKeysMutex); if (apiKeys.count(c2)) ok = true; }
                if (ok) { authenticated = true; string okmsg = "+OK authenticated\r\n"; send(sock, okmsg.c_str(), (int)okmsg.size(), 0); }
                else { string err = "-ERR invalid API key\r\n"; send(sock, err.c_str(), (int)err.size(), 0); }
                continue;
            }
            // enqueue forward task
            { lock_guard<mutex> lg(forwardMutex); forwardQ.push({sock, cmd}); }
            forwardCv.notify_one();
        }
    }
}

int main(int argc, char *argv[]) {
    int PORT = 7000; if (argc>=2) PORT = stoi(argv[1]);
    cout << "[Router] starting on port " << PORT << "\n";
    // initialize connPools and locks
    connPools.resize(cluster.size()); poolLocks.resize(cluster.size());
    // start file watcher
    thread(watchAPIKeys).detach();
    // winsock startup
    WSADATA wsa; if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) { cerr<<"WSAStartup failed\n"; return 1; }
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) { cerr<<"socket failed\n"; WSACleanup(); return 1; }
    sockaddr_in serv{}; serv.sin_family = AF_INET; serv.sin_addr.s_addr = htonl(INADDR_ANY); serv.sin_port = htons((unsigned short)PORT);
    if (bind(listenSock, (sockaddr*)&serv, sizeof(serv)) == SOCKET_ERROR) { cerr<<"bind failed\n"; closesocket(listenSock); WSACleanup(); return 1; }
    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) { cerr<<"listen failed\n"; closesocket(listenSock); WSACleanup(); return 1; }
    // start forward workers
    vector<thread> workers; for (int i=0;i<FORWARD_WORKERS;++i) workers.emplace_back(forwardWorker);
    cout << "[Router] ready. waiting for clients...\n";
    while (true) {
        sockaddr_in caddr{}; int clen = sizeof(caddr);
        SOCKET s = accept(listenSock, (sockaddr*)&caddr, &clen);
        if (s == INVALID_SOCKET) { this_thread::sleep_for(chrono::milliseconds(100)); continue; }
        string ip = inet_ntoa(caddr.sin_addr);
        thread(clientHandler, s, ip).detach();
    }
    // cleanup (not reached)
    shuttingDown.store(true); forwardCv.notify_all();
    for (auto &t : workers) if (t.joinable()) t.join();
    closesocket(listenSock); WSACleanup();
    return 0;
}
