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

extern pthread_mutex_t verrou;
extern pthread_cond_t cond;
extern pthread_cond_t arret;

void *logique_jeu(void *p_jeu)
{
    Jeu *jeu = p_jeu;
    while (jeu->etat != FIN)
    {
        switch (jeu->etat)
        {
        case DISTRIBUTION:
            pthread_mutex_lock(&verrou);
            phase_distribution(jeu);
            pthread_mutex_unlock(&verrou);
            break;
        case EN_JEU:
            pthread_mutex_lock(&verrou);
            phase_enjeu(jeu);
            pthread_mutex_unlock(&verrou);
            break;
        case MAUVAIS_TOUR:
            pthread_mutex_lock(&verrou);
            phase_mauvaistour(jeu);
            pthread_mutex_unlock(&verrou);
            break;
        case SCORE:
            pthread_mutex_lock(&verrou);
            phase_score(jeu);
            pthread_mutex_unlock(&verrou);
            break;
        case PRET:
            pthread_mutex_lock(&verrou);
            pthread_cond_wait(&cond, &verrou);
            pthread_mutex_unlock(&verrou);
            break;
        default:
            break;
        }
    }
    printf("%s\nFermeture du thread de logique...\n%s", JAUNE, BLANC);
    pthread_cond_signal(&arret);
    return NULL;
}

void phase_distribution(Jeu *jeu)
{
    printf("%s\n--- DISTRIBUTION DES CARTES ---\n%s", JAUNE, BLANC);
    jeu->cartes_jouee = calloc(jeu->nb_clients * jeu->manche, sizeof(int));
    jeu->carte_bon_ordre = calloc(jeu->nb_clients * jeu->manche, sizeof(int));
    melange_cartes(jeu);
    distribution(jeu);
    envoi_etat(jeu, "Les cartes ont été distribuées");
    jeu->etat = EN_JEU;
    pthread_cond_broadcast(&cond);
}

void phase_enjeu(Jeu *jeu)
{
    while (jeu->etat == EN_JEU && jeu->nb_clients > 0)
    {
        char buffer[BUFFER_SIZE];
        printf("%s\n--- EN JEU ---\n%s", JAUNE, BLANC);
        printf("%s\nAttente d'une carte...\n%s", JAUNE, BLANC);
        if (jeu->etat == DISTRIBUTION)
        {
            printf("%s\nRelancement de la manche...\n%s", JAUNE, BLANC);
            break;
        }
        pthread_cond_wait(&cond, &verrou);
        if (jeu->carte_actuelle == jeu->carte_bon_ordre[jeu->tour])
        {
            jeu->cartes_jouee[jeu->tour] = jeu->carte_actuelle;
            snprintf(buffer, BUFFER_SIZE, "%sUn joueur a joué une carte valide.\n%s", VERT, BLANC);
            envoi_etat(jeu, buffer);
            jeu->tour++;
        }
        else
        {
            if (jeu->carte_actuelle == 0)
            {
                break;
            }
            jeu->etat = MAUVAIS_TOUR;
            break;
        }

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

void phase_mauvaistour(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    jeu->vies--;
    if (jeu->vies == 0)
    {
        snprintf(buffer, BUFFER_SIZE, "%s\nPlus de vies restantes, jeu terminée.\n%s", MAGENTA, BLANC);
        envoi_message(jeu, buffer);
        jeu->etat = SCORE;
    }
    else
    {
        snprintf(buffer, BUFFER_SIZE, "%s\n-----------------------\nUn joueur a joué une mauvaise carte.\n-----------------------\n%s", ROUGE, BLANC);
        envoi_message(jeu, buffer);
        nouvelle_manche(jeu);
    }
}

void phase_score(Jeu *jeu)
{
    char buffer[BUFFER_SIZE];
    printf("%s\n--- Score ---\n%s", JAUNE, BLANC);
    jeu->cartes_jouee[jeu->tour] = jeu->carte_actuelle;
    char *dernieres_cartes_jouees = tab_to_string(jeu->cartes_jouee, jeu->manche * jeu->nb_clients);
    if (jeu->manche < MAX_MANCHE)
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
    jeu->etat = FIN;
    snprintf(buffer, BUFFER_SIZE, "q");
    envoi_message(jeu, buffer);
}
