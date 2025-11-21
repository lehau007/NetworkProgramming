# Development Workflow Guide

## Overview
This guide provides a week-by-week development plan for the Chess Network Programming project, focusing on transport layer implementation and meeting all 19-point requirements.

---

## Project Timeline (6 Weeks)

```
Week 1: Transport Layer Foundation
Week 2: Authentication & Session Management
Week 3: Lobby & Matchmaking System
Week 4: Chess Engine Integration & Gameplay
Week 5: Advanced Features & Game Management
Week 6: Testing, Polish & Documentation
```

---

## Week 1: Transport Layer Foundation (1 điểm + 2 điểm)

### Goals
- ✅ Stream Processing (1 point)
- ✅ Socket I/O Mechanism (2 points)

### Server Tasks

**Day 1-2: Project Setup**
```bash
# Create project structure
mkdir -p server/{network,session,game,database,utils}
mkdir -p client/{network,ui,game,utils}

# Initialize git
git init
git add .gitignore
git commit -m "Initial project structure"
```

**Day 3-4: Socket Implementation**
- [ ] Create `SocketHandler` class
- [ ] Implement TCP server socket
- [ ] Add `bind()`, `listen()`, `accept()`
- [ ] Test basic connection acceptance

```cpp
// Test: Can server accept connections?
// Terminal 1: ./server
// Terminal 2: nc localhost 8080
```

**Day 5-6: Message Protocol**
- [ ] Implement length-prefixed message framing
- [ ] Create `MessageHandler` for JSON serialization
- [ ] Implement send/receive with buffering
- [ ] Handle partial messages correctly

**Day 7: Threading**
- [ ] Implement thread-per-client model
- [ ] Create `ClientHandler` with pthread
- [ ] Test multiple simultaneous connections

### Client Tasks

**Day 1-3: Connection Manager**
- [ ] Implement `ConnectionManager` class
- [ ] TCP socket connection
- [ ] Send/receive with message framing

**Day 4-5: Protocol Handler**
- [ ] Create message creation helpers
- [ ] JSON serialization/parsing
- [ ] Message queue for async reception

**Day 6-7: Basic UI**
- [ ] Simple connection test UI
- [ ] Display connection status
- [ ] Send test messages

### Deliverables
- ✅ Server accepts multiple clients
- ✅ Clients can connect and exchange messages
- ✅ Messages are properly framed and parsed
- ✅ Multi-threading works correctly

### Testing Checklist
```
□ Server starts and listens on port 8080
□ Client connects successfully
□ Send message from client → received by server
□ Send message from server → received by client
□ 5 clients connect simultaneously
□ Messages don't get mixed up between clients
□ Connection handles gracefully when client disconnects
□ Large messages (>1KB) transmitted correctly
```

---

## Week 2: Authentication & Session (2 điểm + 2 điểm)

### Goals
- ✅ Account Registration & Management (2 points)
- ✅ Login & Session Management (2 points)

### Server Tasks

**Day 1-2: Database Setup**
- [ ] Install PostgreSQL
- [ ] Create database schema
- [ ] Implement `DatabaseConnection` class
- [ ] Test basic queries

```sql
-- Test queries
INSERT INTO users (username, password_hash) VALUES ('test', 'hash');
SELECT * FROM users WHERE username='test';
```

**Day 3-4: Registration**
- [ ] Create `handle_register()` function
- [ ] Validate username uniqueness
- [ ] Hash passwords (use bcrypt or similar)
- [ ] Store user in database
- [ ] Send registration response

**Day 5-6: Login & Sessions**
- [ ] Create `SessionManager` class
- [ ] Implement `handle_login()`
- [ ] Verify credentials against database
- [ ] Create session on successful login
- [ ] Store socket → user mapping

**Day 7: Session Management**
- [ ] Implement session validation
- [ ] Handle logout
- [ ] Auto-cleanup inactive sessions
- [ ] Test session persistence

### Client Tasks

**Day 1-3: Login UI**
- [ ] Create login window
- [ ] Username/password inputs
- [ ] Handle login response
- [ ] Store session data

