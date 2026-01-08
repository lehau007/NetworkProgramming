# Chess Game - Network Programming Project Architecture

## Project Overview

**Focus**: Transport Layer (TCP/IP) and Network Protocol Implementation  
**Language**: C++ (Server), Flexible (Client - Python UI or Web UI)  
**Database**: PostgreSQL  
**Protocol**: Custom JSON-based protocol over TCP

---

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Chess Game System                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐         ┌──────────────┐                      │
│  │   Client A   │◄───────►│              │                      │
│  │  (Player 1)  │   TCP   │              │                      │
│  └──────────────┘Websocket|              |                      | 
|                   over TCP|              │                      │                      
│                           │              │                      │
│  ┌──────────────┐         │    Server    │    ┌─────────────┐   │
│  │   Client B   │◄───────►│  (Multi-     │◄───│ PostgreSQL  │   │
│  │  (Player 2)  │         │   threaded)  │    │  Database   │   │
│  └──────────────┘         │              │    └─────────────┘   │
│                           │              │                      │
│  ┌──────────────┐         │              │                      │
│  │   Client N   │◄───────►│              │                      │
│  │  (Player N)  │         │              │                      │
│  └──────────────┘         └──────────────┘                      │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Transport Layer Design

### TCP Socket Configuration

**Server Socket Settings:**
```cpp
// Socket Type: SOCK_STREAM (TCP for reliable delivery)
int server_sock = socket(AF_INET, SOCK_STREAM, 0);

// Address Configuration
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all interfaces
server_addr.sin_port = htons(8080);         // Port 8080

// Socket Options
int opt = 1;
setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

**Connection Management:**
- **Backlog Queue**: `listen(server_sock, SOMAXCONN)` for maximum pending connections
- **Accept Loop**: Main thread continuously accepts new connections
- **Thread-per-Client**: Each client handled by dedicated pthread
- **Graceful Shutdown**: Proper socket closure and cleanup

### Message Protocol Design

**Frame Structure:**
```
┌─────────────────────────────────────────────────────┐
│ Message Length (4 bytes) │ JSON Payload (Variable) │
└─────────────────────────────────────────────────────┘
```

**Message Format:**
```json
{
    "type": "MESSAGE_TYPE",
    "session_id": "unique_session_identifier",
    "timestamp": 1700000000,
    "data": {
        // Message-specific payload
    }
}
```

**Stream Processing:**
1. Read 4-byte length header
2. Allocate buffer for payload
3. Read exact payload bytes
4. Parse JSON
5. Validate and process

---

## Server Architecture

### Component Breakdown

```
Server Components
│
├── Network Layer (Transport Layer Focus)
│   ├── Socket Manager
│   ├── Connection Handler
│   ├── Message Parser/Serializer
│   └── Send/Receive Buffer Manager
│
├── Session Layer
│   ├── Authentication Manager
│   ├── Session Manager
│   └── Player State Manager
│
├── Application Layer
│   ├── Game Engine (ChessGame)
│   ├── Match Manager
│   ├── Challenge System
│   └── Lobby Manager
│
└── Data Layer
    ├── Database Connection Pool
    ├── User Repository
    └── Game History Repository
```

### Server File Structure

```
Server/
├── main.cpp                    # Entry point
├── server.cpp / server.h       # Server core and socket management
├── network/
│   ├── socket_handler.cpp      # TCP socket operations
│   ├── message_handler.cpp     # Protocol implementation
│   └── buffer_manager.cpp      # Stream buffering
├── session/
│   ├── session_manager.cpp     # Session tracking
│   └── auth_handler.cpp        # Authentication
├── game/
│   ├── chess_game.cpp          # Chess rules engine
│   ├── match_manager.cpp       # Game instance management
│   └── move_validator.cpp      # Move validation
├── database/
│   ├── database_connection.cpp # PostgreSQL connection
│   ├── user_repository.cpp     # User CRUD operations
│   └── game_repository.cpp     # Game history storage
├── utils/
│   ├── logger.cpp              # Logging system
│   └── json_helper.cpp         # JSON parsing utilities
└── config/
    └── .env                    # Configuration (not in git)
```

### Threading Model

```
Main Thread
│
├─► Accept Thread (Continuous)
│   │
│   └─► Spawns Client Threads
│
├─► Client Thread 1 (Player A)
│   ├── Receive messages
│   ├── Parse & validate
│   ├── Execute game logic
│   └── Send responses
│
├─► Client Thread 2 (Player B)
│   └── ...same as above
│
├─► Client Thread N
│   └── ...same as above
│
└─► Cleanup Thread (Optional)
    └── Periodic cleanup of expired sessions
