#ifndef SERVER_H
#define SERVER_H
#include <netinet/in.h>

#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

typedef enum
{
    PRET,
    DISTRIBUTION,
    EN_JEU,
    MAUVAIS_TOUR,
    SCORE,
    FIN
} Status;

typedef struct Client
{
    char pseudo[10];
    int socket_client;
    int liste_cartes[100];
    int robot;
    struct sockaddr_in client_addr;
} InfoClient;

typedef struct
{
    int vies;
    int deck[100];
    int tour;
    int manche;
    int *cartes_jouee;
    int carte_actuelle;
    Status etat;
    InfoClient *liste_joueurs[MAX_CLIENTS];
    int socket_serveur;
    pthread_t thread_principal;
} Jeu;

/* Fonctions pointeur pour les threads*/
void *gestionnaire_client(void *client);
void *logique_jeu(void *partie);
/* Envoi de l'état du jeu*/
void message_etat(InfoClient *client, char *message);
void envoi_etat(char *message);
void envoi_message(char *message);
/* Logique du jeu */
int est_valide();
void nouvelle_manche();
void distribution(Jeu *partie);
void melange_cartes(Jeu *partie);
/* Gestion du signal */
void gestion_signal(int signal);

#endif // SERVER_H