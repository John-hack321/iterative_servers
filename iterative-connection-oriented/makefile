CC     = gcc
CFLAGS = -Wall -Wextra -g

# Common source files - both server and client need these
COMMON = common/auth.c \
         common/user_manager.c \
         common/message_handler.c \
         common/utils.c

# Default target - builds both (for local testing)
all: server_exec client_exec

# Server-only target - for compiling on server machine
server: server_exec
server_exec:
	$(CC) $(CFLAGS) server/server.c $(COMMON) -o server_exec

# Client-only target - for compiling on client machine  
client: client_exec
client_exec:
	$(CC) $(CFLAGS) client/client.c $(COMMON) -o client_exec

# Clean targets
clean:
	rm -f server_exec client_exec

clean-server:
	rm -f server_exec

clean-client:
	rm -f client_exec