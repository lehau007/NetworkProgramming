# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread
LDFLAGS = -lpqxx -lpq

# Targets
all: test_db

# Test database connection
test_db: database_connection.cpp
	$(CXX) $(CXXFLAGS) -o test_db database_connection.cpp $(LDFLAGS)

# Clean build artifacts
clean:
	rm -f test_db *.o

# Run the test
run: test_db
	./test_db

.PHONY: all clean run
