#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <pthread.h>
#include <ctime>
#include <functional>
#include <nlohmann/json.hpp>
#include "chess_game.cpp"

using json = nlohmann::json;

// Challenge structure
struct Challenge {
    std::string challenge_id;
    int challenger_user_id;
    std::string challenger_username;
    int target_user_id;
    std::string target_username;
    std::string preferred_color;  // "white", "black", or "random"
    time_t created_at;
    bool is_active;
};

// Active game instance
struct GameInstance {
    int game_id;
    int white_player_id;
    int black_player_id;
    std::string white_username;
    std::string black_username;
    ChessGame* chess_engine;
    std::vector<std::string> move_history;
    time_t start_time;
    bool is_active;
    bool white_draw_offered;
    bool black_draw_offered;
    int ai_depth;  // Only used when one player is AI (-1)
};

// Callback for broadcasting messages
using BroadcastCallback = std::function<void(int user_id, const json& message)>;

class MatchManager {
private:
    static constexpr int AI_USER_ID = -1;

    // Active challenges and games
    static std::map<std::string, Challenge*> active_challenges;     // challenge_id -> Challenge
    static std::map<int, std::string> challenges_by_challenger;     // user_id -> challenge_id
    static std::map<int, std::string> challenges_by_target;         // user_id -> challenge_id
    
    static std::map<int, GameInstance*> active_games;               // game_id -> GameInstance
    static std::map<int, int> player_to_game;                       // user_id -> game_id
    
    static pthread_mutex_t mutex;
    static BroadcastCallback broadcast_callback;
    static MatchManager* instance;
    
    // Generate unique challenge ID
    std::string generate_challenge_id();
    
    // Helper to broadcast to a user
    void broadcast_to_user(int user_id, const json& message);
    
    MatchManager();
    
public:
    ~MatchManager();
    
    // Singleton accessor
    static MatchManager* get_instance();
    
    // Initialize
    static void initialize();
    static void set_broadcast_callback(BroadcastCallback callback);
    
    // Challenge management
    std::string create_challenge(int challenger_id, const std::string& challenger_username,
                                 int target_id, const std::string& target_username,
                                 const std::string& preferred_color = "random");
    bool accept_challenge(const std::string& challenge_id, int& out_game_id);
    bool decline_challenge(const std::string& challenge_id);
    bool cancel_challenge(const std::string& challenge_id);
    bool accept_ai_challenge(int human_user_id, const std::string& human_username,
                             const std::string& preferred_color, int ai_depth,
                             int& out_game_id);
    
    Challenge* get_challenge(const std::string& challenge_id);
    bool has_pending_challenge(int user_id);  // Either sent or received
    void cleanup_challenge(const std::string& challenge_id);
    
    // Game management
    int create_game(int white_player_id, const std::string& white_username,
                    int black_player_id, const std::string& black_username);
    GameInstance* get_game(int game_id);
    GameInstance* get_game_by_player(int user_id);
    int get_game_id_by_player(int user_id);
    bool is_player_in_game(int user_id);
    
    // Gameplay operations
    bool make_move(int game_id, int player_id, const std::string& move, 
                   json& out_response, int& out_opponent_id);
    bool resign_game(int game_id, int player_id, int& out_winner_id, int& out_loser_id);
    bool handle_player_disconnect(int user_id);
    bool offer_draw(int game_id, int player_id, int& out_opponent_id);
    bool respond_to_draw(int game_id, int player_id, bool accepted, 
                        std::string& out_result, int& out_opponent_id);
    
    // Game state
    json get_game_state(int game_id);
    std::string get_board_fen(int game_id);
    std::vector<std::string> get_move_history(int game_id);
    
    // End game
    void end_game(int game_id, const std::string& result, const std::string& reason);
    void cleanup_game(int game_id);
    
    // Utility
    int get_active_game_count();
    int get_pending_challenge_count();
};

#endif // MATCH_MANAGER_H
