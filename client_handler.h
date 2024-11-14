#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H
#include "server.h"

void distribute_cards_all_players(TheMind *game, char *buffer, ServerData *server_data);
void send_to_all_players(ServerData *server_data, const char *message);
int is_valid(TheMind *game, const Player *player);
void update_game_state(TheMind *game);
void shuffle(int *tab, int size);
#endif 
