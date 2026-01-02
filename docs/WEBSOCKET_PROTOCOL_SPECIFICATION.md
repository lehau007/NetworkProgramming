# WebSocket Protocol Specification

> Based on RFC 6455 - The WebSocket Protocol  
> Reference: https://datatracker.ietf.org/doc/html/rfc6455

## Table of Contents

1. [Introduction](#1-introduction)
2. [Relationship with TCP and HTTP](#2-relationship-with-tcp-and-http)
3. [WebSocket URIs](#3-websocket-uris)
4. [Connection Establishment](#4-connection-establishment)
5. [Opening Handshake](#5-opening-handshake)
6. [Data Framing](#6-data-framing)
7. [Data Transfer](#7-data-transfer)
8. [Control Frames](#8-control-frames)
9. [Closing the Connection](#9-closing-the-connection)
10. [Security Considerations](#10-security-considerations)
11. [Status Codes](#11-status-codes)

---

## 1. Introduction

### 1.1 Background

The WebSocket Protocol enables **full-duplex, bidirectional communication** between a client and a server over a single TCP connection. Before WebSocket, web applications requiring real-time communication had to rely on workarounds such as:

- **HTTP Long Polling** - Server holds the connection open until data is available
- **HTTP Streaming** - Server sends data in chunks over a long-lived connection
- **Multiple HTTP connections** - Separate connections for upstream and downstream

These approaches had significant drawbacks:
- High overhead due to HTTP headers on each request
- Latency from establishing new connections
- Complexity in managing multiple connections

### 1.2 Protocol Overview

The WebSocket Protocol consists of two main parts:

```
┌─────────────────────────────────────────────────────────────┐
│                    WebSocket Protocol                        │
├─────────────────────────────────────────────────────────────┤
│  1. Opening Handshake (HTTP Upgrade)                        │
│     - Client sends HTTP Upgrade request                     │
│     - Server responds with 101 Switching Protocols          │
├─────────────────────────────────────────────────────────────┤
│  2. Data Transfer (WebSocket Frames)                        │
│     - Bidirectional message exchange                        │
│     - Frame-based protocol over TCP                         │
│     - Text and Binary data support                          │
├─────────────────────────────────────────────────────────────┤
│  3. Closing Handshake                                       │
│     - Close frame exchange                                  │
│     - Clean TCP termination                                 │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Relationship with TCP and HTTP

### 2.1 WebSocket and TCP

WebSocket is an **independent TCP-based protocol**. It operates directly on top of TCP:

```
┌─────────────────────────────┐
│    Application Layer        │
│   (WebSocket Messages)      │
├─────────────────────────────┤
│    WebSocket Protocol       │
│   (Framing + Control)       │
├─────────────────────────────┤
│    Transport Layer          │
│        (TCP)                │
├─────────────────────────────┤
│    Network Layer            │
│        (IP)                 │
└─────────────────────────────┘
```

**Key characteristics:**
- Layered over TCP for reliable, ordered delivery
- Adds **framing mechanism** to TCP's byte stream
- Provides **message semantics** (vs TCP's stream semantics)
- Adds **origin-based security** for browser clients

### 2.2 WebSocket and HTTP

The **only relationship** between WebSocket and HTTP is the **opening handshake**:

| Aspect | HTTP | WebSocket |
|--------|------|-----------|
| Connection | Request-Response | Persistent, Full-Duplex |
| Overhead | Headers on every request | Minimal frame headers |
| Initiation | Client initiates | Either party can send |
| State | Stateless | Stateful |

**Why use HTTP for handshake?**

1. **Compatibility** - Works with existing HTTP infrastructure (ports 80/443)
2. **Proxy traversal** - HTTP proxies can forward the Upgrade request
3. **Authentication** - Leverage existing HTTP auth mechanisms
4. **Same-origin policy** - Browser security model integration

### 2.3 Port Usage

| URI Scheme | Port | Security |
|------------|------|----------|
| `ws://`    | 80 (default) | Unencrypted |
| `wss://`   | 443 (default) | TLS encrypted |

---

## 3. WebSocket URIs

### 3.1 URI Syntax

```abnf
ws-URI  = "ws:" "//" host [ ":" port ] path [ "?" query ]
wss-URI = "wss:" "//" host [ ":" port ] path [ "?" query ]
```

### 3.2 Examples

```
ws://example.com/chat
ws://example.com:8080/game?room=123
wss://secure.example.com/api/v1/stream
wss://example.com/socket?token=abc123
```

### 3.3 Resource Name Construction

The **resource-name** sent in the handshake is constructed as:
1. `/` if path is empty
2. The path component
3. `?` + query if query is non-empty

Example: `ws://example.com:8080/game?room=123`
- Resource name: `/game?room=123`

---

## 4. Connection Establishment

### 4.1 Connection Flow

```
     Client                                    Server
        │                                         │
        │  1. TCP Connection (SYN/SYN-ACK/ACK)    │
        │ ─────────────────────────────────────►  │
        │                                         │
        │  2. [Optional] TLS Handshake (wss://)   │
        │ ◄───────────────────────────────────►   │
        │                                         │
        │  3. HTTP Upgrade Request                │
        │ ─────────────────────────────────────►  │
        │                                         │
        │  4. HTTP 101 Switching Protocols        │
        │ ◄─────────────────────────────────────  │
        │                                         │
        │  5. WebSocket Connection OPEN           │
        │ ◄───────────────────────────────────►   │
        │                                         │
        │  6. Bidirectional Data Transfer         │
        │ ◄───────────────────────────────────►   │
        │                                         │
```

### 4.2 Connection States

```
                    ┌──────────┐
                    │CONNECTING│
                    └────┬─────┘
                         │ Handshake Complete
                         ▼
                    ┌──────────┐
              ┌─────│   OPEN   │─────┐
              │     └────┬─────┘     │
              │          │           │
    Send Close│          │Receive    │Receive Close
              │          │Close      │
              ▼          ▼           ▼
         ┌──────────────────────────────┐
         │          CLOSING             │
         └────────────┬─────────────────┘
                      │ TCP Closed
                      ▼
                 ┌──────────┐
                 │  CLOSED  │
                 └──────────┘
```

---

## 5. Opening Handshake

### 5.1 Client Handshake Request

The client initiates the WebSocket connection with an **HTTP Upgrade request**:

```http
GET /chat HTTP/1.1
Host: server.example.com
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Origin: http://example.com
Sec-WebSocket-Protocol: chat, superchat
Sec-WebSocket-Version: 13
```

#### Required Headers

| Header | Description | Example |
|--------|-------------|---------|
| `Host` | Server hostname (and port if non-default) | `server.example.com` |
| `Upgrade` | Must be "websocket" | `websocket` |
| `Connection` | Must include "Upgrade" | `Upgrade` |
| `Sec-WebSocket-Key` | Base64-encoded 16-byte random nonce | `dGhlIHNhbXBsZSBub25jZQ==` |
| `Sec-WebSocket-Version` | Must be 13 | `13` |

#### Optional Headers

| Header | Description |
|--------|-------------|
| `Origin` | Origin of the script (required for browsers) |
| `Sec-WebSocket-Protocol` | Comma-separated list of subprotocols |
| `Sec-WebSocket-Extensions` | Requested extensions (e.g., compression) |

### 5.2 Server Handshake Response

The server accepts the connection with:

```http
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
Sec-WebSocket-Protocol: chat
```

#### Required Headers

| Header | Description |
|--------|-------------|
| `Upgrade` | Must be "websocket" |
| `Connection` | Must be "Upgrade" |
| `Sec-WebSocket-Accept` | Computed acceptance key |

### 5.3 Sec-WebSocket-Accept Calculation

The server **MUST** prove it received the client's handshake by computing the accept key:

```
Sec-WebSocket-Accept = Base64(SHA-1(Sec-WebSocket-Key + GUID))

Where GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
```

**Example calculation:**

```
1. Client sends: Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==

2. Server concatenates with GUID:
   dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11

3. Server computes SHA-1 hash:
   0xb3 0x7a 0x4f 0x2c 0xc0 0x62 0x4f 0x16 0x90 0xf6 
   0x46 0x06 0xcf 0x38 0x59 0x45 0xb2 0xbe 0xc4 0xea

4. Server Base64 encodes the hash:
   s3pPLMBiTxaQ9kYGzzhZRbK+xOo=

5. Server responds: Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

### 5.4 Handshake Failure Scenarios

| HTTP Status | Meaning |
|-------------|---------|
| 400 Bad Request | Malformed handshake |
| 401 Unauthorized | Authentication required |
| 403 Forbidden | Origin not allowed |
| 404 Not Found | Resource not found |
| 426 Upgrade Required | Version not supported |

---

## 6. Data Framing

### 6.1 Frame Structure

After the handshake, data is transmitted using **WebSocket frames**:

```
      0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
```

### 6.2 Frame Fields

| Field | Size | Description |
|-------|------|-------------|
| **FIN** | 1 bit | Final fragment flag. 1 = final frame of message |
| **RSV1-3** | 1 bit each | Reserved for extensions. Must be 0 unless negotiated |
| **Opcode** | 4 bits | Frame type (see below) |
| **MASK** | 1 bit | 1 = payload is masked. Client→Server frames MUST be masked |
| **Payload length** | 7, 7+16, or 7+64 bits | Length of payload data |
| **Masking-key** | 0 or 4 bytes | Present if MASK=1. Random 32-bit value |
| **Payload data** | Variable | Extension data + Application data |

### 6.3 Opcodes

| Opcode | Type | Description |
|--------|------|-------------|
| `0x0` | Continuation | Continuation frame for fragmented messages |
| `0x1` | Text | UTF-8 encoded text data |
| `0x2` | Binary | Arbitrary binary data |
| `0x3-0x7` | Reserved | Reserved for future non-control frames |
| `0x8` | Close | Connection close request |
| `0x9` | Ping | Heartbeat/keepalive request |
| `0xA` | Pong | Response to ping |
| `0xB-0xF` | Reserved | Reserved for future control frames |

### 6.4 Payload Length Encoding

| Payload Length Value | Actual Length | Size |
|---------------------|---------------|------|
| 0-125 | Value itself | 7 bits |
| 126 | Next 2 bytes as 16-bit unsigned integer | 7 + 16 bits |
| 127 | Next 8 bytes as 64-bit unsigned integer | 7 + 64 bits |

### 6.5 Masking

**Client-to-Server frames MUST be masked.** Server-to-client frames MUST NOT be masked.

**Masking Algorithm:**
```
j = i MOD 4
transformed-octet-i = original-octet-i XOR masking-key-octet-j
```

The same algorithm applies for both masking and unmasking (XOR is its own inverse).

**Why masking?**
- Prevents cache poisoning attacks on intermediary proxies
- Ensures WebSocket traffic cannot be mistaken for HTTP requests
- Masking key must be unpredictable (derived from strong entropy source)

### 6.6 Frame Examples

#### Single-frame unmasked text message ("Hello")
```
0x81 0x05 0x48 0x65 0x6c 0x6c 0x6f
│    │    └───────────────────────── Payload: "Hello"
│    └─────────────────────────────── Payload length: 5
└──────────────────────────────────── FIN=1, opcode=0x1 (text)
```

#### Single-frame masked text message ("Hello")
```
0x81 0x85 0x37 0xfa 0x21 0x3d 0x7f 0x9f 0x4d 0x51 0x58
│    │    └───────────────────┬───┘ └─────────────────┘
│    │                        │            Masked payload
│    │                        Masking key
│    └─────────────────────────────── Payload length: 5, MASK=1
└──────────────────────────────────── FIN=1, opcode=0x1 (text)
```

#### Fragmented unmasked text message ("Hello")
```
First frame:  0x01 0x03 0x48 0x65 0x6c       ("Hel") FIN=0, opcode=0x1
Final frame:  0x80 0x02 0x6c 0x6f            ("lo")  FIN=1, opcode=0x0
```

---

## 7. Data Transfer

### 7.1 Sending Data

To send a WebSocket message:

1. Ensure connection is in **OPEN** state
2. Encapsulate data in one or more frames
3. Set appropriate opcode (0x1 for text, 0x2 for binary)
4. Set FIN=1 on final frame
5. **Client MUST mask** all frames
6. Transmit frames over the TCP connection

### 7.2 Receiving Data

To receive a WebSocket message:

1. Parse incoming bytes as WebSocket frames
2. Handle control frames immediately (Ping, Pong, Close)
3. Accumulate data frames (handle fragmentation)
4. When FIN=1 received, deliver complete message
5. **Server MUST unmask** client frames

### 7.3 Fragmentation

Large messages can be split into multiple frames:

```
┌─────────────────────────────────────────────────────────┐
│  Frame 1: FIN=0, opcode=0x1 (text), payload="Part 1"   │
├─────────────────────────────────────────────────────────┤
│  Frame 2: FIN=0, opcode=0x0 (cont), payload="Part 2"   │
├─────────────────────────────────────────────────────────┤
│  Frame 3: FIN=1, opcode=0x0 (cont), payload="Part 3"   │
└─────────────────────────────────────────────────────────┘

Complete message = "Part 1" + "Part 2" + "Part 3"
```

**Rules:**
- First frame has actual opcode (0x1 or 0x2)
- Continuation frames have opcode 0x0
- Last frame has FIN=1
- Control frames MAY be injected between fragments
- Fragments MUST be delivered in order

---

## 8. Control Frames

Control frames are used for protocol-level signaling:

### 8.1 Ping (Opcode 0x9)

- Used for keepalive or to verify remote endpoint is responsive
- MAY include application data (up to 125 bytes)
- Recipient MUST respond with Pong frame (unless Close received)

```
┌────────────┐         Ping          ┌────────────┐
│   Client   │ ─────────────────────►│   Server   │
│            │                       │            │
│            │◄──────────────────────│            │
└────────────┘         Pong          └────────────┘
```

### 8.2 Pong (Opcode 0xA)

- Response to Ping frame
- MUST echo the Ping's application data
- MAY be sent unsolicited (unidirectional heartbeat)

### 8.3 Close (Opcode 0x8)

- Initiates the closing handshake
- MAY include status code (2 bytes) + reason (UTF-8 string)
- After sending Close, endpoint MUST NOT send more data frames
- See Section 9 for details

---

## 9. Closing the Connection

### 9.1 Closing Handshake

```
     Client                                    Server
        │                                         │
        │  1. Close Frame (status code + reason)  │
        │ ─────────────────────────────────────►  │
        │                                         │
        │  2. Close Frame (echo status)           │
        │ ◄─────────────────────────────────────  │
        │                                         │
        │  3. Server closes TCP connection        │
        │ ◄──────────── TCP FIN ─────────────────│
        │                                         │
        │  4. Client closes TCP connection        │
        │ ─────────── TCP FIN/ACK ──────────────►│
        │                                         │
```

### 9.2 Close Frame Format

```
+---------+------------------+------------------------+
| 0x88    | length           | status code | reason   |
| (opcode)| (payload length) | (2 bytes)   | (UTF-8)  |
+---------+------------------+------------------------+
```

### 9.3 TCP Connection Closure

- **Server** SHOULD close TCP connection first
- **Client** SHOULD wait for server's TCP close
- Server holds TIME_WAIT state (not client)
- This prevents client from waiting 2MSL before reconnecting

---

## 10. Security Considerations

### 10.1 Origin Validation

- Browsers include `Origin` header in handshake
- Servers SHOULD validate origin to prevent CSRF attacks
- Non-browser clients can spoof origin

### 10.2 Masking Purpose

Masking protects against **cache poisoning attacks**:

```
┌─────────┐    Malicious     ┌─────────┐    Poisoned    ┌─────────┐
│ Client  │───WebSocket─────►│  Proxy  │────Cache──────►│ Victim  │
└─────────┘    (looks like   └─────────┘                └─────────┘
               HTTP GET)
```

Without masking, attackers could craft WebSocket frames that look like HTTP requests to proxies.

### 10.3 TLS (wss://)

- Use `wss://` for encrypted connections
- Provides confidentiality and integrity
- Server authentication via certificates

### 10.4 Authentication

WebSocket doesn't define authentication. Options include:
- HTTP cookies (sent in handshake)
- HTTP Basic/Digest authentication
- Token in URL query parameter
- Token in first WebSocket message

---

## 11. Status Codes

### 11.1 Defined Status Codes

| Code | Name | Description |
|------|------|-------------|
| 1000 | Normal Closure | Connection fulfilled its purpose |
| 1001 | Going Away | Endpoint going down (server shutdown, browser navigating away) |
| 1002 | Protocol Error | Protocol violation |
| 1003 | Unsupported Data | Received data type not acceptable |
| 1004 | Reserved | Reserved for future use |
| 1005 | No Status Rcvd | No status code in Close frame (internal use only) |
| 1006 | Abnormal Closure | Connection closed without Close frame (internal use only) |
| 1007 | Invalid Payload | Data inconsistent with message type (e.g., non-UTF-8 in text) |
| 1008 | Policy Violation | Message violates policy |
| 1009 | Message Too Big | Message too large to process |
| 1010 | Mandatory Ext. | Client expected extension not provided by server |
| 1011 | Internal Error | Server encountered unexpected condition |
| 1015 | TLS Handshake | TLS handshake failure (internal use only) |

### 11.2 Status Code Ranges

| Range | Usage |
|-------|-------|
| 0-999 | Not used |
| 1000-2999 | Protocol-defined (RFC 6455 and extensions) |
| 3000-3999 | Libraries, frameworks, applications (IANA registered) |
| 4000-4999 | Private use |

---

## Summary

The WebSocket Protocol provides:

1. **Efficient bidirectional communication** over a single TCP connection
2. **HTTP-compatible handshake** for firewall/proxy traversal
3. **Low-overhead framing** (2-14 bytes per frame)
4. **Message semantics** on top of TCP's byte stream
5. **Built-in security** via masking and origin checking
6. **TLS support** for encrypted connections

This makes WebSocket ideal for:
- Real-time applications (chat, gaming, trading)
- Live updates (notifications, dashboards)
- Collaborative editing
- IoT communication

---

## References

- [RFC 6455 - The WebSocket Protocol](https://datatracker.ietf.org/doc/html/rfc6455)
- [RFC 7936 - Clarifying Registry Procedures for the WebSocket Subprotocol Name Registry](https://datatracker.ietf.org/doc/html/rfc7936)
- [RFC 8307 - Well-Known URIs for the WebSocket Protocol](https://datatracker.ietf.org/doc/html/rfc8307)
- [RFC 8441 - Bootstrapping WebSockets with HTTP/2](https://datatracker.ietf.org/doc/html/rfc8441)
- [WebSocket API (W3C)](https://www.w3.org/TR/websockets/)
