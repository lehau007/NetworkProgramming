# WebSocket Handler - Quick Start Guide

## 🚀 Overview

A complete WebSocket protocol implementation (RFC 6455) for your C++ chess server. This implementation handles the full WebSocket lifecycle including handshake, frame encoding/decoding, and control frames.

## 📁 Files Structure

```
server/
├── network/
│   ├── socket_handler.h          # TCP socket wrapper
│   ├── socket_handler.cpp
│   ├── websocket_handler.h       # WebSocket protocol implementation
│   └── websocket_handler.cpp
├── websocket_server_example.cpp  # Example server implementation
└── Makefile                      # Build configuration

client/
└── websocket_test.html          # HTML test client

docs/
└── websocket_implementation.md  # Detailed documentation
```

## ⚙️ Prerequisites

### Linux/WSL (Required for compilation)

```bash
# Install build tools
sudo apt-get update
sudo apt-get install build-essential

# Install OpenSSL development libraries
sudo apt-get install libssl-dev

# Install pthread (usually included)
```

## 🔨 Building

```bash
cd server

# Build WebSocket server
make websocket_server

# Or build everything
make all
```

## ▶️ Running

### Start the Server

```bash
# From server directory
./websocket_server

# Or using Makefile
make run_websocket
```

Expected output:
```
=== WebSocket Chess Server ===
Starting server on port 8080...
Server listening on port 8080
Server is ready! Waiting for WebSocket connections...
You can connect using: ws://localhost:8080
```

### Test with HTML Client

1. Open `client/websocket_test.html` in a web browser
2. Click "Connect" button
3. Send messages using the input field
4. Try quick messages or type custom JSON

### Test with Command Line

Using `wscat` (Node.js):
```bash
# Install wscat globally
npm install -g wscat

# Connect to server
wscat -c ws://localhost:8080

# Type messages
> Hello!
< Echo: Hello!
```

Using Python:
```python
pip install websockets

python3 << 'EOF'
import asyncio
import websockets

async def test():
    uri = "ws://localhost:8080"
    async with websockets.connect(uri) as ws:
        await ws.send("Hello Server!")
        response = await ws.recv()
        print(f"Received: {response}")

asyncio.run(test())
EOF
```

## 🎯 Usage Examples

### Basic Server Setup

```cpp
#include "network/socket_handler.h"
#include "network/websocket_handler.h"

int main() {
    // 1. Create and setup socket
    SocketHandler socket_handler(8080);
    socket_handler.initialize();
    socket_handler.bind_socket();
    socket_handler.start_listening();
    
    // 2. Accept client
    int client_socket = socket_handler.accept_connection();
    
    // 3. Create WebSocket handler
    WebSocketHandler ws_handler(client_socket);
    
    // 4. Perform handshake
    if (ws_handler.perform_handshake()) {
        // 5. Communication
        ws_handler.send_text("Welcome!");
        
        std::string message;
        while (ws_handler.receive_message(message)) {
            std::cout << "Received: " << message << std::endl;
            ws_handler.send_text("Echo: " + message);
        }
    }
    
    close(client_socket);
    return 0;
}
```

### Sending Messages

```cpp
// Send text
ws_handler.send_text("Hello, World!");

// Send JSON
std::string json = R"({"type":"GAME_UPDATE","state":"playing"})";
ws_handler.send_text(json);

// Send binary data
std::vector<uint8_t> data = {0x01, 0x02, 0x03};
ws_handler.send_binary(data);

// Send ping (keep-alive)
ws_handler.send_ping();

// Close connection
ws_handler.send_close(1000, "Goodbye");
```

### Receiving Messages

```cpp
std::string message;
if (ws_handler.receive_message(message)) {
    // Process text message
    std::cout << "Received: " << message << std::endl;
}

// Receive binary
std::vector<uint8_t> binary_data;
if (ws_handler.receive_binary(binary_data)) {
    // Process binary data
}
```

## 🔧 Integration with Chess Server

### Example: Handle Game Messages

