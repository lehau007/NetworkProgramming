#include "match_manager.h"
#include "../database/game_repository.h"
#include "../database/user_repository.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>

// Static member initialization
std::map<std::string, Challenge*> MatchManager::active_challenges;
std::map<int, std::string> MatchManager::challenges_by_challenger;
std::map<int, std::string> MatchManager::challenges_by_target;
std::map<int, GameInstance*> MatchManager::active_games;
std::map<int, int> MatchManager::player_to_game;
pthread_mutex_t MatchManager::mutex;
BroadcastCallback MatchManager::broadcast_callback = nullptr;
MatchManager* MatchManager::instance = nullptr;

MatchManager::MatchManager() {
    pthread_mutex_init(&mutex, nullptr);
}

MatchManager::~MatchManager() {
    pthread_mutex_destroy(&mutex);
    
    // Cleanup challenges
    for (auto& pair : active_challenges) {
        delete pair.second;
    }
    active_challenges.clear();
    
    // Cleanup games
    for (auto& pair : active_games) {
        delete pair.second->chess_engine;
        delete pair.second;
    }
    active_games.clear();
}

MatchManager* MatchManager::get_instance() {
    if (instance == nullptr) {
        instance = new MatchManager();
    }
    return instance;
}

void MatchManager::initialize() {
    get_instance();
    std::cout << "[MatchManager] Initialized" << std::endl;
}

void MatchManager::set_broadcast_callback(BroadcastCallback callback) {
    broadcast_callback = callback;
}

std::string MatchManager::generate_challenge_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << "challenge_";
    for (int i = 0; i < 16; i++) {
        ss << std::hex << dis(gen);
    }
    return ss.str();
}

void MatchManager::broadcast_to_user(int user_id, const json& message) {
    if (broadcast_callback) {
        broadcast_callback(user_id, message);
    }
}

// ============================================================================
// CHALLENGE MANAGEMENT
// ============================================================================

std::string MatchManager::create_challenge(int challenger_id, const std::string& challenger_username,
                                           int target_id, const std::string& target_username,
                                           const std::string& preferred_color) {
    pthread_mutex_lock(&mutex);
    
    std::string challenge_id = generate_challenge_id();
    
    Challenge* challenge = new Challenge();
    challenge->challenge_id = challenge_id;
    challenge->challenger_user_id = challenger_id;
    challenge->challenger_username = challenger_username;
    challenge->target_user_id = target_id;
    challenge->target_username = target_username;
    challenge->preferred_color = preferred_color;
    challenge->created_at = std::time(nullptr);
    challenge->is_active = true;
    
    active_challenges[challenge_id] = challenge;
    challenges_by_challenger[challenger_id] = challenge_id;
    challenges_by_target[target_id] = challenge_id;
    
    pthread_mutex_unlock(&mutex);
    
    std::cout << "[MatchManager] Challenge created: " << challenge_id 
              << " from " << challenger_username << " to " << target_username << std::endl;
    
    // Broadcast CHALLENGE_RECEIVED to target player
    json challenge_received;
    challenge_received["type"] = "CHALLENGE_RECEIVED";
    challenge_received["challenge_id"] = challenge_id;
    challenge_received["from_username"] = challenger_username;
    challenge_received["from_user_id"] = challenger_id;
    challenge_received["preferred_color"] = preferred_color;
    challenge_received["timestamp"] = challenge->created_at;
    
    broadcast_to_user(target_id, challenge_received);
    
    return challenge_id;
}

