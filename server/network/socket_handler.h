#ifndef SOCKET_HANDLER_H
#define SOCKET_HANDLER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <string>

class SocketHandler {
private:
    int server_socket;
    struct sockaddr_in server_addr;
    int port;
    
public:
    SocketHandler(int port = 8080);
    ~SocketHandler();
    
    bool initialize();
    bool bind_socket();
    bool start_listening(int backlog = SOMAXCONN);
    int accept_connection();
    void close_connection(int client_socket);
    void shutdown_server();
    
    // Send/Receive with proper error handling
    ssize_t send_data(int socket, const char* data, size_t length);
    ssize_t receive_data(int socket, char* buffer, size_t length);
    
    int get_server_socket() const { return server_socket; }
};

#endif // SOCKET_HANDLER_H
