# Message Type Definitions

This document defines all message types used in the chess server-client communication protocol.

---

## Message Categories

Messages are categorized by direction and purpose:

1. **Client-to-Server Requests** - User-initiated actions
2. **Server-to-Client Responses** - Direct replies to requests
3. **Server-to-Client Broadcasts** - Unsolicited notifications (opponent moves, challenges, etc.)
4. **Bidirectional** - Can be sent by either party

---

## 1. Connection & Session Verification

### Initial Connection Flow

**When a client first connects to the server, the protocol requires:**

1. **WebSocket/TCP connection established**
2. **Client MUST send one of:**
   - `VERIFY_SESSION` - If client has a previous session_id (reconnection)
   - `LOGIN` - If client needs to authenticate
   - `REGISTER` - If client is creating new account

**Session verification is the FIRST action after connection establishment.**

### CLIENT → SERVER

#### VERIFY_SESSION (FIRST MESSAGE)
**Purpose**: Verify existing session_id after connection/reconnection

```json
{
    "type": "VERIFY_SESSION",
    "session_id": "abc123def456...",
    "timestamp": 1700000000
}
```

### SERVER → CLIENT (Responses)

#### SESSION_VALID
**Purpose**: Confirm session is valid and restore connection

```json
{
    "type": "SESSION_VALID",
    "session_id": "abc123def456...",
    "user_data": {
        "user_id": 123,
        "username": "player1",
        "wins": 10,
        "losses": 5,
        "draws": 2,
        "rating": 1450
    },
    "active_game_id": 456,  // null if not in game
    "last_activity": 1700000000,
    "message": "Session restored successfully"
}
```

#### SESSION_INVALID
**Purpose**: Inform client that session_id is invalid/expired

```json
{
    "type": "SESSION_INVALID",
    "reason": "expired",  // "expired", "not_found", "invalid"
    "message": "Session expired. Please log in again."
}
```

**Note:** After receiving `SESSION_INVALID`, client MUST send `LOGIN` or `REGISTER`.

---

## 2. Authentication Messages

### CLIENT → SERVER

#### LOGIN
**Purpose**: Authenticate user with username and password

```json
{
    "type": "LOGIN",
    "username": "player1",
    "password": "hashed_password"
}
```

#### REGISTER
**Purpose**: Create new user account

```json
{
    "type": "REGISTER",
    "username": "newplayer",
    "password": "hashed_password",
    "email": "player@example.com"
}
```

#### LOGOUT
**Purpose**: End current session

```json
{
    "type": "LOGOUT",
    "session_id": "abc123"
}
```

### SERVER → CLIENT (Responses)

#### LOGIN_RESPONSE
**Purpose**: Confirm login success/failure

```json
{
    "type": "LOGIN_RESPONSE",
    "status": "success",  // or "failure"
    "session_id": "abc123",
    "user_data": {
        "user_id": 123,
        "username": "player1",
        "wins": 10,
        "losses": 5,
        "draws": 2,
        "rating": 1450
    },
    "message": "Login successful"  // or error message
}
```

#### REGISTER_RESPONSE
**Purpose**: Confirm registration success/failure

```json
{
    "type": "REGISTER_RESPONSE",
    "status": "success",  // or "failure"
    "user_id": 124,
    "message": "Registration successful"  // or error message
}
```

---

## 2. Lobby & Matchmaking Messages

### CLIENT → SERVER

#### GET_AVAILABLE_PLAYERS
**Purpose**: Request list of online players available for challenge

```json
{
    "type": "GET_AVAILABLE_PLAYERS",
    "session_id": "abc123"
}
```

### GET_RANKING_TABLE
**Purpose**: Request list of all players sorted by point
```json
{
    "type": "GET_RANKING_TABLE",
    "session_id": "abc123"
}
```

#### CHALLENGE
**Purpose**: Send challenge to specific player

```json
{
    "type": "CHALLENGE",
    "session_id": "abc123",
    "target_username": "opponent1",
    "preferred_color": "white"  // optional: "white", "black", or "random"
}
```

#### ACCEPT_CHALLENGE
**Purpose**: Accept received challenge

```json
{
    "type": "ACCEPT_CHALLENGE",
    "session_id": "abc123",
    "challenge_id": "challenge_789"
}
```

#### DECLINE_CHALLENGE
**Purpose**: Decline received challenge

