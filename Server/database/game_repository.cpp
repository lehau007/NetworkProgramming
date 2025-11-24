#include "game_repository.h"
#include "database_connection.cpp"
#include <iostream>
#include <sstream>

// Create new game
int GameRepository::create_game(int white_player_id, int black_player_id) {
    try {
        std::string query = "INSERT INTO game_history (white_player_id, black_player_id, start_time, moves) "
                           "VALUES (" + std::to_string(white_player_id) + ", " 
                           + std::to_string(black_player_id) + ", NOW(), '[]') "
                           "RETURNING game_id";
        
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            return result[0]["game_id"].as<int>();
        }
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Error creating game: " << e.what() << std::endl;
        return -1;
    }
}

// Update game result and moves
bool GameRepository::update_game_result(int game_id, const std::string& result, const std::string& moves_json) {
    try {
        std::string query = "UPDATE game_history SET result = '" + result 
                           + "', moves = '" + moves_json + "' "
                           "WHERE game_id = " + std::to_string(game_id);
        
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error updating game result: " << e.what() << std::endl;
        return false;
    }
}

// End game
bool GameRepository::end_game(int game_id, const std::string& result, const std::string& moves_json) {
    try {
        std::string query = "UPDATE game_history SET "
                           "result = '" + result + "', "
                           "moves = '" + moves_json + "', "
                           "end_time = NOW(), "
                           "duration = EXTRACT(EPOCH FROM (NOW() - start_time))::INT "
                           "WHERE game_id = " + std::to_string(game_id);
        
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error ending game: " << e.what() << std::endl;
        return false;
    }
}

// Get game by ID
std::optional<GameRecord> GameRepository::get_game_by_id(int game_id) {
    try {
        std::string query = "SELECT g.game_id, g.white_player_id, g.black_player_id, "
                           "u1.username as white_username, u2.username as black_username, "
                           "g.result, g.moves, g.start_time, g.end_time, g.duration "
                           "FROM game_history g "
                           "LEFT JOIN users u1 ON g.white_player_id = u1.user_id "
                           "LEFT JOIN users u2 ON g.black_player_id = u2.user_id "
                           "WHERE g.game_id = " + std::to_string(game_id);
        
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            GameRecord game;
            game.game_id = result[0]["game_id"].as<int>();
            game.white_player_id = result[0]["white_player_id"].as<int>();
            game.black_player_id = result[0]["black_player_id"].as<int>();
            game.white_username = result[0]["white_username"].c_str();
            game.black_username = result[0]["black_username"].c_str();
            game.result = result[0]["result"].is_null() ? "" : result[0]["result"].c_str();
            game.moves = result[0]["moves"].is_null() ? "[]" : result[0]["moves"].c_str();
            game.start_time = result[0]["start_time"].c_str();
            game.end_time = result[0]["end_time"].is_null() ? "" : result[0]["end_time"].c_str();
            game.duration = result[0]["duration"].is_null() ? 0 : result[0]["duration"].as<int>();
            return game;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Error getting game by ID: " << e.what() << std::endl;
        return std::nullopt;
    }
}

