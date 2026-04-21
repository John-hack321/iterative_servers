# Chat Application - Iterative Connection-less

A distributed chat application with separate server and client components designed to run on different machines.

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

## Project Structure

```
├── server/          # Server source code
├── client/          # Client source code
├── common/          # Shared source code
├── data/            # Data files
└── makefile         # Build configuration
```

## Usage

1. Copy the entire project directory to both server and client machines
2. On the server machine: run `make server`
3. On the client machine: run `make client`
4. Start the server executable first, then run client executables

## Requirements

- GCC compiler
- Standard C library
- Network connectivity between server and client machines
