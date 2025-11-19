# Chess Server Workflow Documentation

## Overview
This document describes the complete workflow and architecture of the C++ Chess Server, detailing all implemented features and their point allocations.

---

## Features Breakdown (19 Points Total)

### 1. Xử lý truyền dòng (Stream Processing) - 1 điểm
**Implementation**: TCP socket-based stream handling with proper buffering

```cpp
// Socket creation and configuration
int server_sock = socket(AF_INET, SOCK_STREAM, 0);

// Continuous data reception from clients
while (running) {
    bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
    // Process incoming stream data
}
```

**Workflow**:
1. Server creates TCP socket (`SOCK_STREAM`)
2. Accepts incoming connections
3. Continuously reads data stream from client sockets
4. Buffers partial messages until complete
5. Processes complete messages and sends responses

---

### 2. Cài đặt cơ chế vào/ra socket trên server (Socket I/O Mechanism) - 2 điểm

**Implementation**: Multi-threaded socket I/O using pthread

```cpp
// Server socket initialization
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
server_addr.sin_port = htons(8080);

bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
listen(server_sock, 5);

// Accept connections
int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_addr_len);

// Create thread for each client
pthread_t thread_id;
pthread_create(&thread_id, NULL, handle_client, (void*)&client_sock);
```

**Socket Operations**:
- **Input**: `recv()` for receiving data from clients
- **Output**: `send()` for transmitting responses
- **Non-blocking I/O**: Optional for scalability
- **Thread-per-client**: Each connection handled by dedicated thread

**Message Format**:
```json
{
    "type": "message_type",
    "data": { ... }
}
```

---

### 3. Đăng ký và quản lý tài khoản (Account Registration & Management) - 2 điểm

**Database Schema**:
```sql
CREATE TABLE users (
    user_id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(256) NOT NULL,
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    wins INT DEFAULT 0,
    losses INT DEFAULT 0,
    draws INT DEFAULT 0
);
```

**Registration Flow**:
```
Client                          Server                      Database
   |                               |                            |
   |---REGISTER(user,pass,email)-->|                            |
   |                               |---Check username exists--->|
   |                               |<--User not found-----------|
   |                               |                            |
   |                               |---Hash password----------->|
   |                               |---INSERT new user--------->|
   |                               |<--Success------------------|
   |<--REGISTER_SUCCESS------------|                            |
```

**Registration Message**:
```json
{
    "type": "REGISTER",
    "username": "player1",
    "password": "hashed_password",
    "email": "player1@example.com"
}
```

**Response**:
```json
{
    "type": "REGISTER_RESPONSE",
    "status": "success",
    "message": "Account created successfully",
    "user_id": 123
}
```

---

### 4. Đăng nhập và quản lý phiên (Login & Session Management) - 2 điểm

**Session Data Structure**:
```cpp
struct Session {
    int user_id;
    string username;
    int socket_fd;
    time_t login_time;
    bool is_active;
    int current_game_id;  // -1 if not in game
};

// Global session manager
map<int, Session*> active_sessions;        // socket_fd -> Session
map<string, Session*> username_to_session; // username -> Session
pthread_mutex_t session_mutex;
```

**Login Flow**:
```
Client                          Server                      Database
   |                               |                            |
   |---LOGIN(username,password)--->|                            |
   |                               |---Verify credentials------>|
   |                               |<--User data----------------|
   |                               |                            |
   |                               |[Create session]            |
   |                               |[Store socket mapping]      |
   |<--LOGIN_SUCCESS(user_data)----|                            |
```

**Login Message**:
```json
{
    "type": "LOGIN",
    "username": "player1",
    "password": "hashed_password"
}
```

**Response**:
```json
{
    "type": "LOGIN_RESPONSE",
    "status": "success",
    "user_id": 123,
    "username": "player1",
    "stats": {
        "wins": 10,
        "losses": 5,
        "draws": 2
    }
}
```

