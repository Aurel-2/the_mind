CC = gcc
CFLAGS = -g -Wall -lpthread -fsanitize=address
PORT = 8080

SERVEUR_SRC = src/serveur/serveur.c
UTILS_SRC = src/serveur/utils.c
JEU_SRC = src/serveur/jeu.c
JEU_FCTS_SRC = src/serveur/jeu_fonctions.c
GESTIONNAIRE_SRC = src/serveur/gestionnaire_client.c
CLIENT_SRC =  src/client/client.c
SERVEUR_OBJ = $(SERVEUR_SRC:.c=.o) $(UTILS_SRC:.c=.o) $(JEU_SRC:.c=.o) $(GESTIONNAIRE_SRC:.c=.o) $(JEU_FCTS_SRC:.c=.o)
CLIENT_OBJ  = $(CLIENT_SRC:.c=.o)
SERVEUR_EXEC = serveur
CLIENT_EXEC = client

all: $(SERVEUR_EXEC) $(CLIENT_EXEC)

$(SERVEUR_EXEC): $(SERVEUR_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

run-server:
	./$(SERVEUR_EXEC) $(PORT)

run-client: 
	./$(CLIENT_EXEC) $(PORT)
clean:
	rm -f $(SERVEUR_OBJ) $(SERVEUR_EXEC) $(CLIENT_OBJ) $(CLIENT_EXEC)