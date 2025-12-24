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
    // Duplicate Session Modal
    // ============================================
    function showDuplicateSessionModal(message) {
        const modal = document.getElementById('duplicate-session-modal');
        const msgEl = document.getElementById('duplicate-session-message');
        if (msgEl) msgEl.textContent = message || 'Another connection is using this session.';
        modal?.classList.add('show');
    }

    function closeDuplicateSessionModal() {
        const modal = document.getElementById('duplicate-session-modal');
        modal?.classList.remove('show');

        // Clear stored session since it's being used elsewhere
        clearSession();
        if (state.ws) {
            state.ws.close();
        }
        ui.showLogin();
    }

    function forceNewLogin() {
        closeDuplicateSessionModal();
        setTimeout(() => {
            connect();
        }, 500);
    }

    function handleMessage(msg) {
        if (msg.type === 'DUPLICATE_SESSION') {
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
    window.showDuplicateSessionModal = showDuplicateSessionModal;
    window.closeDuplicateSessionModal = closeDuplicateSessionModal;
    window.forceNewLogin = forceNewLogin;

    return {
        handleMessage,
        showDuplicateSessionModal,
    };
}
