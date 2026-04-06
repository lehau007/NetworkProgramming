#include "chess_ai.h"

#include <algorithm>
#include <chrono>
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

ChessAIMoveResult ChessAI::make_move(ChessGame game_state, bool ai_is_white) const {
    ChessAIMoveResult result{"", 0, 0, false};

    if (game_state.isEnded()) return result;
    if (game_state.isWhiteToMove() != ai_is_white) {
        // Called at the wrong time.
        return result;
    }

    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + std::chrono::milliseconds(SEARCH_TIMEOUT_MS);

    const auto legal_moves = game_state.getLegalMovesForCurrentPlayer();
    if (legal_moves.empty()) {
        return result;
    }

    int best_score = std::numeric_limits<int>::min();
    std::string best_move = legal_moves.front();
    bool has_candidate_move = false;
    long long nodes_searched = 0;

    int alpha = std::numeric_limits<int>::min() / 4;
    int beta = std::numeric_limits<int>::max() / 4;

    for (const auto& mv : legal_moves) {
        if (std::chrono::steady_clock::now() >= deadline) {
            result.timed_out = true;
            break;
        }

        ChessGame next = game_state;
        if (!next.move(mv)) {
            continue;
        }

        int score = minimax(next, depth_ - 1, alpha, beta, ai_is_white, 1, deadline, nodes_searched);

        if (!has_candidate_move || score > best_score) {
            best_score = score;
            best_move = mv;
            has_candidate_move = true;
        }
        alpha = std::max(alpha, best_score);
    }

    const auto end = std::chrono::steady_clock::now();
    result.ai_think_ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    result.nodes_searched = nodes_searched;

    if (has_candidate_move) {
        result.move = best_move;
    }

    return result;
}

int ChessAI::minimax(ChessGame position,
                     int depth_left,
                     int alpha,
                     int beta,
                     bool ai_is_white,
                     int ply_from_root,
                     const std::chrono::steady_clock::time_point& deadline,
                     long long& nodes_searched) const {
    nodes_searched++;
    if ((nodes_searched % 64 == 0) && std::chrono::steady_clock::now() >= deadline) {
        return evaluate_for_ai(position, ai_is_white, ply_from_root);
    }

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

            best = std::max(best, minimax(next,
                                          depth_left - 1,
                                          alpha,
                                          beta,
                                          ai_is_white,
                                          ply_from_root + 1,
                                          deadline,
                                          nodes_searched));
            alpha = std::max(alpha, best);
            if (beta <= alpha) break;
        }
        return best;
    }

    int best = std::numeric_limits<int>::max();
    for (const auto& mv : legal_moves) {
        ChessGame next = position;
        if (!next.move(mv)) continue;

        best = std::min(best, minimax(next,
                                      depth_left - 1,
                                      alpha,
                                      beta,
                                      ai_is_white,
                                      ply_from_root + 1,
                                      deadline,
                                      nodes_searched));
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
