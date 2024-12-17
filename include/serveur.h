#ifndef SERVEUR_H
#define SERVEUR_H
#include <netinet/in.h>
#include <sys/time.h>



#define BUFFER_SIZE 1024
#define MAX_MANCHE 12
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
    int socket_client;
    char pseudo[BUFFER_SIZE];
    int *liste_cartes;
    int robot;
    float *liste_temps_reaction;
    struct sockaddr_in client_addr;
} InfoClient;

typedef struct
{
    pid_t processus_pid;
    InfoClient **liste_joueurs;
    pthread_mutex_t verrou_jeu;
    pthread_cond_t cond_jeu;
    pthread_mutex_t verrou_serveur;
    pthread_cond_t cond_serveur;
    int socket_serveur;
    int vies;
    int deck[100];
    int tour;
    int manche;
    int *cartes_jouee;
    int *carte_bon_ordre;
    int carte_actuelle;
    int nb_clients;
    int max_clients;
    Status etat;
} Jeu;

/* SERVEUR */
void gestion_signal(int signal);
void nettoyage();
void free_liste_joueurs();

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
