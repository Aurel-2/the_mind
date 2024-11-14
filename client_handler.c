#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include "client_handler.h"
#define BUFFER_SIZE 1024

void *client_handler(void *game_data)
{
    ClientData *client_data = (ClientData *)game_data;
    Player *player = &client_data->player_data;
    TheMind *game = client_data->server_data->game;
    ServerData *server_data = client_data->server_data;
    char buffer[BUFFER_SIZE];

    snprintf(buffer, BUFFER_SIZE, "Bienvenue dans le jeu ! Veuillez choisir un nom :\n");
    send(player->socket, buffer, strlen(buffer), 0);

    int bytes = recv(player->socket, player->name, sizeof(player->name) - 1, 0);
    if (bytes > 0)
    {
        player->name[bytes] = '\0';
    }

    snprintf(buffer, BUFFER_SIZE, "Le jeu va commencer. Veuillez patienter %s...\n", player->name);
    send(player->socket, buffer, strlen(buffer), 0);

    pthread_mutex_lock(&server_data->mutex);
    while (game->status == WAITING)
    {
        printf("Le joueur %s est dans la file d'attente...\n", player->name);
        pthread_cond_wait(&server_data->cond, &server_data->mutex);
    }
    pthread_mutex_unlock(&server_data->mutex);

    while (game->status != END)
    {

        if (game->status == DISTRIBUTION)
        {
            distribute_cards_all_players(game, buffer, server_data);
        }
        if (game->status == PLAYING)
        {
            bytes = recv(player->socket, buffer, BUFFER_SIZE - 1, 0);
            buffer[bytes] = '\0';
            pthread_mutex_lock(&server_data->mutex);
            game->current_card = atoi(buffer);
            if (is_valid(game, player))
            {
                if (game->played_count < MAX_PLAYERS*game->level)
                {
                    snprintf(buffer, BUFFER_SIZE, "%s a joué la carte %d !\n", player->name, game->current_card);
                    send_to_all_players(server_data, buffer);
                }
                else
                {
                    snprintf(buffer, BUFFER_SIZE, "Manche réussie !\n %s a joué la carte %d !\n",
                             player->name, game->current_card);
                    send_to_all_players(server_data, buffer);
                }
            }
            else
            {
                snprintf(buffer, BUFFER_SIZE, "Carte %d est plus petite !, %s a fait perdre la manche.\n",
                         game->current_card, player->name);
                send_to_all_players(server_data, buffer);
                game->status = BAD_ROUND;
            }
            update_game_state(game);
            pthread_mutex_unlock(&server_data->mutex);
        }

        if (game->status == SCORING)
        {
            puts("Mettre traitement des informations ici");
        }
    }

    close(player->socket);
    return NULL;
}

void distribute_cards_all_players(TheMind *game, char *buffer, ServerData *server_data)
{
    shuffle(game->cards, 100);
    int cards_by_player = game->level;
    if (cards_by_player > 25)
    {
        fprintf(stderr, "Pas assez de cartes pour distribuer à tous les joueurs.\n");
        game->status == SCORING;
        return;
    }
    for (int player_id = 0; player_id < MAX_PLAYERS; player_id++)
    {
        for (int card_id = 0; card_id < cards_by_player; card_id++)
        {
            int index = player_id * cards_by_player + card_id;
            snprintf(buffer, BUFFER_SIZE, "%d", game->cards[index]);
            send(server_data->list_players[player_id].socket, buffer, strlen(buffer), 0);
        }
    }
}

int is_valid(TheMind *game, const Player *player)
{
    // Si aucune carte n'a encore été jouée, la carte est valide
    if (game->played_count == 0)
    {
        return 1;
    }
    // Vérifie si la carte actuelle est plus grande que la dernière carte jouée
    return player->card[game->played_count] > game->played_cards[game->played_count - 1];
}

void update_game_state(TheMind *game)
{
    if (game->played_count < MAX_PLAYERS*game->level)
    {
        game->played_cards[game->played_count] = game->current_card;
        game->played_count++;
    }
    else if (game->played_count == MAX_PLAYERS*game->level)
    {
        game->level++;
        game->status = DISTRIBUTION;
        game->played_count = 0;
    }
    else if (game->status == BAD_ROUND)
    {
        game->lives--;
        game->played_count = 0;
        if (game->lives <= 0)
        {
            game->status = SCORING;
        }
        else
        {
            game->level = 1;
            game->status = DISTRIBUTION;
        }

    }
}

void send_to_all_players(ServerData *server_data, const char *message)
{
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "%s", message);
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        send(server_data->list_players[i].socket, buffer, strlen(buffer), 0);
    }
}
void shuffle(int *cards_list, int size)
{
    for (int i = size - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int tmp = cards_list[i];
        cards_list[i] = cards_list[j];
        cards_list[j] = tmp;
    }
}