export function initLobby({ state, ui, send, clearSession }) {
    function updateUserProfile(data) {
        if (!data) return;

        state.userData = data;
        const usernameEl = document.getElementById('profile-username');
        const ratingEl = document.getElementById('profile-rating');
        const winsEl = document.getElementById('profile-wins');
        const lossesEl = document.getElementById('profile-losses');
        const drawsEl = document.getElementById('profile-draws');

        if (usernameEl) usernameEl.textContent = data.username || 'Player';
        if (ratingEl) ratingEl.textContent = data.rating ?? '1200';
        if (winsEl) winsEl.textContent = data.wins ?? '0';
        if (lossesEl) lossesEl.textContent = data.losses ?? '0';
        if (drawsEl) drawsEl.textContent = data.draws ?? '0';
    }

    function reloadUserProfile() {
        if (!state.sessionId || !state.userData?.user_id) return;
        // VERIFY_SESSION returns fresh user_data
        send({ type: 'VERIFY_SESSION', session_id: state.sessionId });
    }

    function getAvailablePlayers() {
        if (!state.sessionId) {
            alert('Not logged in');
            return;
        }
        send({
            type: 'GET_AVAILABLE_PLAYERS',
            session_id: state.sessionId,
        });
    }

    function challengePlayer(targetUsername) {
        send({
            type: 'CHALLENGE',
            session_id: state.sessionId,
            target_username: targetUsername,
        });
    }

    function logout() {
        if (state.sessionId) {
            send({
                type: 'LOGOUT',
                session_id: state.sessionId,
            });
        }

        clearSession();
        if (state.ws) {
            state.ws.close();
        }
        ui.showLogin();
    }

    function renderPlayerList(players) {
        const playerListDiv = document.getElementById('player-list');
        if (!playerListDiv) return;

        playerListDiv.innerHTML = '<h3>Available Players</h3>';
        players.forEach((player) => {
            if (player.username !== state.username) {
                const playerDiv = document.createElement('div');

                let statusText = '';
                let statusColor = '';
                if (player.status === 'in_game') {
                    statusText = ' [In Game]';
                    statusColor = 'color: #ff6b6b;';
                } else if (player.status === 'busy') {
                    statusText = ' [Busy]';
                    statusColor = 'color: #ffa500;';
                } else {
                    statusText = ' [Available]';
                    statusColor = 'color: #51cf66;';
                }

                playerDiv.innerHTML = `<span>${player.username} (${player.rating})<span style="${statusColor}">${statusText}</span></span>`;

                const challengeButton = document.createElement('button');
                challengeButton.textContent = 'Challenge';
                challengeButton.onclick = () => challengePlayer(player.username);

                if (player.status !== 'available') {
                    challengeButton.disabled = true;
                    challengeButton.style.cursor = 'not-allowed';
                    challengeButton.style.opacity = '0.5';
                }

                playerDiv.appendChild(challengeButton);
                playerListDiv.appendChild(playerDiv);
            }
        });
    }

    function showLeaderboard() {
        if (!state.sessionId) {
            alert('Not logged in');
            return;
        }
        send({
            type: 'GET_LEADERBOARD',
            session_id: state.sessionId,
            limit: 50
        });
    }

    function closeLeaderboard() {
        const modal = document.getElementById('leaderboard-modal');
        if (modal) modal.style.display = 'none';
    }

    function renderLeaderboard(players) {
        const modal = document.getElementById('leaderboard-modal');
        const listDiv = document.getElementById('leaderboard-list');
        
        if (!listDiv || !modal) return;
        
        listDiv.innerHTML = '';
        
        if (players.length === 0) {
            listDiv.innerHTML += '<p style="text-align: center; color: #aaa;">No players found</p>';
        } else {
            players.forEach((player) => {
                const item = document.createElement('div');
                item.className = 'leaderboard-item';
                item.innerHTML = `
                    <span>
                        <span class="leaderboard-rank">#${player.rank}</span>
                        ${player.username}
                    </span>
                    <span>
                        ⭐ ${player.rating} 
                    </span>
                `;
                listDiv.appendChild(item);
            });
        }
        
        modal.style.display = 'flex';
        modal.style.opacity = '1';
        modal.style.visibility = 'visible';
        console.log('Leaderboard displayed with', players.length, 'players');
    }

    function handleMessage(msg) {
        switch (msg.type) {
            case 'PLAYER_LIST':
                renderPlayerList(msg.players || []);
                return true;

            case 'CHALLENGE_RECEIVED': {
                const from = msg.from_username;
                if (confirm(`You have received a challenge from ${from}. Do you accept?`)) {
                    send({ type: 'ACCEPT_CHALLENGE', session_id: state.sessionId, challenge_id: msg.challenge_id });
                } else {
                    send({ type: 'DECLINE_CHALLENGE', session_id: state.sessionId, challenge_id: msg.challenge_id });
                }
                return true;
            }

            case 'LEADERBOARD':
                renderLeaderboard(msg.players || []);
                return true;

            default:
                return false;
        }
    }

    // Keep existing inline HTML onclicks working
    window.getAvailablePlayers = getAvailablePlayers;
    window.challengePlayer = challengePlayer;
    window.showLeaderboard = showLeaderboard;
    window.closeLeaderboard = closeLeaderboard;
    window.logout = logout;

    return {
        handleMessage,
        getAvailablePlayers,
        updateUserProfile,
        reloadUserProfile,
    };
}
