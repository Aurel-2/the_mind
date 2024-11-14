CC = gcc
CFLAGS = -Wall -lpthread
PORT = 8080

SERVER_SRC = server.c 

SERVER_OBJ = $(SERVER_SRC:.c=.o)
SERVER_EXEC = server


CLIENT_SRC =  client.c
CLIENT_EXEC = client
CLIENT_OBJ  = $(CLIENT_SRC:.c=.o)

all: $(SERVER_EXEC) $(CLIENT_EXEC)

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

run-server: $(SERVER_EXEC)
	./$(SERVER_EXEC) $(PORT)

run-client: $(CLIENT_EXEC)
	./$(CLIENT_EXEC) $(PORT)
clean:
	rm -f $(SERVER_OBJ) $(SERVER_EXEC) $(CLIENT_OBJ) $(CLIENT_EXEC) 