#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define SERVEUR_ADDR "127.0.0.1"

typedef struct {
    pthread_mutex_t verrou_jeu;
    int en_connexion;
    int serveur_socket;
} Client;

// Fonction pour recevoir des messages du serveur
void *reception_msg(void *arg)
{
    Client *client = (Client *)arg;
    char buffer[BUFFER_SIZE];
    ssize_t taille_reponse;

    while ((taille_reponse = recv(client->serveur_socket, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[taille_reponse] = '\0';
        if (strcmp(buffer, "q") == 0)
        {
            pthread_mutex_lock(&client->verrou_jeu);
            client->en_connexion = 0; // Mise à jour de l'état de connexion
            pthread_mutex_unlock(&client->verrou_jeu);
            printf("Déconnexion demandée par le serveur\n");
            return NULL;
        }
        printf("%s\n", buffer); // Affiche le message reçu
    }

    if (taille_reponse == 0)
    {
        printf("Fermeture du client\n");
    }
    else if (taille_reponse < 0)
    {
        perror("Erreur de réception");
    }

    pthread_mutex_lock(&client->verrou_jeu);
    client->en_connexion = 0; // Mise à jour de l'état de connexion
    pthread_mutex_unlock(&client->verrou_jeu);
    return NULL;
}

int main(int argc, char const *argv[])
{
    const char *bienvenue = "cat 'welcome'";
    system(bienvenue); // Affiche le message de bienvenue
    printf("\nVeuillez choisir un nom :\n");

    char buffer[BUFFER_SIZE];
    fgets(buffer, sizeof(buffer), stdin); // Lecture du nom choisi par l'utilisateur
    buffer[strcspn(buffer, "\n")] = 0;

    Client client;
    client.en_connexion = 1;
    pthread_mutex_init(&client.verrou_jeu, NULL);

    int serveur_socket;
    const int port = atoi(argv[1]);
    struct sockaddr_in adresse_serveur;

    if ((serveur_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Problème lors de la création du socket");
        exit(1);
    }

    client.serveur_socket = serveur_socket;

    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(port);
    adresse_serveur.sin_addr.s_addr = inet_addr(SERVEUR_ADDR);

    // Connexion au serveur
    if (connect(serveur_socket, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0)
    {
        perror("La connexion a échoué");
        close(serveur_socket);
        exit(1);
    }
    
    // Envoi du nom choisi au serveur
    send(serveur_socket, buffer, strlen(buffer), 0);

    // Création du thread pour gérer la réception des messages
    pthread_t thread_reception;
    pthread_create(&thread_reception, NULL, reception_msg, &client);
    
    // Boucle principale pour envoyer des messages au serveur
    while (client.en_connexion)
    {
        char message[BUFFER_SIZE];
        fgets(message, sizeof(message), stdin); // Lecture du message depuis l'entrée standard
        message[strcspn(message, "\n")] = 0;
        if (strcmp(message, "q") == 0)
        {
            client.en_connexion = 0; // Demande de déconnexion
            break;
        }
        // Envoi du message au serveur
        if (send(serveur_socket, message, strlen(message), 0) < 0)
        {
            perror("Erreur lors de l'envoi du message");
            break;
        }
    }

    pthread_cancel(thread_reception);
    pthread_join(thread_reception, NULL);
    pthread_mutex_destroy(&client.verrou_jeu);
    close(serveur_socket);
    return 0;
}