bool MatchManager::accept_challenge(const std::string& challenge_id, int& out_game_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = active_challenges.find(challenge_id);
    if (it == active_challenges.end() || !it->second->is_active) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    
    Challenge* challenge = it->second;
    
    // Determine colors
    int white_player_id, black_player_id;
    std::string white_username, black_username;
    
    if (challenge->preferred_color == "white") {
        white_player_id = challenge->challenger_user_id;
        white_username = challenge->challenger_username;
        black_player_id = challenge->target_user_id;
        black_username = challenge->target_username;
    } else if (challenge->preferred_color == "black") {
        white_player_id = challenge->target_user_id;
        white_username = challenge->target_username;
        black_player_id = challenge->challenger_user_id;
        black_username = challenge->challenger_username;
    } else {
        // Random assignment
        bool challenger_is_white = (std::rand() % 2 == 0);
        if (challenger_is_white) {
            white_player_id = challenge->challenger_user_id;
            white_username = challenge->challenger_username;
            black_player_id = challenge->target_user_id;
            black_username = challenge->target_username;
        } else {
            white_player_id = challenge->target_user_id;
            white_username = challenge->target_username;
            black_player_id = challenge->challenger_user_id;
            black_username = challenge->challenger_username;
        }
    }
    
    pthread_mutex_unlock(&mutex);
    
    // Create game
    out_game_id = create_game(white_player_id, white_username, black_player_id, black_username);
    
    // Cleanup challenge
    cleanup_challenge(challenge_id);
    
    // Broadcast MATCH_STARTED to both players
    json match_started_white;
    match_started_white["type"] = "MATCH_STARTED";
    match_started_white["game_id"] = out_game_id;
    match_started_white["white_player"] = white_username;
    match_started_white["black_player"] = black_username;
    match_started_white["your_color"] = "white";
    match_started_white["opponent_username"] = black_username;
    
    json match_started_black = match_started_white;
    match_started_black["your_color"] = "black";
    match_started_black["opponent_username"] = white_username;
    
    broadcast_to_user(white_player_id, match_started_white);
    broadcast_to_user(black_player_id, match_started_black);
    
    std::cout << "[MatchManager] Match started - Game ID: " << out_game_id << std::endl;
    
    return true;
}

bool MatchManager::decline_challenge(const std::string& challenge_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = active_challenges.find(challenge_id);
    if (it == active_challenges.end()) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    
    Challenge* challenge = it->second;
    int challenger_id = challenge->challenger_user_id;
    
    pthread_mutex_unlock(&mutex);
    
    // Notify challenger
    json challenge_declined;
    challenge_declined["type"] = "CHALLENGE_DECLINED";
    challenge_declined["challenge_id"] = challenge_id;
    challenge_declined["target_username"] = challenge->target_username;
    
    broadcast_to_user(challenger_id, challenge_declined);
    
    cleanup_challenge(challenge_id);
    
    std::cout << "[MatchManager] Challenge declined: " << challenge_id << std::endl;
    
    return true;
}

bool MatchManager::cancel_challenge(const std::string& challenge_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = active_challenges.find(challenge_id);
    if (it == active_challenges.end()) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    
    Challenge* challenge = it->second;
    int target_id = challenge->target_user_id;
    
    pthread_mutex_unlock(&mutex);
    
    // Notify target
    json challenge_cancelled;
    challenge_cancelled["type"] = "CHALLENGE_CANCELLED";
    challenge_cancelled["challenge_id"] = challenge_id;
    challenge_cancelled["cancelled_by"] = challenge->challenger_username;
    challenge_cancelled["reason"] = "user_cancelled";
    
    broadcast_to_user(target_id, challenge_cancelled);
    
    cleanup_challenge(challenge_id);
    
    std::cout << "[MatchManager] Challenge cancelled: " << challenge_id << std::endl;
    
    return true;
}

Challenge* MatchManager::get_challenge(const std::string& challenge_id) {
    pthread_mutex_lock(&mutex);
    auto it = active_challenges.find(challenge_id);
    Challenge* result = (it != active_challenges.end()) ? it->second : nullptr;
    pthread_mutex_unlock(&mutex);
    return result;
}

bool MatchManager::has_pending_challenge(int user_id) {
    pthread_mutex_lock(&mutex);
    bool has_challenge = (challenges_by_challenger.find(user_id) != challenges_by_challenger.end() ||
                         challenges_by_target.find(user_id) != challenges_by_target.end());
    pthread_mutex_unlock(&mutex);
    return has_challenge;
}

void MatchManager::cleanup_challenge(const std::string& challenge_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = active_challenges.find(challenge_id);
    if (it != active_challenges.end()) {
        Challenge* challenge = it->second;
        
        challenges_by_challenger.erase(challenge->challenger_user_id);
        challenges_by_target.erase(challenge->target_user_id);
        
        delete challenge;
        active_challenges.erase(it);
    }
    
    pthread_mutex_unlock(&mutex);
}

// ============================================================================
// GAME MANAGEMENT
// ============================================================================

