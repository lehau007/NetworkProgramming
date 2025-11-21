# Client Architecture - Detailed Guide

## Overview
This document provides implementation guidance for the chess client, with flexibility for Python UI or Web UI.

---

## Client Option 1: Python Desktop Application

### Project Structure
```
client/
├── main.py                     # Entry point
├── config/
│   └── settings.py             # Configuration
├── network/
│   ├── connection.py           # TCP connection management
│   ├── protocol.py             # Message protocol
│   └── message_types.py        # Message definitions
├── ui/
│   ├── login_window.py         # Login screen
│   ├── register_window.py      # Registration screen
│   ├── lobby_window.py         # Player lobby
│   ├── game_window.py          # Chess game UI
│   └── components/
│       ├── chess_board.py      # Board widget
│       ├── piece.py            # Chess piece rendering
│       └── player_info.py      # Player stats display
├── game/
│   ├── game_state.py           # Local game state
│   └── move_handler.py         # Move input/validation
└── utils/
    ├── logger.py               # Client-side logging
    └── helpers.py              # Utility functions
```

### Core Components

#### 1. Connection Manager (`network/connection.py`)

```python
import socket
import json
import struct
import threading
from queue import Queue

class ConnectionManager:
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.socket = None
        self.connected = False
        self.message_queue = Queue()
        self.receive_thread = None
        
    def connect(self):
        """Establish TCP connection to server"""
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.connected = True
            
            # Start receive thread
            self.receive_thread = threading.Thread(target=self._receive_loop, daemon=True)
            self.receive_thread.start()
            
            return True
        except Exception as e:
            print(f"Connection failed: {e}")
            return False
    
    def disconnect(self):
        """Close connection gracefully"""
        self.connected = False
        if self.socket:
            self.socket.close()
            self.socket = None
    
    def send_message(self, message_dict):
        """Send JSON message with length prefix"""
        if not self.connected:
            return False
        
        try:
            # Serialize to JSON
            json_str = json.dumps(message_dict)
            json_bytes = json_str.encode('utf-8')
            
            # Send length prefix (4 bytes, network byte order)
            length = struct.pack('!I', len(json_bytes))
            self.socket.sendall(length)
            
            # Send JSON payload
            self.socket.sendall(json_bytes)
            
            return True
        except Exception as e:
            print(f"Send error: {e}")
            self.connected = False
            return False
    
    def _receive_loop(self):
        """Background thread to receive messages"""
        while self.connected:
            try:
                # Receive length prefix
                length_bytes = self._receive_exact(4)
                if not length_bytes:
                    break
                
                length = struct.unpack('!I', length_bytes)[0]
                
                # Validate reasonable size
                if length > 1024 * 1024:  # 1MB max
                    print(f"Message too large: {length}")
                    break
                
                # Receive JSON payload
                json_bytes = self._receive_exact(length)
                if not json_bytes:
                    break
                
                # Parse and queue
                message = json.loads(json_bytes.decode('utf-8'))
                self.message_queue.put(message)
                
            except Exception as e:
                print(f"Receive error: {e}")
                self.connected = False
                break
    
    def _receive_exact(self, num_bytes):
        """Receive exact number of bytes"""
        data = b''
        while len(data) < num_bytes:
            chunk = self.socket.recv(num_bytes - len(data))
            if not chunk:
                return None
            data += chunk
        return data
    
    def get_message(self, timeout=None):
        """Get next message from queue (blocking)"""
        try:
            return self.message_queue.get(timeout=timeout)
        except:
            return None
    
    def has_messages(self):
        """Check if messages are available"""
        return not self.message_queue.empty()
```

#### 2. Protocol Handler (`network/protocol.py`)

```python
from network.message_types import MessageType

class ProtocolHandler:
    @staticmethod
    def create_login_message(username, password):
        """Create login request"""
        return {
            "type": MessageType.LOGIN,
            "username": username,
            "password": password  # Should be hashed in production
        }
    
    @staticmethod
    def create_register_message(username, password, email):
        """Create registration request"""
        return {
            "type": MessageType.REGISTER,
            "username": username,
            "password": password,
            "email": email
        }
    
    @staticmethod
    def create_get_players_message():
        """Request available players"""
        return {
            "type": MessageType.GET_AVAILABLE_PLAYERS
        }
    
    @staticmethod
    def create_challenge_message(target_username):
        """Send challenge to player"""
        return {
            "type": MessageType.CHALLENGE,
            "target_username": target_username
        }
    
    @staticmethod
    def create_accept_challenge_message(challenge_id):
        """Accept a challenge"""
        return {
            "type": MessageType.ACCEPT_CHALLENGE,
            "challenge_id": challenge_id
        }
    
    @staticmethod
    def create_decline_challenge_message(challenge_id):
        """Decline a challenge"""
        return {
            "type": MessageType.DECLINE_CHALLENGE,
            "challenge_id": challenge_id
        }
    
    @staticmethod
    def create_move_message(game_id, move):
        """Send chess move"""
        return {
            "type": MessageType.MOVE,
            "game_id": game_id,
            "move": move
        }
    
    @staticmethod
    def create_resign_message(game_id):
        """Resign from game"""
        return {
            "type": MessageType.RESIGN,
            "game_id": game_id
        }
    
    @staticmethod
    def create_draw_offer_message(game_id):
        """Offer draw"""
        return {
            "type": MessageType.DRAW_OFFER,
            "game_id": game_id
        }
```

