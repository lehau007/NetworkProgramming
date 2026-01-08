#include "websocket_handler.h"
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

// WebSocket GUID for handshake
static const std::string WEBSOCKET_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

WebSocketHandler::WebSocketHandler(int socket) 
    : socket_fd(socket), is_handshake_complete(false), fragment_opcode(WebSocketOpcode::CONTINUATION) {
}

WebSocketHandler::~WebSocketHandler() {
    if (is_handshake_complete) {
        send_close();
    }
}

// ==================== Handshake Implementation ====================

bool WebSocketHandler::perform_handshake() {
    // Read HTTP upgrade request - must read until \r\n\r\n
    std::string request;
    char buffer[1024];
    
    while (true) {
        ssize_t received = recv(socket_fd, buffer, sizeof(buffer), 0);
        
        if (received <= 0) {
            std::cerr << "Failed to receive handshake request" << std::endl;
            return false;
        }
        
        request.append(buffer, received);
        
        // Check if we have received the complete HTTP header (ends with \r\n\r\n)
        if (request.find("\r\n\r\n") != std::string::npos) {
            break;
        }
        
        // Safety check: prevent infinite loop if request is too large
        if (request.size() > 8192) {
            std::cerr << "HTTP request too large" << std::endl;
            return false;
        }
    }
    
    // Parse WebSocket key
    std::string websocket_key;
    if (!parse_http_request(request, websocket_key)) {
        std::cerr << "Failed to parse WebSocket handshake" << std::endl;
        return false;
    }
    
    // Generate accept key
    std::string accept_key = generate_accept_key(websocket_key);
    
    // Send handshake response
    std::string response = generate_handshake_response(accept_key);
    ssize_t sent = send(socket_fd, response.c_str(), response.length(), 0);
    
    if (sent != static_cast<ssize_t>(response.length())) {
        std::cerr << "Failed to send handshake response" << std::endl;
        return false;
    }
    
    is_handshake_complete = true;
    std::cout << "WebSocket handshake completed successfully" << std::endl;
    return true;
}

bool WebSocketHandler::parse_http_request(const std::string& request, std::string& websocket_key) {
    // Convert request to lowercase for searching
    std::string request_lower = request;
    std::string key_header = "sec-websocket-key:";
    
    // Find the header position (case-insensitive)
    size_t key_pos = std::string::npos;
    for (size_t i = 0; i < request.length(); i++) {
        request_lower[i] = std::tolower(request[i]);
    }
    
    key_pos = request_lower.find(key_header);
    
    if (key_pos == std::string::npos) {
        std::cerr << "Sec-WebSocket-Key header not found" << std::endl;
        return false;
    }
    
    // Find the start of the value (skip header name and optional spaces)
    size_t key_start = key_pos + key_header.length();
    while (key_start < request.length() && request[key_start] == ' ') {
        key_start++;
    }
    
    // Find the end of the header line
    size_t key_end = request.find("\r\n", key_start);
    
    if (key_end == std::string::npos) {
        std::cerr << "Malformed Sec-WebSocket-Key header" << std::endl;
        return false;
    }
    
    // Extract the key value (from original request, not lowercased)
    websocket_key = request.substr(key_start, key_end - key_start);
    
    // Trim trailing spaces
    while (!websocket_key.empty() && websocket_key.back() == ' ') {
        websocket_key.pop_back();
    }
    
    return !websocket_key.empty();
}

std::string WebSocketHandler::generate_accept_key(const std::string& websocket_key) {
    // Concatenate key with GUID
    std::string combined = websocket_key + WEBSOCKET_GUID;
    
    // SHA-1 hash
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(combined.c_str()), combined.length(), hash);
    
    // Base64 encode
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, hash, SHA_DIGEST_LENGTH);
    BIO_flush(bio);
    
    BUF_MEM* buffer_ptr;
    BIO_get_mem_ptr(bio, &buffer_ptr);
    std::string result(buffer_ptr->data, buffer_ptr->length);
    
    BIO_free_all(bio);
    
    return result;
}

std::string WebSocketHandler::generate_handshake_response(const std::string& accept_key) {
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << accept_key << "\r\n";
    response << "\r\n";
    return response.str();
}

// ==================== Frame Operations ====================