int MatchManager::create_game(int white_player_id, const std::string& white_username,
                              int black_player_id, const std::string& black_username) {
    // Create game in database
    int game_id = GameRepository::create_game(white_player_id, black_player_id);
    
    if (game_id < 0) {
        std::cerr << "[MatchManager] Failed to create game in database" << std::endl;
        return -1;
    }
    
    pthread_mutex_lock(&mutex);
    
    GameInstance* game = new GameInstance();
    game->game_id = game_id;
    game->white_player_id = white_player_id;
    game->black_player_id = black_player_id;
    game->white_username = white_username;
    game->black_username = black_username;
    game->chess_engine = new ChessGame();
    game->start_time = std::time(nullptr);
    game->is_active = true;
    game->white_draw_offered = false;
    game->black_draw_offered = false;
    
    active_games[game_id] = game;
    player_to_game[white_player_id] = game_id;
    player_to_game[black_player_id] = game_id;
    
    pthread_mutex_unlock(&mutex);
    
    std::cout << "[MatchManager] Game created: " << game_id 
              << " - " << white_username << " (white) vs " << black_username << " (black)" << std::endl;
    
    return game_id;
}

GameInstance* MatchManager::get_game(int game_id) {
    pthread_mutex_lock(&mutex);
    auto it = active_games.find(game_id);
    GameInstance* result = (it != active_games.end()) ? it->second : nullptr;
    pthread_mutex_unlock(&mutex);
    return result;
}

GameInstance* MatchManager::get_game_by_player(int user_id) {
    pthread_mutex_lock(&mutex);
    auto it = player_to_game.find(user_id);
    if (it != player_to_game.end()) {
        int game_id = it->second;
        auto game_it = active_games.find(game_id);
        if (game_it != active_games.end()) {
            pthread_mutex_unlock(&mutex);
            return game_it->second;
        }
    }
    pthread_mutex_unlock(&mutex);
    return nullptr;
}

int MatchManager::get_game_id_by_player(int user_id) {
    pthread_mutex_lock(&mutex);
    auto it = player_to_game.find(user_id);
    int result = (it != player_to_game.end()) ? it->second : -1;
    pthread_mutex_unlock(&mutex);
    return result;
}

bool MatchManager::is_player_in_game(int user_id) {
    pthread_mutex_lock(&mutex);
    bool in_game = (player_to_game.find(user_id) != player_to_game.end());
    pthread_mutex_unlock(&mutex);
    return in_game;
}

// ============================================================================
// GAMEPLAY OPERATIONS
// ============================================================================

bool MatchManager::make_move(int game_id, int player_id, const std::string& move, 
                            json& out_response, int& out_opponent_id) {
    GameInstance* game = get_game(game_id);
    if (!game || !game->is_active) {
        return false;
    }
    
    // Verify it's player's turn
    bool is_white_turn = (game->chess_engine->getTurn() % 2 == 0);
    bool player_is_white = (player_id == game->white_player_id);
    
    if (is_white_turn != player_is_white) {
        return false;  // Not player's turn
    }
    
    // Attempt move
    pthread_mutex_lock(&mutex);
    bool move_valid = game->chess_engine->move(move);
    pthread_mutex_unlock(&mutex);
    
    if (!move_valid) {
        return false;
    }
    
    // Move successful - update database and prepare response
    pthread_mutex_lock(&mutex);
    game->move_history.push_back(move);
    GameRepository::add_move_to_game(game_id, move);
    
    // Get game state
    int turn = game->chess_engine->getTurn();
    bool is_ended = game->chess_engine->isEnded();
    GameResult result = game->chess_engine->getResult();
    
    // Check if opponent's king is in check after the move
    // After move, turn has been incremented
    // turn % 2 == 0 means it's WHITE's turn next
    // turn % 2 == 1 means it's BLACK's turn next
    // So we need to check the king of the player whose turn it is NOW
    bool next_player_is_white = (turn % 2 == 0);
    bool opponent_king_in_check = game->chess_engine->isKingInCheck(next_player_is_white);
    
    // Prepare response
    out_response["type"] = "MOVE_ACCEPTED";
    out_response["game_id"] = game_id;
    out_response["move"] = move;
    out_response["move_number"] = turn;
    out_response["is_check"] = opponent_king_in_check;
    out_response["is_checkmate"] = is_ended;
    out_response["board_state"] = game->chess_engine->getFEN();
    out_response["current_turn"] = next_player_is_white ? "white" : "black";
    
    // Prepare opponent move notification
    out_opponent_id = player_is_white ? game->black_player_id : game->white_player_id;
    
    json opponent_move;
    opponent_move["type"] = "OPPONENT_MOVE";
    opponent_move["game_id"] = game_id;
    opponent_move["move"] = move;
    opponent_move["move_number"] = turn;
    opponent_move["is_check"] = opponent_king_in_check;
    opponent_move["captured_piece"] = nullptr;
    opponent_move["timestamp"] = std::time(nullptr);
    opponent_move["board_state"] = game->chess_engine->getFEN();  
    opponent_move["current_turn"] = (game->chess_engine->getTurn() % 2 == 0) ? "white" : "black";  
    opponent_move["white_player"] = game->white_username;  
    opponent_move["black_player"] = game->black_username;  
    
    pthread_mutex_unlock(&mutex);
    
    broadcast_to_user(out_opponent_id, opponent_move);
    
    std::cout << "[MatchManager] Move executed in game " << game_id << ": " << move << std::endl;
    
    // Check if game ended
    if (is_ended) {
        std::string result_str;
        if (result == WHITE_WIN) result_str = "WHITE_WIN";
        else if (result == BLACK_WIN) result_str = "BLACK_WIN";
        else result_str = "DRAW";
        
        end_game(game_id, result_str, "checkmate");
    }
    
    return true;
}

