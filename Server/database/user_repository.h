#ifndef USER_REPOSITORY_H
#define USER_REPOSITORY_H

#include <string>
#include <optional>
#include <vector>
#include <pqxx/pqxx>

struct User {
    int user_id;
    std::string username;
    std::string email;
    std::string created_at;
    int wins;
    int losses;
    int draws;
    int rating;
};

class UserRepository {
public:
    // Create new user (returns user_id if successful, -1 if failed)
    static int create_user(const std::string& username, const std::string& password_hash, const std::string& email = "");
    
    // Retrieve user by ID
    static std::optional<User> get_user_by_id(int user_id);
    
    // Retrieve user by username
    static std::optional<User> get_user_by_username(const std::string& username);
    
    // Authenticate user (returns user_id if valid, -1 if invalid)
    static int authenticate_user(const std::string& username, const std::string& password_hash);
    
    // Update user statistics
    static bool update_user_stats(int user_id, int wins, int losses, int draws, int rating);
    
    // Increment win/loss/draw count
    static bool increment_wins(int user_id);
    static bool increment_losses(int user_id);
    static bool increment_draws(int user_id);
    
    // Update rating
    static bool update_rating(int user_id, int new_rating);
    
    // Get all users (for leaderboard)
    static std::vector<User> get_all_users();
    
    // Get top users by rating
    static std::vector<User> get_top_users(int limit = 10);
    
    // Check if username exists
    static bool username_exists(const std::string& username);
    
    // Delete user
    static bool delete_user(int user_id);
    
    // Get user's password hash (for authentication)
    static std::string get_password_hash(const std::string& username);
};

#endif // USER_REPOSITORY_H