// Get all games for a user
std::vector<GameRecord> GameRepository::get_user_games(int user_id, int limit) {
    std::vector<GameRecord> games;
    try {
        std::string query = "SELECT g.game_id, g.white_player_id, g.black_player_id, "
                           "u1.username as white_username, u2.username as black_username, "
                           "g.result, g.moves, g.start_time, g.end_time, g.duration "
                           "FROM game_history g "
                           "LEFT JOIN users u1 ON g.white_player_id = u1.user_id "
                           "LEFT JOIN users u2 ON g.black_player_id = u2.user_id "
                           "WHERE g.white_player_id = " + std::to_string(user_id) 
                           + " OR g.black_player_id = " + std::to_string(user_id) 
                           + " ORDER BY g.start_time DESC LIMIT " + std::to_string(limit);
        
        auto result = DatabaseConnection::execute_query(query);
        
        for (const auto& row : result) {
            GameRecord game;
            game.game_id = row["game_id"].as<int>();
            game.white_player_id = row["white_player_id"].as<int>();
            game.black_player_id = row["black_player_id"].as<int>();
            game.white_username = row["white_username"].c_str();
            game.black_username = row["black_username"].c_str();
            game.result = row["result"].is_null() ? "" : row["result"].c_str();
            game.moves = row["moves"].is_null() ? "[]" : row["moves"].c_str();
            game.start_time = row["start_time"].c_str();
            game.end_time = row["end_time"].is_null() ? "" : row["end_time"].c_str();
            game.duration = row["duration"].is_null() ? 0 : row["duration"].as<int>();
            games.push_back(game);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting user games: " << e.what() << std::endl;
    }
    return games;
}

// Get recent games
std::vector<GameRecord> GameRepository::get_recent_games(int limit) {
    std::vector<GameRecord> games;
    try {
        std::string query = "SELECT g.game_id, g.white_player_id, g.black_player_id, "
                           "u1.username as white_username, u2.username as black_username, "
                           "g.result, g.moves, g.start_time, g.end_time, g.duration "
                           "FROM game_history g "
                           "LEFT JOIN users u1 ON g.white_player_id = u1.user_id "
                           "LEFT JOIN users u2 ON g.black_player_id = u2.user_id "
                           "ORDER BY g.start_time DESC LIMIT " + std::to_string(limit);
        
        auto result = DatabaseConnection::execute_query(query);
        
        for (const auto& row : result) {
            GameRecord game;
            game.game_id = row["game_id"].as<int>();
            game.white_player_id = row["white_player_id"].as<int>();
            game.black_player_id = row["black_player_id"].as<int>();
            game.white_username = row["white_username"].c_str();
            game.black_username = row["black_username"].c_str();
            game.result = row["result"].is_null() ? "" : row["result"].c_str();
            game.moves = row["moves"].is_null() ? "[]" : row["moves"].c_str();
            game.start_time = row["start_time"].c_str();
            game.end_time = row["end_time"].is_null() ? "" : row["end_time"].c_str();
            game.duration = row["duration"].is_null() ? 0 : row["duration"].as<int>();
            games.push_back(game);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting recent games: " << e.what() << std::endl;
    }
    return games;
}

// Get game statistics for a user
GameRepository::UserGameStats GameRepository::get_user_game_stats(int user_id) {
    UserGameStats stats = {0, 0, 0, 0, 0, 0};
    try {
        std::string query = "SELECT "
                           "COUNT(*) as total_games, "
                           "SUM(CASE WHEN (result = 'WHITE_WIN' AND white_player_id = " + std::to_string(user_id) 
                           + ") OR (result = 'BLACK_WIN' AND black_player_id = " + std::to_string(user_id) 
                           + ") THEN 1 ELSE 0 END) as wins, "
                           "SUM(CASE WHEN (result = 'WHITE_WIN' AND black_player_id = " + std::to_string(user_id) 
                           + ") OR (result = 'BLACK_WIN' AND white_player_id = " + std::to_string(user_id) 
                           + ") THEN 1 ELSE 0 END) as losses, "
                           "SUM(CASE WHEN result = 'DRAW' THEN 1 ELSE 0 END) as draws, "
                           "SUM(CASE WHEN white_player_id = " + std::to_string(user_id) + " THEN 1 ELSE 0 END) as games_as_white, "
                           "SUM(CASE WHEN black_player_id = " + std::to_string(user_id) + " THEN 1 ELSE 0 END) as games_as_black "
                           "FROM game_history "
                           "WHERE white_player_id = " + std::to_string(user_id) 
                           + " OR black_player_id = " + std::to_string(user_id);
        
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            stats.total_games = result[0]["total_games"].as<int>();
            stats.wins = result[0]["wins"].is_null() ? 0 : result[0]["wins"].as<int>();
            stats.losses = result[0]["losses"].is_null() ? 0 : result[0]["losses"].as<int>();
            stats.draws = result[0]["draws"].is_null() ? 0 : result[0]["draws"].as<int>();
            stats.games_as_white = result[0]["games_as_white"].is_null() ? 0 : result[0]["games_as_white"].as<int>();
            stats.games_as_black = result[0]["games_as_black"].is_null() ? 0 : result[0]["games_as_black"].as<int>();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting user game stats: " << e.what() << std::endl;
    }
    return stats;
}

// Get games between two players
std::vector<GameRecord> GameRepository::get_games_between_players(int player1_id, int player2_id) {
    std::vector<GameRecord> games;
    try {
        std::string query = "SELECT g.game_id, g.white_player_id, g.black_player_id, "
                           "u1.username as white_username, u2.username as black_username, "
                           "g.result, g.moves, g.start_time, g.end_time, g.duration "
                           "FROM game_history g "
                           "LEFT JOIN users u1 ON g.white_player_id = u1.user_id "
                           "LEFT JOIN users u2 ON g.black_player_id = u2.user_id "
                           "WHERE (g.white_player_id = " + std::to_string(player1_id) 
                           + " AND g.black_player_id = " + std::to_string(player2_id) + ") "
                           "OR (g.white_player_id = " + std::to_string(player2_id) 
                           + " AND g.black_player_id = " + std::to_string(player1_id) + ") "
                           "ORDER BY g.start_time DESC";
        
        auto result = DatabaseConnection::execute_query(query);
        
        for (const auto& row : result) {
            GameRecord game;
            game.game_id = row["game_id"].as<int>();
            game.white_player_id = row["white_player_id"].as<int>();
            game.black_player_id = row["black_player_id"].as<int>();
            game.white_username = row["white_username"].c_str();
            game.black_username = row["black_username"].c_str();
            game.result = row["result"].is_null() ? "" : row["result"].c_str();
            game.moves = row["moves"].is_null() ? "[]" : row["moves"].c_str();
            game.start_time = row["start_time"].c_str();
            game.end_time = row["end_time"].is_null() ? "" : row["end_time"].c_str();
            game.duration = row["duration"].is_null() ? 0 : row["duration"].as<int>();
            games.push_back(game);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting games between players: " << e.what() << std::endl;
    }
    return games;
}

// Add move to game
bool GameRepository::add_move_to_game(int game_id, const std::string& move) {
    try {
        // Get current moves
        std::string current_moves = get_game_moves(game_id);
        
        // Parse and append new move (simplified - you may want better JSON handling)
        std::string updated_moves = current_moves;
        if (current_moves == "[]" || current_moves.empty()) {
            updated_moves = "[\"" + move + "\"]";
        } else {
            // Remove closing bracket, add comma and new move, add closing bracket
            updated_moves.pop_back(); // Remove ]
            updated_moves += ",\"" + move + "\"]";
        }
        
        std::string query = "UPDATE game_history SET moves = '" + updated_moves 
                           + "' WHERE game_id = " + std::to_string(game_id);
        
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error adding move to game: " << e.what() << std::endl;
        return false;
    }
}

// Get current game moves
std::string GameRepository::get_game_moves(int game_id) {
    try {
        std::string query = "SELECT moves FROM game_history WHERE game_id = " + std::to_string(game_id);
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            return result[0]["moves"].is_null() ? "[]" : result[0]["moves"].c_str();
        }
        return "[]";
    } catch (const std::exception& e) {
        std::cerr << "Error getting game moves: " << e.what() << std::endl;
        return "[]";
    }
}

// Check if game exists
bool GameRepository::game_exists(int game_id) {
    try {
        std::string query = "SELECT COUNT(*) as count FROM game_history WHERE game_id = " + std::to_string(game_id);
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            return result[0]["count"].as<int>() > 0;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error checking game exists: " << e.what() << std::endl;
        return false;
    }
}

// Delete game
bool GameRepository::delete_game(int game_id) {
    try {
        std::string query = "DELETE FROM game_history WHERE game_id = " + std::to_string(game_id);
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting game: " << e.what() << std::endl;
        return false;
    }
}
