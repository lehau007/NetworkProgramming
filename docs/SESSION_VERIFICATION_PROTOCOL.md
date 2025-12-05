# Session Verification Protocol

## Overview

The first action after establishing a connection is **session verification**. This determines whether the client has an existing valid session or needs to authenticate.

---

## Connection Flow

```
┌──────────┐                                    ┌──────────┐
│  Client  │                                    │  Server  │
└────┬─────┘                                    └────┬─────┘
     │                                               │
     ├─[1] TCP/WebSocket Connection ───────────────►│
     │                                               │
     │◄─[2] Connection Established ─────────────────┤
     │                                               │
     │                                               │
     │   ┌─────────────────────────────────────┐    │
     │   │ Client MUST choose ONE of:          │    │
     │   │  A) VERIFY_SESSION (has session_id) │    │
     │   │  B) LOGIN (no session)              │    │
     │   │  C) REGISTER (new account)          │    │
     │   └─────────────────────────────────────┘    │
     │                                               │
```

---

## Scenario A: Session Verification (Reconnection)

**Use Case**: Client has stored session_id from previous connection

```
┌──────────┐                                    ┌──────────┐
│  Client  │                                    │  Server  │
└────┬─────┘                                    └────┬─────┘
     │                                               │
     ├─[1] VERIFY_SESSION ──────────────────────────►│
     │    {                                          │
     │      "type": "VERIFY_SESSION",                │ [Check session_id]
     │      "session_id": "abc123..."                │ [Verify expiry]
     │    }                                          │ [Check activity]
     │                                               │
     │◄─[2a] SESSION_VALID ──────────────────────────┤ (IF VALID)
     │    {                                          │
     │      "type": "SESSION_VALID",                 │
     │      "session_id": "abc123...",               │
     │      "user_data": {...},                      │
     │      "active_game_id": 456                    │
     │    }                                          │
     │                                               │
     │   [Client is now authenticated]              │
     │   [Can send any authenticated request]       │
     │                                               │
```

**Alternative: Session Invalid**

```
     │◄─[2b] SESSION_INVALID ────────────────────────┤ (IF INVALID)
     │    {                                          │
     │      "type": "SESSION_INVALID",               │
     │      "reason": "expired"                      │
     │    }                                          │
     │                                               │
     ├─[3] LOGIN ────────────────────────────────────►│
     │    {                                          │
     │      "type": "LOGIN",                         │
     │      "username": "player1",                   │
     │      "password": "hashed_pass"                │
     │    }                                          │
     │                                               │
     │◄─[4] LOGIN_RESPONSE ───────────────────────────┤
     │    {                                          │
     │      "type": "LOGIN_RESPONSE",                │
     │      "status": "success",                     │
     │      "session_id": "new123...",               │
     │      "user_data": {...}                       │
     │    }                                          │
```

**Alternative: Duplicate Session (Another Connection Active)**

```
     │◄─[2c] DUPLICATE_SESSION ──────────────────────┤ (IF ALREADY CONNECTED)
     │    {                                          │
     │      "type": "DUPLICATE_SESSION",             │
     │      "session_id": "abc123...",               │
     │      "reason": "already_connected",           │
     │      "message": "Multiple connections with    │
     │                  the same session are not     │
     │                  allowed."                    │
     │    }                                          │
     │                                               │
     │   [Connection should be closed]              │
     │   [User must close other connection first]   │
     │                                               │
```

**Note**: This occurs when the session is valid in the database, but another WebSocket/TCP connection is already using it. The server enforces single connection per session policy.

---

## Scenario B: Fresh Login (No Session)

**Use Case**: Client doesn't have session_id (first time or expired)

```
┌──────────┐                                    ┌──────────┐
│  Client  │                                    │  Server  │
└────┬─────┘                                    └────┬─────┘
     │                                               │
     ├─[1] LOGIN ────────────────────────────────────►│
     │    {                                          │
     │      "type": "LOGIN",                         │ [Verify credentials]
     │      "username": "player1",                   │ [Query database]
     │      "password": "hashed_pass"                │ [Create session]
     │    }                                          │
     │                                               │
     │◄─[2] LOGIN_RESPONSE ───────────────────────────┤
     │    {                                          │
     │      "type": "LOGIN_RESPONSE",                │
     │      "status": "success",                     │
     │      "session_id": "xyz789...",               │
     │      "user_data": {                           │
     │        "user_id": 123,                        │
     │        "username": "player1",                 │
     │        "rating": 1450,                        │
     │        "wins": 10,                            │
     │        "losses": 5,                           │
     │        "draws": 2                             │
     │      }                                        │
     │    }                                          │
     │                                               │
     │   [Client MUST store session_id]             │
     │   [Use session_id in all future requests]    │
     │                                               │
```

---

## Scenario C: Registration (New Account)

**Use Case**: New user creating account

