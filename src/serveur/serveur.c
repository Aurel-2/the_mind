#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#include "../../include/serveur.h"

pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t verrouarret = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t arret = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
Jeu *jeu;
int main(int argc, char const *argv[])
{
    printf("%s*****%s SERVEUR - THE MIND %s******\n%s", ROUGE, VERT, ROUGE, BLANC);
    srand(time(NULL));
    signal(SIGUSR1, gestion_signal);
    jeu = malloc(sizeof(Jeu));
    if (jeu == NULL)
    {
        perror("Erreur d'allocation mémoire pour jeu");
        exit(1);
    }

    jeu->vies = 2;
    jeu->tour = 0;
    jeu->manche = 1;
    jeu->carte_actuelle = 0;
    jeu->nb_clients = 0;
    for (int i = 0; i < 100; i++)
    {
        jeu->deck[i] = i + 1;
    }
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        jeu->liste_joueurs[i] = NULL;
    }
    jeu->etat = PRET;
    jeu->processus_pid = getpid();

    struct sockaddr_in adresse_serveur, adresse_client;
    socklen_t addr_len = sizeof(adresse_client);
    const int port = atoi(argv[1]);
    const int socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
    jeu->socket_serveur = socket_serveur;

    if (socket_serveur < 0)
    {
        perror("Erreur: Socket.");
        free(jeu);
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
        free(jeu);
        exit(1);
    }

    pthread_t jeu_logique;
    if (pthread_create(&jeu_logique, NULL, logique_jeu, (void *)jeu) != 0)
    {
        perror("Erreur lors de la création de thread logique_jeu");
        close(socket_serveur);
        free(jeu);
        exit(1);
    }

    listen(socket_serveur, 5);
    printf("%s\nServeur en attente sur le port : %d\n%s", JAUNE, port, BLANC);

    while (jeu->nb_clients < MAX_CLIENTS)
    {
        InfoClient *nouveau_client = malloc(sizeof(InfoClient));
        if (nouveau_client == NULL)
        {
            perror("Erreur d'allocation mémoire pour nouveau_client");
            continue;
        }

        nouveau_client->socket_client = accept(socket_serveur, (struct sockaddr *)&nouveau_client->client_addr, &addr_len);
        if (nouveau_client->socket_client < 0)
        {
            perror("Erreur d'acceptation de la connexion");
            free(nouveau_client);
            continue;
        }

        pthread_mutex_lock(&verrou);

        if (jeu->nb_clients >= MAX_CLIENTS)
        {
            printf("\nNombre maximum de clients atteint. Déconnexion du client.\n");
            close(nouveau_client->socket_client);
            free(nouveau_client);
            pthread_mutex_unlock(&verrou);
            continue;
        }

        jeu->nb_clients++;
        
        jeu->liste_joueurs[jeu->nb_clients - 1] = nouveau_client;
        printf("\nNouveau client connecté. Nombre de clients : %d\n", jeu->nb_clients);
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

        if (jeu->nb_clients == MAX_CLIENTS && jeu->etat == PRET)
        {
            printf("\nLe jeu commence.\n");
            jeu->etat = DISTRIBUTION;
            pthread_cond_broadcast(&cond);
        }
    }
    pthread_mutex_lock(&verrouarret);
    while (jeu->etat != FIN)
    {
        pthread_cond_wait(&arret, &verrouarret);
    }
    pthread_mutex_unlock(&verrouarret);
    pthread_cond_destroy(&arret);
    pthread_mutex_destroy(&verrouarret);
    pthread_mutex_lock(&verrou);
    free(jeu->cartes_jouee);
    free(jeu->carte_bon_ordre);
    close(jeu->socket_serveur);
    free(jeu);
    pthread_mutex_unlock(&verrou);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&verrou);
    pthread_join(jeu_logique, NULL);

    return 0;
}

void gestion_signal(int signal)
{
    printf("%s\nFermeture du serveur en cours...\n%s", VERT, BLANC);
    pthread_cond_signal(&arret);
    pthread_cond_signal(&cond);
}
