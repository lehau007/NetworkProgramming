#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <string>
#include <map>
#include <pthread.h>
#include <ctime>
#include "../database/session_repository.h"

struct Session {
    std::string session_id;
    int user_id;
    std::string username;
    int client_socket;
    time_t created_at;
    time_t last_activity;
    std::string ip_address;
    bool is_active;
    bool authenticated;
};

class SessionManager {
private:
    // In-memory cache for quick lookups (socket mapping)
    std::map<std::string, Session> sessions_by_id;        // session_id -> Session (cache)
    std::map<int, std::string> sessions_by_socket;        // socket -> session_id (runtime only)
    std::map<int, std::string> sessions_by_user_id;       // user_id -> session_id (cache)
    
    pthread_mutex_t mutex;
    
    static const int SESSION_TIMEOUT = 1800;  // 30 minutes in seconds
    static const int SESSION_ID_LENGTH = 32;
    
    // Generate random session ID
    std::string generate_session_id();
    
    // Singleton instance
    static SessionManager* instance;
    
    SessionManager();
    
public:
    ~SessionManager();
    
    // Singleton accessor
    static SessionManager* get_instance();
    
    // Session management (uses database + cache)
    std::string create_session(int user_id, const std::string& username, int client_socket, const std::string& ip_address);
    bool verify_session(const std::string& session_id);
    bool verify_session_by_socket(int client_socket);
    Session* get_session(const std::string& session_id);
    Session* get_session_by_socket(int client_socket);
    Session* get_session_by_user_id(int user_id);
    
    // Session operations
    bool update_activity(const std::string& session_id);
    bool update_activity_by_socket(int client_socket);
    void remove_session_in_cache(const std::string& session_id);
    void remove_session_in_database(const std::string& session_id);
    void remove_session_by_socket_in_cache(int client_socket);
    void remove_session_by_user_id_in_cache(int user_id);
    void remove_session_by_socket_in_database(int client_socket);
    void remove_session_by_user_id_in_database(int user_id);
    
    // Authentication
    bool is_authenticated(const std::string& session_id);
    bool is_authenticated_by_socket(int client_socket);
    void mark_authenticated(const std::string& session_id, int user_id, const std::string& username);
    
    // Cleanup
    void cleanup_expired_sessions();
    int get_active_session_count();
    
    // For reconnection support
    bool has_active_session(int user_id);
    std::string get_session_id_by_user(int user_id);
    
    // Socket mapping (runtime only - not persisted)
    // Returns false if session is already mapped to a different socket (duplicate connection)
    bool update_socket_mapping(const std::string& session_id, int client_socket);
    void remove_socket_mapping(int client_socket);
    bool is_user_connected(int user_id);
    
    // Cache management
    void load_session_to_cache(const std::string& session_id);
    void invalidate_cache(const std::string& session_id);
};

#endif