bool MatchManager::handle_player_disconnect(int user_id) {
    int game_id = get_game_id_by_player(user_id);
    if (game_id == -1) {
        return false;  // Player not in a game
    }
    
    GameInstance* game = get_game(game_id);
    if (!game || !game->is_active) {
        return false;
    }
    
    bool player_is_white = (user_id == game->white_player_id);
    int winner_id = player_is_white ? game->black_player_id : game->white_player_id;
    std::string winner_username = player_is_white ? game->black_username : game->white_username;
    std::string loser_username = player_is_white ? game->white_username : game->black_username;
    std::string result = player_is_white ? "BLACK_WIN" : "WHITE_WIN";
    
    // Mark game as inactive
    pthread_mutex_lock(&mutex);
    game->is_active = false;
    
    // Convert moves to JSON string
    json moves_json = json::array();
    for (const auto& move : game->move_history) {
        moves_json.push_back(move);
    }
    std::string moves_str = moves_json.dump();
    pthread_mutex_unlock(&mutex);
    
    // Update database
    GameRepository::end_game(game_id, result, moves_str);
    
    // Update user stats
    if (result == "WHITE_WIN") {
        UserRepository::increment_wins(game->white_player_id);
        UserRepository::increment_losses(game->black_player_id);
    } else {
        UserRepository::increment_wins(game->black_player_id);
        UserRepository::increment_losses(game->white_player_id);
    }
    
    // Only notify the opponent (winner), NOT the disconnected player
    json game_ended;
    game_ended["type"] = "GAME_ENDED";
    game_ended["game_id"] = game_id;
    game_ended["result"] = result;
    game_ended["reason"] = "opponent_disconnected";
    game_ended["winner"] = winner_username;
    game_ended["loser"] = loser_username;
    game_ended["move_count"] = game->move_history.size();
    game_ended["duration_seconds"] = std::time(nullptr) - game->start_time;

    game_ended["white_player"] = game->white_username;
    game_ended["black_player"] = game->black_username;
    
    // Add move history (game log)
    game_ended["move_history"] = json::array();
    for (const auto& move : game->move_history) {
        game_ended["move_history"].push_back(move);
    }
    
    broadcast_to_user(winner_id, game_ended);  // Only send to winner (opponent)
    
    std::cout << "[MatchManager] Player " << user_id << " disconnected from game " << game_id 
              << ", " << winner_username << " wins" << std::endl;
    
    // Cleanup game
    cleanup_game(game_id);
    
    return true;
}

