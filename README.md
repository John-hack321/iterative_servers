# Chat Application - Assignment 3

A distributed one-on-one chat application with two different transport layer implementations for SCS3304.

## Project Overview

This project demonstrates the differences between connectionless (UDP) and connection-oriented (TCP) communication models in distributed systems. Both implementations use an iterative server design that serves one client at a time.

## Directory Structure

```
chat-app-a3/
├── README.md                           # This file - general overview
├── iterative-connectionless/            # UDP implementation
│   ├── README.md                       # UDP-specific documentation
│   ├── makefile                        # Build configuration
│   ├── data/                           # User and message storage
│   ├── common/                         # Shared source code
│   ├── server/                         # UDP server implementation
│   └── client/                         # UDP client implementation
│
└── iterative-connectionless-oriented/   # TCP implementation
    ├── README.md                       # TCP-specific documentation
    ├── makefile                        # Build configuration
    ├── data/                           # User and message storage
    ├── common/                         # Shared source code
    ├── server/                         # TCP server implementation
    └── client/                         # TCP client implementation
```

## Two Implementations

### 1. Iterative Connectionless (UDP)
- **Transport**: UDP
- **Connection Model**: Connectionless - no persistent connection
- **Message Boundaries**: Natural (datagram boundaries)
- **Reliability**: Unreliable, may lose packets
- **Overhead**: Lower
- **Port**: 9090

### 2. Iterative Connection-Oriented (TCP)
- **Transport**: TCP
- **Connection Model**: Connection-oriented - persistent TCP connection
- **Message Boundaries**: Requires length-prefixed framing
- **Reliability**: Reliable, ordered delivery
- **Overhead**: Higher (handshake, headers)
- **Port**: 9091

## Common Features

Both implementations share:
- **Iterative Server Design**: One client served at a time
- **User Management**: Registration, login, logout, search
- **Messaging**: Send/receive messages, inbox, recent history
- **Multi-Machine Support**: Server and client on different machines
- **Authentication**: djb2 password hashing
- **Data Persistence**: File-based storage

## Quick Start

### For UDP Version
```bash
cd iterative-connectionless
make server        # On server machine
make client        # On client machine
./client_exec <server_ip>
```

### For TCP Version
```bash
cd iterative-connectionless-oriented
make server        # On server machine
make client        # On client machine
./client_exec <server_ip>
```

## Compilation Notes

Both implementations have been configured to avoid compilation issues when building on separate machines:

- **Separate compilation targets**: `make server` and `make client`
- **Unique executable names**: `server_exec` and `client_exec`
- **Individual cleanup**: `make clean-server` and `make clean-client`
- **Cross-platform compatibility**: Standard C with POSIX networking

## Key Differences

| Feature | UDP (Connectionless) | TCP (Connection-Oriented) |
|---------|---------------------|---------------------------|
| Transport | UDP | TCP |
| Connection | No connection state | Persistent connection |
| Reliability | Unreliable | Reliable |
| Order | Not guaranteed | Guaranteed |
| Message Boundaries | Natural | Requires framing |
| Flow Control | None | Built-in |
| Overhead | Lower | Higher |

## Testing Scenarios

1. **Single Client**: Both versions work identically from user perspective
2. **Multiple Clients**: Both queue additional clients (TCP in OS backlog, UDP naturally)
3. **Network Issues**: TCP handles packet loss, UDP may drop messages
4. **Performance**: UDP lower overhead, TCP more reliable

## Requirements

- GCC compiler
- Standard C library
- POSIX-compatible system (Linux/Unix/macOS)
- Network connectivity between machines
- Two machines for full distributed testing

## Academic Context

This assignment demonstrates understanding of:
- Transport layer protocols (UDP vs TCP)
- Client-server architecture
- Iterative vs concurrent server design
- Network programming fundamentals
- Cross-platform compilation issues
- Distributed system challenges

## Usage Tips

1. **Start server first** on both implementations
2. **Use different ports** (9090 for UDP, 9091 for TCP) to avoid conflicts
3. **Test on same machine first** for debugging, then deploy to different machines
4. **Check firewall settings** if connections fail
5. **Monitor data files** in `data/` directory to understand persistence

For detailed implementation-specific information, see the README files in each respective directory.