# Server Architecture - Detailed Guide

## Overview
This document provides detailed implementation guidance for the C++ chess server with focus on transport layer and network protocol.

---

## Network Layer Implementation

### 1. Socket Handler (`socket_handler.cpp/h`)

**Purpose**: Manage TCP socket operations and connection lifecycle

```cpp
// socket_handler.h
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
};

#endif
```

**Implementation:**
```cpp
// socket_handler.cpp
#include "socket_handler.h"
#include <unistd.h>
#include <cstring>
#include <iostream>

SocketHandler::SocketHandler(int port) : port(port), server_socket(-1) {}

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
    
    return client_socket;
}

ssize_t SocketHandler::send_data(int socket, const char* data, size_t length) {
    ssize_t total_sent = 0;
    while (total_sent < length) {
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
    }
}

void SocketHandler::shutdown_server() {
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
}
```

---

### 2. Message Handler (`message_handler.cpp/h`)

**Purpose**: Implement protocol - message framing, parsing, serialization

```cpp
// message_handler.h
#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <json/json.h>  // or your preferred JSON library

class MessageHandler {
public:
    // Send message with length prefix
    static bool send_message(int socket, const Json::Value& message);
    
    // Receive complete message (handles buffering)
    static bool receive_message(int socket, Json::Value& message);
    
    // Parse JSON from string
    static bool parse_json(const std::string& json_str, Json::Value& result);
    
    // Serialize JSON to string
    static std::string serialize_json(const Json::Value& json);
    
private:
    static bool send_length_prefix(int socket, uint32_t length);
    static bool receive_length_prefix(int socket, uint32_t& length);
    static bool receive_exact(int socket, char* buffer, size_t length);
};

#endif
```

**Implementation:**
```cpp
// message_handler.cpp
#include "message_handler.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>

bool MessageHandler::send_message(int socket, const Json::Value& message) {
    // Serialize JSON
    std::string json_str = serialize_json(message);
    uint32_t length = json_str.length();
    
    // Send length prefix (network byte order)
    if (!send_length_prefix(socket, length)) {
        return false;
    }
    
    // Send JSON payload
    ssize_t sent = send(socket, json_str.c_str(), length, 0);
    if (sent != length) {
        std::cerr << "Failed to send complete message" << std::endl;
        return false;
    }
    
    return true;
}

bool MessageHandler::receive_message(int socket, Json::Value& message) {
    // Receive length prefix
    uint32_t length;
    if (!receive_length_prefix(socket, length)) {
        return false;
    }
    
    // Validate reasonable length (prevent DoS)
    if (length > 1024 * 1024) {  // 1MB max
        std::cerr << "Message too large: " << length << std::endl;
        return false;
    }
    
    // Receive exact payload
    char* buffer = new char[length + 1];
    if (!receive_exact(socket, buffer, length)) {
        delete[] buffer;
        return false;
    }
    buffer[length] = '\0';
    
    // Parse JSON
    std::string json_str(buffer, length);
    delete[] buffer;
    
    return parse_json(json_str, message);
}

bool MessageHandler::send_length_prefix(int socket, uint32_t length) {
    uint32_t net_length = htonl(length);
    ssize_t sent = send(socket, &net_length, sizeof(net_length), 0);
    return sent == sizeof(net_length);
}

bool MessageHandler::receive_length_prefix(int socket, uint32_t& length) {
    uint32_t net_length;
    if (!receive_exact(socket, (char*)&net_length, sizeof(net_length))) {
        return false;
    }
    length = ntohl(net_length);
    return true;
}

bool MessageHandler::receive_exact(int socket, char* buffer, size_t length) {
    size_t received = 0;
    while (received < length) {
        ssize_t n = recv(socket, buffer + received, length - received, 0);
        if (n <= 0) {
            if (n == 0) {
                std::cerr << "Connection closed by peer" << std::endl;
            } else {
                std::cerr << "Receive error: " << strerror(errno) << std::endl;
            }
            return false;
        }
        received += n;
    }
    return true;
}

bool MessageHandler::parse_json(const std::string& json_str, Json::Value& result) {
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    
    std::string errors;
    bool success = reader->parse(
        json_str.c_str(),
        json_str.c_str() + json_str.length(),
        &result,
        &errors
    );
    
    delete reader;
    
    if (!success) {
        std::cerr << "JSON parse error: " << errors << std::endl;
    }
    
    return success;
}

std::string MessageHandler::serialize_json(const Json::Value& json) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // Compact output
    return Json::writeString(builder, json);
}
```

