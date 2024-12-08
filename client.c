#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define SERVER_ADDR "127.0.0.1"

void *receive_messages(void *arg)
{
    int serveur_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    ssize_t taille_reponse;
    while ((taille_reponse = recv(serveur_socket, buffer, sizeof(buffer), 0)) > 0)
    {
        buffer[taille_reponse] = '\0';
        printf("%s\n", buffer);
    }
    return NULL;
}

int main(int argc, char const *argv[])
{
    const char *bienvenue = "cat 'welcome'";
    system(bienvenue);
    printf("Veuillez choisir un nom :\n");
    char buffer[BUFFER_SIZE];
    fgets(buffer, sizeof(buffer), stdin);
    int serveur_socket;
    const int port = atoi(argv[1]);
    struct sockaddr_in adresse_serveur;
    if ((serveur_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Problème lors de la création du socket");
        exit(1);
    }

    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(port);
    adresse_serveur.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (connect(serveur_socket, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0)
    {
        perror("La connexion a échoué");
        exit(1);
    }

    pthread_t thread_reception;
    pthread_create(&thread_reception, NULL, receive_messages, &serveur_socket);

    while (1)
    {
        char buffer[BUFFER_SIZE];
        fgets(buffer, sizeof(buffer)-1, stdin);
        send(serveur_socket, buffer, strlen(buffer), 0);
    }
    close(serveur_socket);
    return 0;
}