```cpp
void handle_chess_client(int client_socket) {
    WebSocketHandler ws(client_socket);
    
    if (!ws.perform_handshake()) {
        return;
    }
    
    std::string message;
    while (ws.receive_message(message)) {
        // Parse JSON
        Json::Value data;
        Json::Reader reader;
        reader.parse(message, data);
        
        std::string type = data["type"].asString();
        
        if (type == "MOVE") {
            // Process chess move
            std::string from = data["from"].asString();
            std::string to = data["to"].asString();
            
            // Validate and apply move
            if (is_valid_move(from, to)) {
                apply_move(from, to);
                
                // Broadcast to both players
                Json::Value response;
                response["type"] = "MOVE_MADE";
                response["from"] = from;
                response["to"] = to;
                
                ws.send_text(Json::writeString(builder, response));
            }
        }
    }
}
```

## 🐛 Troubleshooting

### Build Errors

**Error:** `cannot find -lssl`
```bash
# Install OpenSSL
sudo apt-get install libssl-dev
```

**Error:** `sys/socket.h not found`
```bash
# You must compile on Linux/WSL, not Windows directly
# Use WSL on Windows or a Linux environment
```

### Runtime Errors

**Server won't start - Port already in use**
```bash
# Check what's using port 8080
netstat -tulpn | grep 8080

# Kill the process
kill -9 <PID>

# Or use a different port
./websocket_server 8081
```

**Connection refused**
```bash
# Check if server is running
ps aux | grep websocket_server

# Check firewall
sudo ufw allow 8080
```

**Browser can't connect**
- Make sure you're using `ws://` not `wss://`
- Check browser console for errors
- Verify server is listening: `netstat -an | grep 8080`

## 📊 Features

✅ **Complete WebSocket Protocol**
- HTTP upgrade handshake
- Frame encoding/decoding
- Payload masking/unmasking
- Fragmented message support

✅ **Control Frames**
- PING/PONG for keep-alive
- Graceful CLOSE handling
- Automatic control frame responses

✅ **Security**
- Max payload size limit (10MB)
- Proper masking validation
- Connection state management

✅ **Developer Friendly**
- Simple API
- Example implementations
- HTML test client
- Comprehensive documentation

## 📚 API Reference

### WebSocketHandler Class

#### Constructor
```cpp
WebSocketHandler(int socket)
```

#### Methods

**Handshake**
- `bool perform_handshake()` - Execute WebSocket handshake
- `bool is_connected()` - Check if connection is established

**Send Operations**
- `bool send_text(const string& message)` - Send text message
- `bool send_binary(const vector<uint8_t>& data)` - Send binary data
- `bool send_ping(const string& data = "")` - Send ping frame
- `bool send_pong(const vector<uint8_t>& data)` - Send pong frame
- `bool send_close(uint16_t code = 1000, const string& reason = "")` - Close connection

**Receive Operations**
- `bool receive_message(string& message)` - Receive text message
- `bool receive_binary(vector<uint8_t>& data)` - Receive binary data

## 🔐 WebSocket Close Codes

| Code | Description            |
|------|------------------------|
| 1000 | Normal Closure         |
| 1001 | Going Away             |
| 1002 | Protocol Error         |
| 1003 | Unsupported Data       |
| 1006 | Abnormal Closure       |
| 1007 | Invalid Payload        |
| 1008 | Policy Violation       |
| 1009 | Message Too Big        |
| 1011 | Internal Server Error  |

## 🚦 Next Steps

1. **Test the basic server**: Run the example and connect with the HTML client
2. **Integrate with chess logic**: Add game state management
3. **Add authentication**: Implement user login/session management
4. **Implement matchmaking**: Connect players together
5. **Add TLS/SSL**: Upgrade to secure WebSockets (WSS)

## 📖 Documentation

See `docs/websocket_implementation.md` for:
- Detailed protocol explanation
- Frame format details
- Advanced usage examples
- Security best practices
- Performance optimization

## 🤝 Support

For issues or questions:
1. Check the troubleshooting section above
2. Review `docs/websocket_implementation.md`
3. Check server logs for error messages
4. Use browser developer tools to inspect WebSocket traffic

---

**Version:** 1.0  
**Protocol:** RFC 6455 (WebSocket Protocol)  
**License:** See project LICENSE
