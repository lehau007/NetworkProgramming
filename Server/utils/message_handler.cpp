#include "message_handler.h"
#include "message_types.h"
#include "../network/websocket_handler.h"
#include <iostream>
#include <ctime>

MessageHandler::MessageHandler(int socket) : client_socket(socket) {
    session_mgr = SessionManager::get_instance();
}

MessageHandler::~MessageHandler() {
    // Cleanup if needed
}

void MessageHandler::send_response(const json& response) {
    WebSocketHandler ws(client_socket);
    std::string response_str = response.dump();
    ws.send_text(response_str);
}

void MessageHandler::send_error(const std::string& error_code, const std::string& message, const std::string& severity) {
    json error_response;
    error_response["type"] = MessageTypes::ERROR;
    error_response["error_code"] = error_code;
    error_response["message"] = message;
    error_response["severity"] = severity;
    error_response["timestamp"] = std::time(nullptr);
    
    send_response(error_response);
}

Session* MessageHandler::validate_session(const std::string& session_id) {
    if (!session_mgr->verify_session(session_id)) {
        return nullptr;
    }
    return session_mgr->get_session(session_id);
}

void MessageHandler::handle_message(const std::string& message_str) {
    try {
        json message = json::parse(message_str);
        
        if (!message.contains("type")) {
            send_error("INVALID_MESSAGE", "Message must contain 'type' field");
            return;
        }
        
        std::string msg_type = message["type"].get<std::string>();
        
        // Route to appropriate handler
        if (msg_type == MessageTypes::VERIFY_SESSION) {
            handle_verify_session(message);
        } else if (msg_type == MessageTypes::LOGIN) {
            handle_login(message);
        } else if (msg_type == MessageTypes::REGISTER) {
            handle_register(message);
        } else if (msg_type == MessageTypes::LOGOUT) {
            handle_logout(message);
        } else if (msg_type == MessageTypes::GET_AVAILABLE_PLAYERS) {
            handle_get_available_players(message);
        } else if (msg_type == MessageTypes::CHALLENGE) {
            handle_challenge(message);
        } else if (msg_type == MessageTypes::ACCEPT_CHALLENGE) {
            handle_accept_challenge(message);
        } else if (msg_type == MessageTypes::DECLINE_CHALLENGE) {
            handle_decline_challenge(message);
        } else if (msg_type == MessageTypes::CANCEL_CHALLENGE) {
            handle_cancel_challenge(message);
        } else if (msg_type == MessageTypes::MOVE) {
            handle_move(message);
        } else if (msg_type == MessageTypes::RESIGN) {
            handle_resign(message);
        } else if (msg_type == MessageTypes::DRAW_OFFER) {
            handle_draw_offer(message);
        } else if (msg_type == MessageTypes::DRAW_RESPONSE) {
            handle_draw_response(message);
        } else if (msg_type == MessageTypes::REQUEST_REMATCH) {
            handle_request_rematch(message);
        } else if (msg_type == MessageTypes::GET_GAME_STATE) {
            handle_get_game_state(message);
        } else if (msg_type == MessageTypes::GET_GAME_HISTORY) {
            handle_get_game_history(message);
        } else if (msg_type == MessageTypes::GET_LEADERBOARD) {
            handle_get_leaderboard(message);
        } else if (msg_type == MessageTypes::PING) {
            handle_ping(message);
        } else if (msg_type == MessageTypes::CHAT_MESSAGE) {
            handle_chat_message(message);
        } else {
            send_error("UNKNOWN_MESSAGE_TYPE", "Unknown message type: " + msg_type);
        }
        
    } catch (const json::parse_error& e) {
        send_error("PARSE_ERROR", std::string("Failed to parse JSON: ") + e.what());
    } catch (const std::exception& e) {
        send_error("INTERNAL_ERROR", std::string("Internal error: ") + e.what());
    }
}

