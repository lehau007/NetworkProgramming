#include "session_repository.h"
#include <iostream>
#include <fstream>
#include <map>

std::string SessionRepository::get_connection_string() {
    std::map<std::string, std::string> env;
    std::ifstream file("/mnt/c/Users/msilaptop/Desktop/NetworkProgramming/Project/NetworkProgramming/server/config/.env");

    std::string line;
    
    if (file.is_open()) {
        while (getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                env[key] = value;
            }
        }
        file.close();
    }
    
    std::string dbname = env.count("DB_NAME") ? env["DB_NAME"] : "chess-app";
    std::string user = env.count("DB_USER") ? env["DB_USER"] : "postgres";
    std::string password = env.count("DB_PASSWORD") ? env["DB_PASSWORD"] : "";
    std::string host = env.count("DB_HOST") ? env["DB_HOST"] : "localhost";
    std::string port = env.count("DB_PORT") ? env["DB_PORT"] : "5432";
    
    return "dbname=" + dbname + " user=" + user + " password=" + password + 
           " host=" + host + " port=" + port + " connect_timeout=5";
}

bool SessionRepository::create_session(const std::string& session_id, int user_id, const std::string& ip_address) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        // First, delete any existing session for this user (enforce single session per user)
        txn.exec_params(
            "DELETE FROM active_sessions WHERE user_id = $1",
            user_id
        );
        
        // Insert new session
        txn.exec_params(
            "INSERT INTO active_sessions (session_id, user_id, login_time, last_activity, ip_address) "
            "VALUES ($1, $2, NOW(), NOW(), $3)",
            session_id, user_id, ip_address
        );
        
        txn.commit();
        std::cout << "[SessionRepository] Created session " << session_id 
                  << " for user_id " << user_id << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error creating session: " << e.what() << std::endl;
        return false;
    }
}

bool SessionRepository::verify_session(const std::string& session_id) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec_params(
            "SELECT session_id FROM active_sessions WHERE session_id = $1",
            session_id
        );
        
        txn.commit();
        return !result.empty();
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error verifying session: " << e.what() << std::endl;
        return false;
    }
}

int SessionRepository::get_user_id_by_session(const std::string& session_id) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec_params(
            "SELECT user_id FROM active_sessions WHERE session_id = $1",
            session_id
        );
        
        txn.commit();
        
        if (result.empty()) {
            return -1;
        }
        
        return result[0]["user_id"].as<int>();
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error getting user_id: " << e.what() << std::endl;
        return -1;
    }
}

bool SessionRepository::update_activity(const std::string& session_id) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec_params(
            "UPDATE active_sessions SET last_activity = NOW() WHERE session_id = $1",
            session_id
        );
        
        txn.commit();
        return result.affected_rows() > 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error updating activity: " << e.what() << std::endl;
        return false;
    }
}

bool SessionRepository::delete_session(const std::string& session_id) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec_params(
            "DELETE FROM active_sessions WHERE session_id = $1",
            session_id
        );
        
        txn.commit();
        
        std::cout << "[SessionRepository] Deleted session " << session_id << std::endl;
        return result.affected_rows() > 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error deleting session: " << e.what() << std::endl;
        return false;
    }
}

bool SessionRepository::delete_session_by_user_id(int user_id) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec_params(
            "DELETE FROM active_sessions WHERE user_id = $1",
            user_id
        );
        
        txn.commit();
        
        std::cout << "[SessionRepository] Deleted sessions for user_id " << user_id << std::endl;
        return result.affected_rows() > 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error deleting session by user_id: " << e.what() << std::endl;
        return false;
    }
}

bool SessionRepository::has_active_session(int user_id) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec_params(
            "SELECT session_id FROM active_sessions WHERE user_id = $1",
            user_id
        );
        
        txn.commit();
        return !result.empty();
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error checking active session: " << e.what() << std::endl;
        return false;
    }
}

std::string SessionRepository::get_session_id_by_user(int user_id) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec_params(
            "SELECT session_id FROM active_sessions WHERE user_id = $1",
            user_id
        );
        
        txn.commit();
        
        if (result.empty()) {
            return "";
        }
        
        return result[0]["session_id"].c_str();
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error getting session_id by user: " << e.what() << std::endl;
        return "";
    }
}

int SessionRepository::cleanup_expired_sessions(int timeout_seconds) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        // Delete sessions where last_activity is older than timeout
        pqxx::result result = txn.exec_params(
            "DELETE FROM active_sessions "
            "WHERE EXTRACT(EPOCH FROM (NOW() - last_activity)) > $1",
            timeout_seconds
        );
        
        txn.commit();
        
        int deleted_count = result.affected_rows();
        if (deleted_count > 0) {
            std::cout << "[SessionRepository] Cleaned up " << deleted_count 
                      << " expired sessions" << std::endl;
        }
        
        return deleted_count;
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error cleaning up sessions: " << e.what() << std::endl;
        return 0;
    }
}

int SessionRepository::get_active_session_count() {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec("SELECT COUNT(*) as count FROM active_sessions");
        
        txn.commit();
        
        if (result.empty()) {
            return 0;
        }
        
        return result[0]["count"].as<int>();
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error getting session count: " << e.what() << std::endl;
        return 0;
    }
}

std::optional<SessionRepository::SessionInfo> SessionRepository::get_session_info(const std::string& session_id) {
    try {
        pqxx::connection conn(get_connection_string());
        pqxx::work txn(conn);
        
        pqxx::result result = txn.exec_params(
            "SELECT session_id, user_id, login_time, last_activity, ip_address "
            "FROM active_sessions WHERE session_id = $1",
            session_id
        );
        
        txn.commit();
        
        if (result.empty()) {
            return std::nullopt;
        }
        
        SessionInfo info;
        info.session_id = result[0]["session_id"].c_str();
        info.user_id = result[0]["user_id"].as<int>();
        info.login_time = result[0]["login_time"].c_str();
        info.last_activity = result[0]["last_activity"].c_str();
        info.ip_address = result[0]["ip_address"].c_str();
        
        return info;
        
    } catch (const std::exception& e) {
        std::cerr << "[SessionRepository] Error getting session info: " << e.what() << std::endl;
        return std::nullopt;
    }
}
