# Database-Backed Session Management

## Architecture Overview

The session management system uses a **hybrid architecture** combining:
1. **PostgreSQL database** - Source of truth for session persistence
2. **In-memory cache** - Fast lookups for active connections

This design ensures sessions survive server restarts while maintaining high performance.

---

## Database Schema

### `active_sessions` Table

```sql
CREATE TABLE active_sessions (
    session_id VARCHAR(64) PRIMARY KEY,
    user_id INT REFERENCES users(user_id) ON DELETE CASCADE,
    login_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ip_address VARCHAR(45)
);

CREATE INDEX idx_sessions_user_id ON active_sessions(user_id);
```

**Fields:**
- `session_id` - Unique 32-character hex string (primary key)
- `user_id` - Foreign key to users table
- `login_time` - When session was created
- `last_activity` - Last activity timestamp (updated on each request)
- `ip_address` - Client IP address (IPv4/IPv6)

---

## Architecture Layers

### 1. SessionRepository (Database Layer)

**File:** `database/session_repository.h/cpp`

**Responsibilities:**
- Direct database operations
- CRUD operations on `active_sessions` table
- Session validation and cleanup
- No business logic, pure data access

**Key Methods:**
```cpp
bool create_session(session_id, user_id, ip_address);
bool verify_session(session_id);
int get_user_id_by_session(session_id);
bool update_activity(session_id);
bool delete_session(session_id);
int cleanup_expired_sessions(timeout_seconds);
```

### 2. SessionManager (Business Logic + Cache)

**File:** `session/session_manager.h/cpp`

**Responsibilities:**
- Session lifecycle management
- In-memory caching for performance
- Socket-to-session mapping (runtime only)
- Coordinate between database and cache

**Key Methods:**
```cpp
string create_session(user_id, username, socket, ip);
bool verify_session(session_id);
Session* get_session_by_socket(socket);
void update_activity(session_id);
void remove_session(session_id);
void cleanup_expired_sessions();
```

---

## Data Flow

### Session Creation (Login)

```
Client → Server → SessionManager
                      ↓
                  Generate session_id
                      ↓
            SessionRepository::create_session()
                      ↓
              [Database INSERT/UPDATE]
                      ↓
              Store in memory cache
                      ↓
            Return session_id to client
```

**SQL Executed:**
```sql
-- Delete old session for this user (enforce single session)
DELETE FROM active_sessions WHERE user_id = $1;

-- Insert new session
INSERT INTO active_sessions (session_id, user_id, login_time, last_activity, ip_address)
VALUES ($1, $2, NOW(), NOW(), $3);
```

### Session Verification (Every Request)

```
Client sends session_id → SessionManager::verify_session()
                                ↓
                    SessionRepository::verify_session()
                                ↓
                        [Database SELECT]
                                ↓
                    Check if session exists
                                ↓
                Load to cache if not cached
                                ↓
                Update last_activity in DB
                                ↓
                    Return valid/invalid
```

**SQL Executed:**
```sql
-- Check session exists
SELECT session_id FROM active_sessions WHERE session_id = $1;

-- Update activity timestamp
UPDATE active_sessions SET last_activity = NOW() WHERE session_id = $1;
```

### Session Cleanup (Background Thread)

```
Periodic timer (every 60 seconds)
            ↓
SessionManager::cleanup_expired_sessions()
            ↓
SessionRepository::cleanup_expired_sessions(1800)
            ↓
    [Database DELETE expired sessions]
            ↓
    Clear entire cache to resync
```

**SQL Executed:**
```sql
-- Delete sessions inactive for > 30 minutes
DELETE FROM active_sessions 
WHERE EXTRACT(EPOCH FROM (NOW() - last_activity)) > 1800;
```

---

## Cache Strategy

### What is Cached?

**In-Memory Maps:**
1. `sessions_by_id` - session_id → Session struct
2. `sessions_by_socket` - socket descriptor → session_id (runtime only)
3. `sessions_by_user_id` - user_id → session_id

### When is Cache Used?

**Cache-First (Fast Path):**
- `get_session_by_socket()` - Socket mapping only exists in memory
- `update_socket_mapping()` - Runtime socket tracking

**Database-First (Authoritative):**
- `verify_session()` - Always checks database first
- `create_session()` - Always writes to database
- `cleanup_expired_sessions()` - Database is source of truth

### Cache Invalidation

**Cache is cleared when:**
1. Session removed - specific entry removed
2. Cleanup runs - entire cache cleared to resync with DB
3. Session not found in DB - invalidate cache entry

---

## Session Lifecycle

### 1. User Logs In

```cpp
// After successful authentication
SessionManager* mgr = SessionManager::get_instance();
string session_id = mgr->create_session(
    user_id,        // From database authentication
    username,       // User's username
    client_socket,  // TCP socket descriptor
    "127.0.0.1"     // Client IP address
);

// Send session_id to client
send_json_response(client_socket, {
    "type": "LOGIN_RESPONSE",
    "status": "success",
    "session_id": session_id,
    "user_data": {...}
});
```

