/* Standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/* Libraries POSIX */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
/* Threads */
#include <pthread.h>
#include "include/server.h"
#define BUFFER_SIZE 1024

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int nb_clients = 0;
GameState *game;

int main(int argc, char const *argv[])
{
    srand(time(NULL));
    game = malloc(sizeof(GameState));
    game->lives = 4;
    game->round = 0;
    game->level = 1;
    game->previous_card = 0;
    game->played_cards = malloc(sizeof(int));
    for (size_t i = 0; i < 100; i++)
    {
        game->deck[i] = i + 1;
    }
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        game->players_list[i] = NULL;
    }
    game->state = PRET;

    struct sockaddr_in adresse_serveur, adresse_client;
    socklen_t addr_len = sizeof(adresse_client);
    const int port = atoi(argv[1]);
    const int socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_serveur < 0)
    {
        perror("Erreur: Socket.");
        exit(1);
    }

    memset(&adresse_serveur, 0, sizeof(adresse_serveur));
    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(port);
    adresse_serveur.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(socket_serveur, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0)
    {
        perror("Erreur: Binding.");
        close(socket_serveur);
        exit(1);
    }
    pthread_t gameL;
    pthread_create(&gameL, NULL, game_logic, (void *)game);
    pthread_detach(gameL);

    listen(socket_serveur, 5);
    printf("Serveur en attente sur le port : %d\n", port);

    while (game->state != FIN)
    {
        InfoClient *nouveau_client = malloc(sizeof(InfoClient));
        nouveau_client->socket_client = accept(socket_serveur, (struct sockaddr *)&nouveau_client->client_addr,
                                               &addr_len);
        if (nouveau_client->socket_client < 0)
        {
            perror("Erreur d'acceptation de la connexion");
            free(nouveau_client);
            continue;
        }

        pthread_mutex_lock(&lock);
        if (nb_clients >= MAX_CLIENTS)
        {
            printf("Nombre maximum de clients atteint. Déconnexion du client.\n");
            close(nouveau_client->socket_client);
            free(nouveau_client);
            pthread_mutex_unlock(&lock);
            continue;
        }

        nb_clients++;
        game->players_list[nb_clients - 1] = nouveau_client;
        printf("Nouveau client connecté. Nombre de clients : %d\n", nb_clients);
        pthread_mutex_unlock(&lock);

        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handlers, (void *)nouveau_client) != 0)
        {
            perror("Erreur création de thread");
            close(nouveau_client->socket_client);
            free(nouveau_client);
            continue;
        }
        pthread_detach(thread);

        if (nb_clients == MAX_CLIENTS && game->state == PRET)
        {
            printf("Le jeu commence.\n");
            game->state = DISTRIBUTION;
            pthread_cond_broadcast(&cond);
        }
    }
    close(socket_serveur);
    return 0;
}

void *game_logic(void *arg)
{
    GameState *game = (GameState *)arg;
    while (game->state != FIN)
    {
        pthread_mutex_lock(&lock);
        if (game->state == DISTRIBUTION)
        {
            sleep(1);
            printf("Mélange des cartes...\n");
            for (int i = 100 - 1; i > 0; i--)
            {
                int j = rand() % (i + 1);
                int tmp = game->deck[i];
                game->deck[i] = game->deck[j];
                game->deck[j] = tmp;
            }
            int cards_by_player = game->level;
            printf("Attribution des cartes aux joueurs...\n");
            for (int player_id = 0; player_id < MAX_CLIENTS; player_id++)
            {
                for (int card_id = 0; card_id < cards_by_player; card_id++)
                {
                    int index = player_id * cards_by_player + card_id;
                    game->players_list[player_id]->list_cards[card_id] = game->deck[index];
                }
            }
            game->state = EN_JEU;
            for (size_t i = 0; i < MAX_CLIENTS; i++)
            {
                game_state(game->players_list[i]);
            }

            pthread_cond_broadcast(&cond);
        }
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void *client_handlers(void *arg)
{
    InfoClient *info_client = (InfoClient *)arg;
    char buffer[BUFFER_SIZE];
    int lecture_octets;

    /* --Choix du nom-- */
    const ssize_t bytes = recv(info_client->socket_client, info_client->name, sizeof(info_client->name) - 1, 0);
    if (bytes > 0)
    {
        info_client->name[bytes - 1] = '\0';
    }
    if (strcmp(info_client->name, "bot") == 1)
    {
        info_client->bot = 1;
    }
    /* --Attente de joueurs-- */
    pthread_mutex_lock(&lock);
    while (nb_clients < MAX_CLIENTS)
    {
        snprintf(buffer, BUFFER_SIZE, "\e[31mLe jeu est sur le point de commencer...\e[37m\n");
        send(info_client->socket_client, buffer, strlen(buffer), 0);
        pthread_cond_wait(&cond, &lock);
    }
    pthread_mutex_unlock(&lock);

    while ((lecture_octets = recv(info_client->socket_client, buffer, BUFFER_SIZE, 0)) > 0 && game->state != FIN)
    {
        pthread_mutex_lock(&lock);
        if (game->state == DISTRIBUTION)
        {
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);
    }

    printf("Client %d déconnecté\n", info_client->socket_client);
    close(info_client->socket_client);
    pthread_mutex_lock(&lock);
    nb_clients--;
    pthread_mutex_unlock(&lock);
    free(info_client);
    return NULL;
}

void game_state(InfoClient *client)
{
    char buffer[BUFFER_SIZE];

    int server_size = game->level * MAX_CLIENTS;
    int player_size = game->level * MAX_CLIENTS;

    char *server_c = convert_tab(game->played_cards, server_size);
    char *player_c = convert_tab(client->list_cards, player_size);

    snprintf(buffer, BUFFER_SIZE, "--- Partie en cours ---\n Manche : %d\n Vies : %d\n Cartes jouées : %s\n ---------------------\n Vos cartes : %s\n",
             game->level, game->lives, server_c, player_c);

    send(client->socket_client, buffer, strlen(buffer), 0);
    free(server_c);
    free(player_c);
}

char *convert_tab(int *tab, int size)
{
    char *result = (char *)calloc(size * 4 + 1, sizeof(char));
    if (result == NULL)
    {
        perror("Erreur d'allocation mémoire pour convert_tab");
        return NULL;
    }
    result[0] = '\0';
    for (int i = 0; i < size; i++)
    {
        char buffer[16];
        sprintf(buffer, "%d", tab[i]);
        strcat(result, buffer);
        if (i < size - 1)
        {
            strcat(result, " ");
        }
    }

    return result;
}
