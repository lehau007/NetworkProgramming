#ifndef SESSION_REPOSITORY_H
#define SESSION_REPOSITORY_H

#include <string>
#include <optional>
#include <pqxx/pqxx>

class SessionRepository {
public:
    // Create new session in database
    static bool create_session(const std::string& session_id, int user_id, const std::string& ip_address);
    
    // Verify session exists and is valid in database
    static bool verify_session(const std::string& session_id);
    
    // Get user_id from session_id
    static int get_user_id_by_session(const std::string& session_id);
    
    // Update last activity timestamp
    static bool update_activity(const std::string& session_id);
    
    // Delete session from database
    static bool delete_session(const std::string& session_id);
    
    // Delete session by user_id (for duplicate login handling)
    static bool delete_session_by_user_id(int user_id);
    
    // Check if user has active session
    static bool has_active_session(int user_id);
    
    // Get session_id by user_id
    static std::string get_session_id_by_user(int user_id);
    
    // Cleanup expired sessions (older than timeout_seconds)
    static int cleanup_expired_sessions(int timeout_seconds = 1800);
    
    // Get active session count
    static int get_active_session_count();
    
    // Get session info (for debugging/admin)
    struct SessionInfo {
        std::string session_id;
        int user_id;
        std::string login_time;
        std::string last_activity;
        std::string ip_address;
    };
    static std::optional<SessionInfo> get_session_info(const std::string& session_id);
    
private:
    static std::string get_connection_string();
};

#endif // SESSION_REPOSITORY_H