**Session Management Operations**:
1. **Create Session**: On successful login
2. **Validate Session**: Before processing any game request
3. **Destroy Session**: On logout or disconnect
4. **Session Timeout**: Auto-logout after inactivity

---

### 5. Cung cấp danh sách người chơi sẵn sàng (Available Players List) - 2 điểm

**Player State Management**:
```cpp
enum PlayerState {
    ONLINE,           // Logged in but not in game
    IN_GAME,          // Currently playing
    CHALLENGING,      // Sent/received challenge
    OFFLINE
};

struct PlayerInfo {
    int user_id;
    string username;
    PlayerState state;
    int rating;
    time_t last_activity;
};

vector<PlayerInfo> online_players;
pthread_mutex_t players_mutex;
```

**Get Players Flow**:
```
Client                          Server
   |                               |
   |---GET_AVAILABLE_PLAYERS------>|
   |                               |[Lock players_mutex]
   |                               |[Filter ONLINE state]
   |                               |[Exclude requesting user]
   |<--PLAYER_LIST-----------------|
```

**Request**:
```json
{
    "type": "GET_AVAILABLE_PLAYERS"
}
```

**Response**:
```json
{
    "type": "PLAYER_LIST",
    "players": [
        {
            "user_id": 124,
            "username": "player2",
            "rating": 1500,
            "wins": 8,
            "losses": 3
        },
        {
            "user_id": 125,
            "username": "player3",
            "rating": 1650,
            "wins": 15,
            "losses": 7
        }
    ]
}
```

**Auto-Update**: Server broadcasts player list changes to all online clients

---

### 6. Chuyển lời thách đấu (Forward Challenge Request) - 2 điểm

**Challenge Data Structure**:
```cpp
struct Challenge {
    int challenge_id;
    int challenger_id;
    int challenged_id;
    time_t timestamp;
    bool is_pending;
};

map<int, Challenge*> pending_challenges; // challenge_id -> Challenge
```

**Challenge Flow**:
```
Player A                    Server                      Player B
   |                           |                            |
   |---CHALLENGE(player_B)---->|                            |
   |                           |[Verify B is available]     |
   |                           |[Create challenge record]   |
   |                           |[Lookup B's socket]         |
   |                           |---CHALLENGE_RECEIVED------>|
   |<--CHALLENGE_SENT----------|                            |
```

**Challenge Request**:
```json
{
    "type": "CHALLENGE",
    "target_username": "player2",
    "time_control": "10+0"
}
```

**Forward to Target Player**:
```json
{
    "type": "CHALLENGE_RECEIVED",
    "challenge_id": 456,
    "from_user": {
        "user_id": 123,
        "username": "player1",
        "rating": 1450
    },
    "time_control": "10+0"
}
```

**Confirmation to Challenger**:
```json
{
    "type": "CHALLENGE_SENT",
    "status": "success",
    "challenge_id": 456,
    "to_username": "player2"
}
```

---

### 7. Từ chối/Nhận lời thách đấu (Accept/Decline Challenge) - 1 điểm

**Response Flow**:
```
Player B                    Server                      Player A
   |                           |                            |
   |---ACCEPT(challenge_id)--->|                            |
   |                           |[Verify challenge exists]   |
   |                           |[Create game instance]      |
   |                           |[Update player states]      |
   |<--MATCH_STARTED-----------|---MATCH_STARTED---------->|
   |                           |                            |

OR

   |---DECLINE(challenge_id)-->|                            |
   |                           |[Remove challenge]          |
   |<--CHALLENGE_DECLINED------|---CHALLENGE_DECLINED------>|
```

**Accept Message**:
```json
{
    "type": "ACCEPT_CHALLENGE",
    "challenge_id": 456
}
```

**Decline Message**:
```json
{
    "type": "DECLINE_CHALLENGE",
    "challenge_id": 456,
    "reason": "Busy"
}
```