```json
{
    "type": "DECLINE_CHALLENGE",
    "session_id": "abc123",
    "challenge_id": "challenge_789",
    "reason": "busy"  // optional
}
```

#### CANCEL_CHALLENGE
**Purpose**: Cancel a sent challenge before acceptance

```json
{
    "type": "CANCEL_CHALLENGE",
    "session_id": "abc123",
    "challenge_id": "challenge_789"
}
```

### SERVER → CLIENT (Responses)

#### PLAYER_LIST
**Purpose**: Send list of available players

```json
{
    "type": "PLAYER_LIST",
    "players": [
        {
            "username": "opponent1",
            "rating": 1500,
            "status": "available"  // "available", "in_game", "busy"
        },
        {
            "username": "opponent2",
            "rating": 1350,
            "status": "available"
        }
    ]
}
```

#### CHALLENGE_SENT
**Purpose**: Confirm challenge was sent to target player

```json
{
    "type": "CHALLENGE_SENT",
    "challenge_id": "challenge_789",
    "target_username": "opponent1",
    "status": "pending"
}
```

### SERVER → CLIENT (Broadcasts - Unsolicited)

#### CHALLENGE_RECEIVED
**Purpose**: Notify player they received a challenge (UNSOLICITED)

```json
{
    "type": "CHALLENGE_RECEIVED",
    "challenge_id": "challenge_789",
    "from_username": "challenger1",
    "from_rating": 1450,
    "preferred_color": "white",
    "timestamp": 1700000000
}
```

#### MATCH_STARTED
**Purpose**: Notify both players that game has started (UNSOLICITED)

```json
{
    "type": "MATCH_STARTED",
    "game_id": 456,
    "white_player": "player1",
    "black_player": "player2",
    "your_color": "white",  // varies per client
    "opponent_username": "player2",
    "opponent_rating": 1500,
    "time_control": "10+0"  // optional
}
```

#### CHALLENGE_CANCELLED
**Purpose**: Notify player that a challenge was cancelled (UNSOLICITED)

```json
{
    "type": "CHALLENGE_CANCELLED",
    "challenge_id": "challenge_789",
    "cancelled_by": "challenger1",
    "reason": "timeout"  // or "user_cancelled"
}
```

#### PLAYER_STATUS_UPDATE
**Purpose**: Broadcast when players come online/offline (UNSOLICITED)

```json
{
    "type": "PLAYER_STATUS_UPDATE",
    "username": "opponent1",
    "status": "online",  // "online", "offline", "in_game"
    "rating": 1500
}
```

---

## 3. Gameplay Messages

### CLIENT → SERVER

#### MOVE
**Purpose**: Submit chess move

```json
{
    "type": "MOVE",
    "session_id": "abc123",
    "game_id": 456,
    "move": "e2e4",  // UCI notation
    "timestamp": 1700000000
}
```

#### RESIGN
**Purpose**: Resign from current game

```json
{
    "type": "RESIGN",
    "session_id": "abc123",
    "game_id": 456
}
```

#### DRAW_OFFER
**Purpose**: Offer draw to opponent

```json
{
    "type": "DRAW_OFFER",
    "session_id": "abc123",
    "game_id": 456
}
```

#### DRAW_RESPONSE
**Purpose**: Accept or decline draw offer

```json
{
    "type": "DRAW_RESPONSE",
    "session_id": "abc123",
    "game_id": 456,
    "accepted": true  // or false
}
```

#### REQUEST_REMATCH
**Purpose**: Request rematch after game ends

```json
{
    "type": "REQUEST_REMATCH",
    "session_id": "abc123",
    "previous_game_id": 456
}
```

### SERVER → CLIENT (Responses)

#### MOVE_ACCEPTED
**Purpose**: Confirm move was valid and applied

```json
{
    "type": "MOVE_ACCEPTED",
    "game_id": 456,
    "move": "e2e4",
    "move_number": 1,
    "board_state": "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR",  // FEN
    "is_check": false,
    "is_checkmate": false,
    "legal_moves": ["e7e5", "e7e6", "d7d5", ...]  // optional
}
```

#### MOVE_REJECTED
**Purpose**: Inform player their move was invalid

```json
{
    "type": "MOVE_REJECTED",
    "game_id": 456,
    "move": "e2e5",
    "reason": "Illegal move",
    "legal_moves": ["e2e3", "e2e4", "g1f3", ...]
}
```

### SERVER → CLIENT (Broadcasts - Unsolicited)

