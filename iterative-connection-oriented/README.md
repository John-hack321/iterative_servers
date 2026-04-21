# Chat Application - Iterative Connection-Oriented (TCP)

A distributed chat application using TCP (connection-oriented) with iterative server design. The server handles ONE client at a time, queuing others in the OS backlog.

## Key Features

- **Connection-Oriented (TCP)**: Reliable, ordered delivery with three-way handshake
- **Iterative Server**: Single client served at a time, others queued
- **Multi-Machine Support**: Server and client run on different machines
- **Persistent Connection**: TCP connection stays open throughout client session

## Compilation Instructions

### For Server Machine
To compile only the server component:
```bash
make server
```

### For Client Machine  
To compile only the client component:
```bash
make client
```

### For Local Testing
To compile both server and client (for local testing):
```bash
make all
```

## Available Make Targets

- `make server` - Compile server only
- `make client` - Compile client only  
- `make all` - Compile both server and client
- `make clean` - Remove all executables
- `make clean-server` - Remove server executable only
- `make clean-client` - Remove client executable only
- `make help` - Show all available targets

## Usage

1. **Copy the entire project directory** to both server and client machines
2. **On the server machine**: 
   ```bash
   make server
   ./server_exec
   ```
3. **On the client machine**: 
   ```bash
   make client
   ./client_exec <server_ip>
   ```
   Example: `./client_exec 192.168.1.105`

## How It Works

### Connection-Oriented Model
- **TCP Handshake**: Client connects with `connect()`, server accepts with `accept()`
- **Persistent Connection**: Socket stays open for entire client session
- **Reliable Delivery**: Messages arrive in order, guaranteed delivery

### Iterative Server Design
- **One Client at a Time**: Server serves one client completely before accepting next
- **OS Backlog**: Additional clients queued (up to BACKLOG=5) 
- **Sequential Processing**: No fork(), no threads, pure sequential service

### Message Framing
- **Length Prefix**: Each message prefixed with 4-byte length header
- **Network Byte Order**: Uses `htonl()`/`ntohl()` for cross-platform compatibility
- **Stream Handling**: Solves TCP's stream boundary problem

## Project Structure

```
├── server/          # Server source code
├── client/          # Client source code
├── common/          # Shared source code
│   ├── auth.c       # Authentication (djb2 hash)
│   ├── user_manager.c  # User management
│   ├── message_handler.c # Message storage/retrieval
│   ├── utils.c      # Network framing utilities
│   └── protocol.h   # Protocol definitions
├── data/            # Data files (users.txt, messages.txt)
└── makefile         # Build configuration
```

## Protocol

- **Port**: 9091 (TCP)
- **Message Format**: `[4-byte length][message body]`
- **Commands**: REG, LOGIN, LOGOUT, LIST, SEARCH, MSG, SENDERS, RECENT
- **Responses**: ACK:OK, ACK:ERR

## Requirements

- GCC compiler
- Standard C library
- Network connectivity between server and client machines
- TCP/IP network stack

## Difference from Connectionless Version

| Feature | Connectionless (UDP) | Connection-Oriented (TCP) |
|---------|---------------------|---------------------------|
| Transport | UDP | TCP |
| Connection | No connection state | Persistent connection |
| Reliability | Unreliable, may lose packets | Reliable, ordered delivery |
| Server Model | Iterative | Iterative |
| Message Boundaries | Natural (datagrams) | Requires length framing |
| Overhead | Lower | Higher (handshake, headers) |

## Testing Scenarios

1. **Single Client**: Normal operation
2. **Multiple Clients**: Second client waits in backlog
3. **Network Issues**: TCP handles packet loss/reordering
4. **Cross-Platform**: Works across different CPU architectures