#### 3. Message Types (`network/message_types.py`)

```python
class MessageType:
    # Authentication
    LOGIN = "LOGIN"
    LOGIN_RESPONSE = "LOGIN_RESPONSE"
    REGISTER = "REGISTER"
    REGISTER_RESPONSE = "REGISTER_RESPONSE"
    
    # Lobby
    GET_AVAILABLE_PLAYERS = "GET_AVAILABLE_PLAYERS"
    PLAYER_LIST = "PLAYER_LIST"
    
    # Matchmaking
    CHALLENGE = "CHALLENGE"
    CHALLENGE_RECEIVED = "CHALLENGE_RECEIVED"
    CHALLENGE_SENT = "CHALLENGE_SENT"
    ACCEPT_CHALLENGE = "ACCEPT_CHALLENGE"
    DECLINE_CHALLENGE = "DECLINE_CHALLENGE"
    MATCH_STARTED = "MATCH_STARTED"
    
    # Gameplay
    MOVE = "MOVE"
    MOVE_ACCEPTED = "MOVE_ACCEPTED"
    OPPONENT_MOVE = "OPPONENT_MOVE"
    MOVE_REJECTED = "MOVE_REJECTED"
    
    # Game end
    RESIGN = "RESIGN"
    DRAW_OFFER = "DRAW_OFFER"
    DRAW_RESPONSE = "DRAW_RESPONSE"
    GAME_ENDED = "GAME_ENDED"
    GAME_LOG = "GAME_LOG"
    
    # Error
    ERROR = "ERROR"
```

---

### UI Components (PyQt5 Example)

#### 4. Login Window (`ui/login_window.py`)

```python
from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, 
                            QLabel, QLineEdit, QPushButton, QMessageBox)
from PyQt5.QtCore import pyqtSignal
from network.protocol import ProtocolHandler
from network.message_types import MessageType

class LoginWindow(QWidget):
    login_success = pyqtSignal(dict)  # Emit user data on successful login
    
    def __init__(self, connection_manager):
        super().__init__()
        self.connection = connection_manager
        self.init_ui()
    
    def init_ui(self):
        self.setWindowTitle("Chess Login")
        self.setGeometry(100, 100, 400, 300)
        
        layout = QVBoxLayout()
        
        # Title
        title = QLabel("Chess Game Login")
        title.setStyleSheet("font-size: 24px; font-weight: bold;")
        layout.addWidget(title)
        
        # Username
        self.username_input = QLineEdit()
        self.username_input.setPlaceholderText("Username")
        layout.addWidget(QLabel("Username:"))
        layout.addWidget(self.username_input)
        
        # Password
        self.password_input = QLineEdit()
        self.password_input.setPlaceholderText("Password")
        self.password_input.setEchoMode(QLineEdit.Password)
        layout.addWidget(QLabel("Password:"))
        layout.addWidget(self.password_input)
        
        # Buttons
        button_layout = QHBoxLayout()
        
        login_btn = QPushButton("Login")
        login_btn.clicked.connect(self.handle_login)
        button_layout.addWidget(login_btn)
        
        register_btn = QPushButton("Register")
        register_btn.clicked.connect(self.show_register)
        button_layout.addWidget(register_btn)
        
        layout.addLayout(button_layout)
        
        # Status
        self.status_label = QLabel("")
        layout.addWidget(self.status_label)
        
        self.setLayout(layout)
    
    def handle_login(self):
        username = self.username_input.text()
        password = self.password_input.text()
        
        if not username or not password:
            self.status_label.setText("Please enter username and password")
            return
        
        # Send login request
        message = ProtocolHandler.create_login_message(username, password)
        if not self.connection.send_message(message):
            self.status_label.setText("Connection error")
            return
        
        self.status_label.setText("Logging in...")
        
        # Wait for response (in production, use signals/slots)
        response = self.connection.get_message(timeout=5)
        
        if response and response["type"] == MessageType.LOGIN_RESPONSE:
            if response["status"] == "success":
                self.login_success.emit(response["user_data"])
                self.close()
            else:
                self.status_label.setText(f"Login failed: {response.get('message', 'Unknown error')}")
        else:
            self.status_label.setText("No response from server")
    
    def show_register(self):
        # Show registration window (implement similarly)
        pass
```