#### OPPONENT_MOVE
**Purpose**: Notify player of opponent's move (UNSOLICITED)

```json
{
    "type": "OPPONENT_MOVE",
    "game_id": 456,
    "move": "e7e5",
    "move_number": 2,
    "board_state": "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR",
    "is_check": false,
    "captured_piece": null,  // or "pawn", "knight", etc.
    "time_remaining": 600,  // seconds
    "timestamp": 1700000001
}
```

#### GAME_ENDED
**Purpose**: Notify both players that game has concluded (UNSOLICITED)

```json
{
    "type": "GAME_ENDED",
    "game_id": 456,
    "result": "white_win",  // "white_win", "black_win", "draw"
    "reason": "checkmate",  // "checkmate", "resignation", "timeout", "draw_agreement", "stalemate"
    "winner": "player1",
    "loser": "player2",
    "final_board": "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR",
    "move_count": 45,
    "duration_seconds": 1200,
    "rating_changes": {
        "player1": +15,
        "player2": -15
    }
}
```

#### DRAW_OFFER_RECEIVED
**Purpose**: Notify player that opponent offered draw (UNSOLICITED)

```json
{
    "type": "DRAW_OFFER_RECEIVED",
    "game_id": 456,
    "from_username": "opponent1",
    "timestamp": 1700000002
}
```

#### REMATCH_REQUEST_RECEIVED
**Purpose**: Notify player of rematch request (UNSOLICITED)

```json
{
    "type": "REMATCH_REQUEST_RECEIVED",
    "from_username": "opponent1",
    "previous_game_id": 456
}
```

---

## 4. Game State & Information Messages

### CLIENT → SERVER

#### GET_GAME_STATE
**Purpose**: Request current state of active game

```json
{
    "type": "GET_GAME_STATE",
    "session_id": "abc123",
    "game_id": 456
}
```

#### GET_GAME_HISTORY
**Purpose**: Request past game records

```json
{
    "type": "GET_GAME_HISTORY",
    "session_id": "abc123",
    "user_id": 123,  // optional, defaults to self
    "limit": 10,
    "offset": 0
}
```

#### GET_LEADERBOARD
**Purpose**: Request top-rated players

```json
{
    "type": "GET_LEADERBOARD",
    "session_id": "abc123",
    "limit": 50
}
```

### SERVER → CLIENT (Responses)

#### GAME_STATE
**Purpose**: Send current game state

```json
{
    "type": "GAME_STATE",
    "game_id": 456,
    "board_state": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR",
    "current_turn": "white",
    "move_history": ["e2e4", "e7e5", "g1f3"],
    "captured_pieces": {
        "white": [],
        "black": []
    },
    "time_remaining": {
        "white": 600,
        "black": 600
    },
    "is_check": false,
    "legal_moves": ["d2d4", "d2d3", ...]
}
```

#### GAME_HISTORY
**Purpose**: Send past game records

```json
{
    "type": "GAME_HISTORY",
    "games": [
        {
            "game_id": 455,
            "white_player": "player1",
            "black_player": "opponent1",
            "result": "white_win",
            "date": "2024-11-20",
            "moves": ["e2e4", "e7e5", ...],
            "duration_seconds": 1800
        }
    ],
    "total_count": 45
}
```

#### LEADERBOARD
**Purpose**: Send top players list

```json
{
    "type": "LEADERBOARD",
    "players": [
        {
            "rank": 1,
            "username": "grandmaster1",
            "rating": 2100,
            "wins": 150,
            "losses": 50,
            "draws": 20
        },
        {
            "rank": 2,
            "username": "expert2",
            "rating": 1950,
            "wins": 120,
            "losses": 60,
            "draws": 15
        }
    ]
}
```

---

## 5. System Messages

### SERVER → CLIENT (Broadcasts - Unsolicited)

#### ERROR
**Purpose**: Notify client of error condition (UNSOLICITED)

```json
{
    "type": "ERROR",
    "error_code": "AUTH_REQUIRED",  // or "INVALID_MOVE", "SESSION_EXPIRED", etc.
    "message": "Authentication required for this action",
    "severity": "error",  // "warning", "error", "fatal"
    "timestamp": 1700000003
}
```

#### SERVER_SHUTDOWN
**Purpose**: Warn clients of impending shutdown (UNSOLICITED)

