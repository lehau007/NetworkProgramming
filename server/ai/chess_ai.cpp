#include "chess_ai.h"

#include <algorithm>
#include <limits>

ChessAI::ChessAI(int depth) : depth_(depth) {
    set_depth(depth);
}

void ChessAI::set_depth(int depth) {
    // Depth 1+ only; frontend chooses 2 or 3.
    if (depth < 1) depth = 1;
    if (depth > 6) depth = 6; // safety cap
    depth_ = depth;
}

int ChessAI::get_depth() const {
    return depth_;
}

std::string ChessAI::make_move(ChessGame game_state, bool ai_is_white) const {
    if (game_state.isEnded()) return "";
    if (game_state.isWhiteToMove() != ai_is_white) {
        // Called at the wrong time.
        return "";
    }

    const auto legal_moves = game_state.getLegalMovesForCurrentPlayer();
    if (legal_moves.empty()) return "";

    int best_score = std::numeric_limits<int>::min();
    std::string best_move = legal_moves.front();

    int alpha = std::numeric_limits<int>::min() / 4;
    int beta = std::numeric_limits<int>::max() / 4;

    for (const auto& mv : legal_moves) {
        ChessGame next = game_state;
        if (!next.move(mv)) {
            continue;
        }

        int score = minimax(next, depth_ - 1, alpha, beta, ai_is_white, 1);

        if (score > best_score) {
            best_score = score;
            best_move = mv;
        }
        alpha = std::max(alpha, best_score);
    }

    return best_move;
}

int ChessAI::minimax(ChessGame position, int depth_left, int alpha, int beta, bool ai_is_white, int ply_from_root) const {
    if (position.isEnded() || depth_left <= 0) {
        return evaluate_for_ai(position, ai_is_white, ply_from_root);
    }

    const bool side_to_move_is_ai = (position.isWhiteToMove() == ai_is_white);
    const auto legal_moves = position.getLegalMovesForCurrentPlayer();

    if (legal_moves.empty()) {
        // Should usually coincide with isEnded(), but treat as evaluation fallback.
        return evaluate_for_ai(position, ai_is_white, ply_from_root);
    }

    if (side_to_move_is_ai) {
        int best = std::numeric_limits<int>::min();
        for (const auto& mv : legal_moves) {
            ChessGame next = position;
            if (!next.move(mv)) continue;

            best = std::max(best, minimax(next, depth_left - 1, alpha, beta, ai_is_white, ply_from_root + 1));
            alpha = std::max(alpha, best);
            if (beta <= alpha) break;
        }
        return best;
    }

    int best = std::numeric_limits<int>::max();
    for (const auto& mv : legal_moves) {
        ChessGame next = position;
        if (!next.move(mv)) continue;

        best = std::min(best, minimax(next, depth_left - 1, alpha, beta, ai_is_white, ply_from_root + 1));
        beta = std::min(beta, best);
        if (beta <= alpha) break;
    }
    return best;
}

int ChessAI::evaluate_for_ai(ChessGame position, bool ai_is_white, int ply_from_root) const {
    if (position.isEnded()) {
        const auto res = position.getResult();

        // Large win/loss values; prefer faster mate.
        const int mate_score = 100000;
        if (res == WHITE_WIN) {
            return ai_is_white ? (mate_score - ply_from_root) : (-mate_score + ply_from_root);
        }
        if (res == BLACK_WIN) {
            return ai_is_white ? (-mate_score + ply_from_root) : (mate_score - ply_from_root);
        }
        return 0;
    }

    // Material only.
    const int material_white_minus_black = position.evaluateMaterialScore();
    return ai_is_white ? material_white_minus_black : -material_white_minus_black;
}
