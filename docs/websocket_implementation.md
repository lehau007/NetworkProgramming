# WebSocket Implementation Guide

## Overview
This WebSocket handler implementation follows the RFC 6455 WebSocket protocol specification.

## Files Created
- `network/websocket_handler.h` - WebSocket handler class definition
- `network/websocket_handler.cpp` - WebSocket protocol implementation
- `network/socket_handler.h` - Basic TCP socket wrapper
- `network/socket_handler.cpp` - Socket operations implementation
- `websocket_server_example.cpp` - Example server implementation

## Features

### WebSocket Protocol Support
- ✅ HTTP upgrade handshake
- ✅ Frame encoding/decoding
- ✅ Payload masking/unmasking (client-to-server)
- ✅ Support for text and binary messages
- ✅ Fragmented message reassembly
- ✅ Control frames (PING, PONG, CLOSE)
- ✅ Automatic PING response
- ✅ Graceful connection closing

### Security Features
- Maximum payload size limit (10MB default)
- Proper masking validation
- Connection state management

## Usage

### Basic WebSocket Server

```cpp
#include "network/socket_handler.h"
#include "network/websocket_handler.h"

// 1. Create socket handler
SocketHandler socket_handler(8080);
socket_handler.initialize();
socket_handler.bind_socket();
socket_handler.start_listening();

// 2. Accept connection
int client_socket = socket_handler.accept_connection();

// 3. Create WebSocket handler
WebSocketHandler ws_handler(client_socket);

// 4. Perform handshake
if (ws_handler.perform_handshake()) {
    // 5. Send/receive messages
    ws_handler.send_text("Hello!");
    
    std::string message;
    while (ws_handler.receive_message(message)) {
        std::cout << "Received: " << message << std::endl;
    }
}
```

### Sending Messages

```cpp
// Text message
ws_handler.send_text("Hello, World!");

// Binary message
std::vector<uint8_t> data = {0x01, 0x02, 0x03};
ws_handler.send_binary(data);

// Ping
ws_handler.send_ping("keepalive");

// Close with code and reason
ws_handler.send_close(1000, "Normal closure");
```

### Receiving Messages

```cpp
// Receive text message
std::string text_msg;
if (ws_handler.receive_message(text_msg)) {
    std::cout << "Received: " << text_msg << std::endl;
}

// Receive binary message
std::vector<uint8_t> binary_data;
if (ws_handler.receive_binary(binary_data)) {
    // Process binary data
}
```

## Compilation

### Dependencies
- OpenSSL (for SHA-1 and Base64 in handshake)

### Compile Command

```bash
# Compile socket handler
g++ -c network/socket_handler.cpp -o socket_handler.o

# Compile WebSocket handler
g++ -c network/websocket_handler.cpp -o websocket_handler.o -lssl -lcrypto

# Compile example server
g++ -c websocket_server_example.cpp -o websocket_server_example.o

# Link all together
g++ socket_handler.o websocket_handler.o websocket_server_example.o \
    -o websocket_server -pthread -lssl -lcrypto
```

### Makefile Entry

```makefile
# Add to your Makefile
WEBSOCKET_OBJS = network/socket_handler.o network/websocket_handler.o
LDFLAGS += -lssl -lcrypto -pthread

websocket_server: $(WEBSOCKET_OBJS) websocket_server_example.o
	$(CXX) $(WEBSOCKET_OBJS) websocket_server_example.o -o $@ $(LDFLAGS)

network/websocket_handler.o: network/websocket_handler.cpp network/websocket_handler.h
	$(CXX) $(CXXFLAGS) -c network/websocket_handler.cpp -o $@

network/socket_handler.o: network/socket_handler.cpp network/socket_handler.h
	$(CXX) $(CXXFLAGS) -c network/socket_handler.cpp -o $@
```

## Testing

### Using a Browser Client

Create an HTML file to test:

