/* Standard */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Libraries POSIX */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
/* Threads */
#include <pthread.h>
#include <time.h>
/* ------ */
#include "server.h"

#define MAX_PLAYER 4
#define BUFFER_SIZE 1024

void start_game(ServerData *server_data)
{
    for (int i = 0; i < 100; i++)
    {
        server_data->game->cards[i] = i + 1;
    }
    server_data->game->lives = MAX_PLAYERS;
    server_data->game->level = 1;
    server_data->game->current_card = 0;
    server_data->game->played_count = 0;
    server_data->game->status = DISTRIBUTION;
    pthread_cond_broadcast(&server_data->cond);
}

int main(int argc, char const *argv[])
{
    srand(time(NULL));
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    ServerData server_data = {0};
    pthread_mutex_init(&server_data.mutex, NULL);
    pthread_cond_init(&server_data.cond, NULL);
    if (argc < 2)
    {
        perror("Error: No port provided.\n");
        exit(1);
    }
    int port = atoi(argv[1]);
    if (port <= 1024)
    {
        perror("Error: Select a port number greater than 1024.\n");
        exit(1);
    }
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error: Could not create socket.");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Error: Binding issue.");
        close(server_socket);
        exit(1);
    }
    listen(server_socket, 5);
    printf("Server waiting for connection on port %d...\n", port);
    server_data.game = malloc(sizeof(TheMind));
    if (server_data.game == NULL)
    {
        perror("Error: Game allocation issue");
        close(server_socket);
        exit(1);
    }
    server_data.game->status = WAITING;
    memset(server_data.list_players, 0, sizeof(server_data.list_players));
    while (1)
    {
       
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0)
        {
            perror("Error: Accepting client issue.");
            continue;
        }
        pthread_mutex_lock(&server_data.mutex);
        if (server_data.player_num < MAX_PLAYERS && server_data.game->status == WAITING)
        {
            server_data.list_players[server_data.player_num].socket = client_socket;

            ClientData *client_data = malloc(sizeof(ClientData));
            client_data->player_data = server_data.list_players[server_data.player_num];
            client_data->server_data = &server_data;

            pthread_t thread;
            if (pthread_create(&thread, NULL, client_handler, (void *)&client_data) != 0)
            {
                perror("Error: Thread creation issue.");
                close(client_socket);
                pthread_mutex_unlock(&server_data.mutex);
                continue;
            }
            pthread_detach(thread);
            server_data.player_num++;
            if (server_data.player_num == MAX_PLAYERS)
            {
                start_game(&server_data);
            }
            pthread_mutex_unlock(&server_data.mutex);
        }
        else
        {
            pthread_mutex_unlock(&server_data.mutex);
            printf("Room is full, connection refused.\n");
            close(client_socket);
        }
    }
    pthread_mutex_destroy(&server_data.mutex);
    pthread_cond_destroy(&server_data.cond);
    free(server_data.game);
    close(server_socket);
    return 0;
}
