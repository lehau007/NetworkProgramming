#include "session_manager.h"
#include <iostream>
#include <sstream>
#include <random>
#include <iomanip>
#include <cstring>

SessionManager* SessionManager::instance = nullptr;

SessionManager::SessionManager() {
    pthread_mutex_init(&mutex, nullptr);
}

SessionManager::~SessionManager() {
    pthread_mutex_destroy(&mutex);
}

SessionManager* SessionManager::get_instance() {
    if (instance == nullptr) {
        instance = new SessionManager();
    }
    return instance;
}

std::string SessionManager::generate_session_id() {
    // Generate random session ID using random bytes
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    
    std::stringstream ss;
    for (int i = 0; i < SESSION_ID_LENGTH / 2; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2) << dis(gen);
    }
    
    return ss.str();
}

std::string SessionManager::create_session(int user_id, const std::string& username, 
                                          int client_socket, const std::string& ip_address) {
    pthread_mutex_lock(&mutex);
    
    // Generate new session ID
    std::string session_id = generate_session_id();
    
    // Create session in database (this also removes old sessions for the user)
    bool db_success = SessionRepository::create_session(session_id, user_id, ip_address);
    
    if (!db_success) {
        pthread_mutex_unlock(&mutex);
        std::cerr << "[SessionManager] Failed to create session in database" << std::endl;
        return "";
    }
    
    // Remove old session from cache if exists
    if (sessions_by_user_id.find(user_id) != sessions_by_user_id.end()) {
        std::string old_session_id = sessions_by_user_id[user_id];
        Session& old_session = sessions_by_id[old_session_id];
        sessions_by_socket.erase(old_session.client_socket);
        sessions_by_id.erase(old_session_id);
        sessions_by_user_id.erase(user_id);
    }
    
    // Create session object for cache
    Session session;
    session.session_id = session_id;
    session.user_id = user_id;
    session.username = username;
    session.client_socket = client_socket;
    session.created_at = time(nullptr);
    session.last_activity = time(nullptr);
    session.ip_address = ip_address;
    session.is_active = true;
    session.authenticated = true;
    
    // Store in cache
    sessions_by_id[session.session_id] = session;
    sessions_by_socket[client_socket] = session.session_id;
    sessions_by_user_id[user_id] = session.session_id;
    
    pthread_mutex_unlock(&mutex);
    
    std::cout << "[SessionManager] Created session " << session.session_id 
              << " for user " << username << " (ID: " << user_id << ") - stored in DB" << std::endl;
    
    return session.session_id;
}

bool SessionManager::verify_session(const std::string& session_id) {
    // First check database (source of truth)
    bool db_valid = SessionRepository::verify_session(session_id);
    
    if (!db_valid) {
        // Session not in database, remove from cache if exists
        invalidate_cache(session_id);
        return false;
    }
    
    pthread_mutex_lock(&mutex);
    
    // Load session to cache if not already cached
    auto it = sessions_by_id.find(session_id);
    if (it == sessions_by_id.end()) {
        pthread_mutex_unlock(&mutex);
        load_session_to_cache(session_id);
        pthread_mutex_lock(&mutex);
        it = sessions_by_id.find(session_id);
    }
    
    if (it != sessions_by_id.end()) {
        // Update cache activity
        it->second.last_activity = time(nullptr);
        it->second.is_active = true;
    }
    
    pthread_mutex_unlock(&mutex);
    
    // Update database activity
    SessionRepository::update_activity(session_id);
    
    return true;
}

bool SessionManager::verify_session_by_socket(int client_socket) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_socket.find(client_socket);
    if (it == sessions_by_socket.end()) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    
    std::string session_id = it->second;
    pthread_mutex_unlock(&mutex);
    
    return verify_session(session_id);
}

Session* SessionManager::get_session(const std::string& session_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_id.find(session_id);
    if (it == sessions_by_id.end()) {
        pthread_mutex_unlock(&mutex);
        return nullptr;
    }
    
    pthread_mutex_unlock(&mutex);
    return &(it->second);
}

Session* SessionManager::get_session_by_socket(int client_socket) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_socket.find(client_socket);
    if (it == sessions_by_socket.end()) {
        pthread_mutex_unlock(&mutex);
        return nullptr;
    }
    
    std::string session_id = it->second;
    pthread_mutex_unlock(&mutex);
    
    return get_session(session_id);
}

Session* SessionManager::get_session_by_user_id(int user_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_user_id.find(user_id);
    if (it == sessions_by_user_id.end()) {
        pthread_mutex_unlock(&mutex);
        return nullptr;
    }
    
    std::string session_id = it->second;
    pthread_mutex_unlock(&mutex);
    
    return get_session(session_id);
}

bool SessionManager::update_activity(const std::string& session_id) {
    // Update in database
    bool db_success = SessionRepository::update_activity(session_id);
    
    if (!db_success) {
        return false;
    }
    
    // Update in cache
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_id.find(session_id);
    if (it != sessions_by_id.end()) {
        it->second.last_activity = time(nullptr);
    }
    
    pthread_mutex_unlock(&mutex);
    return true;
}

bool SessionManager::update_activity_by_socket(int client_socket) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_socket.find(client_socket);
    if (it == sessions_by_socket.end()) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    
    std::string session_id = it->second;
    pthread_mutex_unlock(&mutex);
    
    return update_activity(session_id);
}