---

### 3. Client Handler (`client_handler.cpp/h`)

**Purpose**: Manage individual client connections in separate threads

```cpp
// client_handler.h
#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <pthread.h>
#include <json/json.h>

struct ClientContext {
    int socket;
    int user_id;
    std::string username;
    bool authenticated;
};

class ClientHandler {
public:
    static void* handle_client(void* arg);
    
private:
    static void process_message(ClientContext* ctx, const Json::Value& message);
    static void handle_login(ClientContext* ctx, const Json::Value& data);
    static void handle_register(ClientContext* ctx, const Json::Value& data);
    static void handle_get_players(ClientContext* ctx);
    static void handle_challenge(ClientContext* ctx, const Json::Value& data);
    static void handle_accept_challenge(ClientContext* ctx, const Json::Value& data);
    static void handle_decline_challenge(ClientContext* ctx, const Json::Value& data);
    static void handle_move(ClientContext* ctx, const Json::Value& data);
    static void handle_resign(ClientContext* ctx, const Json::Value& data);
    static void handle_draw_offer(ClientContext* ctx, const Json::Value& data);
    
    static void send_error(int socket, const std::string& message);
};

#endif
```

**Implementation Structure:**
```cpp
// client_handler.cpp
#include "client_handler.h"
#include "message_handler.h"
#include "session_manager.h"
#include "match_manager.h"
#include <iostream>

void* ClientHandler::handle_client(void* arg) {
    int client_socket = *(int*)arg;
    delete (int*)arg;
    
    ClientContext ctx;
    ctx.socket = client_socket;
    ctx.user_id = -1;
    ctx.authenticated = false;
    
    std::cout << "Client connected: " << client_socket << std::endl;
    
    // Message loop
    while (true) {
        Json::Value message;
        if (!MessageHandler::receive_message(client_socket, message)) {
            break;  // Connection closed or error
        }
        
        process_message(&ctx, message);
    }
    
    // Cleanup
    if (ctx.authenticated) {
        SessionManager::remove_session(ctx.user_id);
    }
    close(client_socket);
    
    std::cout << "Client disconnected: " << client_socket << std::endl;
    
    return nullptr;
}

void ClientHandler::process_message(ClientContext* ctx, const Json::Value& message) {
    std::string type = message["type"].asString();
    
    // Route to appropriate handler
    if (type == "LOGIN") {
        handle_login(ctx, message["data"]);
    } else if (type == "REGISTER") {
        handle_register(ctx, message["data"]);
    } else if (!ctx->authenticated) {
        send_error(ctx->socket, "Authentication required");
    } else if (type == "GET_AVAILABLE_PLAYERS") {
        handle_get_players(ctx);
    } else if (type == "CHALLENGE") {
        handle_challenge(ctx, message["data"]);
    } else if (type == "ACCEPT_CHALLENGE") {
        handle_accept_challenge(ctx, message["data"]);
    } else if (type == "DECLINE_CHALLENGE") {
        handle_decline_challenge(ctx, message["data"]);
    } else if (type == "MOVE") {
        handle_move(ctx, message["data"]);
    } else if (type == "RESIGN") {
        handle_resign(ctx, message["data"]);
    } else if (type == "DRAW_OFFER") {
        handle_draw_offer(ctx, message["data"]);
    } else {
        send_error(ctx->socket, "Unknown message type");
    }
}

void ClientHandler::send_error(int socket, const std::string& message) {
    Json::Value response;
    response["type"] = "ERROR";
    response["message"] = message;
    MessageHandler::send_message(socket, response);
}
```

