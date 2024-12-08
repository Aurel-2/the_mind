#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "include/server.h"
#include "include/utils.h"
extern pthread_mutex_t verrou;
extern pthread_cond_t cond;
extern int nb_clients;
extern Jeu *partie;

void *logique_jeu(void *arg)
{
    Jeu *partie = (Jeu *)arg;
    while (partie->etat != FIN)
    {
        pthread_mutex_lock(&verrou);
        switch (partie->etat)
        {
        case DISTRIBUTION:
            printf("\n--- DISTRIBUTION ---\n");
            melange_cartes(partie);
            distribution(partie);
            partie->etat = EN_JEU;
            envoi_etat('\0');
            pthread_cond_broadcast(&cond);
            break;
        case EN_JEU:
            printf("\n--- EN JEU ---\n");
            while (partie->etat == EN_JEU)
            {
                printf("\nAttente de l'indice d'une carte d'un joueur...\n");
                pthread_cond_broadcast(&cond);
                pthread_cond_wait(&cond, &verrou);
                if (est_valide())
                {
                    char buffer[BUFFER_SIZE];
                    partie->cartes_jouee[partie->tour] = partie->carte_actuelle;
                    printf("\nLa carte jouée : %d\n", partie->cartes_jouee[partie->tour]);

                    if (partie->tour == 0)
                    {
                        snprintf(buffer, BUFFER_SIZE, "C'est le premier tour.\n");
                        envoi_etat(buffer);
                    }
                    else
                    {
                        printf(buffer, BUFFER_SIZE, "Un joueur a joué une bonne carte\n");
                        envoi_etat(buffer);
                    }
                    partie->tour++;
                }
                else
                {
                    partie->etat = MAUVAIS_TOUR;
                    break;
                }

                if (partie->tour == nb_clients * partie->manche && partie->etat != MAUVAIS_TOUR)
                {
                    partie->manche++;
                    char buffer[BUFFER_SIZE];
                    snprintf(buffer, BUFFER_SIZE, "Manche réussie! Vous passez à la manche %d\n", partie->manche);
                    envoi_etat(buffer);
                    nouvelle_manche();
                    break;
                }
            }
            break;
        case MAUVAIS_TOUR:
            partie->vies--;
            if (partie->vies == 0)
            {
                char buffer[BUFFER_SIZE];
                snprintf(buffer, BUFFER_SIZE, "La partie est perdue, affichage du leaderboard.\n");
                envoi_etat(buffer);
                partie->etat = SCORE;
            }
            else
            {
                char buffer[BUFFER_SIZE];
                snprintf(buffer, BUFFER_SIZE, "Un joueur a joué une mauvaise carte\n");
                envoi_etat(buffer);
                nouvelle_manche();
            }
            break;
        case SCORE:
            partie->etat = FIN;
            break;
        default:
            break;
        }
        pthread_mutex_unlock(&verrou);
    }
    return NULL;
}

void melange_cartes(Jeu *partie)
{
    for (int i = 99; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int tmp = partie->deck[i];
        partie->deck[i] = partie->deck[j];
        partie->deck[j] = tmp;
    }
}

void distribution(Jeu *partie)
{
    for (int id_j = 0; id_j < nb_clients; id_j++)
    {

        printf("\nCartes du joueur n° %d : ", id_j);
        for (int id_c = 0; id_c < partie->manche; id_c++)
        {
            int indice = id_j * partie->manche + id_c;
            partie->liste_joueurs[id_j]->liste_cartes[id_c] = partie->deck[indice];
            printf("%d ", partie->deck[indice]);
        }
        printf("\n");
    }
}

int est_valide()
{
    if (partie->tour == 0)
        return 1;
    return partie->carte_actuelle > partie->cartes_jouee[partie->tour - 1];
}

void nouvelle_manche()
{
    partie->etat = DISTRIBUTION;
    partie->tour = 0;
    partie->carte_actuelle = 0;
    free(partie->cartes_jouee);
    partie->cartes_jouee = calloc(nb_clients, sizeof(int));
}

void message_etat(InfoClient *client, char *message)
{
    char buffer[BUFFER_SIZE];
    int taille_serveur = partie->manche * nb_clients;
    int taille_joueur = partie->manche;
    char *cartes_jouees_c = tab_to_string(partie->cartes_jouee, taille_serveur);
    char *cartes_joueur_c = tab_to_string(client->liste_cartes, taille_joueur);

    const char *titre = (partie->tour > 0) ? "--- Partie en cours ---" : "--- Début de la partie ---";

    snprintf(buffer, sizeof(buffer),
             "\n%s\nManche : %d\nTour : %d / %d\nVies : %d\nCartes jouées : %s\nVos cartes : %s\nEtat de la partie : %s\n",
             titre,
             partie->manche,
             partie->tour,
             nb_clients * partie->manche,
             partie->vies,
             cartes_jouees_c,
             cartes_joueur_c,
             message);
    send(client->socket_client, buffer, sizeof(buffer), 0);
    free(cartes_jouees_c);
    free(cartes_joueur_c);
}

void envoi_etat(char *message)
{
    for (int i = 0; i < nb_clients; i++)
    {
        if (partie->liste_joueurs[i] != NULL)
        {
            message_etat(partie->liste_joueurs[i], message);
        }
    }
}
void envoi_message(char *message)
{
    for (int i = 0; i < nb_clients; i++)
    {
        if (partie->liste_joueurs[i] != NULL)
        {
            send(partie->liste_joueurs[i]->socket_client, message, sizeof(message), 0);
        }
    }
}