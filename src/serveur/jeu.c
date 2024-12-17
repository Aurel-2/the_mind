#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include "../../include/serveur.h"
#include "../../include/utils.h"
#include "../../include/jeu_fonctions.h"

// Thread principal de la logique de jeu
void *logique_jeu(void *p_jeu)
{
    Jeu *jeu = p_jeu;
    while (jeu->etat != FIN)
    {
        // Gestion des différentes phases du jeu
        switch (jeu->etat)
        {
        case DISTRIBUTION:
            phase_distribution(jeu);  // Phase de distribution des cartes
            break;
        case EN_JEU:
            phase_enjeu(jeu);  // Phase où le jeu est en cours
            break;
        case MAUVAIS_TOUR:
            phase_mauvaistour(jeu);  // Phase où un joueur a fait une erreur
            break;
        case SCORE:
            phase_score(jeu);  // Phase de calcul des scores
            break;
        case PRET:
            // Attente d'un signal pour commencer une nouvelle phase
            pthread_mutex_lock(&jeu->verrou_jeu);
            pthread_cond_wait(&jeu->cond_jeu, &jeu->verrou_jeu);
            pthread_mutex_unlock(&jeu->verrou_jeu);
            break;
        default:
            break;
        }
    }
    printf("%s\nFermeture du thread de logique...\n%s", JAUNE, BLANC);
    pthread_cond_signal(&jeu->cond_serveur);
    return NULL;
}

// Gère la phase de distribution des cartes aux joueurs
void phase_distribution(Jeu *jeu)
{
    printf("%s\n--- DISTRIBUTION DES CARTES ---\n%s", JAUNE, BLANC);
    pthread_mutex_lock(&jeu->verrou_jeu);
    jeu->cartes_jouee = calloc(jeu->nb_clients * jeu->manche, sizeof(int));
    jeu->carte_bon_ordre = calloc(jeu->nb_clients * jeu->manche, sizeof(int));
    pthread_mutex_unlock(&jeu->verrou_jeu);

    // Mélange et distribution des cartes
    melange_cartes(jeu);
    distribution(jeu);

    pthread_mutex_lock(&jeu->verrou_jeu);
    jeu->etat = EN_JEU;  
    pthread_mutex_unlock(&jeu->verrou_jeu);

    envoi_etat(jeu, "Début de la partie");
    for (int i = 0; i < jeu->max_clients; i++)
    {
        if (jeu->liste_joueurs[i]->robot == 1)
        {
            message_robot(jeu, jeu->liste_joueurs[i]);  
        }
    }

    pthread_cond_broadcast(&jeu->cond_jeu);  // Signalisation pour continuer le jeu
}

// Gère la phase de jeu active où les joueurs jouent leurs cartes
void phase_enjeu(Jeu *jeu)
{
    while (jeu->etat == EN_JEU)
    {
        char buffer[BUFFER_SIZE];
        printf("%s\n--- EN JEU ---\n%s", CYAN, BLANC);
        printf("%s\nAttente d'une carte...\n%s", JAUNE, BLANC);
        pthread_mutex_lock(&jeu->verrou_jeu);
        pthread_cond_wait(&jeu->cond_jeu, &jeu->verrou_jeu);
        pthread_mutex_unlock(&jeu->verrou_jeu);

        if (jeu->etat == FIN)
        {
            printf("%s\nArrêt...\n%s", JAUNE, BLANC);
            break;
        }

        pthread_mutex_lock(&jeu->verrou_jeu);
        // Vérification si la carte jouée est correcte
        if (jeu->carte_actuelle == jeu->carte_bon_ordre[jeu->tour])
        {
            jeu->cartes_jouee[jeu->tour] = jeu->carte_actuelle;
            pthread_mutex_unlock(&jeu->verrou_jeu);
            snprintf(buffer, BUFFER_SIZE, "%sUn joueur a joué une carte valide.\n%s", VERT, BLANC);
            envoi_etat(jeu, buffer);
            jeu->tour++;
        }
        else
        {
            if (jeu->carte_actuelle == 0)
            {
                pthread_mutex_unlock(&jeu->verrou_jeu);
                break;
            }
            jeu->etat = MAUVAIS_TOUR;  // Mauvaise carte jouée
            pthread_mutex_unlock(&jeu->verrou_jeu);
            break;
        }

        // Passage à la manche suivante si toutes les cartes sont jouées
        if (jeu->tour == jeu->nb_clients * jeu->manche && jeu->etat != MAUVAIS_TOUR && jeu->etat != SCORE)
        {
            jeu->manche++;
            snprintf(buffer, BUFFER_SIZE, "%s\nManche %d terminée avec succès !\n%s", VERT, jeu->manche, BLANC);
            envoi_message(jeu, buffer);
            nouvelle_manche(jeu);
            break;
        }
    }
}

// Gère la phase où un joueur joue une mauvaise carte
void phase_mauvaistour(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    pthread_mutex_lock(&jeu->verrou_jeu);
    jeu->vies--;  
    if (jeu->vies == 0)
    {
        pthread_mutex_unlock(&jeu->verrou_jeu);
        snprintf(buffer, BUFFER_SIZE, "%s\nPlus de vies restantes, jeu terminée.\n%s", MAGENTA, BLANC);
        envoi_message(jeu, buffer);
        jeu->etat = SCORE;  
    }
    else
    {
        pthread_mutex_unlock(&jeu->verrou_jeu);
        snprintf(buffer, BUFFER_SIZE, "%s\n-----------------------\nUn joueur a joué une mauvaise carte.\n-----------------------\n%s", ROUGE, BLANC);
        envoi_message(jeu, buffer);
        nouvelle_manche(jeu);  
    }
}

// Gère la phase de score à la fin du jeu
void phase_score(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    printf("%s\n--- Score ---\n%s", JAUNE, BLANC);
    pthread_mutex_lock(&jeu->verrou_jeu);
    jeu->cartes_jouee[jeu->tour] = jeu->carte_actuelle;
    pthread_mutex_unlock(&jeu->verrou_jeu);
    char *dernieres_cartes_jouees = int_to_string(jeu->cartes_jouee, jeu->manche * jeu->nb_clients);

    if (jeu->manche <= MAX_MANCHE)
    {
        snprintf(buffer, BUFFER_SIZE, "%s\nVous avez perdu avec la carte %d.\nCartes jouées durant la manche : %s\n%s",
                 MAGENTA, jeu->carte_actuelle, dernieres_cartes_jouees, BLANC);
    }
    else
    {
        snprintf(buffer, BUFFER_SIZE, "%s\nLe jeu est terminé.\nDernières cartes jouées : %s\n%s",
                 VERT, dernieres_cartes_jouees, BLANC);
    }

    envoi_message(jeu, buffer);
    printf("%s\nEnregistrement des statistiques...\n%s", JAUNE, BLANC);
    insertion_stats(jeu);  
    printf("%s\nEnvoi des statistiques aux joueurs...\n%s", JAUNE, BLANC);
    envoi_stats(jeu);  
    free(dernieres_cartes_jouees);

    pthread_mutex_lock(&jeu->verrou_jeu);
    jeu->etat = FIN;  
    pthread_mutex_unlock(&jeu->verrou_jeu);

    snprintf(buffer, BUFFER_SIZE, "q");
    envoi_message(jeu, buffer);  
}