std::vector<uint8_t> WebSocketHandler::create_frame(WebSocketOpcode opcode, 
                                                     const std::vector<uint8_t>& data, 
                                                     bool fin) {
    std::vector<uint8_t> frame;
    
    // Byte 0: FIN, RSV, Opcode
    uint8_t byte0 = (fin ? 0x80 : 0x00) | static_cast<uint8_t>(opcode);
    frame.push_back(byte0);
    
    // Byte 1: MASK, Payload length
    uint64_t payload_len = data.size();
    
    if (payload_len < 126) {
        frame.push_back(static_cast<uint8_t>(payload_len));
    } else if (payload_len < 65536) {
        frame.push_back(126);
        frame.push_back((payload_len >> 8) & 0xFF);
        frame.push_back(payload_len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((payload_len >> (i * 8)) & 0xFF);
        }
    }
    
    // Payload (server doesn't mask)
    frame.insert(frame.end(), data.begin(), data.end());
    
    return frame;
}

bool WebSocketHandler::send_frame(const std::vector<uint8_t>& frame_data) {
    size_t total_sent = 0;
    while (total_sent < frame_data.size()) {
        ssize_t sent = send(socket_fd, frame_data.data() + total_sent, 
                           frame_data.size() - total_sent, 0);
        if (sent <= 0) {
            std::cerr << "Failed to send frame: " << strerror(errno) << std::endl;
            return false;
        }
        total_sent += sent;
    }
    return true;
}

bool WebSocketHandler::receive_frame(WebSocketFrame& frame) {
    if (!read_frame_header(frame)) {
        return false;
    }
    
    if (!read_frame_payload(frame)) {
        return false;
    }
    
    // Unmask payload if masked
    if (frame.masked) {
        unmask_payload(frame.payload, frame.masking_key);
    }
    
    return true;
}

bool WebSocketHandler::read_frame_header(WebSocketFrame& frame) {
    uint8_t header[2];
    
    // Read first 2 bytes
    ssize_t received = recv(socket_fd, header, 2, MSG_WAITALL);
    if (received != 2) {
        return false;
    }
    
    // Parse byte 0
    frame.fin = (header[0] & 0x80) != 0;
    frame.rsv1 = (header[0] & 0x40) != 0;
    frame.rsv2 = (header[0] & 0x20) != 0;
    frame.rsv3 = (header[0] & 0x10) != 0;
    frame.opcode = static_cast<WebSocketOpcode>(header[0] & 0x0F);
    
    // Parse byte 1
    frame.masked = (header[1] & 0x80) != 0;
    uint8_t payload_len = header[1] & 0x7F;
    
    // Read extended payload length
    if (payload_len == 126) {
        uint8_t extended[2];
        if (recv(socket_fd, extended, 2, MSG_WAITALL) != 2) {
            return false;
        }
        frame.payload_length = (extended[0] << 8) | extended[1];
    } else if (payload_len == 127) {
        uint8_t extended[8];
        if (recv(socket_fd, extended, 8, MSG_WAITALL) != 8) {
            return false;
        }
        frame.payload_length = 0;
        for (int i = 0; i < 8; i++) {
            frame.payload_length = (frame.payload_length << 8) | extended[i];
        }
    } else {
        frame.payload_length = payload_len;
    }
    
    // Read masking key if present
    if (frame.masked) {
        if (recv(socket_fd, frame.masking_key, 4, MSG_WAITALL) != 4) {
            return false;
        }
    }
    
    return true;
}

bool WebSocketHandler::read_frame_payload(WebSocketFrame& frame) {
    if (frame.payload_length == 0) {
        return true;
    }
    
    // Limit payload size to prevent DoS
    if (frame.payload_length > 10 * 1024 * 1024) {  // 10MB max
        std::cerr << "Payload too large: " << frame.payload_length << std::endl;
        return false;
    }
    
    frame.payload.resize(frame.payload_length);
    size_t total_received = 0;
    
    while (total_received < frame.payload_length) {
        ssize_t received = recv(socket_fd, 
                               frame.payload.data() + total_received,
                               frame.payload_length - total_received, 
                               0);
        if (received <= 0) {
            return false;
        }
        total_received += received;
    }
    
    return true;
}

