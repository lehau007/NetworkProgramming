# Chess Server - Message Types & Protocol

## Overview

This server implements a complete message-based protocol for the chess game system with:

- ✅ **All message types defined** (40+ message types)
- ✅ **Request/Response handlers** for client-initiated actions
- ✅ **Broadcast support** for unsolicited server messages
- ✅ **Session management** with database persistence
- ✅ **WebSocket protocol** for real-time communication
- ✅ **User authentication** and registration
- ✅ **Game history** and leaderboard queries

---

## Message Categories

### 1. Connection & Session (3 types)
- `VERIFY_SESSION` → `SESSION_VALID` / `SESSION_INVALID`

### 2. Authentication (5 types)
- `LOGIN` → `LOGIN_RESPONSE`
- `REGISTER` → `REGISTER_RESPONSE`
- `LOGOUT`

### 3. Lobby (3 types)
- `GET_AVAILABLE_PLAYERS` → `PLAYER_LIST`
- `PLAYER_STATUS_UPDATE` (broadcast)

### 4. Matchmaking (8 types)
- `CHALLENGE` → `CHALLENGE_SENT`
- `CHALLENGE_RECEIVED` (broadcast)
- `ACCEPT_CHALLENGE` → `MATCH_STARTED`
- `DECLINE_CHALLENGE`
- `CANCEL_CHALLENGE`
- `CHALLENGE_CANCELLED` (broadcast)
- `MATCH_STARTED` (broadcast)

### 5. Gameplay (11 types)
- `MOVE` → `MOVE_ACCEPTED` / `MOVE_REJECTED`
- `OPPONENT_MOVE` (broadcast)
- `RESIGN` → `GAME_ENDED`
- `DRAW_OFFER` / `DRAW_RESPONSE`
- `DRAW_OFFER_RECEIVED` (broadcast)
- `REQUEST_REMATCH` / `REMATCH_REQUEST_RECEIVED`
- `GAME_ENDED` (broadcast)

### 6. Game State (6 types)
- `GET_GAME_STATE` → `GAME_STATE`
- `GET_GAME_HISTORY` → `GAME_HISTORY`
- `GET_LEADERBOARD` → `LEADERBOARD`

### 7. System (6 types)
- `PING` → `PONG`
- `ERROR` (broadcast)
- `SESSION_EXPIRED` (broadcast)
- `SERVER_SHUTDOWN` (broadcast)
- `RECONNECT_SUCCESS`

### 8. Chat (2 types - optional)
- `CHAT_MESSAGE` → `CHAT_MESSAGE_RECEIVED` (broadcast)

---

## Implementation Status

### ✅ Fully Implemented

#### Connection & Session
- [x] `VERIFY_SESSION` - Validates existing session_id from database
- [x] `SESSION_VALID` - Returns user data and active game
- [x] `SESSION_INVALID` - Prompts re-login

#### Authentication
- [x] `LOGIN` - Database authentication with password hash
- [x] `LOGIN_RESPONSE` - Creates session, returns session_id
- [x] `REGISTER` - Creates user in database
- [x] `REGISTER_RESPONSE` - Returns user_id
- [x] `LOGOUT` - Removes session from database

#### Lobby
- [x] `GET_AVAILABLE_PLAYERS` - Queries all users from database
- [x] `PLAYER_LIST` - Returns username, rating, status

#### Game State
- [x] `GET_GAME_HISTORY` - Queries game_history table
- [x] `GAME_HISTORY` - Returns past games
- [x] `GET_LEADERBOARD` - Queries top users by rating
- [x] `LEADERBOARD` - Returns ranked players

#### System
- [x] `PING` / `PONG` - Heartbeat/latency check
- [x] `ERROR` - Generic error responses

### 🔧 Stubs (TODO)

#### Matchmaking
- [ ] `CHALLENGE` - Needs MatchManager
- [ ] `CHALLENGE_RECEIVED` (broadcast)
- [ ] `ACCEPT_CHALLENGE` - Needs GameManager
- [ ] `DECLINE_CHALLENGE`
- [ ] `CANCEL_CHALLENGE`
- [ ] `CHALLENGE_CANCELLED` (broadcast)
- [ ] `MATCH_STARTED` (broadcast)

#### Gameplay
- [ ] `MOVE` - Needs ChessGame integration
- [ ] `MOVE_ACCEPTED` / `MOVE_REJECTED`
- [ ] `OPPONENT_MOVE` (broadcast)
- [ ] `RESIGN` - Needs GameManager
- [ ] `DRAW_OFFER` / `DRAW_RESPONSE`
- [ ] `DRAW_OFFER_RECEIVED` (broadcast)
- [ ] `REQUEST_REMATCH`
- [ ] `REMATCH_REQUEST_RECEIVED` (broadcast)
- [ ] `GAME_ENDED` (broadcast)