**Match Started Notification** (to both players):
```json
{
    "type": "MATCH_STARTED",
    "game_id": 789,
    "opponent": {
        "user_id": 123,
        "username": "player1",
        "rating": 1450
    },
    "your_color": "white",
    "time_control": "10+0"
}
```

---

### 8. Chuyển thông tin nước đi (Forward Move Information) - 2 điểm

**Game Instance Structure**:
```cpp
struct GameInstance {
    int game_id;
    int white_player_id;
    int black_player_id;
    ChessGame* chess_engine;
    vector<string> move_history;
    time_t start_time;
    bool is_active;
};

map<int, GameInstance*> active_games; // game_id -> GameInstance
```

**Move Forwarding Flow**:
```
Player A                    Server                      Player B
   |                           |                            |
   |---MOVE(e2e4)------------->|                            |
   |                           |[Verify turn]               |
   |                           |[Validate with ChessGame]   |
   |                           |[Execute move]              |
   |                           |[Log move]                  |
   |<--MOVE_ACCEPTED-----------|---OPPONENT_MOVE(e2e4)----->|
```

**Move Message**:
```json
{
    "type": "MOVE",
    "game_id": 789,
    "move": "e2e4",
    "timestamp": 1700000000
}
```

**Forward to Opponent**:
```json
{
    "type": "OPPONENT_MOVE",
    "game_id": 789,
    "move": "e2e4",
    "move_number": 1,
    "time_remaining": {
        "white": 600,
        "black": 600
    }
}
```

**Thread Safety**:
```cpp
pthread_mutex_t game_mutex;

void forward_move(int game_id, string move, int player_id) {
    pthread_mutex_lock(&game_mutex);
    
    GameInstance* game = active_games[game_id];
    int opponent_id = (game->white_player_id == player_id) 
                      ? game->black_player_id 
                      : game->white_player_id;
    
    Session* opponent_session = get_session_by_user_id(opponent_id);
    send(opponent_session->socket_fd, move_json.c_str(), move_json.length(), 0);
    
    pthread_mutex_unlock(&game_mutex);
}
```

---

### 9. Kiểm tra tính hợp lệ của nước đi (Move Validation) - 1 điểm

**Validation Process**:
```cpp
bool validate_move(int game_id, int player_id, string move) {
    GameInstance* game = active_games[game_id];
    
    // 1. Check if it's player's turn
    bool is_white_turn = (game->chess_engine->getTurn() % 2 == 0);
    bool player_is_white = (game->white_player_id == player_id);
    
    if (is_white_turn != player_is_white) {
        return false; // Not player's turn
    }
    
    // 2. Validate move using chess engine
    if (!game->chess_engine->checkMove(move)) {
        return false; // Invalid chess move
    }
    
    // 3. Check move format (e.g., "e2e4")
    if (move.length() != 4) {
        return false;
    }
    
    return true;
}
```

**Validation Checks**:
1. **Turn verification**: Correct player moving
2. **Move format**: Valid notation (e.g., "e2e4")
3. **Piece rules**: Using `ChessGame::checkMove()`
4. **Path clearance**: For sliding pieces
5. **Check/Checkmate**: King safety
6. **Special moves**: Castling, en passant

**Invalid Move Response**:
```json
{
    "type": "MOVE_REJECTED",
    "game_id": 789,
    "reason": "Not your turn",
    "error_code": "INVALID_TURN"
}
```

---

### 10. Xác định kết quả ván cờ (Determine Game Result) - 1 điểm

**Result Detection**:
```cpp
void check_game_result(int game_id) {
    GameInstance* game = active_games[game_id];
    
    if (game->chess_engine->checkGameEnd()) {
        GameResult result = game->chess_engine->getResult();
        
        // Update player statistics in database
        update_player_stats(game->white_player_id, 
                          game->black_player_id, 
                          result);
        
        // Notify both players
        notify_game_end(game_id, result);
        
        // Mark game as completed
        game->is_active = false;
    }
}
```

