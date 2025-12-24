#ifndef GAME_REPOSITORY_H
#define GAME_REPOSITORY_H

#include <string>
#include <optional>
#include <vector>
#include <pqxx/pqxx>

struct GameRecord {
    int game_id;
    int white_player_id;
    int black_player_id;
    std::string white_username;
    std::string black_username;
    std::string result;  // 'WHITE_WIN', 'BLACK_WIN', 'DRAW', 'ABORTED'
    std::string moves;   // JSON array of moves
    std::string start_time;
    std::string end_time;
    int duration;        // in seconds
};

class GameRepository {
public:
    // Create new game (returns game_id if successful, -1 if failed)
    static int create_game(int white_player_id, int black_player_id);
    
    // Update game result and moves
    static bool update_game_result(int game_id, const std::string& result, const std::string& moves_json);
    
    // End game (sets end_time and calculates duration)
    static bool end_game(int game_id, const std::string& result, const std::string& moves_json);
    
    // Get game by ID
    static std::optional<GameRecord> get_game_by_id(int game_id);
    
    // Get all games for a user
    static std::vector<GameRecord> get_user_games(int user_id, int limit = 20);
    
    // Get recent games
    static std::vector<GameRecord> get_recent_games(int limit = 10);
    
    // Get game statistics for a user
    struct UserGameStats {
        int total_games;
        int wins;
        int losses;
        int draws;
        int games_as_white;
        int games_as_black;
    };
    static UserGameStats get_user_game_stats(int user_id);
    
    // Get games between two players
    static std::vector<GameRecord> get_games_between_players(int player1_id, int player2_id);
    
    // Add move to game
    static bool add_move_to_game(int game_id, const std::string& move);
    
    // Get current game moves
    static std::string get_game_moves(int game_id);
    
    // Check if game exists
    static bool game_exists(int game_id);
    
    // Delete game
    static bool delete_game(int game_id);
};

#endif // GAME_REPOSITORY_H
