#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <chrono>
#include <string>
#include <vector>

// NOTE: This project currently includes the engine as a .cpp "header".
#include "../game/chess_game.cpp"

struct ChessAIMoveResult {
    std::string move;
    int ai_think_ms;
    long long nodes_searched;
    bool timed_out;
};

class ChessAI {
public:
    explicit ChessAI(int depth = 2);

    void set_depth(int depth);
    int get_depth() const;

    // Returns move in coordinate notation, e.g. "e2e4".
    // Expects it to be AI's turn; returns "" if no legal moves.
    ChessAIMoveResult make_move(ChessGame game_state, bool ai_is_white) const;

private:
    static constexpr int SEARCH_TIMEOUT_MS = 2000;
    int depth_;

    int minimax(ChessGame position,
                int depth_left,
                int alpha,
                int beta,
                bool ai_is_white,
                int ply_from_root,
                const std::chrono::steady_clock::time_point& deadline,
                long long& nodes_searched) const;
    int evaluate_for_ai(ChessGame position, bool ai_is_white, int ply_from_root) const;
};

#endif
