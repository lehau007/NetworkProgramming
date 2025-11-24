#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <string>

namespace MessageTypes {
    // Connection & Session
    const std::string VERIFY_SESSION = "VERIFY_SESSION";
    const std::string SESSION_VALID = "SESSION_VALID";
    const std::string SESSION_INVALID = "SESSION_INVALID";
    
    // Authentication
    const std::string LOGIN = "LOGIN";
    const std::string LOGIN_RESPONSE = "LOGIN_RESPONSE";
    const std::string REGISTER = "REGISTER";
    const std::string REGISTER_RESPONSE = "REGISTER_RESPONSE";
    const std::string LOGOUT = "LOGOUT";
    
    // Lobby
    const std::string GET_AVAILABLE_PLAYERS = "GET_AVAILABLE_PLAYERS";
    const std::string PLAYER_LIST = "PLAYER_LIST";
    const std::string PLAYER_STATUS_UPDATE = "PLAYER_STATUS_UPDATE";  // UNSOLICITED
    
    // Matchmaking
    const std::string CHALLENGE = "CHALLENGE";
    const std::string CHALLENGE_SENT = "CHALLENGE_SENT";
    const std::string CHALLENGE_RECEIVED = "CHALLENGE_RECEIVED";  // UNSOLICITED
    const std::string ACCEPT_CHALLENGE = "ACCEPT_CHALLENGE";
    const std::string DECLINE_CHALLENGE = "DECLINE_CHALLENGE";
    const std::string CANCEL_CHALLENGE = "CANCEL_CHALLENGE";
    const std::string CHALLENGE_CANCELLED = "CHALLENGE_CANCELLED";  // UNSOLICITED
    const std::string MATCH_STARTED = "MATCH_STARTED";  // UNSOLICITED
    
    // Gameplay
    const std::string MOVE = "MOVE";
    const std::string MOVE_ACCEPTED = "MOVE_ACCEPTED";
    const std::string MOVE_REJECTED = "MOVE_REJECTED";
    const std::string OPPONENT_MOVE = "OPPONENT_MOVE";  // UNSOLICITED
    const std::string RESIGN = "RESIGN";
    const std::string DRAW_OFFER = "DRAW_OFFER";
    const std::string DRAW_OFFER_RECEIVED = "DRAW_OFFER_RECEIVED";  // UNSOLICITED
    const std::string DRAW_RESPONSE = "DRAW_RESPONSE";
    const std::string REQUEST_REMATCH = "REQUEST_REMATCH";
    const std::string REMATCH_REQUEST_RECEIVED = "REMATCH_REQUEST_RECEIVED";  // UNSOLICITED
    const std::string GAME_ENDED = "GAME_ENDED";  // UNSOLICITED
    
    // Game State
    const std::string GET_GAME_STATE = "GET_GAME_STATE";
    const std::string GAME_STATE = "GAME_STATE";
    const std::string GET_GAME_HISTORY = "GET_GAME_HISTORY";
    const std::string GAME_HISTORY = "GAME_HISTORY";
    const std::string GET_LEADERBOARD = "GET_LEADERBOARD";
    const std::string LEADERBOARD = "LEADERBOARD";
    
    // System
    const std::string ERROR = "ERROR";  // UNSOLICITED
    const std::string SESSION_EXPIRED = "SESSION_EXPIRED";  // UNSOLICITED
    const std::string SERVER_SHUTDOWN = "SERVER_SHUTDOWN";  // UNSOLICITED
    const std::string RECONNECT_SUCCESS = "RECONNECT_SUCCESS";
    const std::string PING = "PING";
    const std::string PONG = "PONG";
    
    // Chat (optional)
    const std::string CHAT_MESSAGE = "CHAT_MESSAGE";
    const std::string CHAT_MESSAGE_RECEIVED = "CHAT_MESSAGE_RECEIVED";  // UNSOLICITED
}

#endif // MESSAGE_TYPES_H