#### 5. Chess Board (`ui/components/chess_board.py`)

```python
from PyQt5.QtWidgets import QWidget, QGridLayout, QPushButton
from PyQt5.QtCore import pyqtSignal
from PyQt5.QtGui import QIcon, QPixmap

class ChessBoard(QWidget):
    move_made = pyqtSignal(str)  # Emit move in format "e2e4"
    
    def __init__(self):
        super().__init__()
        self.selected_square = None
        self.board_state = self.init_board()
        self.squares = {}
        self.init_ui()
    
    def init_board(self):
        """Initialize board state"""
        # 8x8 board representation
        return [
            ['r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'],  # Black back rank
            ['p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'],  # Black pawns
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['.', '.', '.', '.', '.', '.', '.', '.'],
            ['P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'],  # White pawns
            ['R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'],  # White back rank
        ]
    
    def init_ui(self):
        layout = QGridLayout()
        layout.setSpacing(0)
        
        # Create 8x8 grid of squares
        for row in range(8):
            for col in range(8):
                square = QPushButton()
                square.setFixedSize(60, 60)
                
                # Alternating colors
                is_light = (row + col) % 2 == 0
                color = "#F0D9B5" if is_light else "#B58863"
                square.setStyleSheet(f"background-color: {color};")
                
                # Store position
                square.setProperty("row", row)
                square.setProperty("col", col)
                square.clicked.connect(self.square_clicked)
                
                # Add to grid
                layout.addWidget(square, row, col)
                self.squares[(row, col)] = square
        
        self.setLayout(layout)
        self.update_pieces()
    
    def update_pieces(self):
        """Update piece images on board"""
        piece_images = {
            'K': '♔', 'Q': '♕', 'R': '♖', 'B': '♗', 'N': '♘', 'P': '♙',
            'k': '♚', 'q': '♛', 'r': '♜', 'b': '♝', 'n': '♞', 'p': '♟'
        }
        
        for row in range(8):
            for col in range(8):
                piece = self.board_state[row][col]
                square = self.squares[(row, col)]
                
                if piece != '.':
                    square.setText(piece_images.get(piece, piece))
                    square.setStyleSheet(square.styleSheet() + "; font-size: 36px;")
                else:
                    square.setText("")
    
    def square_clicked(self):
        """Handle square click for move input"""
        sender = self.sender()
        row = sender.property("row")
        col = sender.property("col")
        
        if self.selected_square is None:
            # Select piece
            if self.board_state[row][col] != '.':
                self.selected_square = (row, col)
                sender.setStyleSheet(sender.styleSheet() + "; border: 3px solid yellow;")
        else:
            # Make move
            from_row, from_col = self.selected_square
            to_row, to_col = row, col
            
            # Convert to chess notation (e.g., "e2e4")
            move = self.position_to_notation(from_row, from_col) + \
                   self.position_to_notation(to_row, to_col)
            
            self.move_made.emit(move)
            
            # Clear selection
            self.clear_selection()
            self.selected_square = None
    
    def position_to_notation(self, row, col):
        """Convert row/col to chess notation"""
        files = 'abcdefgh'
        ranks = '87654321'
        return files[col] + ranks[row]
    
    def clear_selection(self):
        """Clear square highlights"""
        for square in self.squares.values():
            # Reset to original color
            row = square.property("row")
            col = square.property("col")
            is_light = (row + col) % 2 == 0
            color = "#F0D9B5" if is_light else "#B58863"
            square.setStyleSheet(f"background-color: {color};")
    
    def update_board_state(self, new_state):
        """Update board with new state from server"""
        self.board_state = new_state
        self.update_pieces()
```

---

### Main Application (`main.py`)

