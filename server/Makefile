# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread -Inetwork -Idatabase -Isession -Iutils -Igame -Iai
LDFLAGS = -lpqxx -lpq -lssl -lcrypto

# Object files
SOCKET_OBJS = network/socket_handler.o network/websocket_handler.o
SESSION_OBJS = session/session_manager.o database/session_repository.o
UTILS_OBJS = utils/message_handler.o
DATABASE_OBJS = database/user_repository.o database/game_repository.o
GAME_OBJS = game/match_manager.o
AI_OBJS = ai/chess_ai.o

# Targets
all: test_db chess_server websocket_server test_user_repo test_game_repo test_session_mgr

# Test database connection
test_db: database/database_connection.cpp
	$(CXX) $(CXXFLAGS) -o test_db database/database_connection.cpp $(LDFLAGS)

# Test user repository
test_user_repo: database/test_user_repository.cpp database/user_repository.h
	$(CXX) $(CXXFLAGS) -o test_user_repo database/test_user_repository.cpp $(LDFLAGS)

# Test game repository
test_game_repo: database/test_game_repository.cpp database/game_repository.h
	$(CXX) $(CXXFLAGS) -o test_game_repo database/test_game_repository.cpp $(LDFLAGS)

# Test session manager
test_session_mgr: database/test_session_manager.cpp database/session_repository.cpp session/session_manager.cpp
	$(CXX) $(CXXFLAGS) -Isession -o test_session_mgr database/test_session_manager.cpp database/session_repository.cpp session/session_manager.cpp $(LDFLAGS)

# Chess server with message handlers
chess_server: $(SOCKET_OBJS) $(SESSION_OBJS) $(UTILS_OBJS) $(DATABASE_OBJS) $(GAME_OBJS) $(AI_OBJS) server.o
	$(CXX) $(SOCKET_OBJS) $(SESSION_OBJS) $(UTILS_OBJS) $(DATABASE_OBJS) $(GAME_OBJS) $(AI_OBJS) server.o -o chess_server $(LDFLAGS)

# WebSocket server
websocket_server: $(SOCKET_OBJS) websocket_server_example.o
	$(CXX) $(SOCKET_OBJS) websocket_server_example.o -o websocket_server $(LDFLAGS)

# Compile network objects
network/socket_handler.o: network/socket_handler.cpp network/socket_handler.h
	$(CXX) $(CXXFLAGS) -c network/socket_handler.cpp -o network/socket_handler.o

network/websocket_handler.o: network/websocket_handler.cpp network/websocket_handler.h
	$(CXX) $(CXXFLAGS) -c network/websocket_handler.cpp -o network/websocket_handler.o

websocket_server_example.o: websocket_server_example.cpp
	$(CXX) $(CXXFLAGS) -c websocket_server_example.cpp -o websocket_server_example.o

# Compile server objects
server.o: server.cpp
	$(CXX) $(CXXFLAGS) -c server.cpp -o server.o

# Compile session objects
session/session_manager.o: session/session_manager.cpp session/session_manager.h
	$(CXX) $(CXXFLAGS) -c session/session_manager.cpp -o session/session_manager.o

database/session_repository.o: database/session_repository.cpp database/session_repository.h
	$(CXX) $(CXXFLAGS) -c database/session_repository.cpp -o database/session_repository.o

# Compile utils objects
utils/message_handler.o: utils/message_handler.cpp utils/message_handler.h utils/message_types.h
	$(CXX) $(CXXFLAGS) -c utils/message_handler.cpp -o utils/message_handler.o

# Compile database objects
database/user_repository.o: database/user_repository.cpp database/user_repository.h
	$(CXX) $(CXXFLAGS) -c database/user_repository.cpp -o database/user_repository.o

database/game_repository.o: database/game_repository.cpp database/game_repository.h
	$(CXX) $(CXXFLAGS) -c database/game_repository.cpp -o database/game_repository.o

# Compile game objects
game/match_manager.o: game/match_manager.cpp game/match_manager.h game/chess_game.cpp
	$(CXX) $(CXXFLAGS) -c game/match_manager.cpp -o game/match_manager.o

# Compile AI objects
ai/chess_ai.o: ai/chess_ai.cpp ai/chess_ai.h
	$(CXX) $(CXXFLAGS) -c ai/chess_ai.cpp -o ai/chess_ai.o

# Clean build artifacts
clean:
	rm -f test_db test_user_repo test_game_repo test_session_mgr chess_server websocket_server *.o network/*.o session/*.o database/*.o utils/*.o game/*.o ai/*.o

# Setup database schema
setup_db:
	psql -U postgres -d chess-app -f database/schema.sql

# Run tests
run: test_db
	./test_db

run_user_test: test_user_repo
	./test_user_repo

run_game_test: test_game_repo
	./test_game_repo

run_session_test: test_session_mgr
	./test_session_mgr

# Run WebSocket server
run_websocket: websocket_server
	./websocket_server

# Run chess server
run_server: chess_server
	./chess_server

.PHONY: all clean run run_user_test run_game_test run_session_test run_websocket run_server setup_db
