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

#include "session/session_manager.h"
#include "game/match_manager.h"
#include "network/websocket_handler.h"
#include "network/socket_handler.h"
#include "utils/message_handler.h"
#include "utils/message_types.h"

using namespace std;

struct ClientThreadArgs {
    int client_socket;
    string client_ip;
};

void* handle_client_connection(void *arg) {
    ClientThreadArgs* args = (ClientThreadArgs*)arg;
    int client_socket = args->client_socket;
    string client_ip = args->client_ip;
    delete args;

    cout << "[Server] New connection from " << client_ip << endl;

    // Create WebSocket handler
    WebSocketHandler ws_handler(client_socket);
    
    // Perform WebSocket handshake
    if (!ws_handler.perform_handshake()) {
        cerr << "[Server] WebSocket handshake failed for " << client_ip << endl;
        close(client_socket);
        return nullptr;
    }

    cout << "[Server] WebSocket connection established with " << client_ip << endl;
    
    // Create message handler for this client
    MessageHandler msg_handler(client_socket);
    
    // Main message loop
    string message;
    while (ws_handler.receive_message(message)) {
        if (message.empty()) {
            continue;
        }
        
        cout << "[Server] Received message: " << message.substr(0, 100) << "..." << endl;
        
        // Handle the message
        msg_handler.handle_message(message);
        
        // Update session activity
        SessionManager::get_instance()->update_activity_by_socket(client_socket);
    }
    
    cout << "[Server] Client " << client_ip << " disconnected" << endl;
    
    // Cleanup session on disconnect
    SessionManager::get_instance()->remove_session_by_socket(client_socket);
    
    close(client_socket);
    return nullptr;
}

// Session cleanup thread
void* session_cleanup_worker(void* arg) {
    SessionManager* session_mgr = SessionManager::get_instance();
    
    while (true) {
        sleep(60);  // Run every 60 seconds
        session_mgr->cleanup_expired_sessions();
    }
    
    return nullptr;
}

int main() {
    cout << "========================================" << endl;
    cout << "    Chess Server - Network Protocol    " << endl;
    cout << "========================================" << endl;
    cout << "Starting server on port 8080..." << endl;
    
    // Initialize managers
    SessionManager::get_instance();
    MatchManager::initialize();
    
    // Set up broadcast callback for MatchManager
    MatchManager::set_broadcast_callback([](int user_id, const json& message) {
        SessionManager* session_mgr = SessionManager::get_instance();
        Session* target_session = session_mgr->get_session_by_user_id(user_id);
        
        if (target_session && target_session->client_socket > 0) {
            WebSocketHandler ws(target_session->client_socket);
            std::string message_str = message.dump();
            ws.send_text(message_str);
        }
    });
    
    cout << "[Server] MatchManager initialized with broadcast callback" << endl;
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    if (server_sock < 0) {
        cerr << "[Error] Failed to create socket: " << strerror(errno) << endl;
        return 1;
    }

    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "[Error] Failed to set socket options: " << strerror(errno) << endl;
        close(server_sock);
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
    server_addr.sin_port = htons(8080);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "[Error] Failed to bind socket: " << strerror(errno) << endl;
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, SOMAXCONN) < 0) {
        cerr << "[Error] Failed to listen on socket: " << strerror(errno) << endl;
        close(server_sock);
        return 1;
    }

    cout << "[Server] Listening on 0.0.0.0:8080" << endl;
    cout << "[Server] Waiting for connections..." << endl;
    
    // Start session cleanup thread
    pthread_t cleanup_thread;
    pthread_create(&cleanup_thread, nullptr, session_cleanup_worker, nullptr);
    pthread_detach(cleanup_thread);
    
    cout << "[Server] Session cleanup thread started" << endl;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Accept connections indefinitely
    while (true) {
        int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_addr_len);

        if (client_sock < 0) {
            cerr << "[Error] Failed to accept connection: " << strerror(errno) << endl;
            continue;  // Continue accepting other connections
        }
        
        // Get client IP address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        
        cout << "[Server] Accepted connection from " << client_ip << endl;
        
        // Create thread to handle client
        pthread_t thread_id;
        ClientThreadArgs* args = new ClientThreadArgs{client_sock, string(client_ip)};
        
        if (pthread_create(&thread_id, nullptr, handle_client_connection, args) != 0) {
            cerr << "[Error] Failed to create thread: " << strerror(errno) << endl;
            delete args;
            close(client_sock);
            continue;
        }
        
        pthread_detach(thread_id);
        
        cout << "[Server] Active sessions: " << SessionManager::get_instance()->get_active_session_count() 
             << " | Active games: " << MatchManager::get_instance()->get_active_game_count() << endl;
    }

    close(server_sock);
    return 0;
}