**Day 4-5: Registration UI**
- [ ] Create registration window
- [ ] Form validation
- [ ] Handle registration response
- [ ] Auto-login after registration

**Day 6-7: Session Handling**
- [ ] Store login credentials (optionally)
- [ ] Auto-reconnect with session
- [ ] Handle session expiry

### Deliverables
- ✅ Users can register accounts
- ✅ Users can login with credentials
- ✅ Sessions are maintained across messages
- ✅ Database stores user data persistently

### Testing Checklist
```
□ Register new user successfully
□ Duplicate username rejected
□ Invalid password format rejected
□ Login with correct credentials succeeds
□ Login with wrong password fails
□ Session persists across multiple messages
□ Logout removes session
□ Inactive session auto-expires (5 min test)
□ Database contains correct user data
□ Password is properly hashed in DB
```

---

## Week 3: Lobby & Matchmaking (2 điểm + 2 điểm + 1 điểm)

### Goals
- ✅ Available Players List (2 points)
- ✅ Challenge Forwarding (2 points)
- ✅ Accept/Decline Challenge (1 point)

### Server Tasks

**Day 1-2: Player List**
- [ ] Implement player state tracking
- [ ] Create `handle_get_players()`
- [ ] Filter online players
- [ ] Exclude players in-game
- [ ] Broadcast player list updates

```cpp
struct PlayerInfo {
    int user_id;
    string username;
    PlayerState state;  // ONLINE, IN_GAME, CHALLENGING
};
```

**Day 3-4: Challenge System**
- [ ] Create `Challenge` data structure
- [ ] Implement `handle_challenge()`
- [ ] Validate target player availability
- [ ] Forward challenge to target
- [ ] Store pending challenges

**Day 5-6: Challenge Response**
- [ ] Implement `handle_accept_challenge()`
- [ ] Implement `handle_decline_challenge()`
- [ ] Create game instance on accept
- [ ] Notify both players of result
- [ ] Update player states

**Day 7: Testing & Refinement**
- [ ] Handle edge cases (offline players, etc.)
- [ ] Test concurrent challenges
- [ ] Implement challenge timeout

### Client Tasks

**Day 1-3: Lobby UI**
- [ ] Display available players list
- [ ] Refresh button/auto-refresh
- [ ] Show player stats (wins/losses)
- [ ] Visual state indicators

**Day 4-5: Challenge UI**
- [ ] "Challenge" button per player
- [ ] Challenge sent notification
- [ ] Incoming challenge dialog
- [ ] Accept/Decline buttons

**Day 6-7: Match Notification**
- [ ] Handle MATCH_STARTED message
- [ ] Transition to game screen
- [ ] Display opponent info

### Deliverables
- ✅ Players see list of online users
- ✅ Players can send challenges
- ✅ Challenges are forwarded correctly
- ✅ Accept/decline works properly
- ✅ Game instance created on acceptance

### Testing Checklist
```
□ Login with 3 different clients
□ Each client sees other 2 in player list
□ Player list updates when user logs in/out
□ Client A challenges Client B
□ Client B receives challenge notification
□ Client B accepts → both get MATCH_STARTED
□ Client B declines → both get notification
□ Can't challenge player already in game
□ Can't challenge yourself
□ Challenge expires after 60 seconds
```

---

## Week 4: Chess Engine & Gameplay (2 điểm + 1 điểm + 1 điểm)

### Goals
- ✅ Move Forwarding (2 points)
- ✅ Move Validation (1 point)
- ✅ Game Result Detection (1 point)

### Server Tasks

**Day 1-2: Game Instance Manager**
- [ ] Create `GameInstance` structure
- [ ] Implement `MatchManager` class
- [ ] Store active games map
- [ ] Track game state per instance

**Day 3-4: Move Handling**
- [ ] Implement `handle_move()`
- [ ] Validate it's player's turn
- [ ] Validate with ChessGame engine
- [ ] Update board state
- [ ] Forward move to opponent

```cpp
bool validate_move(int game_id, int player_id, string move) {
    // 1. Check turn
    // 2. Validate with chess engine
    // 3. Execute if valid
}
```