```
┌──────────┐                                    ┌──────────┐
│  Client  │                                    │  Server  │
└────┬─────┘                                    └────┬─────┘
     │                                               │
     ├─[1] REGISTER ─────────────────────────────────►│
     │    {                                          │
     │      "type": "REGISTER",                      │ [Check username]
     │      "username": "newplayer",                 │ [Hash password]
     │      "password": "hashed_pass",               │ [Insert to DB]
     │      "email": "new@example.com"               │ [Create session]
     │    }                                          │
     │                                               │
     │◄─[2] REGISTER_RESPONSE ────────────────────────┤
     │    {                                          │
     │      "type": "REGISTER_RESPONSE",             │
     │      "status": "success",                     │
     │      "user_id": 124,                          │
     │      "message": "Account created"             │
     │    }                                          │
     │                                               │
     ├─[3] LOGIN ────────────────────────────────────►│
     │    {                                          │
     │      "type": "LOGIN",                         │
     │      "username": "newplayer",                 │
     │      "password": "hashed_pass"                │
     │    }                                          │
     │                                               │
     │◄─[4] LOGIN_RESPONSE ───────────────────────────┤
     │    {                                          │
     │      "type": "LOGIN_RESPONSE",                │
     │      "status": "success",                     │
     │      "session_id": "new456...",               │
     │      "user_data": {...}                       │
     │    }                                          │
```

---

## Server-Side Session Verification Logic

### On Connection Establishment

```cpp
void handle_client_connection(int client_socket) {
    SessionManager* session_mgr = SessionManager::get_instance();
    
    // Wait for first message
    std::string first_message = receive_json_message(client_socket);
    Json::Value msg = parse_json(first_message);
    
    std::string msg_type = msg["type"].asString();
    
    if (msg_type == "VERIFY_SESSION") {
        std::string session_id = msg["session_id"].asString();
        
        if (session_mgr->verify_session(session_id)) {
            // Session is valid
            Session* session = session_mgr->get_session(session_id);
            
            // Update socket mapping
            session->client_socket = client_socket;
            session_mgr->update_activity(session_id);
            
            // Send SESSION_VALID response
            Json::Value response;
            response["type"] = "SESSION_VALID";
            response["session_id"] = session_id;
            response["user_data"]["user_id"] = session->user_id;
            response["user_data"]["username"] = session->username;
            // ... add more user data from database
            
            send_json_message(client_socket, response);
            
            // Client is authenticated, proceed to message loop
            handle_authenticated_client(client_socket, session);
            
        } else {
            // Session is invalid
            Json::Value response;
            response["type"] = "SESSION_INVALID";
            response["reason"] = "expired";
            response["message"] = "Session expired. Please log in.";
            
            send_json_message(client_socket, response);
            
            // Wait for LOGIN or REGISTER
            handle_unauthenticated_client(client_socket);
        }
        
    } else if (msg_type == "LOGIN") {
        handle_login(client_socket, msg);
        
    } else if (msg_type == "REGISTER") {
        handle_register(client_socket, msg);
        
    } else {
        // Invalid first message
        Json::Value error;
        error["type"] = "ERROR";
        error["message"] = "First message must be VERIFY_SESSION, LOGIN, or REGISTER";
        send_json_message(client_socket, error);
        close(client_socket);
    }
}
```

### Session Timeout Handling

```cpp
// Background thread to cleanup expired sessions
void* session_cleanup_thread(void* arg) {
    SessionManager* session_mgr = SessionManager::get_instance();
    
    while (server_running) {
        sleep(60);  // Check every minute
        
        session_mgr->cleanup_expired_sessions();
        
        std::cout << "[Session Cleanup] Active sessions: " 
                  << session_mgr->get_active_session_count() << std::endl;
    }
    
    return nullptr;
}
```

---

## Client-Side Implementation

### JavaScript/WebSocket Example

```javascript
class ChessClient {
    constructor() {
        this.ws = null;
        this.sessionId = localStorage.getItem('session_id');
    }
    
    connect() {
        this.ws = new WebSocket('ws://localhost:8080');
        
        this.ws.onopen = () => {
            console.log('Connected to server');
            
            // FIRST ACTION: Verify session or login
            if (this.sessionId) {
                this.verifySession();
            } else {
                this.showLoginScreen();
            }
        };
        
        this.ws.onmessage = (event) => {
            const msg = JSON.parse(event.data);
            this.handleMessage(msg);
        };
    }
    
    verifySession() {
        const msg = {
            type: 'VERIFY_SESSION',
            session_id: this.sessionId,
            timestamp: Date.now()
        };
        this.ws.send(JSON.stringify(msg));
    }
    
    handleMessage(msg) {
        switch(msg.type) {
            case 'SESSION_VALID':
                console.log('Session restored:', msg.user_data);
                this.sessionId = msg.session_id;
                localStorage.setItem('session_id', this.sessionId);
                this.showMainScreen(msg.user_data);
                break;
                
            case 'SESSION_INVALID':
                console.log('Session invalid:', msg.reason);
                localStorage.removeItem('session_id');
                this.sessionId = null;
                this.showLoginScreen();
                break;
                
            case 'DUPLICATE_SESSION':
                console.log('Duplicate session:', msg.message);
                alert('Another connection is already using this session. Please close the other browser tab/window first.');
                this.ws.close();
                break;
                
            case 'LOGIN_RESPONSE':
                if (msg.status === 'success') {
                    this.sessionId = msg.session_id;
                    localStorage.setItem('session_id', this.sessionId);
                    this.showMainScreen(msg.user_data);
                } else {
                    alert('Login failed: ' + msg.message);
                }
                break;
                
            // ... handle other messages
        }
    }
    
    login(username, password) {
        const msg = {
            type: 'LOGIN',
            username: username,
            password: hashPassword(password)  // Hash on client side
        };
        this.ws.send(JSON.stringify(msg));
    }
}
```

