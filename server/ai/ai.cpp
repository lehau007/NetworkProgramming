
#include "ai.h"

#include <algorithm>
#include <cctype>
#include <limits>

// NOTE: chess_game is implemented as a header-style .cpp in this codebase.
#include "../game/chess_game.cpp"

namespace {
	constexpr int kMateScore = 1'000'000;
	constexpr int kInf = std::numeric_limits<int>::max() / 4;

	bool is_white_turn(const ChessGame& game) {
		return (game.getTurn() % 2 == 0);
	}

	bool parse_color_is_white(const std::string& color) {
		std::string lowered;
		lowered.reserve(color.size());
		for (char c : color) lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
		return lowered == "white";
	}

	std::string sq(int file, int rank) {
		// file: 0..7 => a..h, rank: 0..7 => 1..8
		std::string s;
		s.push_back(static_cast<char>('a' + file));
		s.push_back(static_cast<char>('1' + rank));
		return s;
	}

	bool is_pawn_promotion_move_string(const std::string& move) {
		if (move.size() != 4) return false;
		// fromRank to toRank
		char toRank = move[3];
		return toRank == '1' || toRank == '8';
	}
}

AI::AI(int depth) : depth_(depth) {
	set_depth(depth);
}

void AI::set_depth(int depth) {
	// Your UI uses 2 or 3; keep it bounded.
	if (depth < 1) depth = 1;
	if (depth > 4) depth = 4;
	depth_ = depth;
}

int AI::get_depth() const {
	return depth_;
}

std::string AI::make_move(const std::vector<std::string>& move_history, const std::string& ai_color) {
	const bool ai_is_white = parse_color_is_white(ai_color);

	ChessGame game;
	for (const auto& m : move_history) {
		// If history is corrupted, bail.
		if (!game.move(m)) {
			return "";
		}
	}

	if (game.isEnded()) {
		return "";
	}

	const bool ai_to_move = (is_white_turn(game) == ai_is_white);
	if (!ai_to_move) {
		return "";
	}

	SearchResult best = alphabeta(game, depth_, -kInf, kInf, ai_is_white, 0);
	return best.move;
}

AI::SearchResult AI::alphabeta(ChessGame& game, int depth, int alpha, int beta, bool ai_is_white, int ply) {
	// Terminal or depth reached
	if (depth == 0 || game.isEnded()) {
		return {evaluate(game, ai_is_white, ply), ""};
	}

	const bool side_to_move_is_white = is_white_turn(game);
	const bool maximizing = (side_to_move_is_white == ai_is_white);

	std::vector<std::string> moves = generate_legal_moves(game);
	if (moves.empty()) {
		// No legal moves: engine should mark ended on move, but be defensive.
		return {evaluate(game, ai_is_white, ply), ""};
	}

	SearchResult best;
	best.score = maximizing ? -kInf : kInf;

	for (const auto& m : moves) {
		ChessGame child = game;
		if (!child.move(m)) {
			continue;
		}

		SearchResult res = alphabeta(child, depth - 1, alpha, beta, ai_is_white, ply + 1);
		res.move = m;

		if (maximizing) {
			if (res.score > best.score) best = res;
			alpha = std::max(alpha, best.score);
			if (beta <= alpha) break;
		} else {
			if (res.score < best.score) best = res;
			beta = std::min(beta, best.score);
			if (beta <= alpha) break;
		}
	}

	if (best.move.empty() && !moves.empty()) {
		// Fallback: return first legal move.
		best.move = moves.front();
		ChessGame child = game;
		if (child.move(best.move)) {
			best.score = evaluate(child, ai_is_white, ply + 1);
		} else {
			best.score = evaluate(game, ai_is_white, ply);
		}
	}

	return best;
}

std::vector<std::string> AI::generate_legal_moves(ChessGame& game) {
	std::vector<std::string> moves;
	moves.reserve(128);

	// Brute-force generation: try all from->to squares and let ChessGame validate.
	for (int fromFile = 0; fromFile < 8; ++fromFile) {
		for (int fromRank = 0; fromRank < 8; ++fromRank) {
			const std::string from = sq(fromFile, fromRank);
			for (int toFile = 0; toFile < 8; ++toFile) {
				for (int toRank = 0; toRank < 8; ++toRank) {
					if (fromFile == toFile && fromRank == toRank) continue;
					const std::string to = sq(toFile, toRank);
					std::string m = from + to;

					if (game.checkMove(m)) {
						moves.push_back(m);
						continue;
					}

					// Promotion handling for current engine: try queen promotion suffix.
					// This is only attempted when moving into rank 1 or 8.
					if (is_pawn_promotion_move_string(m)) {
						std::string promo = m;
						promo.push_back('q');
						if (game.checkMove(promo)) {
							moves.push_back(promo);
						}
					}
				}
			}
		}
	}

	return moves;
}

int AI::evaluate(ChessGame& game, bool ai_is_white, int ply) {
	if (game.isEnded()) {
		GameResult result = game.getResult();
		if (result == DRAW) return 0;
		const bool white_won = (result == WHITE_WIN);
		const bool ai_won = (white_won && ai_is_white) || (!white_won && !ai_is_white);
		// Prefer faster mates, avoid slower losses.
		int mate = kMateScore - (ply * 100);
		return ai_won ? mate : -mate;
	}

	// Material from FEN, measured from White's perspective.
	const int white_pov = material_eval_from_fen(game.getFEN());
	return ai_is_white ? white_pov : -white_pov;
}

int AI::material_eval_from_fen(const std::string& fen) {
	// Parse until first space (piece placement)
	int score = 0;
	for (char c : fen) {
		if (c == ' ') break;
		switch (c) {
			case 'P': score += 100; break;
			case 'N': score += 320; break;
			case 'B': score += 330; break;
			case 'R': score += 500; break;
			case 'Q': score += 900; break;
			case 'K': score += 20000; break;
			case 'p': score -= 100; break;
			case 'n': score -= 320; break;
			case 'b': score -= 330; break;
			case 'r': score -= 500; break;
			case 'q': score -= 900; break;
			case 'k': score -= 20000; break;
			default: break;
		}
	}
	return score;
}
