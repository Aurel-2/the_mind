#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include "../../include/jeu_fonctions.h"
#include "../../include/utils.h"

void melange_cartes(Jeu *jeu)
{
    pthread_mutex_lock(&jeu->verrou_jeu);
    for (int i = 99; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int tmp = jeu->deck[i];
        jeu->deck[i] = jeu->deck[j];
        jeu->deck[j] = tmp;
    }
    pthread_mutex_unlock(&jeu->verrou_jeu);
}
void distribution(Jeu *jeu)
{

    int indice_tri = 0;
    pthread_mutex_lock(&jeu->verrou_jeu);
    for (int j = 0; j < jeu->max_clients; j++)
    {
        jeu->liste_joueurs[j]->liste_cartes = calloc(jeu->manche, sizeof(int));
        printf("%s\nCartes du joueur n° %d : ", JAUNE, j);
        for (int c = 0; c < jeu->manche; c++)
        {
            int indice = j * jeu->manche + c;
            jeu->liste_joueurs[j]->liste_cartes[c] = jeu->deck[indice];
            jeu->carte_bon_ordre[indice_tri] = jeu->deck[indice];
            indice_tri++;
            printf("%d ", jeu->deck[indice]);
        }
        printf("\n%s", BLANC);
    }
    tri(jeu->carte_bon_ordre, jeu->manche * jeu->max_clients);
    printf("\n%sCartes de tous les joueurs dans l'ordre croissant : ", JAUNE);
    for (int i = 0; i < jeu->max_clients * jeu->manche; i++)
    {
        printf("%d ", jeu->carte_bon_ordre[i]);
    }
    printf("\n%s", BLANC);

    pthread_mutex_unlock(&jeu->verrou_jeu);
}
void nouvelle_manche(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    pthread_mutex_lock(&jeu->verrou_jeu);

    if (jeu->manche == MAX_MANCHE)
    {
        snprintf(buffer, BUFFER_SIZE, "%s\nVous avez complété toutes les manches, bravo !\n%s", VERT, BLANC);
        envoi_message(jeu, buffer);
        jeu->etat = SCORE;
    }
    else
    {
        jeu->etat = DISTRIBUTION;
        jeu->tour = 0;
        jeu->carte_actuelle = 0;
        free(jeu->cartes_jouee);
        free(jeu->carte_bon_ordre);
    }
    for (int i = 0; i < jeu->max_clients; i++)
    {

        free(jeu->liste_joueurs[i]->liste_cartes);
    }
    pthread_mutex_unlock(&jeu->verrou_jeu);
}
void insertion_stats(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    float total_moyenne_react = 0.0;
    for (int i = 0; i < jeu->max_clients; i++)
    {
        total_moyenne_react += moyenne(jeu->liste_joueurs[i]->liste_temps_reaction, MAX_MANCHE * 2);
    }
    time_t t;
    time(&t);
    struct tm date = *localtime(&t);
    snprintf(buffer, BUFFER_SIZE, "%0.3f %d %d %d-%d-%d", total_moyenne_react, jeu->carte_actuelle, jeu->manche,
             date.tm_mday, date.tm_mon + 1, date.tm_year + 1900);

    FILE *fichier = fopen("donnees.txt", "a");
    if (fichier == NULL)
    {
        perror("Le fichier donnees.txt n'existe pas");
        kill(jeu->processus_pid, SIGINT);
    }

    fprintf(fichier, "%s\n", buffer);
    fclose(fichier);
}

void envoi_stats(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    char classement[BUFFER_SIZE] = "";
    char *commande = "cat donnees.txt | sort -k3,3nr | head -n 10";
    FILE *fichier = popen(commande, "r");
    if (fichier == NULL)
    {
        perror("Le fichier donnees.txt n'existe pas");
        kill(jeu->processus_pid, SIGINT);
    }

    strcat(classement, "\n--- Top 10 --- \n");
    while (fgets(buffer, sizeof(buffer), fichier) != NULL)
    {
        strcat(classement, buffer);
    }
    envoi_message(jeu, classement);
    pclose(fichier);
}

void message_etat(Jeu *jeu, InfoClient *client, char *message)
{
    char buffer[BUFFER_SIZE];
    int taille_serveur = jeu->manche * jeu->nb_clients;
    int taille_joueur = jeu->manche;
    char *cartes_serveur_c = int_to_string(jeu->cartes_jouee, taille_serveur);
    char *cartes_joueur_c = int_to_string(client->liste_cartes, taille_joueur);
    char *titre = (jeu->tour > 0) ? "--- Partie en cours ---" : "--- Début de la jeu ---";
    snprintf(buffer, BUFFER_SIZE,
             "\n%s\n%sManche : %d%s\n%sTour : %d / %d%s\n%sVies : %d%s\n%sCartes jouées : %s%s\n%sVos cartes : %s\n%sEtat de la jeu : %s%s%s\n-----------------------\n",
             titre,
             MAGENTA,
             jeu->manche,
             BLANC,
             BLEU,
             jeu->tour + 1,
             jeu->nb_clients * jeu->manche,
             BLANC,
             VERT,
             jeu->vies,
             BLANC,
             CYAN,
             cartes_serveur_c,
             BLANC,
             JAUNE,
             cartes_joueur_c,
             BLANC,
             VERT,
             message,
             BLANC);

    if (send(client->socket_client, buffer, strlen(buffer), 0) == -1)
    {
        perror("Erreur lors de l'envoi du message d'état.");
        kill(jeu->processus_pid, SIGINT);
    }
    free(cartes_serveur_c);
    free(cartes_joueur_c);
}

void envoi_etat(Jeu *jeu, char *message)
{
    for (int j = 0; j < jeu->max_clients; j++)
    {
        if (jeu->liste_joueurs[j] != NULL)
        {
            if (jeu->liste_joueurs[j]->robot == 0)
            {
                message_etat(jeu, jeu->liste_joueurs[j], message);
            }

        }
    }
}
void envoi_message(Jeu *jeu, char *message)
{
    for (int j = 0; j < jeu->max_clients; j++)
    {
        if (jeu->liste_joueurs[j] != NULL)
        {
            if (jeu->liste_joueurs[j]->robot == 0)
            {
                send(jeu->liste_joueurs[j]->socket_client, message, strlen(message), 0);
            }
            else
            {
                if (jeu->etat == SCORE)
                {
                    if (strlen(message) <= 2)
                    {
                        send(jeu->liste_joueurs[j]->socket_client, message, strlen(message), 0);
                    }
                }
            }
        }
    }
}

void message_robot(Jeu *jeu, InfoClient *client)
{
    char buffer_robot[BUFFER_SIZE];
    char *cartes_robot = int_to_string(client->liste_cartes, jeu->manche);
    snprintf(buffer_robot, BUFFER_SIZE, "%s", cartes_robot);
    if (send(client->socket_client, buffer_robot, strlen(buffer_robot), 0) == -1)
    {
        perror("Erreur lors de l'envoi du tableau du robot");
        kill(jeu->processus_pid, SIGINT);
    }
    free(cartes_robot);
}