**Day 5-6: Game Result**
- [ ] Detect checkmate
- [ ] Detect stalemate
- [ ] Handle resignation
- [ ] Send game end notifications
- [ ] Update player statistics

**Day 7: Game History**
- [ ] Log all moves
- [ ] Store game in database
- [ ] Generate game log message

### Client Tasks

**Day 1-3: Chess Board UI**
- [ ] Create chess board widget
- [ ] Render pieces
- [ ] Handle click input
- [ ] Highlight selected piece
- [ ] Show legal moves (optional)

**Day 4-5: Move Input**
- [ ] Capture from/to squares
- [ ] Convert to notation (e.g., "e2e4")
- [ ] Send move to server
- [ ] Handle move acceptance/rejection

**Day 6-7: Board Updates**
- [ ] Handle OPPONENT_MOVE
- [ ] Update board display
- [ ] Show move history
- [ ] Display game status

### Deliverables
- ✅ Complete chess game playable
- ✅ Moves validated server-side
- ✅ Invalid moves rejected
- ✅ Game ends detected correctly
- ✅ Board synchronized between players

### Testing Checklist
```
□ Start match between 2 clients
□ Make legal move → accepted and forwarded
□ Make illegal move → rejected
□ Moves alternate correctly
□ Can't move opponent's pieces
□ Can't move when not your turn
□ Checkmate detected and game ends
□ Stalemate detected
□ Move history recorded
□ Game stored in database after end
```

---

## Week 5: Advanced Features (1 điểm + 2 điểm + 1 điểm)

### Goals
- ✅ Logging (1 point)
- ✅ Game Result & Log Transmission (2 points)
- ✅ Game Abort/Draw (1 point)

### Server Tasks

**Day 1-2: Logging System**
- [ ] Create `Logger` class
- [ ] Log all connections
- [ ] Log authentication events
- [ ] Log game events
- [ ] Log errors
- [ ] Thread-safe file writing

```
[2025-11-21 10:30:45] INFO: User 'player1' logged in
[2025-11-21 10:31:12] INFO: Challenge: player1 → player2
[2025-11-21 10:31:15] INFO: Match 123 started
[2025-11-21 10:35:42] INFO: Match 123 ended: WHITE_WIN
```

**Day 3-4: Game Log Transmission**
- [ ] Generate complete game log
- [ ] Include all moves with timestamps
- [ ] Add descriptive notation
- [ ] Send to both players
- [ ] Store in database

**Day 5-6: Draw & Resignation**
- [ ] Implement `handle_resign()`
- [ ] Implement `handle_draw_offer()`
- [ ] Forward draw offer to opponent
- [ ] Handle draw acceptance/decline
- [ ] Update game result appropriately

**Day 7: Polish**
- [ ] Error handling improvements
- [ ] Edge case handling
- [ ] Code cleanup

### Client Tasks

**Day 1-3: Game Controls**
- [ ] Resign button
- [ ] Draw offer button
- [ ] Handle draw offer dialog
- [ ] Accept/decline draw

**Day 4-5: Game Log Viewer**
- [ ] Display complete game log
- [ ] Show move history
- [ ] Display game result
- [ ] Export game log (optional)

**Day 6-7: Error Handling**
- [ ] Display error messages
- [ ] Handle disconnections
- [ ] Reconnection logic

### Deliverables
- ✅ Comprehensive logging
- ✅ Game logs sent to players
- ✅ Draw offers work
- ✅ Resignation works
- ✅ All features integrated

### Testing Checklist
```
□ Server log file created
□ All events logged with timestamps
□ Game log shows all moves
□ Game log includes result
□ Player can resign mid-game
□ Opponent notified of resignation
□ Draw offer sent to opponent
□ Draw accepted → game ends in draw
□ Draw declined → game continues
□ Log file doesn't corrupt with concurrent writes
```

---

## Week 6: Testing, Polish & Documentation

### Goals
- ✅ Comprehensive testing
- ✅ Bug fixes
- ✅ Documentation
- ✅ Presentation preparation

### Tasks

