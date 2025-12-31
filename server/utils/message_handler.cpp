#include "message_handler.h"
#include "message_types.h"
#include "../network/websocket_handler.h"
#include <iostream>
#include <ctime>

MessageHandler::MessageHandler(int socket, std::string ip_address) : client_socket(socket), ip_address(ip_address) {
    session_mgr = SessionManager::get_instance();
    match_mgr = MatchManager::get_instance();
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

void MessageHandler::broadcast_to_user(int user_id, const json& message) {
    Session* target_session = session_mgr->get_session_by_user_id(user_id);
    if (target_session && target_session->client_socket > 0) {
        WebSocketHandler ws(target_session->client_socket);
        std::string message_str = message.dump();
        ws.send_text(message_str);
    }
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
        } else if (msg_type == MessageTypes::AI_CHALLENGE) {
            handle_ai_challenge(message);
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
    
    if (session_mgr->verify_session(session_id)) { // check session_id in database
        Session* session = session_mgr->get_session(session_id);
        
        // Update socket mapping for this connection
        if (session_mgr->update_socket_mapping(session_id, client_socket) == false) {
            // Session is already associated with a different socket (another connection is active)
            // Send DUPLICATE_SESSION error to inform the user
            json response;
            response["type"] = MessageTypes::DUPLICATE_SESSION;
            response["session_id"] = session_id;
            response["reason"] = "already_connected";
            response["message"] = "Multiple connections with the same session are not allowed. Please close the existing connection first.";
            response["timestamp"] = std::time(nullptr);
            
            send_response(response);
            std::cout << "[MessageHandler] Rejected duplicate session connection for session " << session_id << std::endl;
            return;
        } 
        
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
        
        response["active_game_id"] = nullptr;  // TODO: Get from game manager (this todo is not necessary because session can not have dulicated connection)
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
    
    // Check user in another device
    if (session_mgr->is_user_connected(user_id)) {
        json response;
        response["type"] = MessageTypes::LOGIN_RESPONSE;
        response["status"] = "failure";
        response["message"] = "User already connected from another device";
        
        send_response(response);
        std::cout << "[MessageHandler] Login failed - user already connected: " << username << std::endl;
        return;
    }
    
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
                this->ip_address
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
    session_mgr->remove_session_in_cache(session_id);
    session_mgr->remove_session_in_database(session_id);
    
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

    // Find current user index in the list
    int current_user_index = -1;
    for (size_t i = 0; i < all_users.size(); ++i) {
        if (all_users[i].user_id == session->user_id) {
            current_user_index = i;
            break;
        }
    }
    
    json response;
    response["type"] = MessageTypes::PLAYER_LIST;
    response["players"] = json::array();
    
    int idx = 0;
    for (const auto& user : all_users) {
        if (idx < current_user_index - 10 || idx >current_user_index + 10 || idx == current_user_index) {
            idx += 1;
            continue;
        }

        // Check if user is online (has active session)
        auto session_pointer = session_mgr->get_session_by_user_id(user.user_id);
        if (!session_pointer) {
            idx += 1;
            continue;
        }
        
        // Check if user is in a game
        bool is_in_game = match_mgr->is_player_in_game(user.user_id);
        
        // Check if user has pending challenge (either sent or received)
        bool has_challenge = match_mgr->has_pending_challenge(user.user_id);
        
        json player;
        player["username"] = user.username;
        player["rating"] = user.rating;
        
        // Set status based on availability
        if (is_in_game) {
            player["status"] = "in_game";
        } else if (has_challenge) {
            player["status"] = "busy";  // Has pending challenge
        } else {
            player["status"] = "available"; 
        }
        
        response["players"].push_back(player);
        idx += 1;
    }
    
    send_response(response);
    std::cout << "[MessageHandler] Sent player list: " << response["players"].size() << " players" << std::endl;
}

// ============================================================================
// MATCHMAKING HANDLERS (STUBS)
// ============================================================================

void MessageHandler::handle_challenge(const json& request) {
    std::cout << "[MessageHandler] CHALLENGE request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("target_username")) {
        send_error("MISSING_FIELD", "session_id and target_username are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    // Check if player is already in a game or has pending challenge
    if (match_mgr->is_player_in_game(session->user_id)) {
        send_error("ALREADY_IN_GAME", "You are already in a game");
        return;
    }
    
    if (match_mgr->has_pending_challenge(session->user_id)) {
        send_error("PENDING_CHALLENGE", "You already have a pending challenge");
        return;
    }
    
    std::string target_username = request["target_username"].get<std::string>();
    std::string preferred_color = request.contains("preferred_color") ? 
                                  request["preferred_color"].get<std::string>() : "random";
    
    // Get target user
    auto target_user_opt = UserRepository::get_user_by_username(target_username);
    if (!target_user_opt.has_value()) {
        send_error("USER_NOT_FOUND", "Target user not found");
        return;
    }
    
    auto& target_user = target_user_opt.value();
    
    // Check if target is online
    Session* target_session = session_mgr->get_session_by_user_id(target_user.user_id);
    if (!target_session) {
        send_error("USER_OFFLINE", "Target user is offline");
        return;
    }
    
    // Check if target is already in a game or has pending challenge
    if (match_mgr->is_player_in_game(target_user.user_id)) {
        send_error("USER_BUSY", "Target user is already in a game");
        return;
    }
    
    if (match_mgr->has_pending_challenge(target_user.user_id)) {
        send_error("USER_BUSY", "Target user has a pending challenge");
        return;
    }
    
    // Create challenge
    std::string challenge_id = match_mgr->create_challenge(
        session->user_id, session->username,
        target_user.user_id, target_username,
        preferred_color
    );
    
    json response;
    response["type"] = MessageTypes::CHALLENGE_SENT;
    response["challenge_id"] = challenge_id;
    response["target_username"] = target_username;
    response["status"] = "pending";
    
    send_response(response);
    std::cout << "[MessageHandler] Challenge sent from " << session->username 
              << " to " << target_username << std::endl;
}

void MessageHandler::handle_ai_challenge(const json& request) {
    std::cout << "[MessageHandler] AI CHALLENGE request" << std::endl;

    if (!request.contains("session_id")) {
        send_error("MISSING_FIELD", "session_id is required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }

    const std::string preferred_color = request.contains("preferred_color")
        ? request["preferred_color"].get<std::string>()
        : "random";

    int depth = 2;
    if (request.contains("depth")) {
        try {
            depth = request["depth"].get<int>();
        } catch (...) {
            depth = 2;
        }
    }

    // Send AI_CHALLENGE_SENT first (per spec)
    json ai_challenge_sent_response;
    ai_challenge_sent_response["type"] = MessageTypes::AI_CHALLENGE_SENT;
    ai_challenge_sent_response["status"] = "accepted";
    send_response(ai_challenge_sent_response);

//     ```json
// {
//     "type": "MATCH_STARTED",
//     "game_id": 456,
//     "white_player": "player1", 
//     "black_player": "player2", 
//     "your_color": "white",  // varies per client
//     "opponent_username": "player2",
//     "opponent_rating": 1500,
//     "time_control": "10+0"  // optional
// }
// ```

    // Create a new game with AI opponent. Note: id of AI is fixed -1.
    int game_id = -1;
    if (!match_mgr->accept_ai_challenge(session->user_id, session->username, preferred_color, depth, game_id)) {
        send_error("AI_CHALLENGE_FAILED", "Failed to create AI game");
        return;
    }

    std::cout << "[MessageHandler] AI game created: " << game_id
              << " for user " << session->username
              << " preferred_color=" << preferred_color
              << " depth=" << depth << std::endl;
}

void MessageHandler::handle_accept_challenge(const json& request) {
    std::cout << "[MessageHandler] ACCEPT_CHALLENGE request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("challenge_id")) {
        send_error("MISSING_FIELD", "session_id and challenge_id are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    std::string challenge_id = request["challenge_id"].get<std::string>();
    
    // Verify challenge exists and is for this user
    Challenge* challenge = match_mgr->get_challenge(challenge_id);
    if (!challenge) {
        send_error("CHALLENGE_NOT_FOUND", "Challenge not found or expired");
        return;
    }
    
    if (challenge->target_user_id != session->user_id) {
        send_error("INVALID_CHALLENGE", "This challenge is not for you");
        return;
    }
    
    // Accept challenge (creates game and broadcasts MATCH_STARTED)
    int game_id;
    if (!match_mgr->accept_challenge(challenge_id, game_id)) {
        send_error("CHALLENGE_ACCEPT_FAILED", "Failed to accept challenge");
        return;
    }
    
    json response;
    response["type"] = "CHALLENGE_ACCEPTED";
    response["challenge_id"] = challenge_id;
    response["game_id"] = game_id;
    response["status"] = "success";
    
    send_response(response);
    std::cout << "[MessageHandler] Challenge accepted: " << challenge_id 
              << ", game created: " << game_id << std::endl;
}

void MessageHandler::handle_decline_challenge(const json& request) {
    std::cout << "[MessageHandler] DECLINE_CHALLENGE request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("challenge_id")) {
        send_error("MISSING_FIELD", "session_id and challenge_id are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    std::string challenge_id = request["challenge_id"].get<std::string>();
    
    // Verify challenge exists and is for this user
    Challenge* challenge = match_mgr->get_challenge(challenge_id);
    if (!challenge) {
        send_error("CHALLENGE_NOT_FOUND", "Challenge not found or expired");
        return;
    }
    
    if (challenge->target_user_id != session->user_id) {
        send_error("INVALID_CHALLENGE", "This challenge is not for you");
        return;
    }
    
    // Decline challenge (notifies challenger)
    if (!match_mgr->decline_challenge(challenge_id)) {
        send_error("CHALLENGE_DECLINE_FAILED", "Failed to decline challenge");
        return;
    }
    
    json response;
    response["type"] = "CHALLENGE_DECLINED_RESPONSE";
    response["challenge_id"] = challenge_id;
    response["status"] = "success";
    
    send_response(response);
    std::cout << "[MessageHandler] Challenge declined: " << challenge_id << std::endl;
}

void MessageHandler::handle_cancel_challenge(const json& request) {
    std::cout << "[MessageHandler] CANCEL_CHALLENGE request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("challenge_id")) {
        send_error("MISSING_FIELD", "session_id and challenge_id are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    std::string challenge_id = request["challenge_id"].get<std::string>();
    
    // Verify challenge exists and was sent by this user
    Challenge* challenge = match_mgr->get_challenge(challenge_id);
    if (!challenge) {
        send_error("CHALLENGE_NOT_FOUND", "Challenge not found or expired");
        return;
    }
    
    if (challenge->challenger_user_id != session->user_id) {
        send_error("INVALID_CHALLENGE", "You did not send this challenge");
        return;
    }
    
    // Cancel challenge (notifies target)
    if (!match_mgr->cancel_challenge(challenge_id)) {
        send_error("CHALLENGE_CANCEL_FAILED", "Failed to cancel challenge");
        return;
    }
    
    json response;
    response["type"] = "CHALLENGE_CANCELLED_RESPONSE";
    response["challenge_id"] = challenge_id;
    response["status"] = "success";
    
    send_response(response);
    std::cout << "[MessageHandler] Challenge cancelled: " << challenge_id << std::endl;
}

// ============================================================================
// GAMEPLAY HANDLERS (STUBS)
// ============================================================================

void MessageHandler::handle_move(const json& request) {
    std::cout << "[MessageHandler] MOVE request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("game_id") || !request.contains("move")) {
        send_error("MISSING_FIELD", "session_id, game_id, and move are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    int game_id = request["game_id"].get<int>();
    std::string move = request["move"].get<std::string>();
    
    // Verify player is in this game
    GameInstance* game = match_mgr->get_game(game_id);
    if (!game) {
        send_error("GAME_NOT_FOUND", "Game not found");
        return;
    }
    
    if (game->white_player_id != session->user_id && game->black_player_id != session->user_id) {
        send_error("NOT_IN_GAME", "You are not a player in this game");
        return;
    }
    
    // Attempt move
    json response;
    int opponent_id;
    if (match_mgr->make_move(game_id, session->user_id, move, response, opponent_id)) {
        send_response(response);
        std::cout << "[MessageHandler] Move executed: " << move << " in game " << game_id << std::endl;
    } else {
        json rejection;
        rejection["type"] = MessageTypes::MOVE_REJECTED;
        rejection["game_id"] = game_id;
        rejection["move"] = move;
        rejection["reason"] = "Illegal move";
        send_response(rejection);
        std::cout << "[MessageHandler] Move rejected: " << move << " in game " << game_id << std::endl;
    }
}

void MessageHandler::handle_resign(const json& request) {
    std::cout << "[MessageHandler] RESIGN request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("game_id")) {
        send_error("MISSING_FIELD", "session_id and game_id are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    int game_id = request["game_id"].get<int>();
    
    // Verify player is in this game
    GameInstance* game = match_mgr->get_game(game_id);
    if (!game) {
        send_error("GAME_NOT_FOUND", "Game not found");
        return;
    }
    
    if (game->white_player_id != session->user_id && game->black_player_id != session->user_id) {
        send_error("NOT_IN_GAME", "You are not a player in this game");
        return;
    }
    
    // Resign game
    int winner_id, loser_id;
    if (match_mgr->resign_game(game_id, session->user_id, winner_id, loser_id)) {
        json response;
        response["type"] = "RESIGN_RESPONSE";
        response["game_id"] = game_id;
        response["status"] = "success";
        response["message"] = "You resigned from the game";
        
        send_response(response);
        std::cout << "[MessageHandler] Player " << session->username << " resigned from game " << game_id << std::endl;
    } else {
        send_error("RESIGN_FAILED", "Failed to resign from game");
    }
}

void MessageHandler::handle_draw_offer(const json& request) {
    std::cout << "[MessageHandler] DRAW_OFFER request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("game_id")) {
        send_error("MISSING_FIELD", "session_id and game_id are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    int game_id = request["game_id"].get<int>();
    
    // Verify player is in this game
    GameInstance* game = match_mgr->get_game(game_id);
    if (!game) {
        send_error("GAME_NOT_FOUND", "Game not found");
        return;
    }
    
    if (game->white_player_id != session->user_id && game->black_player_id != session->user_id) {
        send_error("NOT_IN_GAME", "You are not a player in this game");
        return;
    }
    
    // Offer draw
    int opponent_id;
    if (match_mgr->offer_draw(game_id, session->user_id, opponent_id)) {
        json response;
        response["type"] = "DRAW_OFFER_RESPONSE";
        response["game_id"] = game_id;
        response["status"] = "success";
        response["message"] = "Draw offer sent to opponent";
        
        send_response(response);
        std::cout << "[MessageHandler] Draw offer sent in game " << game_id << std::endl;
    } else {
        send_error("DRAW_OFFER_FAILED", "Failed to offer draw");
    }
}

void MessageHandler::handle_draw_response(const json& request) {
    std::cout << "[MessageHandler] DRAW_RESPONSE request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("game_id") || !request.contains("accepted")) {
        send_error("MISSING_FIELD", "session_id, game_id, and accepted are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    int game_id = request["game_id"].get<int>();
    bool accepted = request["accepted"].get<bool>();
    
    // Verify player is in this game
    GameInstance* game = match_mgr->get_game(game_id);
    if (!game) {
        send_error("GAME_NOT_FOUND", "Game not found");
        return;
    }
    
    if (game->white_player_id != session->user_id && game->black_player_id != session->user_id) {
        send_error("NOT_IN_GAME", "You are not a player in this game");
        return;
    }
    
    // Respond to draw
    std::string result;
    int opponent_id;
    if (match_mgr->respond_to_draw(game_id, session->user_id, accepted, result, opponent_id)) {
        json response;
        response["type"] = "DRAW_RESPONSE_RESPONSE";
        response["game_id"] = game_id;
        response["accepted"] = accepted;
        response["result"] = result;
        response["status"] = "success";
        
        if (accepted) {
            response["message"] = "Draw accepted - game ended";
        } else {
            response["message"] = "Draw declined - game continues";
            
            // Notify opponent of draw decline
            json decline_notification;
            decline_notification["type"] = "DRAW_DECLINED";
            decline_notification["game_id"] = game_id;
            decline_notification["from_username"] = session->username;
            broadcast_to_user(opponent_id, decline_notification);
        }
        
        send_response(response);
        std::cout << "[MessageHandler] Draw " << (accepted ? "accepted" : "declined") 
                  << " in game " << game_id << std::endl;
    } else {
        send_error("DRAW_RESPONSE_FAILED", "No pending draw offer to respond to");
    }
}

void MessageHandler::handle_request_rematch(const json& request) {
    std::cout << "[MessageHandler] REQUEST_REMATCH request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("previous_game_id")) {
        send_error("MISSING_FIELD", "session_id and previous_game_id are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    int previous_game_id = request["previous_game_id"].get<int>();
    
    // Get previous game from database
    auto game_opt = GameRepository::get_game_by_id(previous_game_id);
    if (!game_opt.has_value()) {
        send_error("GAME_NOT_FOUND", "Previous game not found");
        return;
    }
    
    auto& game = game_opt.value();
    
    // Verify player was in the previous game
    if (game.white_player_id != session->user_id && game.black_player_id != session->user_id) {
        send_error("NOT_IN_GAME", "You were not a player in that game");
        return;
    }
    
    // Get opponent's user_id
    int opponent_id = (game.white_player_id == session->user_id) ? 
                      game.black_player_id : game.white_player_id;
    std::string opponent_username = (game.white_player_id == session->user_id) ? 
                                    game.black_username : game.white_username;
    
    // Check if opponent is online
    Session* opponent_session = session_mgr->get_session_by_user_id(opponent_id);
    if (!opponent_session) {
        send_error("USER_OFFLINE", "Opponent is offline");
        return;
    }
    
    // Broadcast rematch request to opponent
    json rematch_notification;
    rematch_notification["type"] = MessageTypes::REMATCH_REQUEST_RECEIVED;
    rematch_notification["from_username"] = session->username;
    rematch_notification["previous_game_id"] = previous_game_id;
    
    broadcast_to_user(opponent_id, rematch_notification);
    
    json response;
    response["type"] = "REMATCH_REQUEST_RESPONSE";
    response["status"] = "success";
    response["message"] = "Rematch request sent to " + opponent_username;
    
    send_response(response);
    std::cout << "[MessageHandler] Rematch request sent from " << session->username 
              << " to " << opponent_username << std::endl;
}

// ============================================================================
// GAME STATE HANDLERS
// ============================================================================

void MessageHandler::handle_get_game_state(const json& request) {
    std::cout << "[MessageHandler] GET_GAME_STATE request" << std::endl;
    
    if (!request.contains("session_id") || !request.contains("game_id")) {
        send_error("MISSING_FIELD", "session_id and game_id are required");
        return;
    }
    
    Session* session = validate_session(request["session_id"].get<std::string>());
    if (!session) {
        send_error("INVALID_SESSION", "Session not found or expired");
        return;
    }
    
    int game_id = request["game_id"].get<int>();
    
    // Get game state from match manager
    json state = match_mgr->get_game_state(game_id);
    
    if (state.contains("error")) {
        send_error("GAME_NOT_FOUND", "Game not found");
        return;
    }
    
    // Verify player is in this game
    GameInstance* game = match_mgr->get_game(game_id);
    if (game && game->white_player_id != session->user_id && game->black_player_id != session->user_id) {
        send_error("NOT_IN_GAME", "You are not a player in this game");
        return;
    }
    
    json response;
    response["type"] = MessageTypes::GAME_STATE;
    response["game_id"] = state["game_id"];
    response["white_player"] = state["white_player"];
    response["black_player"] = state["black_player"];
    response["current_turn"] = state["current_turn"];
    response["move_number"] = state["move_number"];
    response["move_history"] = state["move_history"];
    response["is_active"] = state["is_active"];
    response["is_ended"] = state["is_ended"];
    response["board_state"] = state["board_state"];
    
    if (state.contains("result")) {
        response["result"] = state["result"];
    }
    
    send_response(response);
    std::cout << "[MessageHandler] Sent game state for game " << game_id << std::endl;
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