**Win Conditions**:
1. **Checkmate**: King captured or checkmated
2. **Resignation**: Player surrenders
3. **Time out**: Player runs out of time
4. **Disconnect**: Player loses connection

**Draw Conditions**:
1. **Stalemate**: No legal moves available
2. **Agreement**: Both players agree
3. **50-move rule**: No capture/pawn move in 50 moves
4. **Insufficient material**: Cannot checkmate

**Result Notification**:
```json
{
    "type": "GAME_ENDED",
    "game_id": 789,
    "result": "WHITE_WIN",
    "reason": "CHECKMATE",
    "winner": {
        "user_id": 123,
        "username": "player1"
    },
    "final_position": "rnbqkbnr/pppppppp/..."
}
```

---

### 11. Ghi log (Logging) - 1 điểm

**Logging System**:
```cpp
enum LogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG
};

class Logger {
private:
    ofstream log_file;
    pthread_mutex_t log_mutex;
    
public:
    void log(LogLevel level, string message) {
        pthread_mutex_lock(&log_mutex);
        
        time_t now = time(0);
        string timestamp = ctime(&now);
        timestamp.pop_back(); // Remove newline
        
        log_file << "[" << timestamp << "] "
                 << level_to_string(level) << ": "
                 << message << endl;
        
        pthread_mutex_unlock(&log_mutex);
    }
};
```

**Logged Events**:
1. **Connection events**: Client connect/disconnect
2. **Authentication**: Login/logout attempts
3. **Game events**: Challenge, accept, moves
4. **Errors**: Invalid moves, timeout, crashes
5. **Database operations**: Queries, errors

**Log Format**:
```
[2025-11-19 14:30:45] INFO: Client connected from 192.168.1.100:54321
[2025-11-19 14:30:50] INFO: User 'player1' logged in successfully
[2025-11-19 14:31:00] INFO: Challenge sent from player1 to player2
[2025-11-19 14:31:05] INFO: Challenge accepted, game 789 started
[2025-11-19 14:31:10] INFO: Game 789: player1 moved e2e4
[2025-11-19 14:31:15] ERROR: Invalid move attempt by player2: z9z9
[2025-11-19 14:35:00] INFO: Game 789 ended: WHITE_WIN by checkmate
```

---

### 12. Chuyển kết quả và log ván cờ (Send Game Result & Log) - 2 điểm

**Game Log Structure**:
```cpp
struct GameLog {
    int game_id;
    int white_player_id;
    int black_player_id;
    vector<string> moves;           // All moves in notation
    vector<string> descriptive_log; // Descriptive move history
    time_t start_time;
    time_t end_time;
    GameResult result;
    string result_reason;
};
```

**Send Game Log Flow**:
```
Server                                          Database
   |                                               |
   |[Game ends]                                    |
   |[Generate complete log]                        |
   |---Save game record to DB------------------->  |
   |<--Confirmation--------------------------------|
   |                                               |
   |[Send to Player A]                             |
   |[Send to Player B]                             |
```

**Game Log Message**:
```json
{
    "type": "GAME_LOG",
    "game_id": 789,
    "players": {
        "white": "player1",
        "black": "player2"
    },
    "result": "WHITE_WIN",
    "result_reason": "CHECKMATE",
    "duration": 1800,
    "moves": [
        {
            "number": 1,
            "white": "e2e4",
            "black": "e7e5",
            "white_time": 595,
            "black_time": 593
        },
        {
            "number": 2,
            "white": "g1f3",
            "black": "b8c6",
            "white_time": 590,
            "black_time": 588
        }
    ],
    "descriptive_log": [
        "1. White - Pawn from e2 to e4",
        "1. Black - Pawn from e7 to e5",
        "2. White - Knight from g1 to f3",
        "2. Black - Knight from b8 to c6"
    ],
    "final_position": "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR"
}
```