#### Game State
- [ ] `GET_GAME_STATE` - Needs active game tracking

#### Chat
- [ ] `CHAT_MESSAGE` / `CHAT_MESSAGE_RECEIVED`

---

## Server Architecture

```
server.cpp                           [Main server loop]
    ↓
handle_client_connection()          [Thread per client]
    ↓
WebSocketHandler                    [WebSocket protocol]
    ↓
MessageHandler                      [Route messages]
    ↓
    ├─ SessionManager               [Session verification]
    ├─ UserRepository               [User authentication]
    ├─ GameRepository               [Game history]
    └─ [TODO] MatchManager          [Active games]
```

### File Structure

```
server/
├── server.cpp                      # Main server with WebSocket
├── utils/
│   ├── message_types.h             # All message type constants
│   ├── message_handler.h/cpp       # Message routing and handling
│   └── json_helper.h/cpp           # JSON utilities
├── network/
│   ├── websocket_handler.h/cpp     # WebSocket protocol (RFC 6455)
│   └── socket_handler.h/cpp        # TCP socket wrapper
├── session/
│   └── session_manager.h/cpp       # Session lifecycle + cache
├── database/
│   ├── session_repository.h/cpp    # Session DB operations
│   ├── user_repository.h/cpp       # User DB operations
│   ├── game_repository.h/cpp       # Game DB operations
│   └── schema.sql                  # Database schema
└── game/
    ├── chess_game.cpp              # Chess engine
    └── match_manager.cpp           # [TODO] Active game manager
```

---

## Building & Running

### Prerequisites

```bash
# On WSL/Linux
sudo apt-get install postgresql libpqxx-dev libssl-dev nlohmann-json3-dev
```

### Database Setup

```bash
# Create database
psql -U postgres -c "CREATE DATABASE \"chess-app\";"

# Load schema
cd server
make setup_db
```

### Build Server

```bash
cd server

# Build everything
make

# Or build just the server
make chess_server
```

### Run Server

```bash
# Run chess server
./chess_server

# Or use make
make run_server
```

**Server will start on:** `ws://localhost:8080`

---

## Testing Messages

### Using JavaScript/Browser

```javascript
const ws = new WebSocket('ws://localhost:8080');

ws.onopen = () => {
    console.log('Connected');
    
    // Test 1: Register new user
    ws.send(JSON.stringify({
        type: 'REGISTER',
        username: 'testuser',
        password: 'hash_password_123',
        email: 'test@example.com'
    }));
};

ws.onmessage = (event) => {
    const msg = JSON.parse(event.data);
    console.log('Received:', msg);
    
    if (msg.type === 'REGISTER_RESPONSE' && msg.status === 'success') {
        // Test 2: Login
        ws.send(JSON.stringify({
            type: 'LOGIN',
            username: 'testuser',
            password: 'hash_password_123'
        }));
    }
    
    if (msg.type === 'LOGIN_RESPONSE' && msg.status === 'success') {
        const session_id = msg.session_id;
        
        // Test 3: Get leaderboard
        ws.send(JSON.stringify({
            type: 'GET_LEADERBOARD',
            session_id: session_id,
            limit: 10
        }));
    }
};
```

### Using Python

```python
import websocket
import json

def on_message(ws, message):
    msg = json.loads(message)
    print(f"Received: {msg['type']}")
    print(msg)

def on_open(ws):
    # Test login
    ws.send(json.dumps({
        'type': 'LOGIN',
        'username': 'alice',
        'password': 'hash_alice_123'
    }))

ws = websocket.WebSocketApp(
    'ws://localhost:8080',
    on_message=on_message,
    on_open=on_open
)

ws.run_forever()
```

---

## Example Message Flows

### Flow 1: First Connection (New User)

```
Client                                  Server
  │                                       │
  ├─[1] WebSocket Connect ───────────────►│
  │◄─[2] Connection Established ──────────┤
  │                                       │
  ├─[3] REGISTER ─────────────────────────►│
  │     { username, password, email }     │ [Create user in DB]
  │◄─[4] REGISTER_RESPONSE ────────────────┤
  │     { status: success, user_id }      │
  │                                       │
  ├─[5] LOGIN ────────────────────────────►│
  │     { username, password }            │ [Authenticate]
  │                                       │ [Create session in DB]
  │◄─[6] LOGIN_RESPONSE ───────────────────┤
  │     { status: success, session_id,    │
  │       user_data: {...} }              │
  │                                       │
  ├─[7] GET_AVAILABLE_PLAYERS ────────────►│
  │     { session_id }                    │ [Verify session]
  │                                       │ [Query users]
  │◄─[8] PLAYER_LIST ──────────────────────┤
  │     { players: [...] }                │
```

