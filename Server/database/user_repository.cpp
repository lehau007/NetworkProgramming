#include "user_repository.h"
#include "database_connection.cpp"
#include <iostream>

// Create new user
int UserRepository::create_user(const std::string& username, const std::string& password_hash, const std::string& email) {
    try {
        std::string query = "INSERT INTO users (username, password_hash, email) VALUES ('" 
            + username + "', '" + password_hash + "', '" + email + "') RETURNING user_id";
        
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            return result[0]["user_id"].as<int>();
        }
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Error creating user: " << e.what() << std::endl;
        return -1;
    }
}

// Get user by ID
std::optional<User> UserRepository::get_user_by_id(int user_id) {
    try {
        std::string query = "SELECT user_id, username, email, created_at, wins, losses, draws, rating "
                           "FROM users WHERE user_id = " + std::to_string(user_id);
        
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            User user;
            user.user_id = result[0]["user_id"].as<int>();
            user.username = result[0]["username"].c_str();
            user.email = result[0]["email"].is_null() ? "" : result[0]["email"].c_str();
            user.created_at = result[0]["created_at"].c_str();
            user.wins = result[0]["wins"].as<int>();
            user.losses = result[0]["losses"].as<int>();
            user.draws = result[0]["draws"].as<int>();
            user.rating = result[0]["rating"].as<int>();
            return user;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Error getting user by ID: " << e.what() << std::endl;
        return std::nullopt;
    }
}

// Get user by username
std::optional<User> UserRepository::get_user_by_username(const std::string& username) {
    try {
        std::string query = "SELECT user_id, username, email, created_at, wins, losses, draws, rating "
                           "FROM users WHERE username = '" + username + "'";
        
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            User user;
            user.user_id = result[0]["user_id"].as<int>();
            user.username = result[0]["username"].c_str();
            user.email = result[0]["email"].is_null() ? "" : result[0]["email"].c_str();
            user.created_at = result[0]["created_at"].c_str();
            user.wins = result[0]["wins"].as<int>();
            user.losses = result[0]["losses"].as<int>();
            user.draws = result[0]["draws"].as<int>();
            user.rating = result[0]["rating"].as<int>();
            return user;
        }
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "Error getting user by username: " << e.what() << std::endl;
        return std::nullopt;
    }
}

// Authenticate user
int UserRepository::authenticate_user(const std::string& username, const std::string& password_hash) {
    try {
        std::string query = "SELECT user_id FROM users WHERE username = '" 
            + username + "' AND password_hash = '" + password_hash + "'";
        
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            return result[0]["user_id"].as<int>();
        }
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Error authenticating user: " << e.what() << std::endl;
        return -1;
    }
}

// Update user statistics
bool UserRepository::update_user_stats(int user_id, int wins, int losses, int draws, int rating) {
    try {
        std::string query = "UPDATE users SET wins = " + std::to_string(wins) 
            + ", losses = " + std::to_string(losses) 
            + ", draws = " + std::to_string(draws) 
            + ", rating = " + std::to_string(rating) 
            + " WHERE user_id = " + std::to_string(user_id);
        
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error updating user stats: " << e.what() << std::endl;
        return false;
    }
}

// Increment wins
bool UserRepository::increment_wins(int user_id) {
    try {
        std::string query = "UPDATE users SET wins = wins + 1 WHERE user_id = " + std::to_string(user_id);
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error incrementing wins: " << e.what() << std::endl;
        return false;
    }
}

// Increment losses
bool UserRepository::increment_losses(int user_id) {
    try {
        std::string query = "UPDATE users SET losses = losses + 1 WHERE user_id = " + std::to_string(user_id);
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error incrementing losses: " << e.what() << std::endl;
        return false;
    }
}

// Increment draws
bool UserRepository::increment_draws(int user_id) {
    try {
        std::string query = "UPDATE users SET draws = draws + 1 WHERE user_id = " + std::to_string(user_id);
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error incrementing draws: " << e.what() << std::endl;
        return false;
    }
}

// Update rating
bool UserRepository::update_rating(int user_id, int new_rating) {
    try {
        std::string query = "UPDATE users SET rating = " + std::to_string(new_rating) 
            + " WHERE user_id = " + std::to_string(user_id);
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error updating rating: " << e.what() << std::endl;
        return false;
    }
}

// Get all users
std::vector<User> UserRepository::get_all_users() {
    std::vector<User> users;
    try {
        std::string query = "SELECT user_id, username, email, created_at, wins, losses, draws, rating "
                           "FROM users ORDER BY rating DESC";
        
        auto result = DatabaseConnection::execute_query(query);
        
        for (const auto& row : result) {
            User user;
            user.user_id = row["user_id"].as<int>();
            user.username = row["username"].c_str();
            user.email = row["email"].is_null() ? "" : row["email"].c_str();
            user.created_at = row["created_at"].c_str();
            user.wins = row["wins"].as<int>();
            user.losses = row["losses"].as<int>();
            user.draws = row["draws"].as<int>();
            user.rating = row["rating"].as<int>();
            users.push_back(user);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting all users: " << e.what() << std::endl;
    }
    return users;
}

// Get top users by rating
std::vector<User> UserRepository::get_top_users(int limit) {
    std::vector<User> users;
    try {
        std::string query = "SELECT user_id, username, email, created_at, wins, losses, draws, rating "
                           "FROM users ORDER BY rating DESC LIMIT " + std::to_string(limit);
        
        auto result = DatabaseConnection::execute_query(query);
        
        for (const auto& row : result) {
            User user;
            user.user_id = row["user_id"].as<int>();
            user.username = row["username"].c_str();
            user.email = row["email"].is_null() ? "" : row["email"].c_str();
            user.created_at = row["created_at"].c_str();
            user.wins = row["wins"].as<int>();
            user.losses = row["losses"].as<int>();
            user.draws = row["draws"].as<int>();
            user.rating = row["rating"].as<int>();
            users.push_back(user);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting top users: " << e.what() << std::endl;
    }
    return users;
}

// Check if username exists
bool UserRepository::username_exists(const std::string& username) {
    try {
        std::string query = "SELECT COUNT(*) as count FROM users WHERE username = '" + username + "'";
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            return result[0]["count"].as<int>() > 0;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error checking username exists: " << e.what() << std::endl;
        return false;
    }
}

// Delete user
bool UserRepository::delete_user(int user_id) {
    try {
        std::string query = "DELETE FROM users WHERE user_id = " + std::to_string(user_id);
        DatabaseConnection::execute_query(query);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error deleting user: " << e.what() << std::endl;
        return false;
    }
}

// Get password hash
std::string UserRepository::get_password_hash(const std::string& username) {
    try {
        std::string query = "SELECT password_hash FROM users WHERE username = '" + username + "'";
        auto result = DatabaseConnection::execute_query(query);
        
        if (result.size() > 0) {
            return result[0]["password_hash"].c_str();
        }
        return "";
    } catch (const std::exception& e) {
        std::cerr << "Error getting password hash: " << e.what() << std::endl;
        return "";
    }
}
