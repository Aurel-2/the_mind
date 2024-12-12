#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "include/jeu_fonctions.h"
#include "include/utils.h"

void melange_cartes(Jeu *jeu)
{
    for (int i = 99; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int tmp = jeu->deck[i];
        jeu->deck[i] = jeu->deck[j];
        jeu->deck[j] = tmp;
    }
}
void distribution(Jeu *jeu)
{
    int indice_tri = 0;
    for (int j = 0; j < jeu->nb_clients; j++)
    {
        if (jeu->liste_joueurs[j] != NULL)
        {
            jeu->liste_joueurs[j]->liste_cartes = calloc(jeu->manche, sizeof(int));
            if (jeu->liste_joueurs[j]->liste_cartes == NULL)
            {
                printf("\nErreur d'allocation mémoire pour les cartes du joueur %d.\n", j);
                exit(1);
            }
        }
        printf("%s\nCartes du joueur n° %d : ", MAGENTA, j);
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
    tri(jeu->carte_bon_ordre, jeu->manche * jeu->nb_clients);
    printf("\n%sCartes de tous les joueurs dans l'ordre croissant : ", MAGENTA);
    for (int i = 0; i < jeu->nb_clients * jeu->manche; i++)
    {
        printf("%d ", jeu->carte_bon_ordre[i]);
    }
    printf("\n%s", BLANC);
}

void nouvelle_manche(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
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
        if (jeu->cartes_jouee == NULL || jeu->carte_bon_ordre == NULL)
        {
            printf("\nErreur d'allocation mémoire pour les cartes de la nouvelle manche.\n");
            exit(1);
        }
    }

    for (int i = 0; i < jeu->nb_clients; i++)
    {

        free(jeu->liste_joueurs[i]->liste_cartes);
    }
}

void insertion_stats(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    float total_moyenne_react = 0.0;
    for (int i = 0; i < jeu->nb_clients; i++)
    {
        total_moyenne_react += moyenne(jeu->liste_joueurs[i]->liste_temps_reaction, MAX_MANCHE * 2);
    }

    time_t t;
    time(&t);
    struct tm date = *localtime(&t);
    snprintf(buffer, BUFFER_SIZE, "%0.3f %d %d %d-%d-%d", total_moyenne_react, jeu->carte_actuelle, jeu->manche,
             date.tm_mday, date.tm_mon + 1, date.tm_year + 1900);

    FILE *fichier = fopen("statistiques.txt", "a");
    if (fichier == NULL)
    {
        printf("\nErreur lors de l'ouverture du fichier de statistiques.\n");
        exit(1);
    }
    fprintf(fichier, "%s\n", buffer);
    fclose(fichier);
}

void envoi_stats(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    char classement[BUFFER_SIZE] = "";
    char *commande = "awk '{print $0}' statistiques.txt | sort -k3,3nr | head -n 10";
    FILE *fichier = popen(commande, "r");
    if (fichier == NULL)
    {
        printf("\nErreur lors de l'exécution de la commande pour récupérer les statistiques.\n");
        exit(1);
    }

    strcat(classement, "\n--- Top 10 --- :\n");
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
    int taille_serveur = jeu->manche * MAX_CLIENTS;
    int taille_joueur = jeu->manche;
    char *cartes_serveur_c = tab_to_string(jeu->cartes_jouee, taille_serveur);
    char *cartes_joueur_c = tab_to_string(client->liste_cartes, taille_joueur);
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

    send(client->socket_client, buffer, strlen(buffer), 0);
    free(cartes_serveur_c);
    free(cartes_joueur_c);
}

void envoi_etat(Jeu *jeu, char *message)
{
    for (int j = 0; j < jeu->nb_clients; j++)
    {
        if (jeu->liste_joueurs[j] != NULL)
        {
            message_etat(jeu, jeu->liste_joueurs[j], message);
        }
    }
}

void envoi_message(Jeu *jeu, char *message)
{
    for (int j = 0; j < jeu->nb_clients; j++)
    {
        if (jeu->liste_joueurs[j] != NULL)
        {
            send(jeu->liste_joueurs[j]->socket_client, message, strlen(message), 0);
        }
    }
}
