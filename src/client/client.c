#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define SERVEUR_ADDR "127.0.0.1"
pthread_mutex_t verrou_jeu = PTHREAD_MUTEX_INITIALIZER;
int en_connexion = 1;

void *reception_msg(void *arg)
{
    int serveur_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    ssize_t taille_reponse;
    while ((taille_reponse = recv(serveur_socket, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[taille_reponse] = '\0';
        if (strcmp(buffer, "q") == 0)
        {
            pthread_mutex_lock(&verrou_jeu);
            en_connexion = 0;
            pthread_mutex_unlock(&verrou_jeu);
            printf("Déconnexion demandée par le serveur\n");
            return NULL;
        }
        printf("%s\n", buffer);
    }

    if (taille_reponse == 0)
    {
        printf("Fermeture du client\n");
    }
    else if (taille_reponse < 0)
    {
        perror("Erreur de réception");
    }

    pthread_mutex_lock(&verrou_jeu);
    en_connexion = 0;
    pthread_mutex_unlock(&verrou_jeu);
    return NULL;
}

int main(int argc, char const *argv[])
{
    const char *bienvenue = "cat 'welcome'";
    system(bienvenue);
    printf("\nVeuillez choisir un nom :\n");
    char buffer[BUFFER_SIZE];
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
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
    adresse_serveur.sin_addr.s_addr = inet_addr(SERVEUR_ADDR);

    if (connect(serveur_socket, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0)
    {
        perror("La connexion a échoué");
        close(serveur_socket);
        exit(1);
    }
    send(serveur_socket, buffer, strlen(buffer), 0);
    pthread_t thread_reception;
    pthread_create(&thread_reception, NULL, reception_msg, &serveur_socket);
    while (en_connexion)
    {
        char message[BUFFER_SIZE];
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0;
        if (strcmp(message, "q") == 0)
        {
            en_connexion = 0;
            break;
        }
        if (send(serveur_socket, message, strlen(message), 0) < 0)
        {
            perror("Erreur lors de l'envoi du message");
            break;
        }
    }
    pthread_cancel(thread_reception);
    pthread_join(thread_reception, NULL);
    pthread_mutex_destroy(&verrou_jeu);
    close(serveur_socket);
    return 0;
}