```

**Synchronization:**
```cpp
// Global mutexes for shared resources
pthread_mutex_t session_mutex;      // Protect session map
pthread_mutex_t game_mutex;         // Protect active games
pthread_mutex_t players_mutex;      // Protect player list
pthread_mutex_t db_mutex;           // Database operations (if pooling not used)
pthread_mutex_t log_mutex;          // Log file access
```

### Data Structures

```cpp
// Session Management
struct Session {
    int socket_fd;
    int user_id;
    string username;
    PlayerState state;
    time_t login_time;
    time_t last_activity;
};

map<int, Session*> sessions_by_socket;
map<int, Session*> sessions_by_user_id;
```

**Session Enforcement Policy:**
- **Single connection per session**: The server enforces that only one connection can use a given session at a time
- When a client attempts to verify a session already bound to an active socket, the server responds with `DUPLICATE_SESSION`
- This prevents session hijacking and ensures consistent game state across connections
- The existing connection remains active; the new connection must either wait for the old connection to close or log in with different credentials

```cpp
// Game Instance
struct GameInstance {
    int game_id;
    int white_player_id;
    int black_player_id;
    ChessGame* chess_engine;
    vector<string> move_history;
    time_t start_time;
    bool is_active;
};

map<int, GameInstance*> active_games;

// Challenge System
struct Challenge {
    int challenge_id;
    int challenger_id;
    int challenged_id;
    time_t created_at;
};

map<int, Challenge*> pending_challenges;
```

---

## Client Architecture

### Client Options

**Option 1: Python Desktop UI (Recommended for MVP)**
```
Client (Python)
├── UI Layer (tkinter/PyQt)
├── Network Layer (socket module)
├── Game Display (chess board rendering)
└── Protocol Handler (JSON serialization)
```

**Option 2: Web-Based UI**
```
Client (Web)
├── Frontend (HTML/CSS/JavaScript)
├── WebSocket Connection (Socket.io)
├── Canvas/SVG Chess Board
└── REST API / WebSocket Protocol
```

### Client File Structure (Python Example)

```
client/
├── main.py                     # Entry point
├── ui/
│   ├── login_window.py         # Login screen
│   ├── lobby_window.py         # Player lobby
│   ├── game_window.py          # Chess board UI
│   └── components/
│       ├── chess_board.py      # Board rendering
│       └── piece_widget.py     # Chess piece display
├── network/
│   ├── connection_manager.py   # TCP socket handling
│   ├── message_handler.py      # Protocol implementation
│   └── message_types.py        # Message definitions
├── game/
│   ├── game_state.py           # Client-side game state
│   └── move_input.py           # User move input
└── config/
    └── settings.py             # Server IP, port, etc.
```

### Client Responsibilities

1. **Connection Management**
   - Establish TCP connection to server
   - Maintain heartbeat/keepalive
   - Reconnection logic

2. **User Interface**
   - Display chess board
   - Handle user input (moves)
   - Show game status
   - Display available players

3. **Protocol Handling**
   - Serialize outgoing messages
   - Deserialize incoming messages
   - Handle asynchronous updates

4. **Local Validation (Optional)**
   - Pre-validate moves before sending
   - Reduce server load
   - Better UX

---

## Network Protocol Specification

### Message Types

#### Authentication & Session
```
LOGIN               - User login request
LOGIN_RESPONSE      - Login result
REGISTER            - New user registration
REGISTER_RESPONSE   - Registration result
LOGOUT              - Logout request
HEARTBEAT           - Keep connection alive
```

#### Lobby & Matchmaking
```
GET_AVAILABLE_PLAYERS  - Request player list
PLAYER_LIST            - Available players response
CHALLENGE              - Send challenge to player
CHALLENGE_RECEIVED     - Incoming challenge notification
ACCEPT_CHALLENGE       - Accept a challenge
DECLINE_CHALLENGE      - Decline a challenge
MATCH_STARTED          - Game started notification
```

#### Gameplay
```
MOVE                - Send chess move
MOVE_ACCEPTED       - Move acknowledged
OPPONENT_MOVE       - Opponent's move notification
MOVE_REJECTED       - Invalid move response
DRAW_OFFER          - Offer draw
DRAW_RESPONSE       - Accept/reject draw
RESIGN              - Surrender game
GAME_ENDED          - Game over notification
GAME_LOG            - Complete game history
```

#### Error Handling
```
ERROR               - Error notification
DISCONNECT          - Graceful disconnect
SESSION_EXPIRED     - Session timeout notification
DUPLICATE_SESSION   - Multiple connections with same session rejected
```

### Sample Protocol Exchanges

**Login Flow:**
```
Client → Server:
{
    "type": "LOGIN",
    "username": "player1",
    "password": "hashed_password"
}