**Database State:**
- Old session for user (if exists) is deleted
- New session inserted into `active_sessions`

**Memory State:**
- Session cached in `sessions_by_id`
- Socket mapping created in `sessions_by_socket`
- User mapping created in `sessions_by_user_id`

### 2. Client Reconnects

```cpp
// Client sends VERIFY_SESSION with stored session_id
bool valid = mgr->verify_session(session_id);

if (valid) {
    // Session still valid in database
    Session* session = mgr->get_session(session_id);
    
    // Update socket mapping for new connection
    bool mapping_success = mgr->update_socket_mapping(session_id, new_client_socket);
    
    if (!mapping_success) {
        // Session already has an active socket (another connection)
        // Send DUPLICATE_SESSION error
        send_response(client_socket, {
            {"type", "DUPLICATE_SESSION"},
            {"session_id", session_id},
            {"reason", "already_connected"},
            {"message", "Multiple connections with the same session are not allowed."}
        });
        return;
    }
    
    // Session restored
} else {
    // Session expired or invalid
    // Client must LOGIN again
}
```

**Database State:**
- Session verified in `active_sessions`
- `last_activity` updated to NOW()

**Memory State:**
- Session loaded to cache (if not already cached)
- Socket mapping updated to new socket (only if no existing active socket)

### 3. Client Makes Requests

```cpp
// For each incoming message
Session* session = mgr->get_session_by_socket(client_socket);

if (!session) {
    send_error(client_socket, "No active session");
    return;
}

// Process authenticated request
mgr->update_activity(session->session_id);
```

**Database State:**
- `last_activity` timestamp updated on each request

**Memory State:**
- Fast socket-based lookup (no DB query)
- Activity timestamp updated in cache

### 4. User Logs Out

```cpp
// Client sends LOGOUT
mgr->remove_session_by_socket(client_socket);
```

**Database State:**
- Session deleted from `active_sessions`

**Memory State:**
- All cache entries removed
- Socket mapping removed

### 5. Session Expires

```cpp
// Background thread runs every 60 seconds
pthread_create(&cleanup_thread, NULL, session_cleanup_worker, NULL);

void* session_cleanup_worker(void* arg) {
    while (server_running) {
        sleep(60);
        SessionManager::get_instance()->cleanup_expired_sessions();
    }
    return NULL;
}
```

**Database State:**
- Sessions with `last_activity > 30 minutes ago` deleted

**Memory State:**
- Entire cache cleared after cleanup
- Will be repopulated on-demand

---

## Benefits of This Architecture

### ✅ Session Persistence
Sessions survive server restarts. After server crash, clients can reconnect with stored session_id.

### ✅ Single Session per User
Database enforces one session per user. New login invalidates old session (prevents session stealing).

### ✅ High Performance
Socket-based lookups use in-memory cache (no DB query on every message).

### ✅ Scalability
Database can be moved to separate server. Multiple server instances can share session state.

### ✅ Security
- Sessions expire after 30 minutes inactivity
- Database provides audit trail (login_time, last_activity)
- IP address tracking for security monitoring

### ✅ Reliability
- Database is source of truth
- Cache misses trigger DB lookup and repopulation
- Periodic cleanup prevents stale sessions

---

## Configuration

### Session Timeout

**Default:** 30 minutes (1800 seconds)

**Change in:** `session/session_manager.h`
```cpp
static const int SESSION_TIMEOUT = 1800;  // Change this value
```

### Cleanup Interval

**Default:** 60 seconds

**Change in:** Server main thread
```cpp
sleep(60);  // Change cleanup interval
```

### Session ID Length

**Default:** 32 characters (hex)

**Change in:** `session/session_manager.h`
```cpp
static const int SESSION_ID_LENGTH = 32;  // Change this value
```

---

## Testing

### Build and Run Tests

```bash
# In WSL terminal
cd server

# Build test program
make test_session_mgr

# Run tests
make run_session_test
# or
./test_session_mgr
```

### Test Coverage

The test program verifies:
1. ✅ Create session (writes to database)
2. ✅ Verify session (reads from database)
3. ✅ Active session count (database query)
4. ✅ Duplicate login handling (deletes old session in DB)
5. ✅ Multiple concurrent sessions
6. ✅ Socket-based lookups (cache)
7. ✅ Activity updates (database)
8. ✅ Session removal (database + cache)
9. ✅ User session queries
10. ✅ Cleanup expired sessions

### Manual Database Verification

```bash
# Connect to PostgreSQL
psql -U postgres -d chess-app

# View active sessions
SELECT * FROM active_sessions;

# Check session for specific user
SELECT * FROM active_sessions WHERE user_id = 1;

# Check expired sessions
SELECT *, 
       EXTRACT(EPOCH FROM (NOW() - last_activity)) as seconds_inactive
FROM active_sessions;

# Manual cleanup
DELETE FROM active_sessions 
WHERE EXTRACT(EPOCH FROM (NOW() - last_activity)) > 1800;
```