**Day 1-2: Integration Testing**
- [ ] Test complete game flow
- [ ] Test with multiple concurrent games
- [ ] Load testing (10+ clients)
- [ ] Edge case testing

**Day 3-4: Bug Fixes**
- [ ] Fix all known bugs
- [ ] Handle race conditions
- [ ] Memory leak detection
- [ ] Thread safety verification

**Day 5-6: Documentation**
- [ ] Update README
- [ ] API documentation
- [ ] User manual
- [ ] Architecture diagrams

**Day 7: Presentation Prep**
- [ ] Demo preparation
- [ ] Slides (if needed)
- [ ] Code walkthrough practice

### Final Testing Checklist
```
□ All 19 requirements implemented
□ 10 simultaneous users can play
□ No crashes during normal operation
□ No memory leaks (valgrind clean)
□ Database remains consistent
□ Logs are accurate and complete
□ Code is well-commented
□ README has build instructions
□ All tests pass
□ Demo works reliably
```

---

## Daily Development Routine

### Morning (2-3 hours)
1. Review yesterday's progress
2. Update task list
3. Focus on main feature implementation
4. Commit working code

### Afternoon/Evening (2-3 hours)
1. Testing current features
2. Bug fixes
3. Documentation updates
4. Code review/cleanup
5. Commit day's work

### Git Workflow
```bash
# Daily routine
git checkout -b feature/description
# ... work ...
git add .
git commit -m "Implement feature X"
git push origin feature/description

# After testing
git checkout main
git merge feature/description
git push origin main
```

---

## Code Review Checklist

Before committing major features:

### Functionality
- [ ] Feature works as expected
- [ ] All edge cases handled
- [ ] Error messages are clear

### Code Quality
- [ ] Functions are single-purpose
- [ ] Variable names are descriptive
- [ ] Code is commented
- [ ] No magic numbers

### Thread Safety
- [ ] Shared data protected by mutexes
- [ ] No race conditions
- [ ] Locks released properly
- [ ] No deadlocks possible

### Memory Management
- [ ] All allocations freed
- [ ] No memory leaks
- [ ] Pointers validated

### Testing
- [ ] Unit tests written
- [ ] Integration tests pass
- [ ] Manual testing done

---

## Troubleshooting Guide

### Common Issues

**1. Connection Refused**
```bash
# Check if server is running
ps aux | grep chess_server

# Check if port is in use
netstat -an | grep 8080

# Solution: Start server or change port
```

**2. Message Not Received**
```cpp
// Debug: Print message before sending
std::cout << "Sending: " << json_str << std::endl;

// Debug: Print received bytes
std::cout << "Received " << bytes << " bytes" << std::endl;
```

**3. Thread Deadlock**
```cpp
// Always lock in same order
// BAD:
Thread A: lock(mutex1); lock(mutex2);
Thread B: lock(mutex2); lock(mutex1);  // Deadlock!

// GOOD:
Both: lock(mutex1); lock(mutex2);  // Same order
```

**4. Database Connection Fails**
```bash
# Test PostgreSQL connection
psql -h 172.21.192.1 -U postgres -d chess-app

# Check pg_hba.conf for WSL access
# Check firewall allows port 5432
```

---

## Progress Tracking

### Weekly Review Questions
1. What features were completed?
2. What blockers were encountered?
3. What is at risk for next week?
4. What help is needed?

### Milestone Checklist
- [ ] Week 1: Basic communication working
- [ ] Week 2: Users can login
- [ ] Week 3: Can start a match
- [ ] Week 4: Can play complete game
- [ ] Week 5: All features implemented
- [ ] Week 6: Project complete and tested

---

## Final Deliverables

### Code
- [ ] Server source code
- [ ] Client source code
- [ ] Database schema
- [ ] Build scripts/Makefile
- [ ] README with instructions

### Documentation
- [ ] Project architecture document
- [ ] API documentation
- [ ] User manual
- [ ] Test report

### Demo
- [ ] Working demonstration
- [ ] Multiple clients playing
- [ ] All features shown
- [ ] Q&A preparation

---

**Document Version**: 1.0  
**Last Updated**: 2025-11-21  
**Status**: Development Guide Complete
