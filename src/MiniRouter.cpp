#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

#include <winhttp.h>

using namespace std;

// Configuration
const int ROUTER_PORT = 6300;
const string BACKEND_API_URL = "localhost";
const int BACKEND_API_PORT = 5500;

atomic<bool> shuttingDown(false);

// Tenant info cache
struct TenantInfo {
    string tenantId;
    string host;
    int port;
};

unordered_map<string, TenantInfo> apiKeyCache;
mutex cacheMutex;

// HTTP Client to call Backend API
string httpGet(const string& host, int port, const string& path) {
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    string response;

    try {
        // Initialize WinHTTP
        hSession = WinHttpOpen(
            L"MiniRedis Router/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
        );

        if (!hSession) {
            cerr << "[Router] WinHttpOpen failed\n";
            return "";
        }

        // Convert host to wide string
        wstring wHost(host.begin(), host.end());

        hConnect = WinHttpConnect(
            hSession,
            wHost.c_str(),
            port,
            0
        );

        if (!hConnect) {
            cerr << "[Router] WinHttpConnect failed\n";
            WinHttpCloseHandle(hSession);
            return "";
        }

        // Convert path to wide string
        wstring wPath(path.begin(), path.end());

        hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            wPath.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0
        );

        if (!hRequest) {
            cerr << "[Router] WinHttpOpenRequest failed\n";
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        // Send request
        BOOL bResults = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0
        );

        if (!bResults) {
            cerr << "[Router] WinHttpSendRequest failed\n";
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return "";
        }

        // Receive response
        bResults = WinHttpReceiveResponse(hRequest, NULL);

        if (bResults) {
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;

            do {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                    break;
                }

                char* pszOutBuffer = new char[dwSize + 1];
                ZeroMemory(pszOutBuffer, dwSize + 1);

                if (!WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                    delete[] pszOutBuffer;
                    break;
                }

                response.append(pszOutBuffer, dwDownloaded);
                delete[] pszOutBuffer;

            } while (dwSize > 0);
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

// Parse JSON response (simple parser for tenant_id)
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

// Verify API key via Backend API
bool verifyApiKey(const string& apiKey, TenantInfo& tenantInfo) {
    // Check cache first
    {
        lock_guard<mutex> lock(cacheMutex);
        auto it = apiKeyCache.find(apiKey);
        if (it != apiKeyCache.end()) {
            tenantInfo = it->second;
            cout << "[Router] Cache hit for API key\n";
            return true;
        }
    }

    // Call Backend API
    string path = "/api/verify?key=" + apiKey;
    cout << "[Router] Calling Backend API: GET " << path << "\n";

    string response = httpGet(BACKEND_API_URL, BACKEND_API_PORT, path);

    if (response.empty()) {
        cerr << "[Router] Backend API call failed\n";
        return false;
    }

    cout << "[Router] Backend response: " << response << "\n";

    // Parse response
    string tenantId = extractTenantId(response);
    if (tenantId.empty()) {
        cerr << "[Router] Invalid API key\n";
        return false;
    }

    // For now, map tenant_id to port (you should query this from Backend)
    // Simplified mapping: tenant1 -> 6379, tenant2 -> 6380, etc.
    // TODO: Add endpoint to Backend API to get tenant port by ID

    tenantInfo.tenantId = tenantId;
    tenantInfo.host = "127.0.0.1";

    // Get port from database (for now, hardcoded mapping)
    // You should add GET /api/tenants/{id} endpoint to Backend
    if (tenantId.find("0000") != string::npos) {
        tenantInfo.port = 6379;
    } else if (tenantId.find("0001") != string::npos) {
        tenantInfo.port = 6380;
    } else if (tenantId.find("0002") != string::npos) {
        tenantInfo.port = 6381;
    } else {
        tenantInfo.port = 6379; // default
    }

    // Cache the result
    {
        lock_guard<mutex> lock(cacheMutex);
        apiKeyCache[apiKey] = tenantInfo;
    }

    cout << "[Router] Authenticated tenant: " << tenantId << " -> port " << tenantInfo.port << "\n";
    return true;
}

// Forward client request to tenant node
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

    // Extract API key from first line (format: APIKEY <key>)
    istringstream iss(request);
    string cmd, apiKey;
    iss >> cmd >> apiKey;

    if (cmd != "APIKEY" || apiKey.empty()) {
        string error = "-ERR Authentication required. Send: APIKEY <your-key>\r\n";
        send(clientSock, error.c_str(), error.length(), 0);
        closesocket(clientSock);
        return;
    }

    // Verify API key
    TenantInfo tenantInfo;
    if (!verifyApiKey(apiKey, tenantInfo)) {
        string error = "-ERR Invalid API key\r\n";
        send(clientSock, error.c_str(), error.length(), 0);
        closesocket(clientSock);
        return;
    }

    // Connect to tenant node
    SOCKET tenantSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (tenantSock == INVALID_SOCKET) {
        string error = "-ERR Failed to connect to tenant node\r\n";
        send(clientSock, error.c_str(), error.length(), 0);
        closesocket(clientSock);
        return;
    }

    sockaddr_in tenantAddr;
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

    // Send success message
    string success = "+OK Authenticated. Connected to tenant: " + tenantInfo.tenantId + "\r\n";
    send(clientSock, success.c_str(), success.length(), 0);

    // Proxy traffic between client and tenant node
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

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[Router] WSAStartup failed\n";
        return 1;
    }

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        cerr << "[Router] socket() failed\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(ROUTER_PORT);

    if (::bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "[Router] bind() failed: " << WSAGetLastError() << "\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "[Router] listen() failed\n";
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    cout << "[Router] Listening on port " << ROUTER_PORT << "\n";
    cout << "[Router] Backend API at " << BACKEND_API_URL << ":" << BACKEND_API_PORT << "\n";
    cout << "[Router] Press Ctrl+C to stop\n\n";

    while (!shuttingDown.load()) {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

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
    WSACleanup();

    cout << "[Router] Shutdown complete\n";
    return 0;
}
