-- Chess Game Database Schema
-- PostgreSQL

-- Drop tables if they exist (for clean setup)
DROP TABLE IF EXISTS game_history CASCADE;
DROP TABLE IF EXISTS active_sessions CASCADE;
DROP TABLE IF EXISTS users CASCADE;

-- Users table
CREATE TABLE users (
    user_id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(256) NOT NULL,
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    wins INT DEFAULT 0,
    losses INT DEFAULT 0,
    draws INT DEFAULT 0,
    rating INT DEFAULT 1200
);

-- Game history table
CREATE TABLE game_history (
    game_id SERIAL PRIMARY KEY,
    white_player_id INT REFERENCES users(user_id) ON DELETE CASCADE,
    black_player_id INT REFERENCES users(user_id) ON DELETE CASCADE,
    result VARCHAR(20),  -- 'WHITE_WIN', 'BLACK_WIN', 'DRAW', 'ABORTED'
    moves JSONB,         -- Array of moves in JSON format
    start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    end_time TIMESTAMP,
    duration INT         -- in seconds
);

-- Active sessions (optional - for session management)
CREATE TABLE active_sessions (
    session_id VARCHAR(64) PRIMARY KEY,
    user_id INT REFERENCES users(user_id) ON DELETE CASCADE,
    login_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ip_address VARCHAR(45)
);

-- Create indexes for better performance
CREATE INDEX idx_users_username ON users(username);
CREATE INDEX idx_game_white_player ON game_history(white_player_id);
CREATE INDEX idx_game_black_player ON game_history(black_player_id);
CREATE INDEX idx_game_start_time ON game_history(start_time DESC);
CREATE INDEX idx_sessions_user_id ON active_sessions(user_id);

-- Insert sample users for testing
INSERT INTO users (username, password_hash, email, wins, losses, draws, rating) VALUES
    ('alice', 'hash_alice_123', 'alice@example.com', 10, 5, 2, 1350),
    ('bob', 'hash_bob_456', 'bob@example.com', 8, 7, 3, 1280),
    ('charlie', 'hash_charlie_789', 'charlie@example.com', 15, 3, 1, 1450),
    ('diana', 'hash_diana_012', 'diana@example.com', 5, 10, 2, 1150);

-- Insert sample game history
INSERT INTO game_history (white_player_id, black_player_id, result, moves, start_time, end_time, duration) VALUES
    (1, 2, 'WHITE_WIN', '["e2e4", "e7e5", "Ng1f3", "Nb8c6", "Bf1c4"]'::jsonb, 
     NOW() - INTERVAL '2 days', NOW() - INTERVAL '2 days' + INTERVAL '15 minutes', 900),
    (2, 3, 'BLACK_WIN', '["d2d4", "d7d5", "c2c4", "e7e6"]'::jsonb,
     NOW() - INTERVAL '1 day', NOW() - INTERVAL '1 day' + INTERVAL '22 minutes', 1320),
    (1, 3, 'DRAW', '["e2e4", "c7c5", "Ng1f3", "d7d6"]'::jsonb,
     NOW() - INTERVAL '12 hours', NOW() - INTERVAL '11 hours 45 minutes', 900);

-- Verify data
SELECT 'Users created:' as message, COUNT(*) as count FROM users;
SELECT 'Games created:' as message, COUNT(*) as count FROM game_history;

COMMIT;