```json
{
    "type": "SERVER_SHUTDOWN",
    "message": "Server shutting down for maintenance",
    "shutdown_in_seconds": 60,
    "timestamp": 1700000004
}
```

#### SESSION_EXPIRED
**Purpose**: Notify client their session has expired (UNSOLICITED)

```json
{
    "type": "SESSION_EXPIRED",
    "reason": "inactivity",  // or "kicked", "duplicate_login"
    "message": "Your session has expired. Please log in again."
}
```

#### DUPLICATE_SESSION
**Purpose**: Notify client that another connection is already using this session (UNSOLICITED)

**When sent**: When a client attempts to verify a session that is already associated with an active connection (different socket).

```json
{
    "type": "DUPLICATE_SESSION",
    "session_id": "abc123def456...",
    "reason": "already_connected",
    "message": "Multiple connections with the same session are not allowed. Please close the existing connection first.",
    "timestamp": 1700000004
}
```

**Note**: The server enforces a single connection per session. If a user opens a second browser tab or window and tries to connect with the same session_id, they will receive this message. The existing connection remains active and unaffected.

**Client Action Required**: Close this connection and either:
1. Close the other browser tab/window with the active connection, OR
2. Log out from the active connection before reconnecting

#### RECONNECT_SUCCESS
**Purpose**: Confirm successful reconnection with existing session

```json
{
    "type": "RECONNECT_SUCCESS",
    "session_id": "abc123",
    "active_game_id": 456,  // if currently in game
    "missed_messages": [
        // Array of messages sent while disconnected
    ]
}
```

#### HEARTBEAT / PING
**Purpose**: Keep connection alive / measure latency (BIDIRECTIONAL)

```json
{
    "type": "PING",
    "timestamp": 1700000005
}
```

**Response:**
```json
{
    "type": "PONG",
    "timestamp": 1700000005
}
```

---

## 6. Chat Messages (Optional)

### CLIENT → SERVER

#### CHAT_MESSAGE
**Purpose**: Send chat message to opponent

```json
{
    "type": "CHAT_MESSAGE",
    "session_id": "abc123",
    "game_id": 456,
    "message": "Good game!",
    "timestamp": 1700000006
}
```

### SERVER → CLIENT (Broadcast - Unsolicited)

#### CHAT_MESSAGE_RECEIVED
**Purpose**: Deliver opponent's chat message (UNSOLICITED)

```json
{
    "type": "CHAT_MESSAGE_RECEIVED",
    "game_id": 456,
    "from_username": "opponent1",
    "message": "Thanks! You too!",
    "timestamp": 1700000007
}
```

---

## Message Flow Summary

### Connection Flow (REQUIRED FIRST)
**Immediately after TCP/WebSocket connection:**
- VERIFY_SESSION → SESSION_VALID / SESSION_INVALID (if client has session_id)
- LOGIN → LOGIN_RESPONSE (if no session_id or session invalid)
- REGISTER → REGISTER_RESPONSE (if creating new account)

### Client-Initiated (Request/Response)
All these require user action:
- VERIFY_SESSION → SESSION_VALID / SESSION_INVALID
- LOGIN → LOGIN_RESPONSE
- REGISTER → REGISTER_RESPONSE
- GET_AVAILABLE_PLAYERS → PLAYER_LIST
- CHALLENGE → CHALLENGE_SENT
- ACCEPT_CHALLENGE → MATCH_STARTED
- DECLINE_CHALLENGE → (acknowledgment)
- MOVE → MOVE_ACCEPTED / MOVE_REJECTED
- RESIGN → GAME_ENDED
- DRAW_OFFER → (acknowledgment)
- DRAW_RESPONSE → GAME_ENDED (if accepted)
- GET_GAME_STATE → GAME_STATE
- GET_GAME_HISTORY → GAME_HISTORY
- GET_LEADERBOARD → LEADERBOARD

### Server-Initiated (Unsolicited Broadcasts)
**These messages are sent WITHOUT user request:**

#### During Matchmaking:
- **CHALLENGE_RECEIVED** - When someone challenges you
- **CHALLENGE_CANCELLED** - When challenger cancels
- **MATCH_STARTED** - When opponent accepts your challenge OR you accept theirs
- **PLAYER_STATUS_UPDATE** - When players come online/offline

#### During Gameplay:
- **OPPONENT_MOVE** - Every time opponent makes a move
- **DRAW_OFFER_RECEIVED** - When opponent offers draw
- **GAME_ENDED** - When game concludes (checkmate, resignation, draw, timeout)
- **REMATCH_REQUEST_RECEIVED** - When opponent wants rematch

