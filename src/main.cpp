#include <iostream>

#include <ws2tcpip.h>
#include <winsock2.h>
#include <string>
#include <unordered_map>
#include <sstream>
#include <algorithm>
#include <thread>
#include <mutex>

using namespace std;
#pragma comment(lib, "Ws2_32.lib")


unordered_map<string, string> store;
mutex storeMutex;

string trim(const string &s) {
    size_t start = s.find_first_not_of(" \r\n\t");
    size_t end = s.find_last_not_of(" \r\n\t");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}


void handleClient(SOCKET clientSock, string clientIP) {
    cout << "Connected: " << clientIP << endl;
    char buf[1024];

    while (true) {
        int bytesReceived = recv(clientSock, buf, sizeof(buf) - 1, 0);
        if (bytesReceived <= 0) {
            cout << " Disconnected: " << clientIP << endl;
            closesocket(clientSock);
            return;
        }

        buf[bytesReceived] = '\0';
        string input(buf);

        
        string clean;
        for (char c : input)
            if (isprint((unsigned char)c) || isspace((unsigned char)c))
                clean += c;

        input = trim(clean);
        if (input.empty()) continue;

        istringstream iss(input);
        string command, key, value;
        iss >> command >> key >> value;
        transform(command.begin(), command.end(), command.begin(), ::toupper);

        string response;
        if (command == "SET") {
            if (key.empty() || value.empty()) {
                response = "-ERR wrong number of arguments\r\n";
            } else {
                lock_guard<mutex> lock(storeMutex);
                store[key] = value;
                response = "+OK\r\n";
            }
        } else if (command == "GET") {
            if (key.empty()) {
                response = "-ERR wrong number of arguments\r\n";
            } else {
                lock_guard<mutex> lock(storeMutex);
                if (store.find(key) != store.end()) {
                    const string &val = store[key];
                    response = "$" + to_string(val.size()) + "\r\n" + val + "\r\n";
                } else response = "$-1\r\n";
            }
        } else if (command == "DEL") {
            lock_guard<mutex> lock(storeMutex);
            int deleted = store.erase(key);
            response = ":" + to_string(deleted) + "\r\n";
        } else if (command == "QUIT") {
            response = "+BYE\r\n";
            send(clientSock, response.c_str(), (int)response.size(), 0);
            closesocket(clientSock);
            cout << "[x] Client exited: " << clientIP << endl;
            return;
        } else {
            response = "-ERR unknown command\r\n";
        }

        send(clientSock, response.c_str(), (int)response.size(), 0);
    }
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
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
    serv.sin_port = htons(6379);

    if (bind(listenSock, (sockaddr *)&serv, sizeof(serv)) == SOCKET_ERROR) {
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

    cout << "MiniRedis running on port 6379\n";

    while (true) {
        sockaddr_in client{};
        int clientLen = sizeof(client);
        SOCKET clientSock = accept(listenSock, (sockaddr *)&client, &clientLen);
        if (clientSock == INVALID_SOCKET) {
            cerr << "accept() failed\n";
            continue;
        }

        string clientIP = inet_ntoa(client.sin_addr);
        thread(handleClient, clientSock, clientIP).detach();  // concurrent client handler
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}

