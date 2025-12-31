
#ifndef CHESS_AI_H
#define CHESS_AI_H

#include <string>
#include <vector>

class ChessGame;

class AI {
public:
	explicit AI(int depth = 2);

	void set_depth(int depth);
	int get_depth() const;

	// Returns a move in long algebraic format used by the server, e.g. "e2e4" or "e7e8q".
	// ai_color must be "white" or "black".
	// If it's not AI's turn, or no legal moves exist, returns empty string.
	std::string make_move(const std::vector<std::string>& move_history, const std::string& ai_color);

private:
	int depth_;

	struct SearchResult {
		int score;
		std::string move;
	};

	SearchResult alphabeta(ChessGame& game, int depth, int alpha, int beta, bool ai_is_white, int ply);
	std::vector<std::string> generate_legal_moves(ChessGame& game);
	int evaluate(ChessGame& game, bool ai_is_white, int ply);
	static int material_eval_from_fen(const std::string& fen);
};

#endif