// ============================================================================
// CONNECTION & SESSION HANDLERS
// ============================================================================

void MessageHandler::handle_verify_session(const json& request) {
    std::cout << "[MessageHandler] VERIFY_SESSION request" << std::endl;
    
    if (!request.contains("session_id")) {
        send_error("MISSING_FIELD", "session_id is required");
        return;
    }
    
    std::string session_id = request["session_id"].get<std::string>();
    
    if (session_mgr->verify_session(session_id)) {
        Session* session = session_mgr->get_session(session_id);
        
        // Update socket mapping for this connection
        session_mgr->update_socket_mapping(session_id, client_socket);
        
        // Get user data from database
        auto user_opt = UserRepository::get_user_by_id(session->user_id);
        
        json response;
        response["type"] = MessageTypes::SESSION_VALID;
        response["session_id"] = session_id;
        
        if (user_opt.has_value()) {
            auto& user = user_opt.value();
            response["user_data"]["user_id"] = user.user_id;
            response["user_data"]["username"] = user.username;
            response["user_data"]["wins"] = user.wins;
            response["user_data"]["losses"] = user.losses;
            response["user_data"]["draws"] = user.draws;
            response["user_data"]["rating"] = user.rating;
        }
        
        response["active_game_id"] = nullptr;  // TODO: Get from game manager
        response["last_activity"] = session->last_activity;
        response["message"] = "Session restored successfully";
        
        send_response(response);
        std::cout << "[MessageHandler] Session valid for user " << session->username << std::endl;
        
    } else {
        json response;
        response["type"] = MessageTypes::SESSION_INVALID;
        response["reason"] = "expired";
        response["message"] = "Session expired. Please log in again.";
        
        send_response(response);
        std::cout << "[MessageHandler] Session invalid" << std::endl;
    }
}

// ============================================================================
// AUTHENTICATION HANDLERS
// ============================================================================

void MessageHandler::handle_login(const json& request) {
    std::cout << "[MessageHandler] LOGIN request" << std::endl;
    
    if (!request.contains("username") || !request.contains("password")) {
        send_error("MISSING_FIELD", "username and password are required");
        return;
    }
    
    std::string username = request["username"].get<std::string>();
    std::string password_hash = request["password"].get<std::string>();
    
    // Authenticate user
    int user_id = UserRepository::authenticate_user(username, password_hash);
    
    json response;
    response["type"] = MessageTypes::LOGIN_RESPONSE;
    
    if (user_id > 0) {
        // Get user data
        auto user_opt = UserRepository::get_user_by_id(user_id);
        
        if (user_opt.has_value()) {
            auto& user = user_opt.value();
            
            // Create session
            std::string session_id = session_mgr->create_session(
                user_id, 
                username, 
                client_socket, 
                "127.0.0.1"  // TODO: Get real IP address
            );
            
            response["status"] = "success";
            response["session_id"] = session_id;
            response["user_data"]["user_id"] = user.user_id;
            response["user_data"]["username"] = user.username;
            response["user_data"]["wins"] = user.wins;
            response["user_data"]["losses"] = user.losses;
            response["user_data"]["draws"] = user.draws;
            response["user_data"]["rating"] = user.rating;
            response["message"] = "Login successful";
            
            std::cout << "[MessageHandler] Login successful for " << username << std::endl;
        } else {
            response["status"] = "failure";
            response["message"] = "Failed to retrieve user data";
        }
    } else {
        response["status"] = "failure";
        response["message"] = "Invalid username or password";
        std::cout << "[MessageHandler] Login failed for " << username << std::endl;
    }
    
    send_response(response);
}