void WebSocketHandler::unmask_payload(std::vector<uint8_t>& payload, const uint8_t* mask) {
    for (size_t i = 0; i < payload.size(); i++) {
        payload[i] ^= mask[i % 4];
    }
}

// ==================== Send Operations ====================

bool WebSocketHandler::send_text(const std::string& message) {
    std::vector<uint8_t> data(message.begin(), message.end());
    std::vector<uint8_t> frame = create_frame(WebSocketOpcode::TEXT, data);
    return send_frame(frame);
}

bool WebSocketHandler::send_binary(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> frame = create_frame(WebSocketOpcode::BINARY, data);
    return send_frame(frame);
}

bool WebSocketHandler::send_ping(const std::string& data) {
    std::vector<uint8_t> payload(data.begin(), data.end());
    std::vector<uint8_t> frame = create_frame(WebSocketOpcode::PING, payload);
    return send_frame(frame);
}

bool WebSocketHandler::send_pong(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> frame = create_frame(WebSocketOpcode::PONG, data);
    return send_frame(frame);
}

bool WebSocketHandler::send_close(uint16_t code, const std::string& reason) {
    std::vector<uint8_t> payload;
    
    // Add close code
    payload.push_back((code >> 8) & 0xFF);
    payload.push_back(code & 0xFF);
    
    // Add reason
    payload.insert(payload.end(), reason.begin(), reason.end());
    
    std::vector<uint8_t> frame = create_frame(WebSocketOpcode::CLOSE, payload);
    bool result = send_frame(frame);
    
    is_handshake_complete = false;
    return result;
}

// ==================== Receive Operations ====================

bool WebSocketHandler::receive_message(std::string& message) {
    while (true) {
        WebSocketFrame frame;
        if (!receive_frame(frame)) {
            return false;
        }
        
        switch (frame.opcode) {
            case WebSocketOpcode::TEXT:
            case WebSocketOpcode::CONTINUATION:
                fragment_buffer.insert(fragment_buffer.end(), 
                                     frame.payload.begin(), 
                                     frame.payload.end());
                
                if (frame.fin) {
                    message.assign(fragment_buffer.begin(), fragment_buffer.end());
                    fragment_buffer.clear();
                    return true;
                }
                break;
                
            case WebSocketOpcode::PING:
                handle_ping(frame.payload);
                break;
                
            case WebSocketOpcode::CLOSE:
                handle_close(frame.payload);
                return false;
                
            default:
                std::cerr << "Unexpected opcode: " 
                         << static_cast<int>(frame.opcode) << std::endl;
                break;
        }
    }
}

bool WebSocketHandler::receive_binary(std::vector<uint8_t>& data) {
    while (true) {
        WebSocketFrame frame;
        if (!receive_frame(frame)) {
            return false;
        }
        
        switch (frame.opcode) {
            case WebSocketOpcode::BINARY:
            case WebSocketOpcode::CONTINUATION:
                fragment_buffer.insert(fragment_buffer.end(), 
                                     frame.payload.begin(), 
                                     frame.payload.end());
                
                if (frame.fin) {
                    data = fragment_buffer;
                    fragment_buffer.clear();
                    return true;
                }
                break;
                
            case WebSocketOpcode::PING:
                handle_ping(frame.payload);
                break;
                
            case WebSocketOpcode::CLOSE:
                handle_close(frame.payload);
                return false;
                
            default:
                std::cerr << "Unexpected opcode: " 
                         << static_cast<int>(frame.opcode) << std::endl;
                break;
        }
    }
}

// ==================== Control Frame Handlers ====================

void WebSocketHandler::handle_ping(const std::vector<uint8_t>& payload) {
    std::cout << "Received PING, sending PONG" << std::endl;
    send_pong(payload);
}

void WebSocketHandler::handle_close(const std::vector<uint8_t>& payload) {
    uint16_t code = 1000;
    std::string reason;
    
    if (payload.size() >= 2) {
        code = (payload[0] << 8) | payload[1];
    }
    
    if (payload.size() > 2) {
        reason.assign(payload.begin() + 2, payload.end());
    }
    
    std::cout << "Received CLOSE frame. Code: " << code 
              << ", Reason: " << reason << std::endl;
    
    // Send close response
    send_close(code, reason);
}
