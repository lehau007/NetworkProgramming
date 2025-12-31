export function initLogin({ state, ui, send, connect, clearSession }) {
    function showLogin() {
        ui.showLogin();
    }

    function showRegister() {
        ui.showRegister();
    }

    function login() {
        const user = document.getElementById('username')?.value ?? '';
        const pass = document.getElementById('password')?.value ?? '';
        if (!user || !pass) {
            alert('Please enter username and password');
            return;
        }
        send({
            type: 'LOGIN',
            username: user,
            password: pass,
        });
    }

    function register() {
        const user = document.getElementById('reg-username')?.value ?? '';
        const pass = document.getElementById('reg-password')?.value ?? '';
        const email = document.getElementById('reg-email')?.value ?? '';
        if (!user || !pass) {
            alert('Please enter username and password');
            return;
        }
        send({
            type: 'REGISTER',
            username: user,
            password: pass,
            email,
        });
    }

    // ============================================
    // Duplicate Session Handling
    // ============================================
    function showDuplicateSessionModal(message) {
        const modal = document.getElementById('duplicate-session-modal');
        const msgEl = document.getElementById('duplicate-session-message');
        if (msgEl) msgEl.textContent = message || 'Another connection is using this session.';
        modal?.classList.add('show');
        
        // Automatically close the WebSocket connection
        // DO NOT clear localStorage - this would affect other tabs using the same session
        if (state.ws) {
            state.ws.close();
            state.ws = null;
        }
        
        // Reset local state only (not localStorage)
        state.sessionId = null;
        state.username = null;
        state.currentGameId = null;
        state.userData = null;
        state.myColor = null;
        state.isDuplicateSession = true;
    }

    function handleMessage(msg) {
        if (msg.type === 'DUPLICATE_SESSION') {
            // Mark as duplicate session and show modal (connection will be closed automatically)
            showDuplicateSessionModal(msg.message);
            return true;
        }
        return false;
    }

    // Keep existing inline HTML onclicks working
    window.showLogin = showLogin;
    window.showRegister = showRegister;
    window.login = login;
    window.register = register;

    return {
        handleMessage,
        showDuplicateSessionModal,
    };
}
