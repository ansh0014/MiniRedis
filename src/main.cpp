#define _WINSOCK_DEPRECATED_NO_WARNINGS

// Platform-specific headers
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <errno.h>
    #include <cstring>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    #define WSAGetLastError() errno
#endif

#include <iostream>
#include <string>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <chrono>
#include <atomic>

#include "TenantManager.h"

using namespace std;
using SteadyClock = chrono::steady_clock;
using TimePoint = chrono::time_point<SteadyClock>;

int NODE_PORT = 6379;
string NODE_ADDR = "127.0.0.1";
string TENANT_ID = "tenant1";
int WORKER_COUNT = 4;
int REQUEST_QUEUE_CAPACITY = 1024;

TenantManager tenantMgr;

struct ValueEntry
{
    string value;
    TimePoint expiry;
    size_t bytes;
    string tenantId;
};

unordered_map<string, ValueEntry> store;
mutex storeMutex;
const size_t ENTRY_OVERHEAD = 64;

struct ClientRequest
{
    SOCKET clientSock;
    string tenantId;
    string raw;
};

queue<ClientRequest> reqQueue;
mutex reqMutex;
condition_variable reqCv;
atomic<bool> shuttingDown(false);

struct ExpiryItem
{
    TimePoint expiry;
    string key;
    bool operator>(const ExpiryItem &o) const { return expiry > o.expiry; }
};
priority_queue<ExpiryItem, vector<ExpiryItem>, greater<ExpiryItem>> expiryHeap;
mutex expiryMutex;
condition_variable expiryCv;

string trim(const string &s)
{
    size_t a = s.find_first_not_of(" \r\n\t");
    if (a == string::npos)
        return "";
    size_t b = s.find_last_not_of(" \r\n\t");
    return s.substr(a, b - a + 1);
}

void sendStr(SOCKET s, const string &msg)
{
    const char *buf = msg.c_str();
    int len = (int)msg.size();
    int sent = 0;
    while (sent < len)
    {
        int r = send(s, buf + sent, len - sent, 0);
        if (r == SOCKET_ERROR || r == 0)
            break;
        sent += r;
    }
}

