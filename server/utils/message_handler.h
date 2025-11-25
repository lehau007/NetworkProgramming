#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include <string>
#include <nlohmann/json.hpp>
#include "../session/session_manager.h"
#include "../database/user_repository.h"
#include "../database/game_repository.h"
#include "../game/match_manager.h"

using json = nlohmann::json;

class MessageHandler {
private:
    SessionManager* session_mgr;
    MatchManager* match_mgr;
    int client_socket;
    
    // Helper methods
    void send_response(const json& response);
    void send_error(const std::string& error_code, const std::string& message, const std::string& severity = "error");
    Session* validate_session(const std::string& session_id);
    void broadcast_to_user(int user_id, const json& message);
    
public:
    MessageHandler(int socket);
    ~MessageHandler();
    
    // Main message dispatcher
    void handle_message(const std::string& message_str);
    
    // Connection & Session handlers
    void handle_verify_session(const json& request);
    
    // Authentication handlers
    void handle_login(const json& request);
    void handle_register(const json& request);
    void handle_logout(const json& request);
    
    // Lobby handlers
    void handle_get_available_players(const json& request);
    
    // Matchmaking handlers
    void handle_challenge(const json& request);
    void handle_accept_challenge(const json& request);
    void handle_decline_challenge(const json& request);
    void handle_cancel_challenge(const json& request);
    
    // Gameplay handlers
    void handle_move(const json& request);
    void handle_resign(const json& request);
    void handle_draw_offer(const json& request);
    void handle_draw_response(const json& request);
    void handle_request_rematch(const json& request);
    
    // Game state handlers
    void handle_get_game_state(const json& request);
    void handle_get_game_history(const json& request);
    void handle_get_leaderboard(const json& request);
    
    // System handlers
    void handle_ping(const json& request);
    void handle_chat_message(const json& request);
};

#endif // MESSAGE_HANDLER_H
