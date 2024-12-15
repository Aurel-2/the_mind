#ifndef SERVEUR_H
#define SERVEUR_H
#include <netinet/in.h>
#include <sys/time.h>


#define BLANC "\033[0m"
#define ROUGE "\033[31m"
#define VERT "\033[32m"
#define JAUNE "\033[33m"
#define BLEU "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"

#define BUFFER_SIZE 1024
#define MAX_MANCHE 12
#define MAX_CLIENTS 2

typedef enum {
    PRET,
    DISTRIBUTION,
    EN_JEU,
    MAUVAIS_TOUR,
    SCORE,
    FIN
} Status;

typedef struct Client {
    int socket_client;
    char pseudo[BUFFER_SIZE];
    int *liste_cartes;
    int robot;
    float *liste_temps_reaction;
    struct sockaddr_in client_addr;
} InfoClient;

typedef struct {
    pid_t processus_pid;
    InfoClient *liste_joueurs[MAX_CLIENTS];
    int socket_serveur;
    int vies;
    int deck[100];
    int tour;
    int manche;
    int *cartes_jouee;
    int *carte_bon_ordre;
    int carte_actuelle;
    int nb_clients;
    Status etat;
} Jeu;

/* SERVEUR */
void gestion_signal(int signal);

/* CLIENT */
void *gestionnaire_client(void *p_client);

float calcul_temps_reaction(struct timeval debut, struct timeval fin);

int traitement_carte_jouee(InfoClient *info_client, int indice, float temps, int indice_temps);

void affichage_donnees_client(InfoClient *info_client, int indice, float temps);

void deconnexion_client(InfoClient *info_client);

/* JEU */
void *logique_jeu(void *p_jeu);

void phase_distribution(Jeu *jeu);

void phase_enjeu(Jeu *jeu);

void phase_mauvaistour(Jeu *jeu);

void phase_score(Jeu *jeu);

#endif