```html
<!DOCTYPE html>
<html>
<head>
    <title>WebSocket Test</title>
</head>
<body>
    <h1>WebSocket Test Client</h1>
    <div id="status">Disconnected</div>
    <input type="text" id="message" placeholder="Enter message">
    <button onclick="sendMessage()">Send</button>
    <div id="messages"></div>

    <script>
        const ws = new WebSocket('ws://localhost:8080');
        
        ws.onopen = function() {
            document.getElementById('status').innerText = 'Connected';
            console.log('WebSocket connected');
        };
        
        ws.onmessage = function(event) {
            const div = document.createElement('div');
            div.textContent = 'Received: ' + event.data;
            document.getElementById('messages').appendChild(div);
        };
        
        ws.onclose = function() {
            document.getElementById('status').innerText = 'Disconnected';
            console.log('WebSocket closed');
        };
        
        function sendMessage() {
            const msg = document.getElementById('message').value;
            ws.send(msg);
            document.getElementById('message').value = '';
        }
    </script>
</body>
</html>
```

### Using Python Client

```python
import asyncio
import websockets

async def test_websocket():
    uri = "ws://localhost:8080"
    async with websockets.connect(uri) as websocket:
        # Send message
        await websocket.send("Hello Server!")
        
        # Receive response
        response = await websocket.recv()
        print(f"Received: {response}")

asyncio.run(test_websocket())
```

### Using `wscat` (Node.js tool)

```bash
# Install wscat
npm install -g wscat

# Connect to server
wscat -c ws://localhost:8080

# Type messages and press Enter
> Hello!
< Echo: Hello!
```

## Integration with Chess Server

### Example: Game State Updates

```cpp
void broadcast_game_state(int game_id, const std::string& fen) {
    // Create JSON message
    Json::Value message;
    message["type"] = "GAME_UPDATE";
    message["game_id"] = game_id;
    message["fen"] = fen;
    
    // Serialize to string
    std::string json_str = Json::writeString(builder, message);
    
    // Send to all connected players
    for (auto& [socket, ws_handler] : active_connections) {
        ws_handler.send_text(json_str);
    }
}
```

### Example: Handle Chess Moves

```cpp
void handle_chess_message(WebSocketHandler& ws, const std::string& msg) {
    // Parse JSON
    Json::Value data;
    Json::Reader reader;
    reader.parse(msg, data);
    
    std::string type = data["type"].asString();
    
    if (type == "MOVE") {
        std::string from = data["from"].asString();
        std::string to = data["to"].asString();
        
        // Process move
        bool valid = process_move(from, to);
        
        // Send response
        Json::Value response;
        response["type"] = "MOVE_RESULT";
        response["valid"] = valid;
        
        std::string response_str = Json::writeString(builder, response);
        ws.send_text(response_str);
    }
}
```

## WebSocket Frame Format

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
```

## Close Codes

| Code | Meaning                |
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

## Troubleshooting

### Connection Refused
- Check if server is running: `netstat -an | grep 8080`
- Verify firewall settings
- Ensure port 8080 is not in use

### Handshake Failed
- Verify client sends proper `Sec-WebSocket-Key` header
- Check OpenSSL is properly linked
- Ensure HTTP upgrade headers are correct

### Messages Not Received
- Check masking (clients must mask, servers must not)
- Verify frame parsing logic
- Check payload length calculation

### Build Errors
```bash
# Install OpenSSL development files
sudo apt-get install libssl-dev  # Ubuntu/Debian
sudo yum install openssl-devel   # CentOS/RHEL
```

## Performance Considerations

- Use thread pool instead of creating thread per connection
- Implement connection timeout management
- Add message queue for buffering
- Consider epoll/kqueue for handling many connections
- Implement rate limiting to prevent DoS

## Security Best Practices

1. **Validate all input** - Never trust client data
2. **Limit message size** - Prevent memory exhaustion
3. **Implement timeouts** - Close idle connections
4. **Use WSS (WebSocket Secure)** - Add TLS encryption
5. **Sanitize payloads** - Validate JSON and chess moves
6. **Rate limiting** - Prevent abuse

## Next Steps

1. Add TLS/SSL support for secure WebSockets (WSS)
2. Implement connection pool management
3. Add message compression (permessage-deflate extension)
4. Create comprehensive test suite
5. Add logging and monitoring
6. Implement reconnection handling

## References

- [RFC 6455 - The WebSocket Protocol](https://tools.ietf.org/html/rfc6455)
- [WebSocket API](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket)
