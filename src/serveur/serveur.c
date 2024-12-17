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
#include "../../include/utils.h"

Jeu *jeu;
pthread_t jeu_logique;
void free_liste_joueurs()
{
    for (int i = 0; i < jeu->max_clients; i++)
    {
        if (jeu->liste_joueurs[i] != NULL)
        {
            free(jeu->liste_joueurs[i]);
        }
    }

    free(jeu->liste_joueurs);
}
int main(int argc, char const *argv[])
{
    printf("%s*****%s SERVEUR - THE MIND %s******\n%s", ROUGE, VERT, ROUGE, BLANC);
    srand(time(NULL));
    signal(SIGINT, gestion_signal);
    signal(SIGTERM, gestion_signal);
    jeu = malloc(sizeof(Jeu));
    if (jeu == NULL)
    {
        perror("Erreur d'allocation mémoire pour jeu");
        exit(1);
    }
    printf("%s\nVeuillez saisir un nombre maximum entre 1 et 8 : \n%s", VERT, BLANC);
    scanf("%d", &jeu->max_clients);

    jeu->vies = 2;
    jeu->tour = 0;
    jeu->manche = 1;
    jeu->carte_actuelle = 0;
    jeu->nb_clients = 0;
    jeu->etat = PRET;
    jeu->processus_pid = getpid();

    pthread_mutex_init(&jeu->verrou_jeu, NULL);
    pthread_cond_init(&jeu->cond_jeu, NULL);
    pthread_mutex_init(&jeu->verrou_serveur, NULL);
    pthread_cond_init(&jeu->cond_serveur, NULL);
    for (int i = 0; i < 100; i++)
    {
        jeu->deck[i] = i + 1;
    }
    jeu->liste_joueurs = malloc(sizeof(InfoClient *) * jeu->max_clients);
    for (int i = 0; i < jeu->max_clients; i++)
    {
        jeu->liste_joueurs[i] = malloc(sizeof(InfoClient));
    }
    struct sockaddr_in adresse_serveur, adresse_client;
    socklen_t addr_len = sizeof(adresse_client);
    const int port = atoi(argv[1]);
    const int socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
    jeu->socket_serveur = socket_serveur;

    if (socket_serveur < 0)
    {
        perror("Erreur: Socket.");
        free_liste_joueurs();
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
        free_liste_joueurs();
        free(jeu);
        exit(1);
    }

    if (pthread_create(&jeu_logique, NULL, logique_jeu, (void *)jeu) != 0)
    {
        perror("Erreur lors de la création de logique_jeu");
        close(socket_serveur);
        free_liste_joueurs();
        free(jeu);
        exit(1);
    }

    listen(socket_serveur, 5);
    printf("%s\nServeur en attente sur le port : %d\n%s", JAUNE, port, BLANC);

    while (jeu->nb_clients < jeu->max_clients)
    {
        InfoClient *nouveau_client = malloc(sizeof(InfoClient));


        nouveau_client->socket_client = accept(socket_serveur, (struct sockaddr *)&nouveau_client->client_addr, &addr_len);
        if (nouveau_client->socket_client < 0)
        {
            perror("Erreur d'acceptation de la connexion");
            free(nouveau_client);
            continue;
        }

        pthread_mutex_lock(&jeu->verrou_jeu);

        if (jeu->nb_clients >= jeu->max_clients)
        {
            printf("%s\nNombre maximum de clients atteint.\nDéconnexion du client.\n%s", ROUGE, BLANC);
            close(nouveau_client->socket_client);
            free(nouveau_client);
            pthread_mutex_unlock(&jeu->verrou_jeu);
            continue;
        }

        jeu->liste_joueurs[jeu->nb_clients] = nouveau_client;
        printf("\nNouveau client connecté.\nNombre de clients : %d/%d\n", jeu->nb_clients, jeu->max_clients);
        jeu->nb_clients++;
        
        pthread_mutex_unlock(&jeu->verrou_jeu);

        pthread_t thread_client;
        if (pthread_create(&thread_client, NULL, gestionnaire_client, (void *)nouveau_client) != 0)
        {
            perror("Erreur création de thread_client");
            close(nouveau_client->socket_client);
            free(nouveau_client);
            jeu->nb_clients--;
            continue;
        }
        pthread_detach(thread_client);

        if (jeu->nb_clients == jeu->max_clients && jeu->etat == PRET)
        {
            printf("%s\nLe jeu commence.\n%s", VERT, BLANC);
            jeu->etat = DISTRIBUTION;
            pthread_cond_broadcast(&jeu->cond_jeu);
        }
    }

    pthread_mutex_lock(&jeu->verrou_serveur);
    while (jeu->etat != FIN)
    {
        pthread_cond_wait(&jeu->cond_serveur, &jeu->verrou_serveur);
    }
    pthread_mutex_unlock(&jeu->verrou_serveur);
    nettoyage();
    return 0;
}

void nettoyage()
{
    printf("\n%sNettoyage du serveur en cours...\n%s", CYAN, BLANC);

    pthread_cond_destroy(&jeu->cond_serveur);
    pthread_mutex_destroy(&jeu->verrou_serveur);
    pthread_mutex_lock(&jeu->verrou_jeu);
    free_liste_joueurs();
    free(jeu->cartes_jouee);
    free(jeu->carte_bon_ordre);
    if (jeu->socket_serveur > 0)
    {
        close(jeu->socket_serveur);
    }

    free(jeu);
    pthread_mutex_unlock(&jeu->verrou_jeu);
    pthread_cond_destroy(&jeu->cond_jeu);
    pthread_mutex_destroy(&jeu->verrou_jeu);
    pthread_join(jeu_logique, NULL);
    printf("\n%sTerminé!\n%s", CYAN, BLANC);
}
void gestion_signal(int signal)
{

    printf("\n%sLe jeu a été interrompu par un signal !\n%s", ROUGE, BLANC);
    if (jeu != NULL)
    {
        jeu->etat = FIN;
        pthread_cond_broadcast(&jeu->cond_jeu);
        nettoyage();
        exit(0);
    }
}