---

## Integration with Server

### Example: Client Handler

```cpp
void* handle_client(void* arg) {
    int client_socket = ((ClientContext*)arg)->socket;
    SessionManager* session_mgr = SessionManager::get_instance();
    
    // Wait for first message
    string first_msg = receive_json_message(client_socket);
    Json::Value msg = parse_json(first_msg);
    
    if (msg["type"] == "VERIFY_SESSION") {
        string session_id = msg["session_id"].asString();
        
        if (session_mgr->verify_session(session_id)) {
            // Session valid - try to update socket mapping
            bool mapping_success = session_mgr->update_socket_mapping(session_id, client_socket);
            
            if (!mapping_success) {
                // Another connection is already using this session
                send_json_response(client_socket, {
                    {"type", "DUPLICATE_SESSION"},
                    {"session_id", session_id},
                    {"reason", "already_connected"},
                    {"message", "Multiple connections with the same session are not allowed."}
                });
                close(client_socket);
                return NULL;
            }
            
            send_json_response(client_socket, {
                {"type", "SESSION_VALID"},
                {"session_id", session_id},
                {"user_data", get_user_data(session_id)}
            });
            
            // Enter authenticated message loop
            handle_authenticated_messages(client_socket);
            
        } else {
            send_json_response(client_socket, {
                {"type", "SESSION_INVALID"},
                {"reason", "expired"}
            });
        }
        
    } else if (msg["type"] == "LOGIN") {
        handle_login(client_socket, msg);
    }
    
    // Cleanup on disconnect
    session_mgr->remove_session_by_socket(client_socket);
    close(client_socket);
    
    return NULL;
}
```

---

## Troubleshooting

### Sessions not persisting after restart

**Check:**
1. Database connection is working
2. `active_sessions` table exists
3. Sessions are actually written to DB (check with `psql`)

### Cache/Database out of sync

**Solution:** Run cleanup to clear cache
```cpp
session_mgr->cleanup_expired_sessions();
```

### Sessions expiring too quickly

**Adjust timeout:**
```cpp
// In session_manager.h
static const int SESSION_TIMEOUT = 3600;  // 1 hour
```

### High database load

**Optimization:**
- Increase cache hit rate
- Batch activity updates
- Use connection pooling
- Add database indexes (already included in schema)

---

## Security Considerations

### ✅ Implemented

1. **Session expiration** - 30 minute timeout
2. **Single session enforcement** - One session per user
3. **Single connection per session** - Prevents multiple simultaneous connections with same session_id
4. **IP tracking** - Stored for audit
5. **Secure session ID** - 32-char random hex
6. **Database CASCADE** - Sessions deleted when user deleted
7. **DUPLICATE_SESSION response** - Informs users when another connection is already using their session

### 🔒 Production Recommendations

1. **Use HTTPS/WSS** - Encrypt session_id in transit
2. **Hash IP addresses** - Privacy compliance
3. **Add user agent** - Track device changes
4. **Rate limiting** - Prevent brute force
5. **Session rotation** - Regenerate ID periodically
6. **Anomaly detection** - Alert on IP changes

---

## Performance Characteristics

### Memory Usage
- **Per Session:** ~200 bytes (cache entry)
- **1000 active sessions:** ~200 KB RAM
- **10000 active sessions:** ~2 MB RAM

### Database Load
- **Session creation:** 2 queries (DELETE + INSERT)
- **Session verification:** 2 queries (SELECT + UPDATE)
- **Session cleanup:** 1 query (DELETE with WHERE)
- **Expected QPS:** 100-1000 queries/second

### Latency
- **Cache hit (socket lookup):** < 1 μs
- **Database query (session verify):** 1-10 ms
- **Total verification time:** < 15 ms

---

## Future Enhancements

1. **Redis cache** - Add Redis layer for multi-server setup
2. **Connection pooling** - Reuse database connections
3. **Async updates** - Queue activity updates
4. **Session analytics** - Track login patterns
5. **Admin API** - View/manage sessions remotely

---

## Summary

**Database-backed session management provides:**
- ✅ Persistence across server restarts
- ✅ High performance with caching
- ✅ Single source of truth (PostgreSQL)
- ✅ Automatic cleanup of expired sessions
- ✅ Support for reconnection scenarios
- ✅ Security and audit trail

**Usage:**
```cpp
SessionManager* mgr = SessionManager::get_instance();
string session_id = mgr->create_session(user_id, username, socket, ip);
bool valid = mgr->verify_session(session_id);
mgr->update_activity(session_id);
mgr->remove_session(session_id);
```

This architecture aligns with the predefined system design while adding production-ready session management capabilities.
