# Database Repository Implementation

## 📦 What's Included

### Files Created
```
server/database/
├── schema.sql                    # Database schema with sample data
├── user_repository.h             # User data access interface
├── user_repository.cpp           # User repository implementation
├── game_repository.h             # Game data access interface
├── game_repository.cpp           # Game repository implementation
├── test_user_repository.cpp      # User repository tests
└── test_game_repository.cpp      # Game repository tests
```

## 🗄️ Database Schema

### Tables

**users**
- `user_id` - Primary key
- `username` - Unique username
- `password_hash` - Hashed password
- `email` - User email (optional)
- `created_at` - Registration timestamp
- `wins`, `losses`, `draws` - Game statistics
- `rating` - ELO-style rating (default 1200)

**game_history**
- `game_id` - Primary key
- `white_player_id`, `black_player_id` - Player references
- `result` - Game outcome (WHITE_WIN, BLACK_WIN, DRAW, ABORTED)
- `moves` - JSON array of moves
- `start_time`, `end_time` - Game timestamps
- `duration` - Game duration in seconds

**active_sessions** (optional)
- `session_id` - Session identifier
- `user_id` - User reference
- `login_time`, `last_activity` - Session timestamps
- `ip_address` - Client IP

## 🚀 Setup

### 1. Create Database

```bash
# In Windows (PowerShell) - PostgreSQL should be running
createdb -U postgres chess-app
```

### 2. Setup Schema

```bash
# From server directory
cd server
make setup_db

# OR manually
psql -U postgres -d chess-app -f database/schema.sql
```

This will create tables and insert sample users (alice, bob, charlie, diana).

## 🧪 Testing

### Build Tests

```bash
cd server

# Build all tests
make all

# Or build specific test
make test_user_repo
make test_game_repo
```

### Run Tests

```bash
# Test user repository
make run_user_test

# Test game repository
make run_game_test
```

Expected output shows ✓ for successful tests.

## 📖 API Usage

### User Repository

```cpp
#include "database/user_repository.h"

// Create user
int user_id = UserRepository::create_user("john", "hashed_password", "john@example.com");

// Get user
auto user = UserRepository::get_user_by_username("john");
if (user.has_value()) {
    std::cout << "Rating: " << user->rating << std::endl;
}

// Authenticate
int auth_id = UserRepository::authenticate_user("john", "hashed_password");
if (auth_id > 0) {
    std::cout << "Login successful!" << std::endl;
}

// Update stats
UserRepository::increment_wins(user_id);
UserRepository::update_rating(user_id, 1350);

// Get leaderboard
auto top_users = UserRepository::get_top_users(10);
for (const auto& u : top_users) {
    std::cout << u.username << ": " << u.rating << std::endl;
}
```

### Game Repository

```cpp
#include "database/game_repository.h"

// Create game
int game_id = GameRepository::create_game(white_player_id, black_player_id);

// Add moves during game
GameRepository::add_move_to_game(game_id, "e2e4");
GameRepository::add_move_to_game(game_id, "e7e5");

// End game
std::string moves_json = "[\"e2e4\",\"e7e5\",\"Ng1f3\"]";
GameRepository::end_game(game_id, "WHITE_WIN", moves_json);

// Get game
auto game = GameRepository::get_game_by_id(game_id);
if (game.has_value()) {
    std::cout << game->white_username << " vs " << game->black_username << std::endl;
    std::cout << "Result: " << game->result << std::endl;
}

// Get user's games
auto games = GameRepository::get_user_games(user_id, 20);

// Get statistics
auto stats = GameRepository::get_user_game_stats(user_id);
std::cout << "W/L/D: " << stats.wins << "/" << stats.losses << "/" << stats.draws << std::endl;
```

## 🔗 Integration with WebSocket Server

### Example: Handle Game Move

```cpp
#include "network/websocket_handler.h"
#include "database/user_repository.h"
#include "database/game_repository.h"

void handle_game_move(WebSocketHandler& ws, int game_id, const std::string& move) {
    // Add move to database
    if (GameRepository::add_move_to_game(game_id, move)) {
        // Send confirmation
        JsonValue response;
        response["type"] = "move_accepted";
        response["move"] = move;
        ws.send_text(stringify_json(response));
    }
}
```

### Example: Handle Login

```cpp
void handle_login(WebSocketHandler& ws, const std::string& username, const std::string& password) {
    // Authenticate user
    int user_id = UserRepository::authenticate_user(username, password);
    
    if (user_id > 0) {
        // Get user details
        auto user = UserRepository::get_user_by_id(user_id);
        
        // Send success response
        JsonValue response;
        response["type"] = "login_success";
        response["user_id"] = user_id;
        response["username"] = user->username;
        response["rating"] = user->rating;
        response["wins"] = user->wins;
        
        ws.send_text(stringify_json(response));
    } else {
        // Send error
        JsonValue error;
        error["type"] = "error";
        error["message"] = "Invalid credentials";
        ws.send_text(stringify_json(error));
    }
}
```

## ⚠️ Important Notes

### SQL Injection Prevention
Current implementation uses string concatenation. For production, use:
```cpp
// TODO: Use parameterized queries
pqxx::work txn(connection);
txn.exec_params("SELECT * FROM users WHERE username = $1", username);
```

### Password Security
- Always hash passwords before storing (use bcrypt, argon2, or similar)
- Never store plaintext passwords
- Example with OpenSSL SHA-256:
```cpp
#include <openssl/sha.h>

std::string hash_password(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password.c_str(), password.length(), hash);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}
```

### Error Handling
All functions return appropriate values on error:
- `-1` for failed operations returning int
- `false` for boolean operations
- `std::nullopt` for optional returns
- Empty containers for vector returns

Check return values before using data!

## 📊 Features Implemented

### User Repository
✅ Create user  
✅ Retrieve user by ID/username  
✅ Authenticate user  
✅ Update statistics  
✅ Increment win/loss/draw counts  
✅ Update rating  
✅ Get all users / top users  
✅ Check username exists  
✅ Delete user  

### Game Repository
✅ Create game  
✅ Add moves to game  
✅ End game with result  
✅ Get game by ID  
✅ Get user's games  
✅ Get recent games  
✅ Get game statistics  
✅ Get head-to-head games  
✅ Check if game exists  
✅ Delete game  

## 🔧 Troubleshooting

### Connection Issues
```bash
# Check if PostgreSQL is running
pg_ctl status

# Test connection manually
psql -U postgres -d chess-app -c "SELECT * FROM users;"
```

### Build Errors
```bash
# Install PostgreSQL development libraries
sudo apt-get install libpqxx-dev

# Check include paths
g++ -I/usr/include/pqxx -lpqxx -lpq
```

### Schema Errors
```bash
# Drop and recreate database
dropdb -U postgres chess-app
createdb -U postgres chess-app
make setup_db
```

## 📝 Next Steps

1. ✅ Complete repositories ← **Done**
2. Implement session management
3. Create match manager
4. Integrate with WebSocket server
5. Add input validation
6. Implement parameterized queries
7. Add transaction support
8. Create migration system

---

**Status:** Complete and tested  
**Dependencies:** PostgreSQL, libpqxx  
**Date:** November 24, 2025