---

## Session Management

### Session Manager (`session_manager.cpp/h`)

```cpp
// session_manager.h
#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <map>
#include <string>
#include <pthread.h>

struct Session {
    int socket_fd;
    int user_id;
    std::string username;
    time_t login_time;
    time_t last_activity;
    int current_game_id;
};

class SessionManager {
private:
    static std::map<int, Session*> sessions_by_socket;
    static std::map<int, Session*> sessions_by_user_id;
    static pthread_mutex_t mutex;
    
public:
    static void initialize();
    static bool create_session(int socket, int user_id, const std::string& username);
    static Session* get_session_by_socket(int socket);
    static Session* get_session_by_user_id(int user_id);
    static void update_activity(int socket);
    static void remove_session(int user_id);
    static void cleanup_inactive_sessions(int timeout_seconds);
};

#endif
```

---

## Game Management

### Match Manager (`match_manager.cpp/h`)

```cpp
// match_manager.h
#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

#include "chess_game.h"
#include <map>
#include <pthread.h>

struct GameInstance {
    int game_id;
    int white_player_id;
    int black_player_id;
    ChessGame* chess_engine;
    std::vector<std::string> move_history;
    time_t start_time;
    bool is_active;
};

class MatchManager {
private:
    static std::map<int, GameInstance*> active_games;
    static pthread_mutex_t mutex;
    static int next_game_id;
    
public:
    static void initialize();
    static int create_game(int white_id, int black_id);
    static GameInstance* get_game(int game_id);
    static bool make_move(int game_id, int player_id, const std::string& move);
    static void end_game(int game_id, const std::string& reason);
    static void cleanup_game(int game_id);
};

#endif
```

---

## Development Workflow

### Step-by-Step Implementation Order

**Week 1: Foundation**
1. Create project structure
2. Implement SocketHandler
3. Implement MessageHandler
4. Test with simple echo server
5. Create basic client for testing

**Week 2: Authentication**
1. Set up database connection
2. Implement SessionManager
3. Implement authentication handlers
4. Test login/register flow

**Week 3: Matchmaking**
1. Implement player list management
2. Implement challenge system
3. Create MatchManager skeleton
4. Test challenge flow

**Week 4: Chess Engine**
1. Integrate ChessGame
2. Implement move handlers
3. Test complete game flow
4. Add game result detection

**Week 5: Polish**
1. Add logging
2. Implement draw/resign
3. Add game history
4. Error handling improvements

---

## Testing Guide

### Unit Test Example (Message Handler)

```cpp
// test_message_handler.cpp
#include "message_handler.h"
#include <cassert>
#include <iostream>

void test_json_serialization() {
    Json::Value message;
    message["type"] = "TEST";
    message["data"]["value"] = 123;
    
    std::string json = MessageHandler::serialize_json(message);
    Json::Value parsed;
    assert(MessageHandler::parse_json(json, parsed));
    assert(parsed["type"].asString() == "TEST");
    assert(parsed["data"]["value"].asInt() == 123);
    
    std::cout << "JSON serialization test: PASSED" << std::endl;
}

int main() {
    test_json_serialization();
    return 0;
}
```

### Integration Test (Client-Server)

```bash
# Terminal 1: Start server
./chess_server

# Terminal 2: Test with netcat
echo '{"type":"LOGIN","username":"test","password":"test"}' | nc localhost 8080
```

---

## Debugging Tips

1. **Use extensive logging**
   ```cpp
   std::cout << "[DEBUG] Processing message type: " << type << std::endl;
   ```

2. **Thread debugging**
   ```cpp
   std::cout << "[Thread " << pthread_self() << "] Handling client" << std::endl;
   ```

3. **Socket debugging**
   ```bash
   # Check if port is listening
   netstat -an | grep 8080
   
   # Monitor connections
   watch -n 1 'netstat -an | grep 8080'
   ```

4. **Valgrind for memory leaks**
   ```bash
   valgrind --leak-check=full ./chess_server
   ```

---

**Document Version**: 1.0  
**Status**: Implementation Guide
