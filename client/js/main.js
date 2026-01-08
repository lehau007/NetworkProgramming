import { initLogin } from './login.js';
import { initLobby } from './lobby.js';
import { initGame } from './game.js';

// Dynamic WebSocket URL based on current host
// const wsUrl = `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.hostname}:8080`;
const wsUrl = `wss://unfatalistically-sporting-keaton.ngrok-free.dev/`;

const state = {
    ws: null,
    sessionId: null,
    username: null,
    currentGameId: null,
    userData: null,
    myColor: null,
    isDuplicateSession: false, // Flag to track if connection was closed due to duplicate session
};

// UI elements will be initialized after DOM loads
const ui = {
    loginSection: null,
    registerSection: null,
    lobbySection: null,
    gameSection: null,
    messagesDiv: null,
    connectionStatus: null,
    connectionStatusLobby: null,

    showLogin() {
        this.loginSection.classList.remove('hidden');
        this.registerSection.classList.add('hidden');
        this.lobbySection.classList.add('hidden');
        this.gameSection.classList.add('hidden');
    },

    showRegister() {
        this.loginSection.classList.add('hidden');
        this.registerSection.classList.remove('hidden');
        this.lobbySection.classList.add('hidden');
        this.gameSection.classList.add('hidden');
    },

    showLobby() {
        this.loginSection.classList.add('hidden');
        this.registerSection.classList.add('hidden');
        this.lobbySection.classList.remove('hidden');
        this.gameSection.classList.add('hidden');
    },

    showGame() {
        this.loginSection.classList.add('hidden');
        this.registerSection.classList.add('hidden');
        this.lobbySection.classList.add('hidden');
        this.gameSection.classList.remove('hidden');
    },
};

function setConnectionStatus(text, color = '#aaa') {
    if (ui.connectionStatus) {
        ui.connectionStatus.textContent = text;
        ui.connectionStatus.style.color = color;
    }
    if (ui.connectionStatusLobby) {
        ui.connectionStatusLobby.textContent = text;
        ui.connectionStatusLobby.style.color = color;
    }
}

function addMessage(text, type = 'system') {
    if (!ui.messagesDiv) return;
    const messageDiv = document.createElement('div');
    messageDiv.textContent = text;
    messageDiv.className = type;
    ui.messagesDiv.appendChild(messageDiv);
    ui.messagesDiv.scrollTop = ui.messagesDiv.scrollHeight;
}

function send(data) {
    if (state.ws && state.ws.readyState === WebSocket.OPEN) {
        const msgString = JSON.stringify(data);
        state.ws.send(msgString);
        addMessage(`Sent: ${msgString}`, 'sent');
    } else {
        addMessage('Not connected to server.', 'system');
    }
}

function clearSession() {
    localStorage.removeItem('session_id');
    localStorage.removeItem('username');
    state.sessionId = null;
    state.username = null;
    state.currentGameId = null;
    state.userData = null;
    state.myColor = null;
}

let loginApi;
let lobbyApi;
let gameApi;

function handleMessage(msg) {
    // Let feature modules consume first.
    if (loginApi?.handleMessage?.(msg)) return;
    if (lobbyApi?.handleMessage?.(msg)) return;
    if (gameApi?.handleMessage?.(msg)) return;

    // Fallback: handle core session/auth flows.
    switch (msg.type) {
        case 'SESSION_VALID':
            state.sessionId = msg.session_id;
            state.username = msg?.user_data?.username ?? null;

            localStorage.setItem('session_id', state.sessionId);
            if (state.username) localStorage.setItem('username', state.username);

            lobbyApi?.updateUserProfile?.(msg.user_data);
            ui.showLobby();
            lobbyApi?.getAvailablePlayers?.();
            break;

        case 'SESSION_INVALID':
            clearSession();
            ui.showLogin();
            break;

        case 'LOGIN_RESPONSE':
            if (msg.status === 'success') {
                state.sessionId = msg.session_id;
                state.username = msg?.user_data?.username ?? null;

                localStorage.setItem('session_id', state.sessionId);
                if (state.username) localStorage.setItem('username', state.username);

                lobbyApi?.updateUserProfile?.(msg.user_data);
                ui.showLobby();
                lobbyApi?.getAvailablePlayers?.();
            } else {
                alert(`Login failed: ${msg.message}`);
            }
            break;

        case 'REGISTER_RESPONSE':
            if (msg.status === 'success') {
                alert('Registration successful! Please login.');
                ui.showLogin();
            } else {
                alert(`Registration failed: ${msg.message}`);
            }
            break;

        default:
            break;
    }
}

function connect() {
    if (state.ws) {
        state.ws.close();
    }

    setConnectionStatus(`Connecting to ${wsUrl}…`, '#aaa');
    console.log("Connecting to " + wsUrl);

    state.ws = new WebSocket(wsUrl);

    state.ws.onopen = () => {
        setConnectionStatus('Connected', '#51cf66');
        addMessage('Connected to server.', 'system');
        const savedSession = localStorage.getItem('session_id');
        const savedUsername = localStorage.getItem('username');

        if (savedSession) {
            state.sessionId = savedSession;
            state.username = savedUsername;
            send({ type: 'VERIFY_SESSION', session_id: state.sessionId });
        } else {
            ui.showLogin();
        }
    };

    state.ws.onmessage = (event) => {
        addMessage(`Received: ${event.data}`, 'received');
        const msg = JSON.parse(event.data);
        handleMessage(msg);
    };

    state.ws.onclose = () => {
        setConnectionStatus('Disconnected', '#ff6b6b');
        addMessage('Disconnected from server.', 'system');
        state.ws = null;
        ui.showLogin();
    };

    state.ws.onerror = (error) => {
        setConnectionStatus('Connection error (check console)', '#ff6b6b');
        addMessage('WebSocket error.', 'system');
        console.error('WebSocket Error: ', error);
    };
}

// Initialize after DOM is ready
window.addEventListener('DOMContentLoaded', () => {
    console.log('DOM loaded, initializing client...');
    
    // Now query DOM elements
    ui.loginSection = document.getElementById('login-section');
    ui.registerSection = document.getElementById('register-section');
    ui.lobbySection = document.getElementById('lobby-section');
    ui.gameSection = document.getElementById('game-section');
    ui.messagesDiv = document.getElementById('messages');
    ui.connectionStatus = document.getElementById('connection-status');
    ui.connectionStatusLobby = document.getElementById('connection-status-lobby');
    
    console.log('UI elements:', {
        login: !!ui.loginSection,
        register: !!ui.registerSection,
        lobby: !!ui.lobbySection,
        game: !!ui.gameSection,
        messages: !!ui.messagesDiv,
        status: !!ui.connectionStatus
    });
    
    // Init modules
    const ctx = { state, ui, send, addMessage, connect, clearSession };
    loginApi = initLogin(ctx);
    lobbyApi = initLobby(ctx);
    ctx.lobby = lobbyApi;
    gameApi = initGame(ctx);
    ctx.game = gameApi;
    
    // Make connect callable from console
    window.connect = connect;
    
    // Ensure the board exists before any match starts
    gameApi?.createChessboard?.();
    
    // Start connection
    console.log('Attempting WebSocket connection to:', wsUrl);
    connect();
});
