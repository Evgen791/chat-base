#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>
#include <sqlite3.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

const int PORT = 2000;
const char* db_file = "clients.db";
sqlite3* db;

bool check_credentials(const char* username, const char* password) {
    const char* query = "SELECT * FROM clients WHERE username = ? AND password = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query" << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return true; // Клиент с таким логином и паролем существует
    }

    sqlite3_finalize(stmt);
    return false; // Клиент с таким логином и паролем не существует
}

void run_server() {
    // ... ваш код сервера без изменений
}

void run_client() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        return;
    }

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        return;
    }

    char buffer[1024];

    std::cout << "Enter username: ";
    std::cin.getline(buffer, sizeof(buffer));
    std::string username(buffer);

    std::cout << "Enter password: ";
    std::cin.getline(buffer, sizeof(buffer));
    std::string password(buffer);

    if (!check_credentials(username.c_str(), password.c_str())) {
        std::cout << "Invalid username or password" << std::endl;
        return;
    }

    while (true) {
        std::cout << "Enter message: ";
        std::cin.getline(buffer, sizeof(buffer));

        if (std::cin.eof()) {
            break;
        }

        send(sock_fd, buffer, strlen(buffer), 0);
        int recv_len = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (recv_len <= 0) break;
        buffer[recv_len] = '\0';
        std::cout << "Server: " << buffer << std::endl;
    }

#ifdef _WIN32
    closesocket(sock_fd);
#else
    close(sock_fd);
#endif
}

int main() {
#ifdef _WIN32
    WSAData wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }
#endif

    int rc = sqlite3_open(db_file, &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return 1;
    }

    std::thread server_thread(run_server);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::thread client_thread(run_client);

    server_thread.join();
    client_thread.join();

#ifdef _WIN32
    WSACleanup();
#endif

    sqlite3_close(db);

    return 0;
}
