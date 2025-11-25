#include "user_repository.h"
#include <iostream>

int main() {
    std::cout << "=== User Repository Test ===" << std::endl << std::endl;
    
    // Test 1: Create user
    std::cout << "Test 1: Creating new user..." << std::endl;
    int new_user_id = UserRepository::create_user("testuser", "hashed_password_123", "test@example.com");
    if (new_user_id > 0) {
        std::cout << "✓ User created with ID: " << new_user_id << std::endl;
    } else {
        std::cout << "✗ Failed to create user" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 2: Get user by ID
    std::cout << "Test 2: Retrieving user by ID..." << std::endl;
    auto user = UserRepository::get_user_by_id(new_user_id);
    if (user.has_value()) {
        std::cout << "✓ User found:" << std::endl;
        std::cout << "  - Username: " << user->username << std::endl;
        std::cout << "  - Email: " << user->email << std::endl;
        std::cout << "  - Rating: " << user->rating << std::endl;
        std::cout << "  - W/L/D: " << user->wins << "/" << user->losses << "/" << user->draws << std::endl;
    } else {
        std::cout << "✗ User not found" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 3: Get user by username
    std::cout << "Test 3: Retrieving user by username..." << std::endl;
    auto user2 = UserRepository::get_user_by_username("alice");
    if (user2.has_value()) {
        std::cout << "✓ User 'alice' found:" << std::endl;
        std::cout << "  - ID: " << user2->user_id << std::endl;
        std::cout << "  - Rating: " << user2->rating << std::endl;
        std::cout << "  - W/L/D: " << user2->wins << "/" << user2->losses << "/" << user2->draws << std::endl;
    } else {
        std::cout << "✗ User not found" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 4: Authenticate user
    std::cout << "Test 4: Authenticating user..." << std::endl;
    int auth_id = UserRepository::authenticate_user("alice", "hash_alice_123");
    if (auth_id > 0) {
        std::cout << "✓ Authentication successful! User ID: " << auth_id << std::endl;
    } else {
        std::cout << "✗ Authentication failed" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 5: Update user stats
    std::cout << "Test 5: Incrementing wins..." << std::endl;
    if (UserRepository::increment_wins(1)) {
        std::cout << "✓ Win count incremented" << std::endl;
        auto updated = UserRepository::get_user_by_id(1);
        if (updated.has_value()) {
            std::cout << "  - New wins: " << updated->wins << std::endl;
        }
    } else {
        std::cout << "✗ Failed to update wins" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 6: Get top users
    std::cout << "Test 6: Getting top users by rating..." << std::endl;
    auto top_users = UserRepository::get_top_users(5);
    std::cout << "✓ Top " << top_users.size() << " users:" << std::endl;
    int rank = 1;
    for (const auto& u : top_users) {
        std::cout << "  " << rank++ << ". " << u.username 
                  << " (Rating: " << u.rating << ", W/L/D: " 
                  << u.wins << "/" << u.losses << "/" << u.draws << ")" << std::endl;
    }
    std::cout << std::endl;
    
    // Test 7: Check username exists
    std::cout << "Test 7: Checking if username exists..." << std::endl;
    bool exists = UserRepository::username_exists("alice");
    std::cout << (exists ? "✓" : "✗") << " Username 'alice' " 
              << (exists ? "exists" : "does not exist") << std::endl;
    
    bool not_exists = UserRepository::username_exists("nonexistent_user_xyz");
    std::cout << (!not_exists ? "✓" : "✗") << " Username 'nonexistent_user_xyz' " 
              << (!not_exists ? "does not exist" : "exists") << std::endl;
    std::cout << std::endl;
    
    std::cout << "=== All Tests Complete ===" << std::endl;
    
    return 0;
}