bool MatchManager::resign_game(int game_id, int player_id, int& out_winner_id, int& out_loser_id) {
    GameInstance* game = get_game(game_id);
    if (!game || !game->is_active) {
        return false;
    }
    
    bool player_is_white = (player_id == game->white_player_id);
    
    out_loser_id = player_id;
    out_winner_id = player_is_white ? game->black_player_id : game->white_player_id;
    
    std::string result = player_is_white ? "BLACK_WIN" : "WHITE_WIN";
    end_game(game_id, result, "resignation");
    
    std::cout << "[MatchManager] Player " << player_id << " resigned game " << game_id << std::endl;
    
    return true;
}

bool MatchManager::offer_draw(int game_id, int player_id, int& out_opponent_id) {
    GameInstance* game = get_game(game_id);
    if (!game || !game->is_active) {
        return false;
    }
    
    pthread_mutex_lock(&mutex);
    
    bool player_is_white = (player_id == game->white_player_id);
    if (player_is_white) {
        game->white_draw_offered = true;
    } else {
        game->black_draw_offered = true;
    }
    
    out_opponent_id = player_is_white ? game->black_player_id : game->white_player_id;
    std::string offering_player = player_is_white ? game->white_username : game->black_username;
    
    pthread_mutex_unlock(&mutex);
    
    // Notify opponent
    json draw_offer_received;
    draw_offer_received["type"] = "DRAW_OFFER_RECEIVED";
    draw_offer_received["game_id"] = game_id;
    draw_offer_received["from_username"] = offering_player;
    draw_offer_received["timestamp"] = std::time(nullptr);
    
    broadcast_to_user(out_opponent_id, draw_offer_received);
    
    std::cout << "[MatchManager] Draw offer in game " << game_id << " from player " << player_id << std::endl;
    
    return true;
}

bool MatchManager::respond_to_draw(int game_id, int player_id, bool accepted, 
                                   std::string& out_result, int& out_opponent_id) {
    GameInstance* game = get_game(game_id);
    if (!game || !game->is_active) {
        return false;
    }
    
    pthread_mutex_lock(&mutex);
    
    bool player_is_white = (player_id == game->white_player_id);
    out_opponent_id = player_is_white ? game->black_player_id : game->white_player_id;
    
    // Check if opponent offered draw
    bool opponent_offered = player_is_white ? game->black_draw_offered : game->white_draw_offered;
    
    if (!opponent_offered) {
        pthread_mutex_unlock(&mutex);
        return false;  // No draw offer to respond to
    }
    
    // Clear draw offers
    game->white_draw_offered = false;
    game->black_draw_offered = false;
    
    pthread_mutex_unlock(&mutex);
    
    if (accepted) {
        out_result = "DRAW";
        end_game(game_id, "DRAW", "draw_agreement");
        std::cout << "[MatchManager] Draw accepted in game " << game_id << std::endl;
    } else {
        out_result = "DECLINED";
        std::cout << "[MatchManager] Draw declined in game " << game_id << std::endl;
    }
    
    return true;
}

// ============================================================================
// GAME STATE
// ============================================================================

json MatchManager::get_game_state(int game_id) {
    GameInstance* game = get_game(game_id);
    
    json state;
    if (!game) {
        state["error"] = "Game not found";
        return state;
    }
    
    pthread_mutex_lock(&mutex);
    
    state["game_id"] = game_id;
    state["white_player"] = game->white_username;
    state["black_player"] = game->black_username;
    state["current_turn"] = (game->chess_engine->getTurn() % 2 == 0) ? "white" : "black";
    state["move_number"] = game->chess_engine->getTurn();
    state["is_active"] = game->is_active;
    state["is_ended"] = game->chess_engine->isEnded();
    state["board_state"] = game->chess_engine->getFEN();
    
    // Move history
    state["move_history"] = json::array();
    for (const auto& move : game->move_history) {
        state["move_history"].push_back(move);
    }
    
    // Game result
    if (game->chess_engine->isEnded()) {
        GameResult result = game->chess_engine->getResult();
        if (result == WHITE_WIN) state["result"] = "WHITE_WIN";
        else if (result == BLACK_WIN) state["result"] = "BLACK_WIN";
        else state["result"] = "DRAW";
    }
    
    pthread_mutex_unlock(&mutex);
    
    return state;
}

std::string MatchManager::get_board_fen(int game_id) {
    // TODO: Implement FEN generation from ChessGame
    return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
}

