#include "socket_handler.h"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <errno.h>

SocketHandler::SocketHandler(int port) : server_socket(-1), port(port) {}

SocketHandler::~SocketHandler() {
    shutdown_server();
}

bool SocketHandler::initialize() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        return false;
    }
    
    // Configure address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    return true;
}

bool SocketHandler::bind_socket() {
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool SocketHandler::start_listening(int backlog) {
    if (listen(server_socket, backlog) < 0) {
        std::cerr << "Failed to listen: " << strerror(errno) << std::endl;
        return false;
    }
    std::cout << "Server listening on port " << port << std::endl;
    return true;
}

int SocketHandler::accept_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
        std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
        return -1;
    }
    
    std::cout << "Client connected: socket " << client_socket << std::endl;
    return client_socket;
}

ssize_t SocketHandler::send_data(int socket, const char* data, size_t length) {
    ssize_t total_sent = 0;
    while (total_sent < static_cast<ssize_t>(length)) {
        ssize_t sent = send(socket, data + total_sent, length - total_sent, 0);
        if (sent < 0) {
            std::cerr << "Send failed: " << strerror(errno) << std::endl;
            return -1;
        }
        total_sent += sent;
    }
    return total_sent;
}

ssize_t SocketHandler::receive_data(int socket, char* buffer, size_t length) {
    ssize_t received = recv(socket, buffer, length, 0);
    if (received < 0) {
        std::cerr << "Receive failed: " << strerror(errno) << std::endl;
    }
    return received;
}

void SocketHandler::close_connection(int client_socket) {
    if (client_socket >= 0) {
        close(client_socket);
        std::cout << "Connection closed: socket " << client_socket << std::endl;
    }
}

void SocketHandler::shutdown_server() {
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
        std::cout << "Server shutdown complete" << std::endl;
    }
}