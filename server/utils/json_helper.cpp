#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;
class JsonHelper {
public: 
    static void message_request(string input) {}
    
    static void message_response(string input) {}
    
    static void chess_move_request() {}
    static void chess_move_response() {}
    static void login_request() {}
    static void login_response() {}
    static void logout_request() {}
    static void logout_response() {}
    static void register_request() {}
    static void register_response() {}
};