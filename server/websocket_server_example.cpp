// Example WebSocket Server Implementation
// This demonstrates how to use the SocketHandler and WebSocketHandler classes

#include "network/socket_handler.h"
#include "network/websocket_handler.h"
#include <iostream>
#include <pthread.h>
#include <cstring>
#include <unistd.h>

// Struct type for passing parameters to client thread
struct ClientThreadArgs {
    int client_socket;
};

// Thread function to handle each WebSocket client
void* handle_websocket_client(void* arg) {
    ClientThreadArgs* args = (ClientThreadArgs*)arg;
    int client_socket = args->client_socket;
    delete args;
    
    std::cout << "Starting WebSocket handler for socket " << client_socket << std::endl;
    
    // Create WebSocket handler
    WebSocketHandler ws_handler(client_socket);
    
    // Perform WebSocket handshake
    if (!ws_handler.perform_handshake()) {
        std::cerr << "WebSocket handshake failed" << std::endl;
        close(client_socket);
        return nullptr;
    }
    
    std::cout << "WebSocket handshake successful!" << std::endl;
    
    // Send welcome message
    ws_handler.send_text("Welcome to Chess Server!");
    
    // Message loop
    while (ws_handler.is_connected()) {
        std::string message;
        
        if (ws_handler.receive_message(message)) {
            std::cout << "Received: " << message << std::endl;
            
            // Echo the message back
            std::string response = "Echo: " + message;
            ws_handler.send_text(response);
            
            // Check for quit command
            if (message == "quit" || message == "exit") {
                std::cout << "Client requested disconnect" << std::endl;
                ws_handler.send_close(1000, "Goodbye!");
                break;
            }
        } else {
            std::cout << "Client disconnected or error occurred" << std::endl;
            break;
        }
    }
    
    close(client_socket);
    std::cout << "WebSocket handler terminated for socket " << client_socket << std::endl;
    
    return nullptr;
}

int main() {
    std::cout << "=== WebSocket Chess Server ===" << std::endl;
    std::cout << "Starting server on port 8000..." << std::endl;
    
    // Create socket handler
    SocketHandler socket_handler(8000);
    
    // Initialize and bind socket
    if (!socket_handler.initialize()) {
        std::cerr << "Failed to initialize socket" << std::endl;
        return 1;
    }
    
    if (!socket_handler.bind_socket()) {
        std::cerr << "Failed to bind socket" << std::endl;
        return 1;
    }
    
    if (!socket_handler.start_listening()) {
        std::cerr << "Failed to start listening" << std::endl;
        return 1;
    }
    
    std::cout << "Server is ready! Waiting for WebSocket connections..." << std::endl;
    std::cout << "You can connect using: ws://localhost:8080" << std::endl;
    
    // Accept connections loop
    while (true) {
        int client_socket = socket_handler.accept_connection();
        
        if (client_socket < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }
        
        // Create new thread for client
        pthread_t thread_id;
        ClientThreadArgs* args = new ClientThreadArgs{client_socket};
        
        if (pthread_create(&thread_id, nullptr, handle_websocket_client, args) != 0) {
            std::cerr << "Failed to create thread for client" << std::endl;
            delete args;
            close(client_socket);
            continue;
        }
        
        // Detach thread so it cleans up automatically
        pthread_detach(thread_id);
    }
    
    return 0;
}
