#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "include/server.h"

pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int nb_clients = 0;
Jeu *partie;

int main(int argc, char const *argv[])
{
    printf("***** SERVEUR - THE MIND ******\n");
    srand(time(NULL));
    signal(SIGINT, gestion_signal);
    partie = malloc(sizeof(Jeu));
    if (partie == NULL)
    {
        perror("Erreur d'allocation mémoire pour partie");
        exit(1);
    }
    partie->vies = 4;
    partie->tour = 0;
    partie->manche = 1;
    partie->carte_actuelle = 0;
    partie->cartes_jouee = malloc(sizeof(int) * MAX_CLIENTS);
    if (partie->cartes_jouee == NULL)
    {
        perror("Erreur d'allocation mémoire pour cartes_jouee");
        free(partie);
        exit(1);
    }
    for (size_t i = 0; i < 100; i++)
    {
        partie->deck[i] = i + 1;
    }
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        partie->liste_joueurs[i] = NULL;
    }
    partie->etat = PRET;
    partie->thread_principal = pthread_self();
    struct sockaddr_in adresse_serveur, adresse_client;
    socklen_t addr_len = sizeof(adresse_client);
    const int port = atoi(argv[1]);
    const int socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
    partie->socket_serveur = socket_serveur;
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
    pthread_t jeu_logique;
    if (pthread_create(&jeu_logique, NULL, logique_jeu, (void *)partie) != 0)
    {
        perror("Erreur lors de la création de thread logique_jeu");
        close(socket_serveur);
        free(partie->cartes_jouee);
        free(partie);
        exit(1);
    }
    pthread_detach(jeu_logique);

    listen(socket_serveur, 5);
    printf("\nServeur en attente sur le port : %d\n", port);

    while (1)
    {
        InfoClient *nouveau_client = malloc(sizeof(InfoClient));
        if (nouveau_client == NULL)
        {
            perror("Erreur d'allocation mémoire pour nouveau_client");
            continue;
        }
        nouveau_client->socket_client = accept(socket_serveur, (struct sockaddr *)&nouveau_client->client_addr,
                                               &addr_len);
        if (nouveau_client->socket_client < 0)
        {
            perror("Erreur d'acceptation de la connexion");
            free(nouveau_client);
            continue;
        }

        pthread_mutex_lock(&verrou);
        if (nb_clients >= MAX_CLIENTS)
        {
            printf("\nNombre maximum de clients atteint. Déconnexion du client.\n");
            close(nouveau_client->socket_client);
            free(nouveau_client);
            pthread_mutex_unlock(&verrou);
            continue;
        }

        nb_clients++;
        partie->liste_joueurs[nb_clients - 1] = nouveau_client;
        printf("\nNouveau client connecté. Nombre de clients : %d\n", nb_clients);
        pthread_mutex_unlock(&verrou);

        pthread_t thread;
        if (pthread_create(&thread, NULL, gestionnaire_client, (void *)nouveau_client) != 0)
        {
            perror("Erreur création de thread");
            close(nouveau_client->socket_client);
            free(nouveau_client);
            continue;
        }
        pthread_detach(thread);

        if (nb_clients == MAX_CLIENTS && partie->etat == PRET)
        {
            printf("\nLe jeu commence.\n");
            partie->etat = DISTRIBUTION;
            pthread_cond_broadcast(&cond);
        }
        if (partie->etat == FIN)
        {
            printf("\nTous les joueurs sont partis, fermeture du serveur.\n");
            break;
        }
    }
    printf("\nFermeture du serveur!\n");
    free(partie->cartes_jouee);
    close(partie->socket_serveur);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&verrou);
    free(partie);
    return 0;
}

void gestion_signal(int signal)
{
    printf("\nFermeture du serveur!\n");
    free(partie->cartes_jouee);
    close(partie->socket_serveur);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&verrou);
    free(partie);
    exit(0);
}
