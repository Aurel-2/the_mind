CC = gcc
CFLAGS = -Wall -lpthread
SRC = server.c client_handler.c 
OBJ = $(SRC:.c=.o)
EXEC = server
all: $(EXEC)
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^
%.o: %.c game.h
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJ) $(EXEC)
run: $(EXEC)
	./$(EXEC) 8080
