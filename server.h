#ifndef SERVER_H
#define SERVER_H
#include "game.h" 
#include <pthread.h>

typedef struct {
    int player_num;
    TheMind *game;
    Player list_players[MAX_PLAYERS];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ServerData;

typedef struct 
{
    Player player_data;
    ServerData* server_data;
} ClientData;

void *client_handler(void *p);
void start_game(ServerData *server_data);

#endif // SERVER_H
