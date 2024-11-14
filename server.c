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
/* ------ */
#include "server.h"
#define MIN_PLAYERS 2

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int active_connection = 0;
int next_client_id = 1;
pthread_t *client_handlers = NULL;  


int main(int argc, char const *argv[])
{
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int port = atoi(argv[1]);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Erreur: Socket.");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Erreur: Binding.");
        close(server_socket);
        exit(1);
    }

    listen(server_socket, 5);
    printf("Serveur en attente sur le port : %d\n", port);
    client_handlers = malloc(MIN_PLAYERS * sizeof(pthread_t));  
    while (next_client_id <= MIN_PLAYERS)
    {
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
        pthread_mutex_lock(&client_lock);
        int client = next_client_id++;
        active_connection++; 
        printf("Connexion acceptée: %d - Nombre de connexions : %d\n", inet_ntoa(client_addr.sin_addr), active_connection);
        pthread_create(&client_handlers[client - 1], NULL, client_handler, &client_socket);
        pthread_detach(client_handlers[client - 1]);  
        pthread_mutex_unlock(&client_lock);
    }

    close(server_socket);
    free(client_handlers);  
    return 0;
}

void *client_handler(void *arg)
{
    int sock = *((int *)arg);
    printf("Gestion du client n° %d\n", sock);
    wait_players();
    sleep(2); 
    pthread_mutex_lock(&client_lock);
    active_connection--;
    printf("Client %d déconnecté. Nombre de connexions restantes: %d\n", sock, active_connection);
    pthread_mutex_unlock(&client_lock);

    return NULL;
}


void wait_players()
{
    pthread_mutex_lock(&lock);
    while (active_connection < MIN_PLAYERS)
    {
        printf("En attente de joueurs...\n");
        pthread_cond_wait(&cond, &lock);  
    }

    pthread_cond_broadcast(&cond);  
    pthread_mutex_unlock(&lock);
}
