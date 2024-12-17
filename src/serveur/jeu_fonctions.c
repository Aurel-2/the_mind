#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include "../../include/jeu_fonctions.h"
#include "../../include/utils.h"

// Mélange les cartes dans le deck de manière aléatoire
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

// Distribue les cartes aux joueurs et trie les cartes en ordre croissant
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

    // Trie des cartes dans l'ordre croissant pour la référence du jeu
    tri(jeu->carte_bon_ordre, jeu->manche * jeu->max_clients);
    printf("\n%sCartes de tous les joueurs dans l'ordre croissant : ", JAUNE);
    for (int i = 0; i < jeu->max_clients * jeu->manche; i++)
    {
        printf("%d ", jeu->carte_bon_ordre[i]);
    }
    printf("\n%s", BLANC);

    // Tri des cartes pour les robots
    for (int i = 0; i < jeu->max_clients; i++)
    {
        if (jeu->liste_joueurs[i]->robot == 1)
        {
            tri(jeu->liste_joueurs[i]->liste_cartes, jeu->manche);
        }
    }
    pthread_mutex_unlock(&jeu->verrou_jeu);
}

// Prépare une nouvelle manche ou termine le jeu si la dernière manche est complétée
void nouvelle_manche(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    pthread_mutex_lock(&jeu->verrou_jeu);

    if (jeu->manche == MAX_MANCHE)
    {
        snprintf(buffer, BUFFER_SIZE, "%s\nVous avez complété toutes les manches, bravo !\n%s", VERT, BLANC);
        envoi_message(jeu, buffer);
        jeu->etat = SCORE; // Passage à l'état de score si toutes les manches sont complétées
    }
    else
    {
        // Préparation pour une nouvelle manche
        jeu->etat = DISTRIBUTION;
        jeu->tour = 0;
        jeu->carte_actuelle = 0;
        free(jeu->cartes_jouee);
        free(jeu->carte_bon_ordre);
    }

    // Libération des cartes des joueurs pour la nouvelle distribution
    for (int i = 0; i < jeu->max_clients; i++)
    {
        free(jeu->liste_joueurs[i]->liste_cartes);
    }
    pthread_mutex_unlock(&jeu->verrou_jeu);
}

// Insère les statistiques de jeu dans un fichier texte
void insertion_stats(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    float total_moyenne_react = 0.0;
    for (int i = 0; i < jeu->max_clients; i++)
    {
        total_moyenne_react += moyenne(jeu->liste_joueurs[i]->liste_temps_reaction, MAX_MANCHE * 2);
    }

    // Enregistrement de la date et heure actuelles
    time_t t;
    time(&t);
    struct tm date = *localtime(&t);
    snprintf(buffer, BUFFER_SIZE, "%0.3f %d %d %d-%d-%d", total_moyenne_react, jeu->carte_actuelle, jeu->manche,
             date.tm_mday, date.tm_mon + 1, date.tm_year + 1900);

    // Ouverture du fichier pour ajouter les nouvelles statistiques
    FILE *fichier = fopen("donnees.txt", "a");
    if (fichier == NULL)
    {
        perror("Le fichier donnees.txt n'existe pas");
        kill(jeu->processus_pid, SIGINT);
    }

    // Écriture des statistiques dans le fichier
    fprintf(fichier, "%s\n", buffer);
    fclose(fichier);
}

// Envoie les statistiques de jeu aux joueurs
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

    // Lecture des 10 meilleures entrées et création du message de classement
    strcat(classement, "\n--- Top 10 --- \n");
    while (fgets(buffer, sizeof(buffer), fichier) != NULL)
    {
        strcat(classement, buffer);
    }
    envoi_message(jeu, classement);
    pclose(fichier);
}

// Envoie un message d'état aux joueurs
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

// Envoie l'état du jeu à tous les clients joueurs
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

// Envoie un message à tous les joueurs clients
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

// Envoie le tableau de cartes à un robot client
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