void MessageHandler::handle_register(const json& request) {
    std::cout << "[MessageHandler] REGISTER request" << std::endl;
    
    if (!request.contains("username") || !request.contains("password")) {
        send_error("MISSING_FIELD", "username and password are required");
        return;
    }
    
    std::string username = request["username"].get<std::string>();
    std::string password_hash = request["password"].get<std::string>();
    std::string email = request.contains("email") ? request["email"].get<std::string>() : "";
    
    // Check if username already exists
    if (UserRepository::username_exists(username)) {
        json response;
        response["type"] = MessageTypes::REGISTER_RESPONSE;
        response["status"] = "failure";
        response["message"] = "Username already exists";
        
        send_response(response);
        std::cout << "[MessageHandler] Registration failed - username exists: " << username << std::endl;
        return;
    }
    
    // Create user
    int user_id = UserRepository::create_user(username, password_hash, email);
    
    json response;
    response["type"] = MessageTypes::REGISTER_RESPONSE;
    
    if (user_id > 0) {
        response["status"] = "success";
        response["user_id"] = user_id;
        response["message"] = "Registration successful";
        
        std::cout << "[MessageHandler] Registration successful for " << username << " (ID: " << user_id << ")" << std::endl;
    } else {
        response["status"] = "failure";
        response["message"] = "Failed to create user account";
        std::cout << "[MessageHandler] Registration failed for " << username << std::endl;
    }
    
    send_response(response);
}

void MessageHandler::handle_logout(const json& request) {
    std::cout << "[MessageHandler] LOGOUT request" << std::endl;
    
    if (!request.contains("session_id")) {
        send_error("MISSING_FIELD", "session_id is required");
        return;
    }
    
    std::string session_id = request["session_id"].get<std::string>();
    Session* session = validate_session(session_id);
    
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    std::string username = session->username;
    
    // Remove session
    session_mgr->remove_session(session_id);
    
    json response;
    response["type"] = "LOGOUT_RESPONSE";
    response["status"] = "success";
    response["message"] = "Logged out successfully";
    
    send_response(response);
    std::cout << "[MessageHandler] Logout successful for " << username << std::endl;
}

// ============================================================================
// LOBBY HANDLERS
// ============================================================================

void MessageHandler::handle_get_available_players(const json& request) {
    std::cout << "[MessageHandler] GET_AVAILABLE_PLAYERS request" << std::endl;
    
    if (!request.contains("session_id")) {
        send_error("MISSING_FIELD", "session_id is required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    // Get actual online players from player manager
    auto all_users = UserRepository::get_all_users();
    
    json response;
    response["type"] = MessageTypes::PLAYER_LIST;
    response["players"] = json::array();
    
    for (const auto& user : all_users) {
        if (user.user_id != session->user_id) {
            // One more checking for online
            auto session_pointer = session_mgr->get_session_by_user_id(user.user_id);
            if (!session_pointer) 
                continue;
            json player;
            player["username"] = user.username;
            player["rating"] = user.rating;
            player["status"] = "available";  // TODO: Get from player manager
            response["players"].push_back(player);
        }
    }
    
    send_response(response);
    std::cout << "[MessageHandler] Sent player list: " << response["players"].size() << " players" << std::endl;
}

// ============================================================================
// MATCHMAKING HANDLERS (STUBS)
// ============================================================================

void MessageHandler::handle_challenge(const json& request) {
    std::cout << "[MessageHandler] CHALLENGE request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Challenge feature not yet implemented");
}

void MessageHandler::handle_accept_challenge(const json& request) {
    std::cout << "[MessageHandler] ACCEPT_CHALLENGE request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Accept challenge feature not yet implemented");
}

void MessageHandler::handle_decline_challenge(const json& request) {
    std::cout << "[MessageHandler] DECLINE_CHALLENGE request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Decline challenge feature not yet implemented");
}

void MessageHandler::handle_cancel_challenge(const json& request) {
    std::cout << "[MessageHandler] CANCEL_CHALLENGE request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Cancel challenge feature not yet implemented");
}

// ============================================================================
// GAMEPLAY HANDLERS (STUBS)
// ============================================================================