void SessionManager::remove_session(const std::string& session_id) {
    // Remove from database
    SessionRepository::delete_session(session_id);
    
    // Remove from cache
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_id.find(session_id);
    if (it != sessions_by_id.end()) {
        Session& session = it->second;
        
        std::cout << "[SessionManager] Removing session " << session_id 
                  << " for user " << session.username << " (DB + cache)" << std::endl;
        
        // Remove from all maps
        sessions_by_socket.erase(session.client_socket);
        sessions_by_user_id.erase(session.user_id);
        sessions_by_id.erase(session_id);
    }
    
    pthread_mutex_unlock(&mutex);
}

void SessionManager::remove_session_by_socket(int client_socket) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_socket.find(client_socket);
    if (it != sessions_by_socket.end()) {
        std::string session_id = it->second;
        pthread_mutex_unlock(&mutex);
        remove_session(session_id);
    } else {
        pthread_mutex_unlock(&mutex);
    }
}

void SessionManager::remove_session_by_user_id(int user_id) {
    // Remove from database
    SessionRepository::delete_session_by_user_id(user_id);
    
    // Remove from cache
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_user_id.find(user_id);
    if (it != sessions_by_user_id.end()) {
        std::string session_id = it->second;
        pthread_mutex_unlock(&mutex);
        remove_session(session_id);
    } else {
        pthread_mutex_unlock(&mutex);
    }
}

bool SessionManager::is_authenticated(const std::string& session_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_id.find(session_id);
    if (it == sessions_by_id.end()) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    
    bool authenticated = it->second.authenticated;
    pthread_mutex_unlock(&mutex);
    return authenticated;
}

bool SessionManager::is_authenticated_by_socket(int client_socket) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_socket.find(client_socket);
    if (it == sessions_by_socket.end()) {
        pthread_mutex_unlock(&mutex);
        return false;
    }
    
    std::string session_id = it->second;
    pthread_mutex_unlock(&mutex);
    
    return is_authenticated(session_id);
}

void SessionManager::mark_authenticated(const std::string& session_id, int user_id, 
                                       const std::string& username) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_id.find(session_id);
    if (it != sessions_by_id.end()) {
        it->second.authenticated = true;
        it->second.user_id = user_id;
        it->second.username = username;
        
        // Update user_id mapping
        sessions_by_user_id[user_id] = session_id;
        
        std::cout << "[SessionManager] Session " << session_id 
                  << " authenticated for user " << username << std::endl;
    }
    
    pthread_mutex_unlock(&mutex);
}

void SessionManager::cleanup_expired_sessions() {
    // Cleanup in database (source of truth)
    int cleaned = SessionRepository::cleanup_expired_sessions(SESSION_TIMEOUT);
    
    if (cleaned > 0) {
        // Clear entire cache to resync with database
        pthread_mutex_lock(&mutex);
        sessions_by_id.clear();
        sessions_by_socket.clear();
        sessions_by_user_id.clear();
        pthread_mutex_unlock(&mutex);
        
        std::cout << "[SessionManager] Cleaned up " << cleaned 
                  << " expired sessions from database, cache cleared" << std::endl;
    }
}

int SessionManager::get_active_session_count() {
    // Get count from database (source of truth)
    return SessionRepository::get_active_session_count();
}

bool SessionManager::has_active_session(int user_id) {
    // Check database (source of truth)
    return SessionRepository::has_active_session(user_id);
}

std::string SessionManager::get_session_id_by_user(int user_id) {
    // Get from database (source of truth)
    return SessionRepository::get_session_id_by_user(user_id);
}

void SessionManager::update_socket_mapping(const std::string& session_id, int client_socket) {
    pthread_mutex_lock(&mutex);
    
    sessions_by_socket[client_socket] = session_id;
    
    // Update socket in cached session
    auto it = sessions_by_id.find(session_id);
    if (it != sessions_by_id.end()) {
        it->second.client_socket = client_socket;
    }
    
    pthread_mutex_unlock(&mutex);
}

void SessionManager::remove_socket_mapping(int client_socket) {
    pthread_mutex_lock(&mutex);
    sessions_by_socket.erase(client_socket);
    pthread_mutex_unlock(&mutex);
}

void SessionManager::load_session_to_cache(const std::string& session_id) {
    // Get session info from database
    auto session_info = SessionRepository::get_session_info(session_id);
    
    if (!session_info.has_value()) {
        return;
    }
    
    pthread_mutex_lock(&mutex);
    
    // Create cache entry
    Session session;
    session.session_id = session_info->session_id;
    session.user_id = session_info->user_id;
    session.username = "";  // Will be filled when needed
    session.client_socket = -1;  // Not connected yet
    session.created_at = time(nullptr);
    session.last_activity = time(nullptr);
    session.ip_address = session_info->ip_address;
    session.is_active = true;
    session.authenticated = true;
    
    sessions_by_id[session_id] = session;
    sessions_by_user_id[session.user_id] = session_id;
    
    pthread_mutex_unlock(&mutex);
    
    std::cout << "[SessionManager] Loaded session " << session_id << " to cache" << std::endl;
}

void SessionManager::invalidate_cache(const std::string& session_id) {
    pthread_mutex_lock(&mutex);
    
    auto it = sessions_by_id.find(session_id);
    if (it != sessions_by_id.end()) {
        Session& session = it->second;
        sessions_by_socket.erase(session.client_socket);
        sessions_by_user_id.erase(session.user_id);
        sessions_by_id.erase(session_id);
        
        std::cout << "[SessionManager] Invalidated cache for session " << session_id << std::endl;
    }
    
    pthread_mutex_unlock(&mutex);
}
