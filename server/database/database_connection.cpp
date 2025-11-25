/* In this file, we handle database connections */

// Import all libraries
#include <pqxx/pqxx>
#include <iostream>
#include <string>
#include <fstream>
#include <map>

using namespace std;

class DatabaseConnection {
private:
    static map<string, string> loadEnv() {
        map<string, string> env;

        // Use absolute path for .env file but in linux
        ifstream file("/mnt/c/Users/msilaptop/Desktop/NetworkProgramming/Project/server/config/.env");
        string line;
        
        if (!file.is_open()) {
            cerr << "Warning: .env file not found, using default values" << endl;
            return env;
        }
        
        while (getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != string::npos) {
                string key = line.substr(0, pos);
                string value = line.substr(pos + 1);
                env[key] = value;
            }
        }
        file.close();
        return env;
    }
    
    static string getConnectionString() {
        auto env = loadEnv();
        
        string dbname = env.count("DB_NAME") ? env["DB_NAME"] : "chess-app";
        string user = env.count("DB_USER") ? env["DB_USER"] : "postgres";
        string password = env.count("DB_PASSWORD") ? env["DB_PASSWORD"] : "";
        string host = env.count("DB_HOST") ? env["DB_HOST"] : "localhost";
        string port = env.count("DB_PORT") ? env["DB_PORT"] : "5432";
        
        return "dbname=" + dbname + " user=" + user + " password=" + password + 
               " host=" + host + " port=" + port + " connect_timeout=5";
    }

public: 
    static auto execute_query(string query) {
        try {
            pqxx::connection c(getConnectionString());
            pqxx::work txn(c);
            auto result = txn.exec(query);
            return result;
        } catch (const std::exception& e) {
            cerr << e.what() << endl;
            return pqxx::result();
        }
    }
};

// int main() {
//     cout << "Testing database connection..." << endl;
//     cout << "Loading configuration from .env file..." << endl;
    
//     try {
//         cout << "Attempting to connect to database..." << endl;
//         auto env = DatabaseConnection::execute_query("select * from users");

//         cout << "Connection successful! Users found:" << endl;
//         for (auto row : env)
//             std::cout << "  - " << row["username"].c_str() << "\n";
//     }
//     catch (const std::exception& e) {
//         std::cerr << "Error: " << e.what() << "\n";
//         std::cerr << "\nTroubleshooting tips:" << endl;
//         std::cerr << "1. Check if PostgreSQL is running on Windows" << endl;
//         std::cerr << "2. Verify pg_hba.conf allows connections from WSL" << endl;
//         std::cerr << "3. Check Windows Firewall allows port 5432" << endl;
//         std::cerr << "4. Verify .env file has correct credentials" << endl;
//         return 1;
//     }

//     return 0;
// }