bool parseInt(const string &s, long long &out)
{
    try
    {
        size_t idx;
        long long v = stoll(s, &idx);
        if (idx != s.size())
            return false;
        out = v;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

size_t calcBytes(const string &k, const string &v)
{
    return k.size() + v.size() + ENTRY_OVERHEAD;
}

string handleSET(const string &tenantId, const string &key, const string &value,
                 const string &opt, const string &optVal)
{
    if (key.empty())
        return "-ERR wrong number of arguments for 'SET'\r\n";

    size_t newBytes = calcBytes(key, value);
    unique_lock<mutex> lk(storeMutex);

    auto it = store.find(key);
    size_t oldBytes = 0;

    if (it != store.end())
    {
        if (it->second.tenantId != tenantId)
        {
            return "-ERR key belongs to different tenant\r\n";
        }
        oldBytes = it->second.bytes;
    }

    long long delta = (long long)newBytes - (long long)oldBytes;

    if (delta > 0)
    {
        if (!tenantMgr.allocateMemory(tenantId, (size_t)delta))
        {
            return "-ERR tenant memory limit exceeded\r\n";
        }
    }
    else if (delta < 0)
    {
        tenantMgr.deallocateMemory(tenantId, (size_t)(-delta));
    }

    ValueEntry e;
    e.value = value;
    e.bytes = newBytes;
    e.tenantId = tenantId;
    e.expiry = TimePoint{};

    if (!opt.empty())
    {
        string OPT = opt;
        transform(OPT.begin(), OPT.end(), OPT.begin(), ::toupper);
        if (OPT == "EX")
        {
            long long sec;
            if (!parseInt(optVal, sec) || sec <= 0)
            {
                if (delta > 0)
                    tenantMgr.deallocateMemory(tenantId, (size_t)delta);
                return "-ERR invalid EX value\r\n";
            }
            e.expiry = SteadyClock::now() + chrono::seconds(sec);
        }
        else if (OPT == "PX")
        {
            long long ms;
            if (!parseInt(optVal, ms) || ms <= 0)
            {
                if (delta > 0)
                    tenantMgr.deallocateMemory(tenantId, (size_t)delta);
                return "-ERR invalid PX value\r\n";
            }
            e.expiry = SteadyClock::now() + chrono::milliseconds(ms);
        }
    }

    if (it != store.end())
    {
        it->second = move(e);
    }
    else
    {
        store.emplace(key, move(e));
    }

    // Schedule expiry
    if (store[key].expiry != TimePoint{})
    {
        ExpiryItem item{store[key].expiry, key};
        {
            lock_guard<mutex> lk2(expiryMutex);
            expiryHeap.push(move(item));
        }
        expiryCv.notify_one();
    }

    return "+OK\r\n";
}

string handleGET(const string &tenantId, const string &key)
{
    if (key.empty())
        return "-ERR wrong number of arguments for 'GET'\r\n";

    lock_guard<mutex> lk(storeMutex);
    auto it = store.find(key);
    if (it == store.end())
        return "$-1\r\n";

    if (it->second.tenantId != tenantId)
    {
        return "$-1\r\n";
    }

    if (it->second.expiry != TimePoint{} && SteadyClock::now() >= it->second.expiry)
    {
        size_t bytes = it->second.bytes;
        string tid = it->second.tenantId;
        store.erase(it);
        tenantMgr.deallocateMemory(tid, bytes);
        return "$-1\r\n";
    }

    const string &v = it->second.value;
    return "$" + to_string(v.size()) + "\r\n" + v + "\r\n";
}

string handleDEL(const string &tenantId, const string &key)
{
    if (key.empty())
        return "-ERR wrong number of arguments for 'DEL'\r\n";

    lock_guard<mutex> lk(storeMutex);
    auto it = store.find(key);
    if (it == store.end())
        return ":0\r\n";

    if (it->second.tenantId != tenantId)
    {
        return ":0\r\n";
    }

    size_t bytes = it->second.bytes;
    store.erase(it);
    tenantMgr.deallocateMemory(tenantId, bytes);
    return ":1\r\n";
}

string handleINFO(const string &tenantId)
{
    size_t keys = 0;
    size_t tenantMem = 0;
    {
        lock_guard<mutex> lk(storeMutex);
        for (const auto &pair : store)
        {
            if (pair.second.tenantId == tenantId)
            {
                keys++;
                tenantMem += pair.second.bytes;
            }
        }
    }

    TenantConfig cfg;
    string stats;
    if (tenantMgr.getTenantConfig(tenantId, cfg))
    {
        ostringstream oss;
        oss << "# MiniRedis Node - Tenant Info\r\n"
            << "tenant_id:" << tenantId << "\r\n"
            << "tenant_name:" << cfg.name << "\r\n"
            << "keys:" << keys << "\r\n"
            << "memory_used:" << tenantMem << "\r\n"
            << "memory_limit:" << cfg.memoryLimitBytes << "\r\n"
            << "memory_available:" << cfg.getAvailableMemory() << "\r\n"
            << "usage_percent:" << fixed << cfg.getUsagePercent() << "\r\n";
        stats = oss.str();
    }
    else
    {
        stats = "# MiniRedis Node\r\nkeys:" + to_string(keys) + "\r\nmemory:" + to_string(tenantMem) + "\r\n";
    }

    return "$" + to_string(stats.size()) + "\r\n" + stats + "\r\n";
}

string processCommand(const string &tenantId, const string &raw)
{
    string line = trim(raw);
    if (line.empty())
        return "";

    istringstream iss(line);
    string cmd;
    iss >> cmd;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "SET")
    {
        string key, value;
        if (!(iss >> key))
            return "-ERR wrong number of arguments for 'SET'\r\n";
        if (!(iss >> value))
            return "-ERR wrong number of arguments for 'SET'\r\n";

        string opt, optVal;
        if (iss >> opt)
        {
            string OPT = opt;
            transform(OPT.begin(), OPT.end(), OPT.begin(), ::toupper);
            if (OPT == "EX" || OPT == "PX")
            {
                if (!(iss >> optVal))
                    return "-ERR invalid syntax\r\n";
                opt = OPT;
            }
            else
            {
                opt.clear();
            }
        }
        return handleSET(tenantId, key, value, opt, optVal);
    }
    else if (cmd == "GET")
    {
        string key;
        iss >> key;
        return handleGET(tenantId, key);
    }
    else if (cmd == "DEL")
    {
        string key;
        iss >> key;
        return handleDEL(tenantId, key);
    }
    else if (cmd == "QUIT")
    {
        return "+BYE\r\n";
    }
    else if (cmd == "INFO")
    {
        return handleINFO(tenantId);
    }
    else
    {
        return "-ERR unknown command '" + cmd + "'\r\n";
    }
}

void workerLoop()
{
    while (!shuttingDown.load())
    {
        ClientRequest req;
        {
            unique_lock<mutex> lk(reqMutex);
            reqCv.wait(lk, []
                       { return !reqQueue.empty() || shuttingDown.load(); });
            if (shuttingDown.load() && reqQueue.empty())
                return;
            req = move(reqQueue.front());
            reqQueue.pop();
        }

        string resp = processCommand(req.tenantId, req.raw);
        if (!resp.empty())
            sendStr(req.clientSock, resp);
    }
}

void ttlSweeperLoop()
{
    while (!shuttingDown.load())
    {
        unique_lock<mutex> lk(expiryMutex);
        if (expiryHeap.empty())
        {
            expiryCv.wait_for(lk, chrono::seconds(1));
            continue;
        }

        ExpiryItem top = expiryHeap.top();
        TimePoint now = SteadyClock::now();

        if (now < top.expiry)
        {
            expiryCv.wait_until(lk, top.expiry);
            continue;
        }

        expiryHeap.pop();
        lk.unlock();

        lock_guard<mutex> lk2(storeMutex);
        auto it = store.find(top.key);
        if (it != store.end())
        {
            if (it->second.expiry != TimePoint{} && SteadyClock::now() >= it->second.expiry)
            {
                size_t bytes = it->second.bytes;
                string tid = it->second.tenantId;
                store.erase(it);
                tenantMgr.deallocateMemory(tid, bytes);
                cout << "[Node] Expired key: " << top.key << " (tenant: " << tid << ")\n";
            }
        }
    }
}

void acceptLoop(SOCKET listenSock)
{
    while (!shuttingDown.load())
    {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, (sockaddr *)&clientAddr, &clientLen);

        if (clientSock == INVALID_SOCKET)
        {
            int err = WSAGetLastError();
            if (shuttingDown.load())
                break;
            cerr << "[Accept] failed: " << err << "\n";
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ip, INET_ADDRSTRLEN);
        cout << "[Node] Client connected: " << ip << "\n";

        thread([clientSock, ip]()
               {
            char buf[4096];
            while (true) {
                int bytes = recv(clientSock, buf, (int)sizeof(buf) - 1, 0);
                if (bytes <= 0) {
                    cout << "[Node] Client disconnected: " << ip << "\n";
                    closesocket(clientSock);
                    break;
                }
                
                buf[bytes] = '\0';
                string in(buf);
                size_t pos = 0;
                
                while (pos < in.size()) {
                    size_t nl = in.find_first_of("\r\n", pos);
                    string line;
                    
                    if (nl == string::npos) {
                        line = in.substr(pos);
                        pos = in.size();
                    } else {
                        line = in.substr(pos, nl - pos);
                        size_t j = nl;
                        while (j < in.size() && (in[j] == '\r' || in[j] == '\n')) ++j;
                        pos = j;
                    }
                    
                    line = trim(line);
                    if (line.empty()) continue;
                    
                    // Parse tenant ID from command (format: "tenantId COMMAND args...")
                    istringstream iss(line);
                    string tenantId, restOfCommand;
                    iss >> tenantId;
                    getline(iss, restOfCommand);
                    restOfCommand = trim(restOfCommand);
                    
                    if (restOfCommand.empty()) {
                        sendStr(clientSock, "-ERR invalid command format\r\n");
                        continue;
                    }
                    
                    {
                        unique_lock<mutex> lk(reqMutex);
                        if ((int)reqQueue.size() >= REQUEST_QUEUE_CAPACITY) {
                            sendStr(clientSock, "-ERR server busy\r\n");
                            continue;
                        }
                        reqQueue.push(ClientRequest{clientSock, tenantId, restOfCommand});
                    }
                    reqCv.notify_one();
                }
            } })
            .detach();
    }
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        string a = argv[i];
        if (a == "--port" && i + 1 < argc)
            NODE_PORT = stoi(argv[++i]);
        else if (a == "--workers" && i + 1 < argc)
            WORKER_COUNT = stoi(argv[++i]);
        else if (a == "--queue" && i + 1 < argc)
            REQUEST_QUEUE_CAPACITY = stoi(argv[++i]);
        else if (a == "--addr" && i + 1 < argc)
            NODE_ADDR = argv[++i];
        else if (a == "--tenant" && i + 1 < argc)
            TENANT_ID = argv[++i];
    }

    cout << "=================================\n";
    cout << "  MiniRedis Storage Node v2.0\n";
    cout << "=================================\n";
    cout << "[Node] Port: " << NODE_PORT << "\n";
    cout << "[Node] Workers: " << WORKER_COUNT << "\n";
    cout << "[Node] Serving tenant: " << TENANT_ID << "\n\n";

    tenantMgr.addTenant(TENANT_ID, "Node Tenant", NODE_PORT);

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET)
    {
        cerr << "socket failed\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // Allow socket reuse
    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons((unsigned short)NODE_PORT);

    if (::bind(listenSock, (sockaddr *)&serv, sizeof(serv)) == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        cerr << "[Node] bind failed: " << err << "\n";
        closesocket(listenSock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR)
    {
        cerr << "listen failed\n";
        closesocket(listenSock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    vector<thread> workers;
    for (int i = 0; i < WORKER_COUNT; ++i)
        workers.emplace_back(workerLoop);

    thread(ttlSweeperLoop).detach();
    thread(acceptLoop, listenSock).detach();

    cout << "[Node] Running. Press Ctrl+C to stop.\n";
    while (true)
        this_thread::sleep_for(chrono::seconds(60));

    shuttingDown.store(true);
    reqCv.notify_all();
    expiryCv.notify_all();
    for (auto &t : workers)
        if (t.joinable())
            t.join();
    closesocket(listenSock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