**Database Storage**:
```sql
CREATE TABLE game_history (
    game_id INT PRIMARY KEY AUTO_INCREMENT,
    white_player_id INT,
    black_player_id INT,
    result VARCHAR(20),
    moves TEXT,  -- JSON array of moves
    start_time TIMESTAMP,
    end_time TIMESTAMP,
    FOREIGN KEY (white_player_id) REFERENCES users(user_id),
    FOREIGN KEY (black_player_id) REFERENCES users(user_id)
);
```

---

### 13. Xin ngừng ván cờ (Request Game Abort/Draw) - 1 điểm

**Abort/Draw Request Types**:
```cpp
enum GameStopRequest {
    RESIGN,           // Surrender (opponent wins)
    DRAW_OFFER,       // Offer draw
    ABORT_REQUEST     // Cancel game (mutual)
};
```

**Request Flow**:
```
Player A                    Server                      Player B
   |                           |                            |
   |---RESIGN----------------->|                            |
   |                           |[End game immediately]      |
   |                           |[Update stats]              |
   |<--GAME_ENDED--------------|---GAME_ENDED-------------->|
   |   (BLACK_WIN)             |   (BLACK_WIN by resign)    |

OR

   |---DRAW_OFFER------------->|                            |
   |                           |---DRAW_OFFER_RECEIVED----->|
   |                           |                            |
   |                           |<--ACCEPT_DRAW--------------|
   |<--GAME_ENDED--------------|---GAME_ENDED-------------->|
   |   (DRAW)                  |   (DRAW by agreement)      |
```

**Resign Message**:
```json
{
    "type": "RESIGN",
    "game_id": 789
}
```

**Draw Offer Message**:
```json
{
    "type": "DRAW_OFFER",
    "game_id": 789
}
```

**Draw Offer Notification**:
```json
{
    "type": "DRAW_OFFER_RECEIVED",
    "game_id": 789,
    "from_player": "player1"
}
```

**Draw Response**:
```json
{
    "type": "DRAW_RESPONSE",
    "game_id": 789,
    "accepted": true
}
```

---

## Complete Message Flow Examples

### Example 1: Full Game Sequence
```
Player A (White)           Server                Player B (Black)
      |                      |                         |
      |--LOGIN-------------->|                         |
      |<-LOGIN_OK------------|                         |
      |                      |<-------LOGIN------------|
      |                      |--------LOGIN_OK-------->|
      |                      |                         |
      |--GET_PLAYERS-------->|                         |
      |<-PLAYER_LIST---------|                         |
      |                      |                         |
      |--CHALLENGE(B)------->|                         |
      |                      |--CHALLENGE_RECV-------->|
      |<-CHALLENGE_SENT------|                         |
      |                      |                         |
      |                      |<-------ACCEPT-----------|
      |<-MATCH_START---------|--------MATCH_START----->|
      |                      |                         |
      |--MOVE(e2e4)--------->|                         |
      |                      |[validate]               |
      |<-MOVE_OK-------------|--------OPP_MOVE(e2e4)-->|
      |                      |                         |
      |                      |<-------MOVE(e7e5)-------|
      |<-OPP_MOVE(e7e5)------|--------MOVE_OK--------->|
      |                      |                         |
      |       ...continues until game ends...          |
      |                      |                         |
      |                      |[checkmate detected]     |
      |<-GAME_ENDED----------|--------GAME_ENDED------>|
      |<-GAME_LOG------------|--------GAME_LOG-------->|
```

### Example 2: Challenge Declined
```
Player A                   Server                Player B
      |                      |                         |
      |--CHALLENGE(B)------->|                         |
      |                      |--CHALLENGE_RECV-------->|
      |<-CHALLENGE_SENT------|                         |
      |                      |                         |
      |                      |<-------DECLINE----------|
      |<-CHALLENGE_DECLINED--|--------DECLINE_SENT---->|
```

