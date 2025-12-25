export function initGame({ state, ui, send, addMessage, lobby }) {
    let selectedSquare = null;

    function resign() {
        send({
            type: 'RESIGN',
            session_id: state.sessionId,
            game_id: state.currentGameId,
        });
    }

    function offerDraw() {
        send({
            type: 'DRAW_OFFER',
            session_id: state.sessionId,
            game_id: state.currentGameId,
        });
    }

    function respondToDrawOffer(accepted) {
        send({
            type: 'DRAW_RESPONSE',
            session_id: state.sessionId,
            game_id: state.currentGameId,
            accepted,
        });
    }

    // ============================================
    // Game Log Modal
    // ============================================
    function formatReason(reason) {
        const reasons = {
            checkmate: 'Checkmate',
            resignation: 'Resignation',
            timeout: 'Time Out',
            draw_agreement: 'Draw by Agreement',
            stalemate: 'Stalemate',
            opponent_disconnected: 'Opponent Disconnected',
            insufficient_material: 'Insufficient Material',
        };
        return reasons[reason] || reason || 'Game Over';
    }

    function formatDuration(seconds) {
        const mins = Math.floor(seconds / 60);
        const secs = seconds % 60;
        return `${mins}:${secs.toString().padStart(2, '0')}`;
    }

    function showGameLogModal(gameData) {
        const modal = document.getElementById('game-log-modal');
        const resultSection = document.getElementById('game-result-section');
        const resultTitle = document.getElementById('result-title');
        const resultReason = document.getElementById('result-reason');

        let resultClass = 'draw';
        let resultText = 'Draw!';

        if (gameData.result === 'WHITE_WIN' || gameData.result === 'BLACK_WIN') {
            if (gameData.winner === state.username) {
                resultClass = 'win';
                resultText = 'Victory!';
            } else if (gameData.loser === state.username) {
                resultClass = 'loss';
                resultText = 'Defeat';
            } else {
                const myUsername = state.username;
                const isWhite = myUsername != null && gameData.white_player === myUsername;
                const isBlack = myUsername != null && gameData.black_player === myUsername;

                // isWhite === isBlack is true in two cases:
                // - both false: you're neither player (spectating / username mismatch / missing username)
                // - both true: white_player === black_player === you (should never happen)
                if (myUsername == null) {
                    console.warn('[GameLog] state.username is missing; cannot determine win/loss reliably', {
                        username: myUsername,
                        white_player: gameData.white_player,
                        black_player: gameData.black_player,
                        winner: gameData.winner,
                        loser: gameData.loser,
                    });
                } else if (isWhite && isBlack) {
                    console.error('[GameLog] Invariant violated: same user is both white and black', {
                        username: myUsername,
                        white_player: gameData.white_player,
                        black_player: gameData.black_player,
                    });
                } else if (!isWhite && !isBlack) {
                    console.warn('[GameLog] User is neither white nor black for this game', {
                        username: myUsername,
                        white_player: gameData.white_player,
                        black_player: gameData.black_player,
                        winner: gameData.winner,
                        loser: gameData.loser,
                    });
                }

                if ((gameData.result === 'WHITE_WIN' && isWhite) || (gameData.result === 'BLACK_WIN' && isBlack)) {
                    resultClass = 'win';
                    resultText = 'Victory!';
                } else if ((gameData.result === 'WHITE_WIN' && isBlack) || (gameData.result === 'BLACK_WIN' && isWhite)) {
                    resultClass = 'loss';
                    resultText = 'Defeat';
                } else {
                    resultText = 'Game Over';
                }
            }
        } else {
            resultText = 'Draw';
        }

        if (resultSection) resultSection.className = 'game-result ' + resultClass;
        if (resultTitle) resultTitle.textContent = resultText;
        if (resultReason) resultReason.textContent = formatReason(gameData.reason);

        const whiteEl = document.getElementById('log-white-player');
        const blackEl = document.getElementById('log-black-player');
        const moveCountEl = document.getElementById('log-move-count');
        const durationEl = document.getElementById('log-duration');

        if (whiteEl) whiteEl.textContent = gameData.white_player || '-';
        if (blackEl) blackEl.textContent = gameData.black_player || '-';
        if (moveCountEl) moveCountEl.textContent = gameData.move_count || 0;
        if (durationEl) durationEl.textContent = formatDuration(gameData.duration_seconds || 0);

        const movesGrid = document.getElementById('moves-grid');
        if (movesGrid) {
            movesGrid.innerHTML = '';
            if (gameData.move_history && gameData.move_history.length > 0) {
                for (let i = 0; i < gameData.move_history.length; i += 2) {
                    const moveNumber = Math.floor(i / 2) + 1;
                    const whiteMove = gameData.move_history[i] || '';
                    const blackMove = gameData.move_history[i + 1] || '';

                    const numDiv = document.createElement('div');
                    numDiv.className = 'move-number';
                    numDiv.textContent = moveNumber + '.';

                    const whiteDiv = document.createElement('div');
                    whiteDiv.className = 'move-white';
                    whiteDiv.textContent = whiteMove;

                    const blackDiv = document.createElement('div');
                    blackDiv.className = 'move-black';
                    blackDiv.textContent = blackMove;

                    movesGrid.appendChild(numDiv);
                    movesGrid.appendChild(whiteDiv);
                    movesGrid.appendChild(blackDiv);
                }
            } else {
                movesGrid.innerHTML = '<div style="grid-column: 1/-1; color: #888;">No moves recorded</div>';
            }
        }

        modal?.classList.add('show');
    }

    function closeGameLogModal() {
        const modal = document.getElementById('game-log-modal');
        modal?.classList.remove('show');
        ui.showLobby();
        lobby?.getAvailablePlayers?.();
    }

    // ============================================
    // Chessboard
    // ============================================
    function createChessboard() {
        const chessboard = document.getElementById('chessboard');
        if (!chessboard) return;

        chessboard.innerHTML = '';
        const isFlipped = state.myColor === 'black';

        for (let displayPos = 0; displayPos < 64; displayPos++) {
            const square = document.createElement('div');
            const logicalIndex = isFlipped ? 63 - displayPos : displayPos;

            const row = Math.floor(logicalIndex / 8);
            const col = logicalIndex % 8;

            square.classList.add('square');
            if ((row + col) % 2 === 0) {
                square.classList.add('light');
            } else {
                square.classList.add('dark');
            }

            square.dataset.index = logicalIndex;
            square.onclick = () => onSquareClick(logicalIndex);
            chessboard.appendChild(square);
        }
    }

    function renderPieces(boardState) {
        const squares = document.querySelectorAll('.square');
        squares.forEach((s) => (s.innerHTML = ''));

        const pieceMap = {
            p: '♟',
            r: '♜',
            n: '♞',
            b: '♝',
            q: '♛',
            k: '♚',
            P: '♟',
            R: '♜',
            N: '♞',
            B: '♝',
            Q: '♛',
            K: '♚',
        };

        const rows = boardState.split(' ')[0].split('/');
        let logicalIndex = 0;

        rows.forEach((row) => {
            for (const char of row) {
                if (isNaN(char)) {
                    const square = document.querySelector(`.square[data-index='${logicalIndex}']`);
                    if (square) {
                        const piece = document.createElement('span');
                        piece.classList.add('piece');

                        if (char === char.toUpperCase()) {
                            piece.classList.add('white-piece');
                        } else {
                            piece.classList.add('black-piece');
                        }

                        piece.textContent = pieceMap[char];
                        square.appendChild(piece);
                    }
                    logicalIndex++;
                } else {
                    logicalIndex += parseInt(char);
                }
            }
        });
    }

    function toAlgebraic(index) {
        const row = 8 - Math.floor(index / 8);
        const col = String.fromCharCode('a'.charCodeAt(0) + (index % 8));
        return `${col}${row}`;
    }

    function onSquareClick(index) {
        if (selectedSquare === null) {
            const square = document.querySelector(`.square[data-index='${index}']`);
            if (square && square.hasChildNodes()) {
                selectedSquare = index;
                square.classList.add('selected');
            }
            return;
        }

        const from = selectedSquare;
        const to = index;

        send({
            type: 'MOVE',
            session_id: state.sessionId,
            game_id: state.currentGameId,
            move: `${toAlgebraic(from)}${toAlgebraic(to)}`,
        });

        const fromSquare = document.querySelector(`.square[data-index='${from}']`);
        if (fromSquare) fromSquare.classList.remove('selected');
        selectedSquare = null;
    }

    function updateGameState(gameState) {
        const gameStateDiv = document.getElementById('game-state');
        if (!gameStateDiv) return;

        gameStateDiv.innerHTML = `
            <p id="turn-display"><strong>Turn:</strong> <span id="turn-text">Loading...</span></p>
            <p><strong>White:</strong> ${gameState.white_player}</p>
            <p><strong>Black:</strong> ${gameState.black_player}</p>
        `;

        if (gameState.board_state) {
            renderPieces(gameState.board_state);
        }

        if (gameState.current_turn) {
            updateTurnDisplay(gameState.current_turn);
        }
    }

    function updateTurnDisplay(currentTurn) {
        const turnTextElement = document.getElementById('turn-text');
        if (!turnTextElement) return;

        if (currentTurn === state.myColor) {
            turnTextElement.innerHTML = `<span style="color: #4CAF50; font-weight: bold;">YOUR TURN (${state.myColor})</span>`;
        } else {
            turnTextElement.innerHTML = `<span style="color: #ff9800;">Opponent's turn (${currentTurn})</span>`;
        }
    }

    function clearCheckHighlight() {
        document.querySelectorAll('.in-check').forEach((square) => {
            square.classList.remove('in-check');
        });
    }

    function highlightKingInCheck(colorInCheck) {
        clearCheckHighlight();

        const kingSymbol = '♚';
        const kingClass = colorInCheck === 'white' ? 'white-piece' : 'black-piece';

        const squares = document.querySelectorAll('.square');
        squares.forEach((square) => {
            const piece = square.querySelector('.piece');
            if (piece && piece.textContent === kingSymbol && piece.classList.contains(kingClass)) {
                square.classList.add('in-check');
            }
        });
    }

    function handleMessage(msg) {
        switch (msg.type) {
            case 'DRAW_OFFER_RECEIVED':
                if (confirm("Your opponent has offered a draw. Do you accept?")) {
                    respondToDrawOffer(true);
                } else {
                    respondToDrawOffer(false);
                }
                return true;

            case 'MOVE_ACCEPTED':
                if (msg.board_state) {
                    renderPieces(msg.board_state);
                }
                if (msg.current_turn) {
                    updateTurnDisplay(msg.current_turn);
                }
                if (msg.is_check && msg.current_turn) {
                    highlightKingInCheck(msg.current_turn);
                    addMessage("⚠️ CHECK! Opponent's king is under attack!", 'system');
                } else {
                    clearCheckHighlight();
                }
                return true;

            case 'MOVE_REJECTED':
                alert(`Move rejected: ${msg.reason}`);
                if (selectedSquare !== null) {
                    const square = document.querySelector(`.square[data-index='${selectedSquare}']`);
                    if (square) square.classList.remove('selected');
                    selectedSquare = null;
                }
                return true;

            case 'MATCH_STARTED':
                state.currentGameId = msg.game_id;
                state.myColor = msg.your_color;
                createChessboard();
                ui.showGame();
                send({ type: 'GET_GAME_STATE', session_id: state.sessionId, game_id: msg.game_id });
                return true;

            case 'GAME_STATE':
                state.currentGameId = msg.game_id;
                updateGameState(msg);
                if (msg.current_turn) {
                    updateTurnDisplay(msg.current_turn);
                }
                if (msg.is_check && msg.current_turn && msg.current_turn === state.myColor) {
                    highlightKingInCheck(state.myColor);
                    addMessage('⚠️ You are in CHECK!', 'system');
                } else {
                    clearCheckHighlight();
                }
                ui.showGame();
                return true;

            case 'OPPONENT_MOVE':
                updateGameState(msg);
                if (msg.current_turn) {
                    updateTurnDisplay(msg.current_turn);
                }
                if (msg.is_check && msg.current_turn && msg.current_turn === state.myColor) {
                    highlightKingInCheck(state.myColor);
                    addMessage('⚠️ You are in CHECK! You must defend your king!', 'system');
                } else {
                    clearCheckHighlight();
                }
                return true;

            case 'GAME_ENDED':
                showGameLogModal(msg);
                setTimeout(() => {
                    lobby?.reloadUserProfile?.();
                }, 500);
                return true;

            default:
                return false;
        }
    }

    // Keep existing inline HTML onclicks working
    window.resign = resign;
    window.offerDraw = offerDraw;
    window.respondToDrawOffer = respondToDrawOffer;
    window.closeGameLogModal = closeGameLogModal;

    return {
        handleMessage,
        createChessboard,
        renderPieces,
        updateGameState,
        updateTurnDisplay,
        clearCheckHighlight,
        highlightKingInCheck,
    };
}
