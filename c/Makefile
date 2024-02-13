# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -Werror -pedantic -std=c99 -fopenmp

# Source files
CLIENT_SRC = client.c
SERVER_SRC = server.c

# Executables
CLIENT_EXE = client.out
SERVER_EXE = server.out

# Default target
all: $(CLIENT_EXE) $(SERVER_EXE)

# Client without object file
$(CLIENT_EXE): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Server without object file
$(SERVER_EXE): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $^

# Clean
clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_EXE) $(SERVER_EXE)