### Flow 2: Reconnection (Returning User)

```
Client                                  Server
  │                                       │
  ├─[1] WebSocket Connect ───────────────►│
  │◄─[2] Connection Established ──────────┤
  │                                       │
  ├─[3] VERIFY_SESSION ───────────────────►│
  │     { session_id: "abc123..." }       │ [Check DB]
  │                                       │ [Update socket mapping]
  │◄─[4] SESSION_VALID ────────────────────┤
  │     { session_id, user_data,          │
  │       active_game_id }                │
  │                                       │
  [Client can immediately send requests]  │
```

### Flow 3: Query Game History

```
Client                                  Server
  │                                       │
  ├─[1] GET_GAME_HISTORY ─────────────────►│
  │     { session_id, user_id, limit }    │ [Verify session]
  │                                       │ [Query game_history table]
  │◄─[2] GAME_HISTORY ─────────────────────┤
  │     { games: [                        │
  │       { game_id, white_player,        │
  │         black_player, result, ... }   │
  │     ], total_count }                  │
```

---

## Session Management

### Session Lifecycle

1. **Login** → Session created in `active_sessions` table
2. **Verify** → Session checked in database on each reconnect
3. **Activity** → `last_activity` updated on each request
4. **Timeout** → Sessions expire after 30 minutes inactivity
5. **Cleanup** → Background thread removes expired sessions every 60 seconds

### Session Storage

**Database (PostgreSQL):**
```sql
SELECT * FROM active_sessions;
 session_id | user_id |     login_time      |   last_activity    | ip_address
------------+---------+---------------------+--------------------+------------
 abc123...  |       1 | 2024-11-24 10:00:00 | 2024-11-24 10:15:00| 127.0.0.1
```

**In-Memory Cache:**
```cpp
sessions_by_id: { "abc123..." → Session{...} }
sessions_by_socket: { 42 → "abc123..." }
sessions_by_user_id: { 1 → "abc123..." }
```

---

## Error Handling

All errors are sent as ERROR messages:

```json
{
    "type": "ERROR",
    "error_code": "INVALID_SESSION",
    "message": "Session not found or expired",
    "severity": "error",
    "timestamp": 1700000000
}
```

### Common Error Codes

- `MISSING_FIELD` - Required field missing from request
- `INVALID_SESSION` - Session invalid or expired
- `INVALID_MESSAGE` - Malformed JSON or missing type
- `UNKNOWN_MESSAGE_TYPE` - Unrecognized message type
- `PARSE_ERROR` - Failed to parse JSON
- `INTERNAL_ERROR` - Server-side exception
- `AUTH_REQUIRED` - Action requires authentication
- `NOT_IMPLEMENTED` - Feature stub (TODO)

---

## Next Steps

### Priority 1: Match Manager
Implement active game tracking for:
- Challenge system
- Game creation
- Move validation
- Game state queries

### Priority 2: Chess Engine Integration
Connect ChessGame class to handle:
- Move validation
- Legal move generation
- Check/checkmate detection
- Board state management

### Priority 3: Broadcasting
Implement user-to-socket mapping for:
- OPPONENT_MOVE broadcasts
- CHALLENGE_RECEIVED notifications
- GAME_ENDED notifications
- PLAYER_STATUS_UPDATE broadcasts

### Priority 4: Additional Features
- Chat system
- Rematch support
- Time controls
- Game spectating

---

## Summary

**Current Implementation:**
- ✅ 40+ message types defined
- ✅ Message routing and dispatch system
- ✅ Session verification (first action after connect)
- ✅ User authentication (login/register)
- ✅ Leaderboard and game history queries
- ✅ WebSocket communication
- ✅ Database persistence
- ✅ Thread-per-client architecture
- ✅ Session cleanup background thread

**Message Handler automatically routes:**
- Connection messages → Session verification
- Auth messages → User database
- Lobby messages → Player queries
- Game state messages → Game history
- System messages → Ping/pong
- Unknown messages → Error response

The server is production-ready for session management, authentication, and player queries. Matchmaking and gameplay features require MatchManager and ChessGame integration.