Server → Client:
{
    "type": "LOGIN_RESPONSE",
    "status": "success",
    "session_id": "abc123",
    "user_data": {
        "user_id": 123,
        "username": "player1",
        "wins": 10,
        "losses": 5
    }
}
```

**Move Exchange:**
```
Client A → Server:
{
    "type": "MOVE",
    "game_id": 789,
    "move": "e2e4"
}

Server → Client A:
{
    "type": "MOVE_ACCEPTED",
    "game_id": 789
}

Server → Client B:
{
    "type": "OPPONENT_MOVE",
    "game_id": 789,
    "move": "e2e4",
    "move_number": 1
}
```

---

## Database Schema

### Tables

```sql
-- Users table
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(256) NOT NULL,
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    wins INT DEFAULT 0,
    losses INT DEFAULT 0,
    draws INT DEFAULT 0,
    rating INT DEFAULT 1200
);

-- Game history table
CREATE TABLE game_history (
    game_id SERIAL PRIMARY KEY,
    white_player_id INT REFERENCES users(user_id),
    black_player_id INT REFERENCES users(user_id),
    result VARCHAR(20),  -- 'WHITE_WIN', 'BLACK_WIN', 'DRAW'
    moves JSONB,         -- Array of moves
    start_time TIMESTAMP,
    end_time TIMESTAMP,
    duration INT         -- in seconds
);

-- Active sessions (optional, can be in-memory)
CREATE TABLE active_sessions (
    session_id VARCHAR(64) PRIMARY KEY,
    user_id INT REFERENCES users(user_id),
    login_time TIMESTAMP,
    last_activity TIMESTAMP,
    ip_address VARCHAR(45)
);

-- Server logs (optional)
CREATE TABLE server_logs (
    log_id SERIAL PRIMARY KEY,
    log_level VARCHAR(20),
    message TEXT,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    user_id INT REFERENCES users(user_id)
);
```

---

## Development Phases

### Phase 1: Transport Layer & Basic Communication (Week 1-2)
**Focus: TCP Socket Implementation**

**Server Tasks:**
- [ ] Implement TCP server socket
- [ ] Create accept loop
- [ ] Implement thread-per-client model
- [ ] Basic send/receive functions
- [ ] Message framing (length-prefixed)
- [ ] JSON parsing integration

**Client Tasks:**
- [ ] TCP client connection
- [ ] Send/receive messages
- [ ] Basic UI framework
- [ ] Connection status display

**Deliverable:** Client can connect to server and exchange messages

---

### Phase 2: Authentication & Session Management (Week 2-3)
**Focus: User Management**

**Server Tasks:**
- [ ] Database connection setup
- [ ] User registration handler
- [ ] Login authentication
- [ ] Session creation & management
- [ ] Session validation
- [ ] Logout handling

**Client Tasks:**
- [ ] Login UI
- [ ] Registration UI
- [ ] Session storage
- [ ] Auto-reconnect logic

**Deliverable:** Users can register, login, and maintain sessions

---

### Phase 3: Lobby & Matchmaking (Week 3-4)
**Focus: Player Discovery & Challenges**

**Server Tasks:**
- [ ] Player list management
- [ ] Challenge system
- [ ] Accept/decline logic
- [ ] Match creation
- [ ] Player state tracking

**Client Tasks:**
- [ ] Lobby UI
- [ ] Player list display
- [ ] Challenge send/receive UI
- [ ] Match acceptance dialog

**Deliverable:** Players can see each other and create matches

---

### Phase 4: Chess Game Logic (Week 4-5)
**Focus: Game Engine & Move Validation**

**Server Tasks:**
- [ ] ChessGame engine integration
- [ ] Move validation
- [ ] Turn management
- [ ] Game state synchronization
- [ ] Win/lose/draw detection
- [ ] Game history logging

**Client Tasks:**
- [ ] Chess board UI
- [ ] Move input handling
- [ ] Board state visualization
- [ ] Move highlighting
- [ ] Game status display

**Deliverable:** Complete chess game playable between two clients

---

### Phase 5: Advanced Features (Week 5-6)
**Focus: Polish & Additional Features**

**Server Tasks:**
- [ ] Draw offers
- [ ] Resignation handling
- [ ] Game log transmission
- [ ] Statistics updates
- [ ] Comprehensive logging
- [ ] Error handling improvements

**Client Tasks:**
- [ ] Game history viewer
- [ ] User statistics display
- [ ] Draw/resign UI
- [ ] Error message display
- [ ] UI polish

**Deliverable:** Fully featured chess game with all requirements

---

## Testing Strategy

### Unit Tests
```
Server:
- Socket creation/binding/listening
- Message parsing/serialization
- Chess move validation
- Session management
- Database operations

