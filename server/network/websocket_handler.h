#ifndef WEBSOCKET_HANDLER_H
#define WEBSOCKET_HANDLER_H

#include <string>
#include <vector>
#include <cstdint>

// WebSocket frame opcodes
enum class WebSocketOpcode : uint8_t {
    CONTINUATION = 0x0,
    TEXT = 0x1,
    BINARY = 0x2,
    CLOSE = 0x8,
    PING = 0x9,
    PONG = 0xA
};

// WebSocket frame structure
struct WebSocketFrame {
    bool fin;
    bool rsv1;
    bool rsv2;
    bool rsv3;
    WebSocketOpcode opcode;
    bool masked;
    uint64_t payload_length;
    uint8_t masking_key[4];
    std::vector<uint8_t> payload;
};

class WebSocketHandler {
private:
    int socket_fd;
    bool is_handshake_complete;
    std::vector<uint8_t> fragment_buffer;
    WebSocketOpcode fragment_opcode;
    
    // Handshake helpers
    bool parse_http_request(const std::string& request, std::string& websocket_key);
    std::string generate_accept_key(const std::string& websocket_key);
    std::string generate_handshake_response(const std::string& accept_key);
    
    // Frame helpers
    bool read_frame_header(WebSocketFrame& frame);
    bool read_frame_payload(WebSocketFrame& frame);
    void unmask_payload(std::vector<uint8_t>& payload, const uint8_t* mask);
    std::vector<uint8_t> create_frame(WebSocketOpcode opcode, const std::vector<uint8_t>& data, bool fin = true);
    
    // Control frame handlers
    void handle_ping(const std::vector<uint8_t>& payload);
    void handle_close(const std::vector<uint8_t>& payload);
    
public:
    WebSocketHandler(int socket);
    ~WebSocketHandler();
    
    // Handshake
    bool perform_handshake();
    bool is_connected() const { return is_handshake_complete; }
    
    // Send operations
    bool send_text(const std::string& message);
    bool send_binary(const std::vector<uint8_t>& data);
    bool send_ping(const std::string& data = "");
    bool send_pong(const std::vector<uint8_t>& data);
    bool send_close(uint16_t code = 1000, const std::string& reason = "");
    
    // Receive operations
    bool receive_message(std::string& message);
    bool receive_binary(std::vector<uint8_t>& data);
    
    // Low-level frame operations
    bool send_frame(const std::vector<uint8_t>& frame_data);
    bool receive_frame(WebSocketFrame& frame);
};

#endif // WEBSOCKET_HANDLER_H