### Example 3: Draw Offer
```
Player A                   Server                Player B
      |                      |                         |
      |  (mid-game)          |                         |
      |--DRAW_OFFER--------->|                         |
      |                      |--DRAW_OFFER_RECV------->|
      |                      |                         |
      |                      |<-------ACCEPT_DRAW------|
      |<-GAME_ENDED(DRAW)----|--------GAME_ENDED------>|
      |<-GAME_LOG------------|--------GAME_LOG-------->|
```

---

## Architecture Overview

### Thread Model
```
Main Thread
    |
    +-- Accept Loop (listens for connections)
    |
    +-- Client Thread 1 (Player A)
    |   |
    |   +-- Receive messages
    |   +-- Process commands
    |   +-- Send responses
    |
    +-- Client Thread 2 (Player B)
    |   |
    |   +-- Receive messages
    |   +-- Process commands
    |   +-- Send responses
    |
    +-- Client Thread N...
```

### Data Structures & Synchronization
```cpp
// Global state (protected by mutexes)
map<int, Session*> active_sessions;           // socket -> session
map<string, Session*> username_to_session;    // username -> session
map<int, GameInstance*> active_games;         // game_id -> game
vector<PlayerInfo> online_players;            // available players

// Mutexes
pthread_mutex_t session_mutex;
pthread_mutex_t game_mutex;
pthread_mutex_t players_mutex;
pthread_mutex_t log_mutex;
```

### Message Processing Pipeline
```
1. Receive raw bytes from socket
2. Buffer until complete message (e.g., JSON with delimiter)
3. Parse JSON message
4. Validate session/authentication
5. Route to appropriate handler based on message type
6. Execute business logic (chess move, challenge, etc.)
7. Update shared state (with mutex protection)
8. Generate response
9. Send response to client(s)
10. Log transaction
```

---

## Error Handling

### Error Categories
1. **Network Errors**: Connection lost, timeout
2. **Authentication Errors**: Invalid credentials, session expired
3. **Game Logic Errors**: Invalid move, wrong turn
4. **Database Errors**: Connection failed, query error
5. **Concurrency Errors**: Deadlock detection, race conditions

### Error Response Format
```json
{
    "type": "ERROR",
    "error_code": "INVALID_MOVE",
    "message": "It's not your turn",
    "severity": "WARNING"
}
```

---

## Performance Considerations

### Scalability
- **Thread Pool**: Limit maximum concurrent connections
- **Connection Timeout**: Auto-disconnect idle clients
- **Database Connection Pool**: Reuse DB connections
- **Memory Management**: Clean up ended games periodically

### Optimization
- **Message Buffering**: Reduce system calls
- **Lock Granularity**: Fine-grained mutexes
- **Index Database**: Index on user_id, game_id
- **Caching**: Cache player stats, rankings

---

## Testing Checklist

- [ ] Multiple simultaneous logins
- [ ] Concurrent game sessions
- [ ] Challenge during active game (should reject)
- [ ] Invalid move handling
- [ ] Network disconnect during game
- [ ] Database connection failure recovery
- [ ] Thread safety under load
- [ ] Memory leak detection
- [ ] Log file rotation
- [ ] Session timeout cleanup

---

## Deployment Notes

### Build Commands
```bash
g++ -pthread -o chess_server server.cpp chess_game.cpp database_connection.cpp -lmysqlclient
```

### Run Server
```bash
./chess_server
```

### Configuration
- Port: 8080 (configurable)
- Max connections: 100 (configurable)
- Database: MySQL/MariaDB
- Log file: `/var/log/chess_server.log`

---

## Future Enhancements
1. Spectator mode
2. Tournament system
3. Rating/ELO calculation
4. Move time controls
5. Replay system
6. Admin commands
7. Chat functionality
8. Anti-cheat measures

---

**Document Version**: 1.0  
**Last Updated**: 2025-11-19  
**Total Points Covered**: 19/19 ✓