Client:
- Connection management
- UI components
- Message handling
```

### Integration Tests
```
- Client-server communication
- Multiple concurrent connections
- Game flow (challenge → play → end)
- Database persistence
- Error handling
```

### Load Tests
```
- 10 simultaneous connections
- 50+ concurrent users
- Multiple games running
- Database query performance
```

---

## Build & Deployment

### Server Build

**Makefile:**
```makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread -O2
LDFLAGS = -lpqxx -lpq

TARGET = chess_server
SOURCES = main.cpp server.cpp chess_game.cpp database_connection.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(TARGET)
```

**Build:**
```bash
make clean
make
```

**Run:**
```bash
./chess_server
```

### Client Build (Python)

**Requirements:**
```bash
pip install -r requirements.txt
```

**requirements.txt:**
```
PyQt5==5.15.9        # or tkinter (built-in)
python-chess==1.9.4  # For chess validation (optional)
```

**Run:**
```bash
python main.py
```

---

## Configuration Management

### Server Configuration (.env)
```env
# Server Settings
SERVER_PORT=8080
MAX_CONNECTIONS=100
TIMEOUT_SECONDS=300

# Database Settings
DB_HOST=172.21.192.1
DB_PORT=5432
DB_NAME=chess-app
DB_USER=postgres
DB_PASSWORD=your_password

# Logging
LOG_LEVEL=INFO
LOG_FILE=/var/log/chess_server.log
```

### Client Configuration
```python
# config/settings.py
SERVER_HOST = "localhost"
SERVER_PORT = 8080
RECONNECT_ATTEMPTS = 3
HEARTBEAT_INTERVAL = 30  # seconds
```

---

## Key Implementation Considerations

### Transport Layer Focus

1. **TCP Reliability**
   - Ensure proper error handling for connection failures
   - Implement timeout mechanisms
   - Handle partial message reception

2. **Message Framing**
   - Use length-prefixed messages to avoid fragmentation issues
   - Implement proper buffering for streaming data

3. **Thread Safety**
   - Protect all shared data structures with mutexes
   - Avoid deadlocks through consistent lock ordering

4. **Performance**
   - Minimize lock contention
   - Use non-blocking I/O where appropriate
   - Buffer messages to reduce system calls

### Not Primary Focus (Keep Simple)

- **Security**: Basic password hashing, no encryption required
- **Deployment**: Local network testing sufficient
- **Scalability**: Focus on correctness over extreme performance
- **UI Polish**: Functional UI is sufficient

---

## Grading Criteria Alignment (19 Points)

| Feature | Points | Implementation Priority |
|---------|--------|------------------------|
| Stream Processing | 1 | Phase 1 - Critical |
| Socket I/O Mechanism | 2 | Phase 1 - Critical |
| Account Management | 2 | Phase 2 - High |
| Login & Session | 2 | Phase 2 - High |
| Player List | 2 | Phase 3 - High |
| Challenge Forwarding | 2 | Phase 3 - High |
| Accept/Decline | 1 | Phase 3 - Medium |
| Move Forwarding | 2 | Phase 4 - Critical |
| Move Validation | 1 | Phase 4 - Critical |
| Game Result Detection | 1 | Phase 4 - High |
| Logging | 1 | Phase 5 - Medium |
| Result & Log Transmission | 2 | Phase 5 - Medium |
| Game Abort/Draw | 1 | Phase 5 - Low |

---

## Next Steps

1. **Read this architecture document**
2. **Review the detailed breakdown documents**:
   - `server_architecture.md`
   - `client_architecture.md`
   - `protocol_specification.md`
   - `development_workflow.md`
3. **Set up development environment**
4. **Begin Phase 1 implementation**
5. **Test incrementally after each phase**

---

**Document Version**: 1.0  
**Last Updated**: 2025-11-21  
**Status**: Architecture Design Complete
