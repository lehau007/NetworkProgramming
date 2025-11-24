# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread -Inetwork -Idatabase
LDFLAGS = -lpqxx -lpq -lssl -lcrypto

# Object files
SOCKET_OBJS = network/socket_handler.o network/websocket_handler.o

# Targets
all: test_db websocket_server test_user_repo test_game_repo

# Test database connection
test_db: database/database_connection.cpp
	$(CXX) $(CXXFLAGS) -o test_db database/database_connection.cpp $(LDFLAGS)

# Test user repository
test_user_repo: database/test_user_repository.cpp database/user_repository.h
	$(CXX) $(CXXFLAGS) -o test_user_repo database/test_user_repository.cpp $(LDFLAGS)

# Test game repository
test_game_repo: database/test_game_repository.cpp database/game_repository.h
	$(CXX) $(CXXFLAGS) -o test_game_repo database/test_game_repository.cpp $(LDFLAGS)

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

# Clean build artifacts
clean:
	rm -f test_db test_user_repo test_game_repo websocket_server *.o network/*.o

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

# Run WebSocket server
run_websocket: websocket_server
	./websocket_server

.PHONY: all clean run run_user_test run_game_test run_websocket setup_db