#### System Events:
- **ERROR** - When errors occur (invalid action, session issues, etc.)
- **SESSION_EXPIRED** - When session times out or is invalidated
- **SERVER_SHUTDOWN** - When server is shutting down
- **RECONNECT_SUCCESS** - When client reconnects with existing session

#### Optional:
- **CHAT_MESSAGE_RECEIVED** - When opponent sends chat message

---

## Implementation Notes

### For Server Developers:

1. **Thread Safety**: When broadcasting (OPPONENT_MOVE, GAME_ENDED, etc.), ensure proper mutex locking when accessing socket maps
2. **Socket Tracking**: Maintain bidirectional maps:
   - `socket_to_user_id` - for incoming message routing
   - `user_id_to_socket` - for broadcasting to specific users
3. **Game State**: Match manager should track which sockets are in which games for efficient broadcasting
4. **Error Handling**: Always send ERROR message for invalid requests rather than silent failures

### For Client Developers:

1. **Message Queue**: Implement queue to handle unsolicited messages (OPPONENT_MOVE, CHALLENGE_RECEIVED, etc.)
2. **Event Handlers**: Register callbacks for each unsolicited message type
3. **UI Updates**: Unsolicited messages should trigger immediate UI updates:
   - OPPONENT_MOVE → Update board display
   - CHALLENGE_RECEIVED → Show challenge notification/popup
   - GAME_ENDED → Show game result screen
   - PLAYER_STATUS_UPDATE → Update lobby player list
4. **Reconnection Logic**: Handle SESSION_EXPIRED by prompting re-login

### Message Type Enum (C++)

```cpp
enum class MessageType {
    // Connection & Session
    VERIFY_SESSION,
    SESSION_VALID,
    SESSION_INVALID,
    
    // Authentication
    LOGIN,
    LOGIN_RESPONSE,
    REGISTER,
    REGISTER_RESPONSE,
    LOGOUT,
    
    // Lobby
    GET_AVAILABLE_PLAYERS,
    PLAYER_LIST,
    PLAYER_STATUS_UPDATE,  // UNSOLICITED
    
    // Matchmaking
    CHALLENGE,
    CHALLENGE_SENT,
    CHALLENGE_RECEIVED,    // UNSOLICITED
    ACCEPT_CHALLENGE,
    DECLINE_CHALLENGE,
    CANCEL_CHALLENGE,
    CHALLENGE_CANCELLED,   // UNSOLICITED
    MATCH_STARTED,         // UNSOLICITED
    
    // Gameplay
    MOVE,
    MOVE_ACCEPTED,
    MOVE_REJECTED,
    OPPONENT_MOVE,         // UNSOLICITED
    RESIGN,
    DRAW_OFFER,
    DRAW_OFFER_RECEIVED,   // UNSOLICITED
    DRAW_RESPONSE,
    REQUEST_REMATCH,
    REMATCH_REQUEST_RECEIVED,  // UNSOLICITED
    GAME_ENDED,            // UNSOLICITED
    
    // Game State
    GET_GAME_STATE,
    GAME_STATE,
    GET_GAME_HISTORY,
    GAME_HISTORY,
    GET_LEADERBOARD,
    LEADERBOARD,
    
    // System
    ERROR,                 // UNSOLICITED
    SESSION_EXPIRED,       // UNSOLICITED
    SERVER_SHUTDOWN,       // UNSOLICITED
    DUPLICATE_SESSION,     // UNSOLICITED - Multiple connections with same session
    RECONNECT_SUCCESS,
    PING,
    PONG,
    
    // Chat (optional)
    CHAT_MESSAGE,
    CHAT_MESSAGE_RECEIVED  // UNSOLICITED
};
```

### Message Type String Map (Python)

