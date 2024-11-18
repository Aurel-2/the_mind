#ifndef SERVER_H
#define SERVER_H
#define MAX_CLIENTS 1

typedef enum
{
    PRET,
    DISTRIBUTION,
    EN_JEU,
    MAUVAIS_TOUR,
    SCORE,
    FIN
} Status;

typedef struct Client
{
    char name[50];
    int socket_client;
    int list_cards[100];
    int bot;
    struct sockaddr_in client_addr;
} InfoClient;

typedef struct
{
    int lives;
    int deck[100];
    int round;
    int level;
    InfoClient *players_list[MAX_CLIENTS];
    int* played_cards;
    int previous_card;
    Status state;
} GameState;

void *client_handlers(void *arg);
void *game_logic(void *arg);
char *convert_tab(int *tab, int size);
void game_state(InfoClient *client);

#endif // SERVER_H