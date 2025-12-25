# ♟️ Chess Game - Network Programming Project

> A real-time multiplayer chess server with WebSocket support, built with C++ and PostgreSQL

**Core Network Techniques**: TCP/IP Sockets • Multi-threaded Server • WebSocket Protocol (RFC 6455) • Real-time Broadcasting • Session Management

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-13+-blue.svg)](https://www. postgresql.org/)

---

## 🎯 Project Overview

A fully-featured online chess game server implementing TCP/IP socket programming with WebSocket protocol (RFC 6455).  This project demonstrates network programming concepts including multi-threaded server architecture, real-time bidirectional communication, and database integration.

**Focus**: Transport Layer (TCP/IP) and Application Protocol Implementation

### ✨ Key Features

- ✅ **WebSocket Protocol (RFC 6455)** - Full handshake, frame encoding/decoding, control frames
- ✅ **Multi-Threaded Server** - Thread-per-client architecture with mutex synchronization
- ✅ **Real-Time Gameplay** - Instant move broadcasting and game state synchronization
- ✅ **Complete Chess Engine** - Move validation, checkmate/draw detection
- ✅ **User Authentication** - Registration, login, session management
- ✅ **Challenge System** - Player matching and game creation
- ✅ **PostgreSQL Integration** - User data, game history, and statistics
- ✅ **Game Features** - Resign, draw offers, rematch, game history, leaderboard

---

## 🌐 TCP/IP Protocol Techniques Implemented

### Transport Layer
- **TCP Socket Programming** - `socket()`, `bind()`, `listen()`, `accept()`
- **Connection Management** - Backlog queue, graceful shutdown, connection pooling
- **Stream Processing** - Length-prefixed message framing, buffering, partial reads
- **Error Handling** - Connection failures, timeouts, network interruptions

### Application Layer
- **WebSocket Handshake** - HTTP upgrade with Sec-WebSocket-Key validation
- **Frame Encoding/Decoding** - Opcode parsing, payload masking/unmasking
- **Control Frames** - PING/PONG keep-alive, graceful CLOSE handling
- **Message Protocol** - Custom JSON-based application protocol

### Concurrency & Threading
- **Thread-per-Client Model** - POSIX threads (pthread) for each connection
- **Mutex Synchronization** - `pthread_mutex_t` for shared data structures
- **Thread Safety** - Lock ordering to prevent deadlocks
- **Session Cleanup Thread** - Periodic background cleanup

### Network I/O
- **Blocking I/O** - Simplified read/write with proper error handling
- **Buffer Management** - Send/receive buffers for stream data
- **Broadcasting** - One-to-many message distribution to connected clients

---

## 📁 Project Structure

```
NetworkProgramming/
├── server/                     # C++ Server Implementation
│   ├── network/               # TCP/WebSocket handlers
│   ├── session/               # Authentication & sessions
│   ├── game/                  # Chess engine & match manager
│   ├── database/              # PostgreSQL repositories
│   ├── utils/                 # Message handlers & utilities
│   ├── config/                # Configuration files
│   ├── server. cpp             # Main server entry point
│   └── Makefile              # Build configuration
│
├── client/                    # Web-based clients
|   ├── js  # All script for logic of index page. 
│   ├── index.html     # Full chess game UI
│   ├── test_request.html     # Protocol testing client
│   └── websocket_test.html   # WebSocket connection test
│
├── docs/                      # Additional documentation
│
├── BUILD_GUIDE.md            # Complete build & deployment guide
├── IMPLEMENTATION_SUMMARY.md # Technical implementation details
├── WEBSOCKET_README.md       # WebSocket protocol guide
├── project_architecture.md   # System architecture documentation
└── README.md                 # This file
```

---

## 🚀 Quick Start

### Prerequisites

**Linux/WSL Environment Required** (Compiles on Linux/WSL, not Windows directly)

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    postgresql \
    libpqxx-dev \
    libssl-dev \
    nlohmann-json3-dev \
    g++ \
    make
```

### Database Setup

```bash
# Create database
sudo -u postgres psql -c "CREATE DATABASE \"chess-app\";"

# Load schema
cd server
psql -U postgres -d chess-app -f database/schema.sql
```

### Build & Run Server

```bash
# Build
cd server
make clean
make chess_server

# Run
./chess_server
```

**Expected output:**
```
========================================
    Chess Server - Network Protocol    
========================================
Starting server on port 8080... 
[MatchManager] Initialized
[Server] Listening on 0.0.0.0:8080
[Server] Waiting for connections... 
```

### Test the Server

Open `client/chess_client.html` in your browser and connect to `ws://localhost:8080`

---

## 🎮 How It Works

### System Architecture

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│  Client A   │◄───────►│              │         │             │
│ (Player 1)  │  WS/TCP │    Server    │◄───────►│ PostgreSQL  │
└─────────────┘         │ Multi-thread │         │  Database   │
                        │              │         └─────────────┘
┌─────────────┐         │              │
│  Client B   │◄───────►│              │
│ (Player 2)  │  WS/TCP │              │
└─────────────┘         └──────────────┘
```

### Communication Flow

1. **TCP Connection** - Client initiates TCP handshake to server:8080
2. **WebSocket Handshake** - HTTP upgrade with Sec-WebSocket-Key exchange
3. **Authentication** - User login/registration with session token
4. **Matchmaking** - Challenge other players via JSON messages
5. **Gameplay** - Real-time move exchange with server-side validation
6. **Broadcasting** - Server broadcasts moves to opponent in real-time
7. **Game End** - Result storage and statistics update in PostgreSQL

### Message Protocol

JSON-based messages over WebSocket frames:

```javascript
// Client → Server: Make a move
{
    "type": "MOVE",
    "session_id": "abc123",
    "game_id": 42,
    "move": "e2e4"
}

// Server → Client: Move accepted
{
    "type": "MOVE_ACCEPTED",
    "game_id": 42
}

// Server → Opponent: Broadcast move
{
    "type": "OPPONENT_MOVE",
    "game_id": 42,
    "move": "e2e4",
    "move_number": 1
}
```

---

## 📚 Documentation

| Document | Description |
|----------|-------------|
| **[BUILD_GUIDE.md](BUILD_GUIDE. md)** | Complete compilation, deployment, and testing guide |
| **[IMPLEMENTATION_SUMMARY.md](IMPLEMENTATION_SUMMARY.md)** | Technical details of all implemented features |
| **[WEBSOCKET_README.md](WEBSOCKET_README.md)** | WebSocket protocol implementation guide |
| **[project_architecture.md](project_architecture.md)** | Detailed system architecture and design |

---

## 🔧 Technology Stack

### Server (C++)
- **Language**: C++17
- **Threading**: POSIX threads (pthread)
- **Networking**: BSD Sockets API (TCP/IP)
- **Protocol**: WebSocket (RFC 6455)
- **Database**: libpqxx (PostgreSQL C++ API)
- **JSON**: nlohmann/json
- **Crypto**: OpenSSL (SHA-1 for WebSocket handshake)
- **Build**: GNU Make

### Client (Web)
- **Protocol**: WebSocket (RFC 6455)
- **UI**: HTML5, CSS3, JavaScript
- **Chess Board**: Custom SVG rendering

### Database (PostgreSQL)
- User accounts and authentication
- Active sessions with timestamps
- Game history and moves (JSONB)
- User statistics and ELO ratings

---

## 🎯 Features

### Implemented Features (Production Ready)

#### Authentication & Session Management
- ✅ User registration with email
- ✅ Login/logout with session tokens
- ✅ Session validation and cleanup
- ✅ Duplicate session prevention

#### Matchmaking & Lobby
- ✅ Get available players
- ✅ Send/receive challenges
- ✅ Accept/decline challenges
- ✅ Cancel sent challenges
- ✅ Player status tracking

#### Chess Gameplay
- ✅ Move validation (legal moves only)
- ✅ Real-time move broadcasting
- ✅ Checkmate detection
- ✅ Stalemate/draw detection
- ✅ Resign functionality
- ✅ Draw offers and responses
- ✅ Move history tracking

#### Game State & Statistics
- ✅ Get current game state
- ✅ View game history
- ✅ Leaderboard with rankings
- ✅ Win/loss/draw statistics
- ✅ ELO-style rating system

#### System Features
- ✅ Multi-threaded architecture
- ✅ Thread-safe operations (mutex locks)
- ✅ Connection monitoring (ping/pong)
- ✅ Graceful connection handling
- ✅ Comprehensive error handling
- ✅ Server-side logging

---

## 🧪 Testing

### Run Test Client

```bash
# Open in browser
client/test_request.html
```

### Sample Test Flow

1. **Register** → `REGISTER` → `REGISTER_RESPONSE`
2. **Login** → `LOGIN` → `LOGIN_RESPONSE` (get session_id)
3. **Get Players** → `GET_AVAILABLE_PLAYERS` → `PLAYER_LIST`
4. **Challenge** → `CHALLENGE` → `CHALLENGE_SENT` / `CHALLENGE_RECEIVED` (broadcast)
5. **Accept** → `ACCEPT_CHALLENGE` → `MATCH_STARTED` (broadcast to both)
6. **Play** → `MOVE` → `MOVE_ACCEPTED` / `OPPONENT_MOVE` (broadcast)
7. **End Game** → `RESIGN` / `DRAW_OFFER` → `GAME_ENDED` (broadcast)

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for complete testing workflows.

---

## 🛠️ Development

### Build Targets

```bash
make chess_server       # Build main chess server
make websocket_server   # Build WebSocket example
make clean             # Clean all build files
make all              # Build everything
```

### Configuration

Edit `server/config/. env`:

```env
SERVER_PORT=8080
DB_HOST=localhost
DB_PORT=5432
DB_NAME=chess-app
DB_USER=postgres
DB_PASSWORD=your_password
```

---

## 📈 Performance

- **Concurrent Connections**: Tested with 50+ simultaneous users
- **Thread Model**: Thread-per-client with efficient mutex locking
- **Database**: Connection pooling via libpqxx
- **Message Size**: Max 10MB per WebSocket frame
- **Session Cleanup**: Automatic cleanup every 60 seconds
- **Latency**: <50ms move broadcasting (local network)

---

## 🔐 Security Considerations

### Current Implementation
- Basic password storage (plain text - **NOT production-ready**)
- Session token generation
- Input validation on all messages
- SQL injection prevention (parameterized queries)
- Max payload size limits

### Production Recommendations
- ✏️ Use WSS (WebSocket Secure) with TLS/SSL
- ✏️ Implement bcrypt password hashing
- ✏️ Add rate limiting per IP/session
- ✏️ Enable CSRF protection
- ✏️ Implement proper authentication tokens (JWT)

---

## 🎓 Learning Objectives Achieved

This project demonstrates proficiency in:

✅ **Transport Layer Programming** - TCP socket creation, binding, listening, accepting  
✅ **Application Protocol Design** - Custom JSON-based protocol over WebSocket  
✅ **Multi-threaded Programming** - Thread synchronization with mutexes  
✅ **Network I/O** - Blocking I/O, buffering, message framing  
✅ **Database Integration** - PostgreSQL connection, queries, transactions  
✅ **Real-time Systems** - Bidirectional communication, broadcasting  
✅ **Software Architecture** - Layered architecture, separation of concerns  

---

## 📊 Project Statistics

- **Lines of Code**: ~5,000+ (C++ server)
- **Files**: 30+ source files
- **Message Types**: 25+ protocol messages
- **Database Tables**: 4 (users, games, sessions, logs)
- **Development Time**: 6 weeks (5 phases)

---

## 🤝 Contributing

This is an educational project. Contributions for learning purposes are welcome!

---

## 📝 License

This project is licensed under the **Apache License 2.0** - see [LICENSE](LICENSE) file for details.

---

## 👤 Author

- **lehau007** - Network Programming Project
- **Sang Pham**
- **Doan Dung**

---

## 🙏 Acknowledgments

- WebSocket Protocol: RFC 6455
- Chess Rules: FIDE Laws of Chess
- PostgreSQL C++ Library: libpqxx
- JSON Library: nlohmann/json

---

**♟️ Ready to play chess?  Build the server and start challenging players! **

For detailed setup instructions, see **[BUILD_GUIDE.md](BUILD_GUIDE.md)**
