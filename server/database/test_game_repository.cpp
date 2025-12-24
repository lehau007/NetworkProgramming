#include "game_repository.h"
#include <iostream>

int main() {
    std::cout << "=== Game Repository Test ===" << std::endl << std::endl;
    
    // Test 1: Create game
    std::cout << "Test 1: Creating new game..." << std::endl;
    int game_id = GameRepository::create_game(1, 2);  // Alice vs Bob
    if (game_id > 0) {
        std::cout << "✓ Game created with ID: " << game_id << std::endl;
    } else {
        std::cout << "✗ Failed to create game" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 2: Add moves to game
    std::cout << "Test 2: Adding moves to game..." << std::endl;
    if (GameRepository::add_move_to_game(game_id, "e2e4")) {
        std::cout << "✓ Move 1 added: e2e4" << std::endl;
    }
    if (GameRepository::add_move_to_game(game_id, "e7e5")) {
        std::cout << "✓ Move 2 added: e7e5" << std::endl;
    }
    if (GameRepository::add_move_to_game(game_id, "Ng1f3")) {
        std::cout << "✓ Move 3 added: Ng1f3" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 3: Get game moves
    std::cout << "Test 3: Retrieving game moves..." << std::endl;
    std::string moves = GameRepository::get_game_moves(game_id);
    std::cout << "✓ Current moves: " << moves << std::endl;
    std::cout << std::endl;
    
    // Test 4: End game
    std::cout << "Test 4: Ending game with result..." << std::endl;
    std::string final_moves = "[\"e2e4\",\"e7e5\",\"Ng1f3\",\"Nb8c6\",\"Bf1c4\"]";
    if (GameRepository::end_game(game_id, "WHITE_WIN", final_moves)) {
        std::cout << "✓ Game ended successfully" << std::endl;
    } else {
        std::cout << "✗ Failed to end game" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 5: Get game by ID
    std::cout << "Test 5: Retrieving game by ID..." << std::endl;
    auto game = GameRepository::get_game_by_id(game_id);
    if (game.has_value()) {
        std::cout << "✓ Game found:" << std::endl;
        std::cout << "  - Game ID: " << game->game_id << std::endl;
        std::cout << "  - White: " << game->white_username << " (ID: " << game->white_player_id << ")" << std::endl;
        std::cout << "  - Black: " << game->black_username << " (ID: " << game->black_player_id << ")" << std::endl;
        std::cout << "  - Result: " << game->result << std::endl;
        std::cout << "  - Duration: " << game->duration << " seconds" << std::endl;
        std::cout << "  - Moves: " << game->moves << std::endl;
    } else {
        std::cout << "✗ Game not found" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 6: Get user games
    std::cout << "Test 6: Getting all games for user (Alice)..." << std::endl;
    auto user_games = GameRepository::get_user_games(1, 10);  // Alice's ID = 1
    std::cout << "✓ Found " << user_games.size() << " games for Alice:" << std::endl;
    for (const auto& g : user_games) {
        std::cout << "  - Game #" << g.game_id << ": " 
                  << g.white_username << " vs " << g.black_username 
                  << " → " << g.result << std::endl;
    }
    std::cout << std::endl;
    
    // Test 7: Get recent games
    std::cout << "Test 7: Getting recent games..." << std::endl;
    auto recent_games = GameRepository::get_recent_games(5);
    std::cout << "✓ Recent " << recent_games.size() << " games:" << std::endl;
    for (const auto& g : recent_games) {
        std::cout << "  - " << g.white_username << " vs " << g.black_username 
                  << " → " << g.result 
                  << " (Started: " << g.start_time << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 8: Get user game statistics
    std::cout << "Test 8: Getting game statistics for Alice..." << std::endl;
    auto stats = GameRepository::get_user_game_stats(1);
    std::cout << "✓ Alice's game statistics:" << std::endl;
    std::cout << "  - Total games: " << stats.total_games << std::endl;
    std::cout << "  - Wins: " << stats.wins << std::endl;
    std::cout << "  - Losses: " << stats.losses << std::endl;
    std::cout << "  - Draws: " << stats.draws << std::endl;
    std::cout << "  - Games as White: " << stats.games_as_white << std::endl;
    std::cout << "  - Games as Black: " << stats.games_as_black << std::endl;
    std::cout << std::endl;
    
    // Test 9: Get games between two players
    std::cout << "Test 9: Getting games between Alice and Charlie..." << std::endl;
    auto head_to_head = GameRepository::get_games_between_players(1, 3);
    std::cout << "✓ Found " << head_to_head.size() << " games between them:" << std::endl;
    for (const auto& g : head_to_head) {
        std::cout << "  - Game #" << g.game_id << ": " 
                  << g.white_username << " vs " << g.black_username 
                  << " → " << g.result << std::endl;
    }
    std::cout << std::endl;
    
    // Test 10: Check if game exists
    std::cout << "Test 10: Checking if game exists..." << std::endl;
    bool exists = GameRepository::game_exists(game_id);
    std::cout << (exists ? "✓" : "✗") << " Game #" << game_id 
              << (exists ? " exists" : " does not exist") << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== All Tests Complete ===" << std::endl;
    
    return 0;
}
