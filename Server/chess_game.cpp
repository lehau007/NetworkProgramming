#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <cmath>

using namespace std;

enum PieceType {
    KING,
    QUEEN,
    ROOK,
    BISHOP,
    KNIGHT,
    PAWN,
    NONE
};

enum GameResult {
    ONGOING,
    WHITE_WIN,
    BLACK_WIN,
    DRAW
};

class ChessGame {
private: 
    vector<vector<PieceType>> board;
    vector<vector<bool>> is_white; // true if piece is white, false if black
    vector<string> move_history; // Game log with descriptive moves
    int turn;
    bool is_ended;
    GameResult result;
    
    // Castling rights
    bool white_king_moved;
    bool white_rook_a_moved;
    bool white_rook_h_moved;
    bool black_king_moved;
    bool black_rook_a_moved;
    bool black_rook_h_moved;
    
    // Helper function to parse chess notation (e.g., "e2" -> row=6, col=4)
    bool parsePosition(const string& pos, int& row, int& col) {
        if (pos.length() != 2) return false;
        
        col = tolower(pos[0]) - 'a';
        row = 8 - (pos[1] - '0');
        
        return (col >= 0 && col < 8 && row >= 0 && row < 8);
    }
    
    // Convert piece type to string
    string pieceToString(PieceType piece) {
        switch (piece) {
            case KING: return "King";
            case QUEEN: return "Queen";
            case ROOK: return "Rook";
            case BISHOP: return "Bishop";
            case KNIGHT: return "Knight";
            case PAWN: return "Pawn";
            default: return "Unknown";
        }
    }
    
    // Convert position to chess notation
    string positionToNotation(int row, int col) {
        string notation;
        notation += char('a' + col);
        notation += char('8' - row);
        return notation;
    }
    
    // Generate descriptive log entry
    string generateLogEntry(int fromRow, int fromCol, int toRow, int toCol, bool isCapture, bool isCastling) {
        string log;
        string playerColor = (turn % 2 == 0) ? "White" : "Black";
        int moveNumber = (turn / 2) + 1;
        
        log += to_string(moveNumber) + ". " + playerColor + " - ";
        
        if (isCastling) {
            bool isKingside = (toCol > fromCol);
            log += isKingside ? "Castles kingside (O-O)" : "Castles queenside (O-O-O)";
        } else {
            PieceType piece = board[fromRow][fromCol];
            log += pieceToString(piece);
            log += " from " + positionToNotation(fromRow, fromCol);
            log += " to " + positionToNotation(toRow, toCol);
            
            if (isCapture) {
                log += " (captures " + pieceToString(board[toRow][toCol]) + ")";
            }
        }
        
        return log;
    }
    
    // Check if a position is valid
    bool isValidPosition(int row, int col) {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }
    