void MessageHandler::handle_move(const json& request) {
    std::cout << "[MessageHandler] MOVE request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Move feature not yet implemented");
}

void MessageHandler::handle_resign(const json& request) {
    std::cout << "[MessageHandler] RESIGN request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Resign feature not yet implemented");
}

void MessageHandler::handle_draw_offer(const json& request) {
    std::cout << "[MessageHandler] DRAW_OFFER request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Draw offer feature not yet implemented");
}

void MessageHandler::handle_draw_response(const json& request) {
    std::cout << "[MessageHandler] DRAW_RESPONSE request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Draw response feature not yet implemented");
}

void MessageHandler::handle_request_rematch(const json& request) {
    std::cout << "[MessageHandler] REQUEST_REMATCH request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Rematch feature not yet implemented");
}

// ============================================================================
// GAME STATE HANDLERS
// ============================================================================

void MessageHandler::handle_get_game_state(const json& request) {
    std::cout << "[MessageHandler] GET_GAME_STATE request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Get game state feature not yet implemented");
}

void MessageHandler::handle_get_game_history(const json& request) {
    std::cout << "[MessageHandler] GET_GAME_HISTORY request" << std::endl;
    
    if (!request.contains("session_id")) {
        send_error("MISSING_FIELD", "session_id is required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    int user_id = request.contains("user_id") ? request["user_id"].get<int>() : session->user_id;
    int limit = request.contains("limit") ? request["limit"].get<int>() : 10;
    
    // Get game history
    auto games = GameRepository::get_user_games(user_id, limit);
    
    json response;
    response["type"] = MessageTypes::GAME_HISTORY;
    response["games"] = json::array();
    
    for (const auto& game : games) {
        json game_json;
        game_json["game_id"] = game.game_id;
        game_json["white_player_id"] = game.white_player_id;
        game_json["black_player_id"] = game.black_player_id;
        game_json["result"] = game.result;
        game_json["date"] = game.start_time;
        game_json["duration_seconds"] = game.duration;
        // game_json["moves"] = game.moves;  // TODO: Parse JSONB moves
        
        response["games"].push_back(game_json);
    }
    
    response["total_count"] = games.size();
    
    send_response(response);
    std::cout << "[MessageHandler] Sent game history: " << games.size() << " games" << std::endl;
}

void MessageHandler::handle_get_leaderboard(const json& request) {
    std::cout << "[MessageHandler] GET_LEADERBOARD request" << std::endl;
    
    if (!request.contains("session_id")) {
        send_error("MISSING_FIELD", "session_id is required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    int limit = request.contains("limit") ? request["limit"].get<int>() : 50;
    
    // Get top users
    auto top_users = UserRepository::get_top_users(limit);
    
    json response;
    response["type"] = MessageTypes::LEADERBOARD;
    response["players"] = json::array();
    
    int rank = 1;
    for (const auto& user : top_users) {
        json player;
        player["rank"] = rank++;
        player["username"] = user.username;
        player["rating"] = user.rating;
        player["wins"] = user.wins;
        player["losses"] = user.losses;
        player["draws"] = user.draws;
        
        response["players"].push_back(player);
    }
    
    send_response(response);
    std::cout << "[MessageHandler] Sent leaderboard: " << top_users.size() << " players" << std::endl;
}

// ============================================================================
// SYSTEM HANDLERS
// ============================================================================

void MessageHandler::handle_ping(const json& request) {
    json response;
    response["type"] = MessageTypes::PONG;
    
    if (request.contains("timestamp")) {
        response["timestamp"] = request["timestamp"];
    } else {
        response["timestamp"] = std::time(nullptr);
    }
    
    send_response(response);
}

void MessageHandler::handle_chat_message(const json& request) {
    std::cout << "[MessageHandler] CHAT_MESSAGE request (stub)" << std::endl;
    send_error("NOT_IMPLEMENTED", "Chat feature not yet implemented");
}
