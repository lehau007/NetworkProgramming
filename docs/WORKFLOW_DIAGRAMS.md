# Chess Server - Detailed Workflow Diagrams

## Table of Contents
1. [Complete System Flow](#complete-system-flow)
2. [Authentication Flow](#authentication-flow)
3. [Matchmaking Flow](#matchmaking-flow)
4. [Game Play Flow](#gameplay-flow)
5. [WebSocket Communication Flow](#websocket-communication-flow)
6. [Database Operations Flow](#database-operations-flow)
7. [Thread Architecture](#thread-architecture)

---

## Complete System Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CHESS GAME SYSTEM OVERVIEW                          │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────┐                                              ┌──────────────┐
│  Client A    │                                              │  Client B    │
│  (Browser/   │                                              │  (Browser/   │
│   Desktop)   │                                              │   Desktop)   │
└──────┬───────┘                                              └──────┬───────┘
       │                                                             │
       │ WebSocket                                    WebSocket     │
       │ Connection                                   Connection    │
       ▼                                                             ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│                              SERVER (C++)                                    │
├──────────────────────────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────────────────────────────┐     │
│  │                    NETWORK LAYER (Port 8080)                       │     │
│  │  ┌──────────────┐         ┌──────────────────────────────┐        │     │
│  │  │SocketHandler │────────▶│   WebSocket Handler          │        │     │
│  │  │ (TCP Listen) │         │ • Handshake                  │        │     │
│  │  └──────────────┘         │ • Frame encode/decode        │        │     │
│  │                           │ • PING/PONG/CLOSE            │        │     │
│  │                           └──────────────────────────────┘        │     │
│  └──────────────┬─────────────────────────────────────────────────────     │
│                 │                                                           │
│  ┌──────────────▼──────────────────────────────────────────────────────┐   │
│  │                    SESSION MANAGEMENT                               │   │
│  │  ┌─────────────────┐    ┌──────────────────┐                       │   │
│  │  │ SessionManager  │◀──▶│  Active Sessions │                       │   │
│  │  │                 │    │  Map<socket,user>│                       │   │
│  │  └─────────────────┘    └──────────────────┘                       │   │
│  └──────────────┬──────────────────────────────────────────────────────┘   │
│                 │                                                           │
│  ┌──────────────▼──────────────────────────────────────────────────────┐   │
│  │                    APPLICATION LAYER                                │   │
│  │  ┌─────────────────┐    ┌─────────────────┐   ┌────────────────┐  │   │
│  │  │  MatchManager   │◀──▶│  Active Games   │◀─▶│  ChessEngine   │  │   │
│  │  │                 │    │  Map<id, game>  │   │  (Rules)       │  │   │
│  │  └─────────────────┘    └─────────────────┘   └────────────────┘  │   │
│  └──────────────┬──────────────────────────────────────────────────────┘   │
│                 │                                                           │
│  ┌──────────────▼──────────────────────────────────────────────────────┐   │
│  │                    DATA LAYER                                       │   │
│  │  ┌──────────────────┐    ┌──────────────────┐                      │   │
│  │  │ UserRepository   │    │  GameRepository  │                      │   │
│  │  │ • create_user    │    │  • create_game   │                      │   │
│  │  │ • authenticate   │    │  • add_move      │                      │   │
│  │  │ • update_stats   │    │  • end_game      │                      │   │
│  │  └────────┬─────────┘    └────────┬─────────┘                      │   │
│  └───────────┼──────────────────────┼─────────────────────────────────┘   │
└──────────────┼──────────────────────┼───────────────────────────────────────┘
               │                      │
               ▼                      ▼
┌──────────────────────────────────────────────────────────────────────────────┐
│                         PostgreSQL Database                                  │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────┐            │
│  │  users          │  │  game_history   │  │ active_sessions  │            │
│  │  • user_id      │  │  • game_id      │  │ • session_id     │            │
│  │  • username     │  │  • white/black  │  │ • user_id        │            │
│  │  • password     │  │  • result       │  │ • login_time     │            │
│  │  • wins/losses  │  │  • moves (JSON) │  │ • ip_address     │            │
│  │  • rating       │  │  • start/end    │  └──────────────────┘            │
│  └─────────────────┘  └─────────────────┘                                   │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## Authentication Flow

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                         USER AUTHENTICATION FLOW                             │
└──────────────────────────────────────────────────────────────────────────────┘

CLIENT                    WEBSOCKET              SESSION MGR        DATABASE
  │                          │                        │                │
  │                          │                        │                │
  ├─[1] Connect WebSocket──▶│                        │                │
  │                          │                        │                │
  │                          ├─[2] Perform           │                │
  │                          │     Handshake         │                │
  │                          │     (HTTP Upgrade)    │                │
  │◀─────Handshake OK────────┤                        │                │
  │                          │                        │                │
  │                          │                        │                │
  ├─[3] REGISTER MESSAGE────▶│                        │                │
  │    {                     │                        │                │
  │      "type": "REGISTER", │                        │                │
  │      "username": "alice",│                        │                │
  │      "password": "hash", │                        │                │
  │      "email": "a@x.com"  │                        │                │
  │    }                     │                        │                │
  │                          │                        │                │
  │                          ├─[4] Check username────────────────────▶│
  │                          │     UserRepository::                   │
  │                          │     username_exists()                  │
  │                          │                        │                │
  │                          │◀─────False─────────────────────────────┤
  │                          │     (username available)               │
  │                          │                        │                │
  │                          ├─[5] Create user────────────────────────▶│
  │                          │     UserRepository::                   │
  │                          │     create_user()                      │
  │                          │                        │                │
  │                          │     INSERT INTO users                  │
  │                          │     VALUES (...)                       │
  │                          │                        │                │
  │                          │◀─────user_id = 42──────────────────────┤
  │                          │                        │                │
  │◀─[6] REGISTER_RESPONSE───┤                        │                │
  │    {                     │                        │                │
  │      "type": "...",      │                        │                │
  │      "status": "success",│                        │                │
  │      "user_id": 42       │                        │                │
  │    }                     │                        │                │
  │                          │                        │                │
  │                          │                        │                │
  ├─[7] LOGIN MESSAGE────────▶│                        │                │
  │    {                     │                        │                │
  │      "type": "LOGIN",    │                        │                │
  │      "username": "alice",│                        │                │
  │      "password": "hash"  │                        │                │
  │    }                     │                        │                │
  │                          │                        │                │
  │                          ├─[8] Authenticate───────────────────────▶│
  │                          │     UserRepository::                   │
  │                          │     authenticate_user()                │
  │                          │                        │                │
  │                          │     SELECT user_id                     │
  │                          │     FROM users                         │
  │                          │     WHERE username='alice'             │
  │                          │     AND password_hash='hash'           │
  │                          │                        │                │
  │                          │◀─────user_id = 42──────────────────────┤
  │                          │                        │                │
  │                          ├─[9] Create Session────▶│                │
  │                          │     SessionManager::   │                │
  │                          │     create_session()   │                │
  │                          │                        │                │
  │                          │     sessions[socket] = {               │
  │                          │       user_id: 42,     │                │
  │                          │       username: "alice"│                │
  │                          │       socket_fd: 5     │                │
  │                          │     }                  │                │
  │                          │                        │                │
  │                          ├─[10] Get user info─────────────────────▶│
  │                          │      UserRepository::                  │
  │                          │      get_user_by_id(42)                │
  │                          │                        │                │
  │                          │◀────User data──────────────────────────┤
  │                          │     {wins: 10, rating: 1350, ...}      │
  │                          │                        │                │
  │◀─[11] LOGIN_RESPONSE─────┤                        │                │
  │    {                     │                        │                │
  │      "type": "LOGIN_...",│                        │                │
  │      "status": "success",│                        │                │
  │      "user_id": 42,      │                        │                │
  │      "username": "alice",│                        │                │
  │      "rating": 1350,     │                        │                │
  │      "wins": 10          │                        │                │
  │    }                     │                        │                │
  │                          │                        │                │
  │   [User is now logged in and can access game features]            │
  │                          │                        │                │
```

---

## Matchmaking Flow

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                         CHALLENGE & MATCHMAKING FLOW                         │
└──────────────────────────────────────────────────────────────────────────────┘

ALICE (Client A)      SERVER              BOB (Client B)        MATCH MGR
  │                      │                       │                   │
  │                      │                       │                   │
  ├─[1] GET_PLAYERS─────▶│                       │                   │
  │                      │                       │                   │
  │                      ├─[2] Query sessions───▶│                   │
  │                      │   Get all online      │                   │
  │                      │   users (not in game) │                   │
  │                      │                       │                   │
  │◀─[3] PLAYER_LIST─────┤                       │                   │
  │    [                 │                       │                   │
  │      {"id": 2,       │                       │                   │
  │       "name": "bob", │                       │                   │
  │       "rating": 1280,│                       │                   │
  │       "status": "online"},                   │                   │
  │      ...             │                       │                   │
  │    ]                 │                       │                   │
  │                      │                       │                   │
  ├─[4] CHALLENGE────────▶│                       │                   │
  │    {                 │                       │                   │
  │      "type": "CHALLENGE",                    │                   │
  │      "target_user_id": 2                     │                   │
  │    }                 │                       │                   │
  │                      │                       │                   │
  │                      ├─[5] Validate─────────▶│                   │
  │                      │   • Is Bob online?    │                   │
  │                      │   • Is Bob available? │                   │
  │                      │   (not in game)       │                   │
  │                      │                       │                   │
  │                      │◀──OK (available)──────┤                   │
  │                      │                       │                   │
  │                      ├─[6] Store challenge───────────────────────▶│
  │                      │     challenge_id = 123│                   │
  │                      │     challenger: Alice │                   │
  │                      │     challenged: Bob   │                   │
  │                      │                       │                   │
  │◀─[7] CHALLENGE_SENT──┤                       │                   │
  │    {                 │                       │                   │
  │      "status": "sent",                       │                   │
  │      "challenge_id": 123                     │                   │
  │    }                 │                       │                   │
  │                      │                       │                   │
  │                      ├─[8] Forward to Bob───▶│                   │
  │                      │    CHALLENGE_RECEIVED │                   │
  │                      │    {                  │                   │
  │                      │      "challenge_id": 123                  │
  │                      │      "from_user_id": 1│                   │
  │                      │      "from_username": "alice"             │
  │                      │      "rating": 1350   │                   │
  │                      │    }                  │                   │
  │                      │                       │                   │
  │                      │                       │                   │
  │                      │◀─[9] ACCEPT_CHALLENGE─┤                   │
  │                      │    {                  │                   │
  │                      │      "challenge_id": 123                  │
  │                      │    }                  │                   │
  │                      │                       │                   │
  │                      ├─[10] Create Game──────────────────────────▶│
  │                      │      MatchManager::                        │
  │                      │      create_game(alice_id, bob_id)        │
  │                      │                       │                   │
  │                      │                       │      ┌────────────┴──┐
  │                      │                       │      │ game_id = 789 │
  │                      │                       │      │ white: Alice  │
  │                      │                       │      │ black: Bob    │
  │                      │                       │      │ state: ACTIVE │
  │                      │                       │      └────────────┬──┘
  │                      │                       │                   │
  │                      │◀──────game_id = 789───────────────────────┤
  │                      │                       │                   │
  │                      ├─[11] Save to DB───────────────────────────▶│
  │                      │      GameRepository::                      │
  │                      │      create_game(1, 2) ──────▶ [Database]  │
  │                      │                       │                   │
  │◀─[12] MATCH_STARTED──┤                       │                   │
  │    {                 │                       │                   │
  │      "game_id": 789, │                       │                   │
  │      "your_color": "white",                  │                   │
  │      "opponent": "bob",                      │                   │
  │      "opponent_rating": 1280                 │                   │
  │    }                 │                       │                   │
  │                      │                       │                   │
  │                      ├─[13] MATCH_STARTED───▶│                   │
  │                      │    {                  │                   │
  │                      │      "game_id": 789,  │                   │
  │                      │      "your_color": "black",               │
  │                      │      "opponent": "alice",                 │
  │                      │      "opponent_rating": 1350              │
  │                      │    }                  │                   │
  │                      │                       │                   │
  │   [Both players now transition to game screen]                   │
  │                      │                       │                   │
```

---

## Gameplay Flow

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                           CHESS GAME PLAY FLOW                               │
└──────────────────────────────────────────────────────────────────────────────┘

ALICE (White)        SERVER/MATCH MGR         BOB (Black)         DATABASE
  │                         │                      │                  │
  │    [Game Started - Alice plays white, moves first]               │
  │                         │                      │                  │
  ├─[1] MOVE───────────────▶│                      │                  │
  │    {                    │                      │                  │
  │      "game_id": 789,    │                      │                  │
  │      "move": "e2e4"     │                      │                  │
  │    }                    │                      │                  │
  │                         │                      │                  │
  │                    ┌────▼────────┐             │                  │
  │                    │ Validate    │             │                  │
  │                    │ 1. Is game active?        │                  │
  │                    │ 2. Is it Alice's turn?    │                  │
  │                    │ 3. Is move legal?         │                  │
  │                    │    (ChessEngine)          │                  │
  │                    └────┬────────┘             │                  │
  │                         │                      │                  │
  │                         ├─[2] Apply move       │                  │
  │                         │     Update board      │                  │
  │                         │     Switch turn       │                  │
  │                         │     Check game state  │                  │
  │                         │                      │                  │
  │                         ├─[3] Save move────────────────────────────▶│
  │                         │     GameRepository::                    │
  │                         │     add_move_to_game(789, "e2e4")       │
  │                         │                      │                  │
  │◀─[4] MOVE_ACCEPTED──────┤                      │                  │
  │    {                    │                      │                  │
  │      "status": "ok"     │                      │                  │
  │    }                    │                      │                  │
  │                         │                      │                  │
  │                         ├─[5] Forward to Bob──▶│                  │
  │                         │    OPPONENT_MOVE     │                  │
  │                         │    {                 │                  │
  │                         │      "move": "e2e4", │                  │
  │                         │      "move_number": 1,                  │
  │                         │      "fen": "rnbq..."│                  │
  │                         │    }                 │                  │
  │                         │                      │                  │
  │                         │                      │                  │
  │                         │◀─[6] MOVE────────────┤                  │
  │                         │    {                 │                  │
  │                         │      "game_id": 789, │                  │
  │                         │      "move": "e7e5"  │                  │
  │                         │    }                 │                  │
  │                         │                      │                  │
  │                    ┌────▼────────┐             │                  │
  │                    │ Validate    │             │                  │
  │                    │ • Bob's turn?│             │                  │
  │                    │ • Legal move?│             │                  │
  │                    │ • Apply      │             │                  │
  │                    │ • Check for: │             │                  │
  │                    │   - Checkmate│             │                  │
  │                    │   - Stalemate│             │                  │
  │                    │   - Draw     │             │                  │
  │                    └────┬────────┘             │                  │
  │                         │                      │                  │
  │                         ├─[7] Save move────────────────────────────▶│
  │                         │                      │                  │
  │◀─[8] OPPONENT_MOVE──────┤                      │                  │
  │    {                    │                      │                  │
  │      "move": "e7e5",    │    MOVE_ACCEPTED────▶│                  │
  │      "move_number": 2   │                      │                  │
  │    }                    │                      │                  │
  │                         │                      │                  │
  │                         │                      │                  │
  │   [...game continues with moves back and forth...]                │
  │                         │                      │                  │
  │                         │                      │                  │
  ├─[9] MOVE (Checkmate)───▶│                      │                  │
  │    "Qh5xf7#"            │                      │                  │
  │                         │                      │                  │
  │                    ┌────▼────────┐             │                  │
  │                    │ Validate    │             │                  │
  │                    │ Apply move  │             │                  │
  │                    │             │             │                  │
  │                    │ ⚠️ CHECKMATE DETECTED!    │                  │
  │                    │ King in check│             │                  │
  │                    │ No legal moves             │                  │
  │                    │             │             │                  │
  │                    │ Game Over!  │             │                  │
  │                    │ Winner: White (Alice)     │                  │
  │                    └────┬────────┘             │                  │
  │                         │                      │                  │
  │                         ├─[10] End Game────────────────────────────▶│
  │                         │      GameRepository::                   │
  │                         │      end_game(789, "WHITE_WIN", moves)  │
  │                         │                      │                  │
  │                         │      UPDATE game_history               │
  │                         │      SET result = 'WHITE_WIN',         │
  │                         │          end_time = NOW()              │
  │                         │                      │                  │
  │                         ├─[11] Update Stats────────────────────────▶│
  │                         │      UserRepository::                   │
  │                         │      increment_wins(alice_id)           │
  │                         │      increment_losses(bob_id)           │
  │                         │                      │                  │
  │◀─[12] GAME_ENDED────────┤                      │                  │
  │    {                    │                      │                  │
  │      "result": "win",   │    GAME_ENDED───────▶│                  │
  │      "reason": "checkmate",  {                 │                  │
  │      "winner": "white", │      "result": "loss",                  │
  │      "moves": [...]     │      "reason": "checkmate",             │
  │    }                    │      "winner": "white"                  │
  │                         │    }                 │                  │
  │                         │                      │                  │
  │   [Game complete - players can view history, return to lobby]     │
  │                         │                      │                  │
```

---

## WebSocket Communication Flow

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                      WEBSOCKET FRAME COMMUNICATION                           │
└──────────────────────────────────────────────────────────────────────────────┘

CLIENT BROWSER                      SERVER (WebSocketHandler)
  │                                           │
  ├─[1] HTTP Upgrade Request─────────────────▶│
  │    GET /chat HTTP/1.1                     │
  │    Upgrade: websocket                     │
  │    Connection: Upgrade                    │
  │    Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==
  │                                           │
  │                                      ┌────▼──────┐
  │                                      │ Parse key │
  │                                      │ Concat GUID│
  │                                      │ SHA-1 hash│
  │                                      │ Base64    │
  │                                      └────┬──────┘
  │                                           │
  │◀─[2] HTTP 101 Switching Protocols─────────┤
  │    HTTP/1.1 101 Switching Protocols       │
  │    Upgrade: websocket                     │
  │    Connection: Upgrade                    │
  │    Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=
  │                                           │
  │    ✓ WebSocket connection established     │
  │                                           │
  ├─[3] TEXT FRAME (JSON message)────────────▶│
  │                                           │
  │    Frame Structure:                       │
  │    ┌───────────────────────┐              │
  │    │ FIN=1 OPCODE=0x1(TEXT)│              │
  │    │ MASK=1 LEN=45         │         ┌────▼──────┐
  │    │ MASKING_KEY=XXXX      │         │ Read frame│
  │    │ PAYLOAD (masked):     │         │ header    │
  │    │   {"type":"LOGIN"...} │         │ Read len  │
  │    └───────────────────────┘         │ Read mask │
  │                                      │ Read data │
  │                                      │ XOR unmask│
  │                                      └────┬──────┘
  │                                           │
  │                                      ┌────▼──────┐
  │                                      │ Parse JSON│
  │                                      │ Route msg │
  │                                      │ Process   │
  │                                      └────┬──────┘
  │                                           │
  │◀─[4] TEXT FRAME (Response)─────────────────┤
  │                                           │
  │    Frame Structure:                       │
  │    ┌───────────────────────┐              │
  │    │ FIN=1 OPCODE=0x1      │              │
  │    │ MASK=0 LEN=52         │              │
  │    │ PAYLOAD (unmasked):   │              │
  │    │   {"type":"LOGIN_..."}│              │
  │    └───────────────────────┘              │
  │                                           │
  ├─[5] PING (keepalive)─────────────────────▶│
  │    ┌───────────────────────┐              │
  │    │ FIN=1 OPCODE=0x9(PING)│         ┌────▼──────┐
  │    │ MASK=1 LEN=0          │         │ Receive   │
  │    │ PAYLOAD=""            │         │ PING      │
  │    └───────────────────────┘         └────┬──────┘
  │                                           │
  │◀─[6] PONG (auto-response)──────────────────┤
  │    ┌───────────────────────┐              │
  │    │ FIN=1 OPCODE=0xA(PONG)│              │
  │    │ MASK=0 LEN=0          │              │
  │    │ PAYLOAD=""            │              │
  │    └───────────────────────┘              │
  │                                           │
  ├─[7] CLOSE (disconnect)───────────────────▶│
  │    ┌───────────────────────┐              │
  │    │ FIN=1 OPCODE=0x8(CLOSE)          ┌───▼──────┐
  │    │ MASK=1 LEN=2          │          │ Receive  │
  │    │ PAYLOAD=0x03E8(1000)  │          │ CLOSE    │
  │    │ "Normal closure"      │          │ frame    │
  │    └───────────────────────┘          └───┬──────┘
  │                                           │
  │◀─[8] CLOSE (echo)──────────────────────────┤
  │    ┌───────────────────────┐              │
  │    │ FIN=1 OPCODE=0x8      │              │
  │    │ MASK=0 LEN=2          │              │
  │    │ PAYLOAD=0x03E8        │              │
  │    └───────────────────────┘              │
  │                                           │
  │    ✓ Connection closed gracefully         │
  │                                           │

OPCODE REFERENCE:
  0x0 = CONTINUATION  0x1 = TEXT      0x2 = BINARY
  0x8 = CLOSE         0x9 = PING      0xA = PONG
```

---

## Database Operations Flow

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                         DATABASE OPERATIONS FLOW                             │
└──────────────────────────────────────────────────────────────────────────────┘

APPLICATION LAYER          REPOSITORY LAYER              DATABASE
  │                              │                           │
  │                              │                           │
  ├─[1] Create User─────────────▶│                           │
  │    username="alice"          │                           │
  │    password="hashed"         │                           │
  │                              │                           │
  │                         ┌────▼────────┐                  │
  │                         │ UserRepository::               │
  │                         │ create_user()                  │
  │                         └────┬────────┘                  │
  │                              │                           │
  │                              ├─SQL Query─────────────────▶│
  │                              │  INSERT INTO users        │
  │                              │  (username, password_hash,│
  │                              │   email, rating)          │
  │                              │  VALUES                   │
  │                              │  ('alice', 'hash',        │
  │                              │   'a@x.com', 1200)        │
  │                              │  RETURNING user_id;       │
  │                              │                           │
  │                              │◀────user_id = 42──────────┤
  │◀─────user_id = 42────────────┤                           │
  │                              │                           │
  │                              │                           │
  ├─[2] Start Game──────────────▶│                           │
  │    white_id=42, black_id=43  │                           │
  │                              │                           │
  │                         ┌────▼────────┐                  │
  │                         │ GameRepository::               │
  │                         │ create_game()                  │
  │                         └────┬────────┘                  │
  │                              │                           │
  │                              ├─SQL Query─────────────────▶│
  │                              │  INSERT INTO game_history │
  │                              │  (white_player_id,        │
  │                              │   black_player_id,        │
  │                              │   start_time, moves)      │
  │                              │  VALUES (42, 43,          │
  │                              │   NOW(), '[]')            │
  │                              │  RETURNING game_id;       │
  │                              │                           │
  │                              │◀────game_id = 789─────────┤
  │◀─────game_id = 789───────────┤                           │
  │                              │                           │
  │                              │                           │
  ├─[3] Add Move────────────────▶│                           │
  │    game_id=789, move="e2e4"  │                           │
  │                              │                           │
  │                         ┌────▼────────┐                  │
  │                         │ GameRepository::               │
  │                         │ add_move_to_game()             │
  │                         │ 1. Get current moves           │
  │                         │ 2. Append new move             │
  │                         │ 3. Update database             │
  │                         └────┬────────┘                  │
  │                              │                           │
  │                              ├─SQL Query (GET)───────────▶│
  │                              │  SELECT moves             │
  │                              │  FROM game_history        │
  │                              │  WHERE game_id = 789;     │
  │                              │                           │
  │                              │◀────moves = "[]"──────────┤
  │                              │                           │
  │                              ├─SQL Query (UPDATE)────────▶│
  │                              │  UPDATE game_history      │
  │                              │  SET moves = '["e2e4"]'   │
  │                              │  WHERE game_id = 789;     │
  │                              │                           │
  │                              │◀────OK────────────────────┤
  │◀─────success─────────────────┤                           │
  │                              │                           │
  │                              │                           │
  ├─[4] End Game────────────────▶│                           │
  │    game_id=789               │                           │
  │    result="WHITE_WIN"        │                           │
  │    moves='["e2e4","e7e5"...] │                           │
  │                              │                           │
  │                         ┌────▼────────┐                  │
  │                         │ GameRepository::               │
  │                         │ end_game()                     │
  │                         └────┬────────┘                  │
  │                              │                           │
  │                              ├─SQL Transaction───────────▶│
  │                              │  BEGIN;                   │
  │                              │                           │
  │                              │  UPDATE game_history      │
  │                              │  SET result = 'WHITE_WIN',│
  │                              │      moves = '[...]',     │
  │                              │      end_time = NOW(),    │
  │                              │      duration =           │
  │                              │        EXTRACT(EPOCH FROM │
  │                              │         NOW()-start_time) │
  │                              │  WHERE game_id = 789;     │
  │                              │                           │
  │                              │  UPDATE users             │
  │                              │  SET wins = wins + 1      │
  │                              │  WHERE user_id = 42;      │
  │                              │                           │
  │                              │  UPDATE users             │
  │                              │  SET losses = losses + 1  │
  │                              │  WHERE user_id = 43;      │
  │                              │                           │
  │                              │  COMMIT;                  │
  │                              │                           │
  │                              │◀────OK────────────────────┤
  │◀─────success─────────────────┤                           │
  │                              │                           │
```

---

## Thread Architecture

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                         SERVER THREAD ARCHITECTURE                           │
└──────────────────────────────────────────────────────────────────────────────┘

                           MAIN THREAD
                                │
                    ┌───────────┴───────────┐
                    │  main()               │
                    │  • Initialize server  │
                    │  • Create socket      │
                    │  • Bind & listen      │
                    └───────────┬───────────┘
                                │
                                │ spawn
                                ▼
                    ┌───────────────────────┐
                    │  ACCEPT THREAD        │◀──┐
                    │  • accept() - blocking│   │
                    │  • Get client socket  │   │
                    └───────────┬───────────┘   │
                                │               │
                 ┌──────────────┼──────────┐    │
                 │              │          │    │
      spawn      ▼              ▼          ▼    │ loop
    ┌─────────────────┐  ┌──────────────────┐  │
    │ CLIENT THREAD 1 │  │ CLIENT THREAD 2  │  │
    │ (pthread)       │  │ (pthread)        │  │
    │                 │  │                  │  │
    │ ┌─────────────┐ │  │ ┌──────────────┐ │  │
    │ │ WebSocket   │ │  │ │ WebSocket    │ │  │
    │ │ Handshake   │ │  │ │ Handshake    │ │  │
    │ └──────┬──────┘ │  │ └──────┬───────┘ │  │
    │        │        │  │        │         │  │
    │ ┌──────▼──────┐ │  │ ┌──────▼───────┐ │  │
    │ │ Message     │ │  │ │ Message      │ │  │
    │ │ Loop        │ │  │ │ Loop         │ │  │
    │ │ • Receive   │ │  │ │ • Receive    │ │  │
    │ │ • Parse     │ │  │ │ • Parse      │ │  │
    │ │ • Process   │ │  │ │ • Process    │ │  │
    │ │ • Send      │ │  │ │ • Send       │ │  │
    │ └──────┬──────┘ │  │ └──────┬───────┘ │  │
    └────────┼────────┘  └────────┼─────────┘  │
             │                    │             │
             │                    │             │
             └────────┬───────────┘             │
                      │                         │
         ┌────────────▼──────────────┐          │
         │   SHARED RESOURCES        │          │
         │   (Mutex Protected)       │          │
         ├───────────────────────────┤          │
         │ • sessions_by_socket      │──────────┘
         │ • sessions_by_user_id     │   Continue
         │ • active_games            │   accepting
         │ • pending_challenges      │
         │ • database connection pool│
         │ • log file                │
         └───────────────────────────┘

SYNCHRONIZATION PRIMITIVES:

┌────────────────────────────────────────┐
│ pthread_mutex_t session_mutex          │  Lock order:
│   • Protects session maps              │  1. session_mutex
│   • Used during login/logout           │  2. game_mutex
│                                        │  3. log_mutex
│ pthread_mutex_t game_mutex             │  (prevents deadlock)
│   • Protects active_games map          │
│   • Used during game create/update     │
│                                        │
│ pthread_mutex_t log_mutex              │
│   • Protects log file writes           │
│   • Used by all threads                │
└────────────────────────────────────────┘

THREAD LIFECYCLE:

Client Thread 1 (example):
  │
  ├─ [Created] pthread_create()
  │
  ├─ [Running] handle_websocket_client()
  │   │
  │   ├─ Perform handshake
  │   │
  │   ├─ [Loop] while(connected)
  │   │   │
  │   │   ├─ receive_message()
  │   │   ├─ process_message()
  │   │   │   └─ acquire mutex → access shared data → release mutex
  │   │   └─ send_response()
  │   │
  │   └─ [Exit Loop] client disconnect / error
  │
  ├─ [Cleanup]
  │   ├─ acquire session_mutex
  │   ├─ remove session
  │   ├─ release session_mutex
  │   └─ close(socket)
  │
  └─ [Terminated] return nullptr
```

---

## Summary

These diagrams cover:

1. **Complete System Flow** - Overall architecture
2. **Authentication Flow** - User registration and login
3. **Matchmaking Flow** - Challenge and game creation
4. **Gameplay Flow** - Move validation and game completion
5. **WebSocket Communication** - Frame-level protocol
6. **Database Operations** - Repository pattern and SQL
7. **Thread Architecture** - Multi-threading and synchronization

Each diagram shows the step-by-step interaction between components with message formats, validation steps, and data flow.

---

**Document Version:** 1.0  
**Last Updated:** November 24, 2025  
**Status:** Complete
