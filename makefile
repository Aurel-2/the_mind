CC = gcc
CFLAGS = -g -Wall -lpthread
PORT = 8080

SERVER_SRC = server.c 
UTILS_SRC = utils.c
JEU_SRC = jeu.c
GESTIONNAIRE_SRC = gestionnaire_client.c
CLIENT_SRC =  client.c
SERVER_OBJ = $(SERVER_SRC:.c=.o) $(UTILS_SRC:.c=.o) $(JEU_SRC:.c=.o) $(GESTIONNAIRE_SRC:.c=.o)
CLIENT_OBJ  = $(CLIENT_SRC:.c=.o)
SERVER_EXEC = server
CLIENT_EXEC = client

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