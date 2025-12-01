#define _WINSOCK_DEPRECATED_NO_WARNINGS

// Platform-specific headers
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <winhttp.h>
    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "winhttp.lib")
    typedef int socklen_t;
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
    #include <errno.h>
    #include <curl/curl.h>
    #include <cstring>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    #define WSAGetLastError() errno
    #define SD_SEND SHUT_WR
#endif

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

using namespace std;

const int ROUTER_PORT = 6300;
const string BACKEND_API_HOST = "backend";
const int BACKEND_API_PORT = 5500;

atomic<bool> shuttingDown(false);

struct TenantInfo {
    string tenantId;
    string host;
    int port;
};

unordered_map<string, TenantInfo> apiKeyCache;
mutex cacheMutex;

// HTTP GET - Platform specific implementations
#ifdef _WIN32
string httpGet(const string& host, int port, const string& path) {
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    string response;

    try {
        hSession = WinHttpOpen(L"MiniRedis Router/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) return "";

        wstring wHost(host.begin(), host.end());
        hConnect = WinHttpConnect(hSession, wHost.c_str(), port, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return "";
        }

        wstring wPath(path.begin(), path.end());
        hRequest = WinHttpOpenRequest(hConnect, L"GET", wPath.c_str(), NULL,
                                     WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                              WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
            if (WinHttpReceiveResponse(hRequest, NULL)) {
                DWORD dwSize, dwDownloaded;
                do {
                    dwSize = 0;
                    if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
                    char* buffer = new char[dwSize + 1];
                    memset(buffer, 0, dwSize + 1);
                    if (WinHttpReadData(hRequest, buffer, dwSize, &dwDownloaded)) {
                        response.append(buffer, dwDownloaded);
                    }
                    delete[] buffer;
                } while (dwSize > 0);
            }
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
    } catch (...) {
        if (hRequest) WinHttpCloseHandle(hRequest);
        if (hConnect) WinHttpCloseHandle(hConnect);
        if (hSession) WinHttpCloseHandle(hSession);
    }

    return response;
}
#else
size_t WriteCallback(void* contents, size_t size, size_t nmemb, string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

string httpGet(const string& host, int port, const string& path) {
    CURL* curl = curl_easy_init();
    string response;

    if (curl) {
        string url = "http://" + host + ":" + to_string(port) + path;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            cerr << "[Router] curl failed: " << curl_easy_strerror(res) << "\n";
        }

        curl_easy_cleanup(curl);
    }

    return response;
}
#endif

string extractTenantId(const string& json) {
    size_t pos = json.find("\"tenant_id\"");
    if (pos == string::npos) return "";

    size_t colon = json.find(":", pos);
    if (colon == string::npos) return "";

    size_t start = json.find("\"", colon);
    if (start == string::npos) return "";
    start++;

    size_t end = json.find("\"", start);
    if (end == string::npos) return "";

    return json.substr(start, end - start);
}

bool verifyApiKey(const string& apiKey, TenantInfo& tenantInfo) {
    {
        lock_guard<mutex> lock(cacheMutex);
        auto it = apiKeyCache.find(apiKey);
        if (it != apiKeyCache.end()) {
            tenantInfo = it->second;
            cout << "[Router] Cache hit for API key\n";
            return true;
        }
    }

    string path = "/api/verify?key=" + apiKey;
    cout << "[Router] Calling Backend API: GET " << path << "\n";

    string response = httpGet(BACKEND_API_HOST, BACKEND_API_PORT, path);

    if (response.empty()) {
        cerr << "[Router] Backend API call failed\n";
        return false;
    }

    cout << "[Router] Backend response: " << response << "\n";

    string tenantId = extractTenantId(response);
    if (tenantId.empty()) {
        cerr << "[Router] Invalid API key\n";
        return false;
    }

    tenantInfo.tenantId = tenantId;
    tenantInfo.host = "redis-node1";
    
    // Map tenant to port (should query from backend in production)
    if (tenantId.find("0000") != string::npos) {
        tenantInfo.port = 6379;
    } else if (tenantId.find("0001") != string::npos) {
        tenantInfo.port = 6380;
    } else if (tenantId.find("0002") != string::npos) {
        tenantInfo.port = 6381;
    } else {
        tenantInfo.port = 6379;
    }

    {
        lock_guard<mutex> lock(cacheMutex);
        apiKeyCache[apiKey] = tenantInfo;
    }

    cout << "[Router] Authenticated tenant: " << tenantId << " -> port " << tenantInfo.port << "\n";
    return true;
}

void handleClient(SOCKET clientSock, const string& clientIp) {
    char buffer[4096];
    int bytesReceived = recv(clientSock, buffer, sizeof(buffer) - 1, 0);

    if (bytesReceived <= 0) {
        closesocket(clientSock);
        return;
    }

    buffer[bytesReceived] = '\0';
    string request(buffer);

    cout << "[Router] Received from " << clientIp << ": " << request.substr(0, 50) << "...\n";

    istringstream iss(request);
    string cmd, apiKey;
    iss >> cmd >> apiKey;

    if (cmd != "APIKEY" || apiKey.empty()) {
        string error = "-ERR Authentication required. Send: APIKEY <your-key>\r\n";
        send(clientSock, error.c_str(), error.length(), 0);
        closesocket(clientSock);
        return;
    }

    TenantInfo tenantInfo;
    if (!verifyApiKey(apiKey, tenantInfo)) {
        string error = "-ERR Invalid API key\r\n";
        send(clientSock, error.c_str(), error.length(), 0);
        closesocket(clientSock);
        return;
    }

    SOCKET tenantSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tenantSock == INVALID_SOCKET) {
        string error = "-ERR Failed to connect to tenant node\r\n";
        send(clientSock, error.c_str(), error.length(), 0);
        closesocket(clientSock);
        return;
    }

    sockaddr_in tenantAddr;
    memset(&tenantAddr, 0, sizeof(tenantAddr));
    tenantAddr.sin_family = AF_INET;
    tenantAddr.sin_port = htons(tenantInfo.port);
    inet_pton(AF_INET, tenantInfo.host.c_str(), &tenantAddr.sin_addr);

    if (connect(tenantSock, (sockaddr*)&tenantAddr, sizeof(tenantAddr)) == SOCKET_ERROR) {
        string error = "-ERR Tenant node unavailable\r\n";
        send(clientSock, error.c_str(), error.length(), 0);
        closesocket(tenantSock);
        closesocket(clientSock);
        return;
    }

    cout << "[Router] Connected to tenant node at " << tenantInfo.host << ":" << tenantInfo.port << "\n";

    string success = "+OK Authenticated. Connected to tenant: " + tenantInfo.tenantId + "\r\n";
    send(clientSock, success.c_str(), success.length(), 0);

    thread clientToTenant([clientSock, tenantSock]() {
        char buf[4096];
        int n;
        while ((n = recv(clientSock, buf, sizeof(buf), 0)) > 0) {
            send(tenantSock, buf, n, 0);
        }
        shutdown(tenantSock, SD_SEND);
    });

    thread tenantToClient([clientSock, tenantSock]() {
        char buf[4096];
        int n;
        while ((n = recv(tenantSock, buf, sizeof(buf), 0)) > 0) {
            send(clientSock, buf, n, 0);
        }
        shutdown(clientSock, SD_SEND);
    });

    clientToTenant.join();
    tenantToClient.join();

    closesocket(clientSock);
    closesocket(tenantSock);

    cout << "[Router] Client disconnected\n";
}

int main() {
    cout << "=================================\n";
    cout << "  MiniRedis Router v2.0\n";
    cout << "  Backend-Authenticated Routing\n";
    cout << "=================================\n\n";

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[Router] WSAStartup failed\n";
        return 1;
    }
#endif

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        cerr << "[Router] socket() failed\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(ROUTER_PORT);

    if (::bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "[Router] bind() failed: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "[Router] listen() failed\n";
        closesocket(listenSock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    cout << "[Router] Listening on port " << ROUTER_PORT << "\n";
    cout << "[Router] Backend API at " << BACKEND_API_HOST << ":" << BACKEND_API_PORT << "\n";
    cout << "[Router] Press Ctrl+C to stop\n\n";

    while (!shuttingDown.load()) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);

        SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock == INVALID_SOCKET) {
            if (!shuttingDown.load()) {
                cerr << "[Router] accept() failed\n";
            }
            continue;
        }

        char clientIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

        cout << "[Router] New connection from " << clientIp << "\n";

        thread(handleClient, clientSock, string(clientIp)).detach();
    }

    closesocket(listenSock);

#ifdef _WIN32
    WSACleanup();
#endif

    cout << "[Router] Shutdown complete\n";
    return 0;
}