### Python/Desktop Example

```python
import json
import socket

class ChessClient:
    def __init__(self):
        self.sock = None
        self.session_id = self.load_session_id()
        
    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect(('localhost', 8080))
        
        # FIRST ACTION: Verify session or login
        if self.session_id:
            self.verify_session()
        else:
            self.show_login_window()
    
    def verify_session(self):
        msg = {
            'type': 'VERIFY_SESSION',
            'session_id': self.session_id,
            'timestamp': int(time.time())
        }
        self.send_message(msg)
        
        # Wait for response
        response = self.receive_message()
        
        if response['type'] == 'SESSION_VALID':
            print(f"Session restored for {response['user_data']['username']}")
            self.authenticated = True
            self.user_data = response['user_data']
            self.show_main_screen()
            
        elif response['type'] == 'SESSION_INVALID':
            print(f"Session invalid: {response['reason']}")
            self.session_id = None
            self.save_session_id(None)
            self.show_login_window()
    
    def login(self, username, password):
        msg = {
            'type': 'LOGIN',
            'username': username,
            'password': hash_password(password)
        }
        self.send_message(msg)
        
        response = self.receive_message()
        
        if response['status'] == 'success':
            self.session_id = response['session_id']
            self.save_session_id(self.session_id)
            self.user_data = response['user_data']
            self.authenticated = True
            self.show_main_screen()
        else:
            self.show_error(response['message'])
    
    def load_session_id(self):
        try:
            with open('.session', 'r') as f:
                return f.read().strip()
        except:
            return None
    
    def save_session_id(self, session_id):
        if session_id:
            with open('.session', 'w') as f:
                f.write(session_id)
        else:
            try:
                os.remove('.session')
            except:
                pass
```

---

## Security Considerations

1. **Session ID Generation**: Use cryptographically secure random generator (32+ bytes)
2. **Session Timeout**: Default 30 minutes of inactivity
3. **Session Invalidation**: On logout, server MUST remove session
4. **Duplicate Sessions**: If user logs in from different client, old session should be invalidated
5. **Transport Security**: Use TLS/WSS in production to encrypt session_id transmission
6. **Session Storage**: 
   - Client: Store in localStorage (web) or secure file (desktop)
   - Server: Store in memory with periodic cleanup thread

---

## Benefits of Session Verification First

1. **Fast Reconnection**: Users don't need to re-enter credentials after disconnect
2. **Better UX**: Seamless experience when connection drops temporarily
3. **State Recovery**: Server can restore active game state
4. **Reduced Load**: Fewer database queries for authentication
5. **Security**: Separate session management from authentication logic

---

## Error Handling

### Scenario: Invalid First Message

```json
// Client sends wrong message type first
{
    "type": "GET_AVAILABLE_PLAYERS"  // WRONG - not authenticated yet
}

// Server response
{
    "type": "ERROR",
    "error_code": "AUTH_REQUIRED",
    "message": "First message must be VERIFY_SESSION, LOGIN, or REGISTER",
    "severity": "fatal"
}

// Server closes connection
```

### Scenario: Session Expired During Gameplay

```json
// Server detects session expired during activity
{
    "type": "SESSION_EXPIRED",
    "reason": "inactivity",
    "message": "Your session has expired. Please reconnect."
}

// Client must reconnect and send LOGIN
```

---

## Summary

**Connection Protocol:**

1. **Establish TCP/WebSocket connection**
2. **Client sends ONE of:**
   - `VERIFY_SESSION` (if has session_id) → `SESSION_VALID` / `SESSION_INVALID`
   - `LOGIN` → `LOGIN_RESPONSE`
   - `REGISTER` → `REGISTER_RESPONSE` → `LOGIN`
3. **After authentication, client can send any other message type**
4. **All subsequent messages should include session_id** (or rely on socket mapping)

**This ensures:**
- Clean separation of authentication and application logic
- Support for reconnection scenarios
- Consistent security model
- Better error handling