    // Check if king is under attack (for castling validation)
    bool isSquareUnderAttack(int row, int col, bool byWhite) {
        // Check all opponent pieces to see if they can attack this square
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (board[i][j] != NONE && is_white[i][j] == byWhite) {
                    // For pawns, check diagonal attacks only
                    if (board[i][j] == PAWN) {
                        int direction = byWhite ? -1 : 1;
                        if (row == i + direction && abs(col - j) == 1) {
                            return true;
                        }
                    } else if (isValidPieceMove(board[i][j], i, j, row, col, byWhite)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
    
    // Check if castling is valid
    bool canCastle(int fromRow, int fromCol, int toRow, int toCol, bool isWhite) {
        // Must be king moving two squares horizontally on back rank
        if (board[fromRow][fromCol] != KING) return false;
        if (fromRow != toRow) return false;
        if (abs(toCol - fromCol) != 2) return false;
        
        int expectedRow = isWhite ? 7 : 0;
        if (fromRow != expectedRow) return false;
        
        // Check if king has moved
        if (isWhite && white_king_moved) return false;
        if (!isWhite && black_king_moved) return false;
        
        // Determine if kingside or queenside castling
        bool isKingside = (toCol > fromCol);
        int rookCol = isKingside ? 7 : 0;
        
        // Check if rook has moved
        if (isWhite) {
            if (isKingside && white_rook_h_moved) return false;
            if (!isKingside && white_rook_a_moved) return false;
        } else {
            if (isKingside && black_rook_h_moved) return false;
            if (!isKingside && black_rook_a_moved) return false;
        }
        
        // Check if rook is still in place
        if (board[expectedRow][rookCol] != ROOK) return false;
        if (is_white[expectedRow][rookCol] != isWhite) return false;
        
        // Check if squares between king and rook are empty
        int startCol = min(fromCol, rookCol);
        int endCol = max(fromCol, rookCol);
        for (int col = startCol + 1; col < endCol; col++) {
            if (board[expectedRow][col] != NONE) return false;
        }
        
        // Check if king is in check
        if (isSquareUnderAttack(fromRow, fromCol, !isWhite)) return false;
        
        // Check if king passes through or lands on a square under attack
        int direction = isKingside ? 1 : -1;
        for (int col = fromCol; col != toCol + direction; col += direction) {
            if (isSquareUnderAttack(expectedRow, col, !isWhite)) return false;
        }
        
        return true;
    }
    
    // Check if path is clear for sliding pieces (rook, bishop, queen)
    bool isPathClear(int fromRow, int fromCol, int toRow, int toCol) {
        int rowDir = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
        int colDir = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;
        
        int currRow = fromRow + rowDir;
        int currCol = fromCol + colDir;
        
        while (currRow != toRow || currCol != toCol) {
            if (board[currRow][currCol] != NONE) return false;
            currRow += rowDir;
            currCol += colDir;
        }
        
        return true;
    }
    
    // Validate move for specific piece type
    bool isValidPieceMove(PieceType piece, int fromRow, int fromCol, int toRow, int toCol, bool isWhite) {
        int rowDiff = toRow - fromRow;
        int colDiff = toCol - fromCol;
        
        switch (piece) {
            case PAWN: {
                int direction = isWhite ? -1 : 1; // white moves up, black moves down
                int startRow = isWhite ? 6 : 1;
                
                // Forward move
                if (colDiff == 0) {
                    if (rowDiff == direction && board[toRow][toCol] == NONE) return true;
                    if (fromRow == startRow && rowDiff == 2 * direction && 
                        board[toRow][toCol] == NONE && 
                        board[fromRow + direction][fromCol] == NONE) return true;
                }
                // Capture
                else if (abs(colDiff) == 1 && rowDiff == direction) {
                    if (board[toRow][toCol] != NONE && is_white[toRow][toCol] != isWhite) 
                        return true;
                }
                return false;
            }
            
            case KNIGHT:
                return (abs(rowDiff) == 2 && abs(colDiff) == 1) || 
                       (abs(rowDiff) == 1 && abs(colDiff) == 2);
            
            case BISHOP:
                return abs(rowDiff) == abs(colDiff) && isPathClear(fromRow, fromCol, toRow, toCol);
            
            case ROOK:
                return (rowDiff == 0 || colDiff == 0) && isPathClear(fromRow, fromCol, toRow, toCol);
            
            case QUEEN:
                return ((rowDiff == 0 || colDiff == 0) || (abs(rowDiff) == abs(colDiff))) && 
                       isPathClear(fromRow, fromCol, toRow, toCol);
            
            case KING:
                return abs(rowDiff) <= 1 && abs(colDiff) <= 1;
            
            default:
                return false;
        }
    }
    
public: 
    ChessGame() {
        board.resize(8, vector<PieceType>(8, NONE));
        is_white.resize(8, vector<bool>(8, true));
        move_history.clear();
        turn = 0;
        is_ended = false;
        result = ONGOING;
        
        // Initialize castling rights
        white_king_moved = false;
        white_rook_a_moved = false;
        white_rook_h_moved = false;
        black_king_moved = false;
        black_rook_a_moved = false;
        black_rook_h_moved = false;
        
        initializeBoard();
    }
    
    void initializeBoard() {
        // Black pieces (top)
        board[0] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
        board[1] = vector<PieceType>(8, PAWN);
        for (int i = 0; i < 8; i++) {
            is_white[0][i] = false;
            is_white[1][i] = false;
        }
        
        // White pieces (bottom)
        board[6] = vector<PieceType>(8, PAWN);
        board[7] = {ROOK, KNIGHT, BISHOP, QUEEN, KING, BISHOP, KNIGHT, ROOK};
        for (int i = 0; i < 8; i++) {
            is_white[6][i] = true;
            is_white[7][i] = true;
        }
    }

    bool checkMove(string move) {
        if (is_ended) return false;
        
        // Expected format: "e2e4" (from position to position)
        if (move.length() != 4) return false;
        
        string from = move.substr(0, 2);
        string to = move.substr(2, 2);
        
        int fromRow, fromCol, toRow, toCol;
        if (!parsePosition(from, fromRow, fromCol) || !parsePosition(to, toRow, toCol)) 
            return false;
        
        // Check if there's a piece at source
        if (board[fromRow][fromCol] == NONE) return false;
        
        // Check if it's the correct player's turn
        bool currentPlayerIsWhite = (turn % 2 == 0);
        if (is_white[fromRow][fromCol] != currentPlayerIsWhite) return false;
        
        // Check if trying to capture own piece
        if (board[toRow][toCol] != NONE && is_white[toRow][toCol] == currentPlayerIsWhite) 
            return false;
        
        // Check for castling
        PieceType piece = board[fromRow][fromCol];
        if (piece == KING && abs(toCol - fromCol) == 2) {
            return canCastle(fromRow, fromCol, toRow, toCol, currentPlayerIsWhite);
        }
        
        // Validate piece-specific move
        return isValidPieceMove(piece, fromRow, fromCol, toRow, toCol, currentPlayerIsWhite);
    }

    bool move(string move) {
        if (!checkMove(move)) return false;
        
        string from = move.substr(0, 2);
        string to = move.substr(2, 2);
        
        int fromRow, fromCol, toRow, toCol;
        parsePosition(from, fromRow, fromCol);
        parsePosition(to, toRow, toCol);
        
        PieceType piece = board[fromRow][fromCol];
        bool pieceIsWhite = is_white[fromRow][fromCol];
        bool isCapture = (board[toRow][toCol] != NONE);
        bool isCastling = false;
        
        // Handle castling
        if (piece == KING && abs(toCol - fromCol) == 2) {
            isCastling = true;
            
            // Generate log entry before moving
            string logEntry = generateLogEntry(fromRow, fromCol, toRow, toCol, false, true);
            move_history.push_back(logEntry);
            
            // Move king
            board[toRow][toCol] = KING;
            is_white[toRow][toCol] = pieceIsWhite;
            board[fromRow][fromCol] = NONE;
            
            // Move rook
            bool isKingside = (toCol > fromCol);
            int rookFromCol = isKingside ? 7 : 0;
            int rookToCol = isKingside ? toCol - 1 : toCol + 1;
            
            board[toRow][rookToCol] = ROOK;
            is_white[toRow][rookToCol] = pieceIsWhite;
            board[toRow][rookFromCol] = NONE;
            
            // Update castling rights
            if (pieceIsWhite) {
                white_king_moved = true;
            } else {
                black_king_moved = true;
            }
            
            turn++;
            checkGameEnd();
            return true;
        }
        
        // Generate log entry before moving (for regular moves)
        string logEntry = generateLogEntry(fromRow, fromCol, toRow, toCol, isCapture, false);
        
        // Check if capturing a king (game ending condition)
        if (board[toRow][toCol] == KING) {
            is_ended = true;
            result = (turn % 2 == 0) ? WHITE_WIN : BLACK_WIN;
            logEntry += " - CHECKMATE!";
        }
        
        // Update castling rights before moving
        if (piece == KING) {
            if (pieceIsWhite) white_king_moved = true;
            else black_king_moved = true;
        } else if (piece == ROOK) {
            if (pieceIsWhite) {
                if (fromRow == 7 && fromCol == 0) white_rook_a_moved = true;
                if (fromRow == 7 && fromCol == 7) white_rook_h_moved = true;
            } else {
                if (fromRow == 0 && fromCol == 0) black_rook_a_moved = true;
                if (fromRow == 0 && fromCol == 7) black_rook_h_moved = true;
            }
        }
        
        // Execute move
        board[toRow][toCol] = board[fromRow][fromCol];
        is_white[toRow][toCol] = is_white[fromRow][fromCol];
        board[fromRow][fromCol] = NONE;
        
        move_history.push_back(logEntry);
        turn++;
        
        checkGameEnd();
        return true;
    }

    bool checkGameEnd() {
        if (is_ended) return true;
        
        // Check for draw by move limit (simplified: 100 moves without capture)
        if (turn >= 200) {
            is_ended = true;
            result = DRAW;
            return true;
        }
        
        // Count kings
        int whiteKings = 0, blackKings = 0;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (board[i][j] == KING) {
                    if (is_white[i][j]) whiteKings++;
                    else blackKings++;
                }
            }
        }
        
        // Check if a king has been captured
        if (whiteKings == 0) {
            is_ended = true;
            result = BLACK_WIN;
            return true;
        }
        if (blackKings == 0) {
            is_ended = true;
            result = WHITE_WIN;
            return true;
        }
        
        return false;
    }
    
    // Utility methods
    void displayBoard() {
        cout << "  a b c d e f g h\n";
        for (int i = 0; i < 8; i++) {
            cout << (8 - i) << " ";
            for (int j = 0; j < 8; j++) {
                char symbol = '.';
                if (board[i][j] != NONE) {
                    switch (board[i][j]) {
                        case KING: symbol = 'K'; break;
                        case QUEEN: symbol = 'Q'; break;
                        case ROOK: symbol = 'R'; break;
                        case BISHOP: symbol = 'B'; break;
                        case KNIGHT: symbol = 'N'; break;
                        case PAWN: symbol = 'P'; break;
                        default: symbol = '?';
                    }
                    if (!is_white[i][j]) symbol = tolower(symbol);
                }
                cout << symbol << " ";
            }
            cout << (8 - i) << "\n";
        }
        cout << "  a b c d e f g h\n";
    }
    
    void displayGameLog() {
        cout << "\n=== Game Log ===\n";
        for (size_t i = 0; i < move_history.size(); i++) {
            cout << move_history[i] << "\n";
        }
        cout << "================\n";
    }
    
    GameResult getResult() { return result; }
    bool isEnded() { return is_ended; }
    int getTurn() { return turn; }
};

int main() {
    ChessGame game;
    
    cout << "Chess Game Started!\n";
    cout << "Move format: e2e4 (from-square to-square)\n";
    cout << "Type 'log' to view game history\n\n";
    
    game.displayBoard();
    
    string input;
    while (!game.isEnded()) {
        cout << "\nTurn " << (game.getTurn() + 1) << " (" 
             << (game.getTurn() % 2 == 0 ? "White" : "Black") << "): ";
        cin >> input;
        
        if (input == "log") {
            game.displayGameLog();
            continue;
        }
        
        if (game.move(input)) {
            cout << "Move executed successfully!\n";
            game.displayBoard();
        } else {
            cout << "Invalid move! Try again.\n";
        }
    }
    
    cout << "\nGame Over! Result: ";
    switch (game.getResult()) {
        case WHITE_WIN: cout << "White wins!\n"; break;
        case BLACK_WIN: cout << "Black wins!\n"; break;
        case DRAW: cout << "Draw!\n"; break;
        default: cout << "Unknown\n";
    }
    
    game.displayGameLog();
    
    return 0;
}