std::vector<std::string> MatchManager::get_move_history(int game_id) {
    GameInstance* game = get_game(game_id);
    if (!game) {
        return std::vector<std::string>();
    }
    
    pthread_mutex_lock(&mutex);
    std::vector<std::string> history = game->move_history;
    pthread_mutex_unlock(&mutex);
    
    return history;
}

// ============================================================================
// END GAME
// ============================================================================

void MatchManager::end_game(int game_id, const std::string& result, const std::string& reason) {
    GameInstance* game = get_game(game_id);
    if (!game) {
        return;
    }
    
    pthread_mutex_lock(&mutex);
    game->is_active = false;
    
    // Convert moves to JSON string
    json moves_json = json::array();
    for (const auto& move : game->move_history) {
        moves_json.push_back(move);
    }
    std::string moves_str = moves_json.dump();
    
    int white_id = game->white_player_id;
    int black_id = game->black_player_id;
    std::string white_username = game->white_username;
    std::string black_username = game->black_username;
    
    pthread_mutex_unlock(&mutex);
    
    // Update database
    GameRepository::end_game(game_id, result, moves_str);
    
    // Update user stats
    if (result == "WHITE_WIN") {
        UserRepository::increment_wins(white_id);
        auto white_user = UserRepository::get_user_by_id(white_id);
        if (white_user.has_value()) {
            UserRepository::update_rating(white_id, white_user.value().rating + 3);
        }
        
        auto black_user = UserRepository::get_user_by_id(black_id);
        if (black_user.has_value()) {
            UserRepository::update_rating(black_id, black_user.value().rating - 3);
        }
        UserRepository::increment_losses(black_id);
    } else if (result == "BLACK_WIN") {
        auto black_user = UserRepository::get_user_by_id(black_id);
        if (black_user.has_value()) {
            UserRepository::update_rating(black_id, black_user.value().rating + 3);
        }
        UserRepository::increment_wins(black_id);

        auto white_user = UserRepository::get_user_by_id(white_id);
        if (white_user.has_value()) {
            UserRepository::update_rating(white_id, white_user.value().rating - 3);
        }
        UserRepository::increment_losses(white_id);
    } else {
        UserRepository::increment_draws(white_id);
        UserRepository::increment_draws(black_id);
    }
    
    // Broadcast GAME_ENDED to both players
    json game_ended;
    game_ended["type"] = "GAME_ENDED";
    game_ended["game_id"] = game_id;
    game_ended["result"] = result;
    game_ended["reason"] = reason;
    
    if (result == "WHITE_WIN") {
        game_ended["winner"] = white_username;
        game_ended["loser"] = black_username;
    } else if (result == "BLACK_WIN") {
        game_ended["winner"] = black_username;
        game_ended["loser"] = white_username;
    }
    
    game_ended["move_count"] = game->move_history.size();
    game_ended["duration_seconds"] = std::time(nullptr) - game->start_time;
    game_ended["white_player"] = white_username;
    game_ended["black_player"] = black_username;
    
    // Add move history (game log)
    game_ended["move_history"] = json::array();
    for (const auto& move : game->move_history) {
        game_ended["move_history"].push_back(move);
    }
    
    broadcast_to_user(white_id, game_ended);
    broadcast_to_user(black_id, game_ended);
    
    std::cout << "[MatchManager] Game ended: " << game_id << " - " << result << " (" << reason << ")" << std::endl;
    
    // Cleanup after a delay (or immediately)
    cleanup_game(game_id);
}

void MatchManager::cleanup_game(int game_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = active_games.find(game_id);
    if (it != active_games.end()) {
        GameInstance* game = it->second;
        
        player_to_game.erase(game->white_player_id);
        player_to_game.erase(game->black_player_id);
        
        delete game->chess_engine;
        delete game;
        active_games.erase(it);
        
        std::cout << "[MatchManager] Cleaned up game: " << game_id << std::endl;
    }
    
    pthread_mutex_unlock(&mutex);
}

// ============================================================================
// UTILITY
// ============================================================================

int MatchManager::get_active_game_count() {
    pthread_mutex_lock(&mutex);
    int count = active_games.size();
    pthread_mutex_unlock(&mutex);
    return count;
}

int MatchManager::get_pending_challenge_count() {
    pthread_mutex_lock(&mutex);
    int count = active_challenges.size();
    pthread_mutex_unlock(&mutex);
    return count;
}
