#include <iostream>
#include "../session/session_manager.h"
#include "../database/user_repository.h"

void test_session_management() {
    std::cout << "=== Testing Database-Backed Session Management ===" << std::endl;
    
    SessionManager* session_mgr = SessionManager::get_instance();
    
    // Test 1: Create session
    std::cout << "\n[Test 1] Creating session for user alice..." << std::endl;
    std::string session_id1 = session_mgr->create_session(1, "alice", 100, "127.0.0.1");
    std::cout << "Created session_id: " << session_id1 << std::endl;
    
    // Test 2: Verify session
    std::cout << "\n[Test 2] Verifying session..." << std::endl;
    bool valid = session_mgr->verify_session(session_id1);
    std::cout << "Session valid: " << (valid ? "YES" : "NO") << std::endl;
    
    // Test 3: Check active session count
    std::cout << "\n[Test 3] Active session count..." << std::endl;
    int count = session_mgr->get_active_session_count();
    std::cout << "Active sessions: " << count << std::endl;
    
    // Test 4: Create another session for same user (should replace old one)
    std::cout << "\n[Test 4] Creating second session for same user (duplicate login)..." << std::endl;
    std::string session_id2 = session_mgr->create_session(1, "alice", 101, "127.0.0.1");
    std::cout << "New session_id: " << session_id2 << std::endl;
    
    // Old session should be invalid now
    std::cout << "Old session valid: " << (session_mgr->verify_session(session_id1) ? "YES" : "NO") << std::endl;
    std::cout << "New session valid: " << (session_mgr->verify_session(session_id2) ? "YES" : "NO") << std::endl;
    
    count = session_mgr->get_active_session_count();
    std::cout << "Active sessions after duplicate login: " << count << std::endl;
    
    // Test 5: Create session for different user
    std::cout << "\n[Test 5] Creating session for user bob..." << std::endl;
    std::string session_id3 = session_mgr->create_session(2, "bob", 102, "127.0.0.1");
    std::cout << "Created session_id: " << session_id3 << std::endl;
    
    count = session_mgr->get_active_session_count();
    std::cout << "Total active sessions: " << count << std::endl;
    
    // Test 6: Get session by socket
    std::cout << "\n[Test 6] Getting session by socket..." << std::endl;
    Session* session = session_mgr->get_session_by_socket(102);
    if (session) {
        std::cout << "Found session for socket 102: user=" << session->username << std::endl;
    }
    
    // Test 7: Update activity
    std::cout << "\n[Test 7] Updating session activity..." << std::endl;
    bool updated = session_mgr->update_activity(session_id2);
    std::cout << "Activity updated: " << (updated ? "YES" : "NO") << std::endl;
    
    // Test 8: Check if user has active session
    std::cout << "\n[Test 8] Checking if user has active session..." << std::endl;
    bool has_session = session_mgr->has_active_session(1);
    std::cout << "User 1 (alice) has active session: " << (has_session ? "YES" : "NO") << std::endl;
    
    // Test 9: Get session_id by user_id
    std::cout << "\n[Test 9] Getting session_id by user_id..." << std::endl;
    std::string found_session_id = session_mgr->get_session_id_by_user(1);
    std::cout << "Session ID for user 1: " << found_session_id << std::endl;
    std::cout << "Matches current session: " << (found_session_id == session_id2 ? "YES" : "NO") << std::endl;
    
    // Test 10: Remove session
    std::cout << "\n[Test 10] Removing session..." << std::endl;
    session_mgr->remove_session(session_id3);
    count = session_mgr->get_active_session_count();
    std::cout << "Active sessions after removal: " << count << std::endl;
    
    // Test 11: Verify removed session
    std::cout << "\n[Test 11] Verifying removed session..." << std::endl;
    valid = session_mgr->verify_session(session_id3);
    std::cout << "Removed session valid: " << (valid ? "YES" : "NO") << std::endl;
    
    // Test 12: Cleanup (manual trigger)
    std::cout << "\n[Test 12] Running cleanup..." << std::endl;
    session_mgr->cleanup_expired_sessions();
    count = session_mgr->get_active_session_count();
    std::cout << "Active sessions after cleanup: " << count << std::endl;
    
    // Final cleanup
    std::cout << "\n[Cleanup] Removing remaining sessions..." << std::endl;
    session_mgr->remove_session(session_id2);
    count = session_mgr->get_active_session_count();
    std::cout << "Final active session count: " << count << std::endl;
    
    std::cout << "\n=== Session Management Tests Complete ===" << std::endl;
}

int main() {
    std::cout << "Database-Backed Session Manager Test\n" << std::endl;
    std::cout << "This test verifies session persistence in PostgreSQL database" << std::endl;
    std::cout << "Database table: active_sessions" << std::endl;
    std::cout << "Features: Create, verify, update, remove sessions with DB persistence\n" << std::endl;
    
    try {
        test_session_management();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nTest failed with error: " << e.what() << std::endl;
        std::cerr << "\nPlease ensure:" << std::endl;
        std::cerr << "1. PostgreSQL is running" << std::endl;
        std::cerr << "2. Database 'chess-app' exists" << std::endl;
        std::cerr << "3. Table 'active_sessions' is created (run schema.sql)" << std::endl;
        std::cerr << "4. .env file has correct database credentials" << std::endl;
        return 1;
    }
}