```python
class MessageType:
    # Connection & Session
    VERIFY_SESSION = "VERIFY_SESSION"
    SESSION_VALID = "SESSION_VALID"
    SESSION_INVALID = "SESSION_INVALID"
    
    # Authentication
    LOGIN = "LOGIN"
    LOGIN_RESPONSE = "LOGIN_RESPONSE"
    REGISTER = "REGISTER"
    REGISTER_RESPONSE = "REGISTER_RESPONSE"
    LOGOUT = "LOGOUT"
    
    # Lobby
    GET_AVAILABLE_PLAYERS = "GET_AVAILABLE_PLAYERS"
    PLAYER_LIST = "PLAYER_LIST"
    PLAYER_STATUS_UPDATE = "PLAYER_STATUS_UPDATE"  # UNSOLICITED
    
    # Matchmaking
    CHALLENGE = "CHALLENGE"
    CHALLENGE_SENT = "CHALLENGE_SENT"
    CHALLENGE_RECEIVED = "CHALLENGE_RECEIVED"  # UNSOLICITED
    ACCEPT_CHALLENGE = "ACCEPT_CHALLENGE"
    DECLINE_CHALLENGE = "DECLINE_CHALLENGE"
    CANCEL_CHALLENGE = "CANCEL_CHALLENGE"
    CHALLENGE_CANCELLED = "CHALLENGE_CANCELLED"  # UNSOLICITED
    MATCH_STARTED = "MATCH_STARTED"  # UNSOLICITED
    
    # Gameplay
    MOVE = "MOVE"
    MOVE_ACCEPTED = "MOVE_ACCEPTED"
    MOVE_REJECTED = "MOVE_REJECTED"
    OPPONENT_MOVE = "OPPONENT_MOVE"  # UNSOLICITED
    RESIGN = "RESIGN"
    DRAW_OFFER = "DRAW_OFFER"
    DRAW_OFFER_RECEIVED = "DRAW_OFFER_RECEIVED"  # UNSOLICITED
    DRAW_RESPONSE = "DRAW_RESPONSE"
    REQUEST_REMATCH = "REQUEST_REMATCH"
    REMATCH_REQUEST_RECEIVED = "REMATCH_REQUEST_RECEIVED"  # UNSOLICITED
    GAME_ENDED = "GAME_ENDED"  # UNSOLICITED
    
    # Game State
    GET_GAME_STATE = "GET_GAME_STATE"
    GAME_STATE = "GAME_STATE"
    GET_GAME_HISTORY = "GET_GAME_HISTORY"
    GAME_HISTORY = "GAME_HISTORY"
    GET_LEADERBOARD = "GET_LEADERBOARD"
    LEADERBOARD = "LEADERBOARD"
    
    # System
    ERROR = "ERROR"  # UNSOLICITED
    SESSION_EXPIRED = "SESSION_EXPIRED"  # UNSOLICITED
    SERVER_SHUTDOWN = "SERVER_SHUTDOWN"  # UNSOLICITED
    DUPLICATE_SESSION = "DUPLICATE_SESSION"  # UNSOLICITED - Multiple connections with same session
    RECONNECT_SUCCESS = "RECONNECT_SUCCESS"
    PING = "PING"
    PONG = "PONG"
    
    # Chat
    CHAT_MESSAGE = "CHAT_MESSAGE"
    CHAT_MESSAGE_RECEIVED = "CHAT_MESSAGE_RECEIVED"  # UNSOLICITED
```

---

## Quick Reference: Unsolicited (Server-Initiated) Messages

Messages that the server sends **WITHOUT** a client request:

| Message Type | When Sent | Recipient |
|-------------|-----------|-----------|
| `CHALLENGE_RECEIVED` | Someone challenges the player | Challenge target |
| `CHALLENGE_CANCELLED` | Challenger cancels before acceptance | Challenge target |
| `MATCH_STARTED` | Challenge accepted | Both players |
| `PLAYER_STATUS_UPDATE` | Player status changes | All connected clients |
| `OPPONENT_MOVE` | Opponent makes a move | Other player in game |
| `DRAW_OFFER_RECEIVED` | Opponent offers draw | Other player in game |
| `GAME_ENDED` | Game concludes | Both players in game |
| `REMATCH_REQUEST_RECEIVED` | Opponent requests rematch | Other player |
| `ERROR` | Error condition occurs | Affected client(s) |
| `SESSION_EXPIRED` | Session times out | Session owner |
| `DUPLICATE_SESSION` | Another connection already using session | New connection attempt |
| `SERVER_SHUTDOWN` | Server shutting down | All connected clients |
| `CHAT_MESSAGE_RECEIVED` | Opponent sends chat | Other player in game |

**These messages require client-side event handlers to process asynchronously!**

---

## Protocol Version

**Current Version**: 1.0  
**Date**: November 24, 2024  
**Compatibility**: All messages should include protocol version for future compatibility checking.