```python
import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QStackedWidget
from network.connection import ConnectionManager
from ui.login_window import LoginWindow
from ui.lobby_window import LobbyWindow
from ui.game_window import GameWindow
from config.settings import SERVER_HOST, SERVER_PORT

class ChessClient(QMainWindow):
    def __init__(self):
        super().__init__()
        self.connection = ConnectionManager(SERVER_HOST, SERVER_PORT)
        
        # Connect to server
        if not self.connection.connect():
            print("Failed to connect to server")
            sys.exit(1)
        
        # Window stack
        self.stack = QStackedWidget()
        self.setCentralWidget(self.stack)
        
        # Create windows
        self.login_window = LoginWindow(self.connection)
        self.lobby_window = LobbyWindow(self.connection)
        self.game_window = GameWindow(self.connection)
        
        # Add to stack
        self.stack.addWidget(self.login_window)
        self.stack.addWidget(self.lobby_window)
        self.stack.addWidget(self.game_window)
        
        # Connect signals
        self.login_window.login_success.connect(self.show_lobby)
        
        # Show login
        self.stack.setCurrentWidget(self.login_window)
        
        self.setWindowTitle("Chess Client")
        self.setGeometry(100, 100, 800, 600)
    
    def show_lobby(self, user_data):
        """Switch to lobby after login"""
        self.lobby_window.set_user_data(user_data)
        self.stack.setCurrentWidget(self.lobby_window)
    
    def closeEvent(self, event):
        """Clean up on close"""
        self.connection.disconnect()
        event.accept()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    client = ChessClient()
    client.show()
    sys.exit(app.exec_())
```

---

## Client Option 2: Web-Based UI

### Technology Stack
- **Frontend**: HTML/CSS/JavaScript
- **Communication**: WebSocket or Socket.io
- **Chess Board**: chess.js + chessboard.js libraries

### Basic Structure

```html
<!-- index.html -->
<!DOCTYPE html>
<html>
<head>
    <title>Chess Game</title>
    <link rel="stylesheet" href="css/chessboard.css">
    <link rel="stylesheet" href="css/style.css">
</head>
<body>
    <div id="login-screen">
        <h1>Chess Login</h1>
        <input type="text" id="username" placeholder="Username">
        <input type="password" id="password" placeholder="Password">
        <button onclick="login()">Login</button>
    </div>
    
    <div id="game-screen" style="display:none;">
        <div id="board" style="width: 400px"></div>
        <div id="status"></div>
    </div>
    
    <script src="js/chess.js"></script>
    <script src="js/chessboard.js"></script>
    <script src="js/connection.js"></script>
    <script src="js/game.js"></script>
</body>
</html>
```

```javascript
// js/connection.js
class Connection {
    constructor(host, port) {
        this.socket = new WebSocket(`ws://${host}:${port}`);
        this.handlers = {};
        
        this.socket.onmessage = (event) => {
            const message = JSON.parse(event.data);
            const handler = this.handlers[message.type];
            if (handler) {
                handler(message);
            }
        };
    }
    
    send(message) {
        this.socket.send(JSON.stringify(message));
    }
    
    on(messageType, handler) {
        this.handlers[messageType] = handler;
    }
}

// Usage
const conn = new Connection('localhost', 8080);

function login() {
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;
    
    conn.send({
        type: 'LOGIN',
        username: username,
        password: password
    });
}

conn.on('LOGIN_RESPONSE', (message) => {
    if (message.status === 'success') {
        document.getElementById('login-screen').style.display = 'none';
        document.getElementById('game-screen').style.display = 'block';
    }
});
```

---

## Development Workflow

### Phase 1: Basic Connection (Week 1)
- [x] Implement ConnectionManager
- [x] Test TCP connection
- [x] Implement message protocol
- [x] Test send/receive

### Phase 2: Authentication UI (Week 2)
- [x] Create login window
- [x] Create register window
- [x] Handle authentication responses
- [x] Store session data

### Phase 3: Lobby (Week 3)
- [x] Display player list
- [x] Challenge system UI
- [x] Accept/decline dialogs

### Phase 4: Game UI (Week 4-5)
- [x] Chess board rendering
- [x] Move input handling
- [x] Board state updates
- [x] Game status display

### Phase 5: Polish (Week 6)
- [x] Error handling
- [x] Reconnection logic
- [x] Game history viewer
- [x] Settings/preferences

---

## Testing Client

### Manual Testing Steps
1. Start server
2. Run client
3. Test login
4. Test player list
5. Test challenge flow
6. Test complete game
7. Test error cases

### Automated Testing (pytest example)

```python
# tests/test_connection.py
import pytest
from network.connection import ConnectionManager

def test_connection():
    conn = ConnectionManager('localhost', 8080)
    assert conn.connect() == True
    conn.disconnect()

def test_message_send():
    conn = ConnectionManager('localhost', 8080)
    conn.connect()
    
    message = {"type": "TEST", "data": "test"}
    assert conn.send_message(message) == True
    
    conn.disconnect()
```

---

**Document Version**: 1.0  
**Status**: Implementation Guide
