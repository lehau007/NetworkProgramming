#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <string>
#include <vector>

// NOTE: This project currently includes the engine as a .cpp "header".
#include "../game/chess_game.cpp"

class ChessAI {
public:
    explicit ChessAI(int depth = 2);

    void set_depth(int depth);
    int get_depth() const;

    // Returns move in coordinate notation, e.g. "e2e4".
    // Expects it to be AI's turn; returns "" if no legal moves.
    std::string make_move(ChessGame game_state, bool ai_is_white) const;

private:
    int depth_;

    int minimax(ChessGame position, int depth_left, int alpha, int beta, bool ai_is_white, int ply_from_root) const;
    int evaluate_for_ai(ChessGame position, bool ai_is_white, int ply_from_root) const;
};

#endif
