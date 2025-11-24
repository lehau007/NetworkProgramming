#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

using namespace std;

struct ClientThreadArgs {
    int client_socket;
};

void* handle_client_connection(void *arg) {
    ClientThreadArgs* args = (ClientThreadArgs*)arg;
    int client_socket = args->client_socket;
     
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_sock < 0) {
        cerr << "Failed to create socket: " << strerror(errno) << endl;
        close(server_sock);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // Stand for any incoming interface
    server_addr.sin_port = htons(8080);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Failed to bind socket: " << strerror(errno) << endl;
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 5) < 0) {
        cerr << "Failed to listen on socket: " << strerror(errno) << endl;
        close(server_sock);
        return 1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    for (int i = 0; i < 5; i++) {
        int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_addr_len);

        if (client_sock < 0) {
            cerr << "Failed to accept connection: " << strerror(errno) << endl;
            close(server_sock);
            return 1;
        }
        
        // Handle the accepted connection here (e.g., create a thread to handle client)

    }

    return 0;
}