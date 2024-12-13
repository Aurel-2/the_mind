#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>

#include "../../include/serveur.h"
#include "../../include/jeu_fonctions.h"

extern pthread_mutex_t verrou;
extern pthread_cond_t cond;
extern Jeu *jeu;

void *gestionnaire_client(void *p_client)
{
    InfoClient *info_client = p_client;
    int est_vivant = 1;
    char name[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    struct timeval debut;
    struct timeval fin;
    pthread_mutex_lock(&verrou);
    info_client->liste_temps_reaction = calloc(100, sizeof(float));
    if (info_client->liste_temps_reaction == NULL)
    {
        printf("Erreur d'allocation mémoire.\n");
        pthread_mutex_unlock(&verrou);
        return NULL;
    }
    pthread_mutex_unlock(&verrou);

    ssize_t bytes = recv(info_client->socket_client, name, sizeof(name) - 1, 0);
    if (bytes <= 0)
    {
        printf("%sErreur lors de la réception du nom du client.\n%s", ROUGE, BLANC);
        deconnexion_client(info_client);
        return NULL;
    }

    name[bytes] = '\0';
    snprintf(info_client->pseudo, sizeof(info_client->pseudo), "%s", name);
    pthread_mutex_lock(&verrou);
    while (jeu->nb_clients < MAX_CLIENTS)
    {
        snprintf(buffer, BUFFER_SIZE, "%sLe jeu est sur le point de commencer...\n%s", VERT, BLANC);
        send(info_client->socket_client, buffer, strlen(buffer), 0);
        pthread_cond_wait(&cond, &verrou);
    }
    pthread_mutex_unlock(&verrou);

    while (jeu->etat != FIN && est_vivant == 1)
    {
        pthread_mutex_lock(&verrou);
        while (jeu->etat == DISTRIBUTION)
        {
            pthread_cond_wait(&cond, &verrou);
        }
        pthread_mutex_unlock(&verrou);

        while (jeu->etat == EN_JEU)
        {
            gettimeofday(&debut, NULL);
            ssize_t bytes = recv(info_client->socket_client, buffer, BUFFER_SIZE, 0);
            gettimeofday(&fin, NULL);
            if (bytes <= 0)
            {
                if (bytes == 0)
                {
                    est_vivant = 0;
                }
                break;
            }
            buffer[bytes] = '\0';
            const int indice = atoi(buffer) - 1;
            if (indice >= 0 && indice < jeu->manche)
            {
                int indice_temps = 0;
                pthread_mutex_lock(&verrou);
                const float temps = calcul_temps_reaction(debut, fin);
                if (traitement_carte_jouee(info_client, indice, temps, indice_temps) == 0)
                {
                    snprintf(buffer, BUFFER_SIZE, "%s\nIndice de carte invalide : carte déjà jouée\n%s", JAUNE, BLANC);
                    send(info_client->socket_client, buffer, strlen(buffer), 0);
                }
                pthread_mutex_unlock(&verrou);
            }
            else
            {
                snprintf(buffer, BUFFER_SIZE, "%s\nIndice de carte invalide : hors limites\n%s", ROUGE, BLANC);
                send(info_client->socket_client, buffer, strlen(buffer), 0);
            }
        }
        while (jeu->etat == MAUVAIS_TOUR || jeu->etat == SCORE)
        {
            pthread_mutex_lock(&verrou);
            pthread_cond_wait(&cond, &verrou);
            pthread_mutex_unlock(&verrou);
        }
    }
    deconnexion_client(info_client);
    return NULL;
}

void deconnexion_client(InfoClient *info_client)
{
    pthread_mutex_lock(&verrou);
    free(info_client->liste_cartes);
    free(info_client->liste_temps_reaction);
    jeu->nb_clients--;

    if (jeu->nb_clients == 0 && jeu->etat != PRET)
    {
        printf("%s\nAucun joueur en ligne - fermeture du serveur.\n%s", BLEU, BLANC);
        jeu->etat = FIN;
        kill(jeu->processus_pid, SIGUSR1);
    }
    else
    {
        nouvelle_manche(jeu);
        envoi_message(jeu, "Déconnexion d'un joueur. Redémarrage de la manche sans perte de vie.");
    }

    close(info_client->socket_client);
    pthread_mutex_unlock(&verrou);
    pthread_cond_broadcast(&cond);
}

void affichage_donnees_client(InfoClient *info_client, int indice, float temps)
{
    printf("%s\n-------------------------- Joueur %s ---------------------------\n%s", CYAN, info_client->pseudo, BLANC);
    printf("\nManche actuelle : %d, Indice reçu : %d, Carte correspondante : %d\n", jeu->manche, indice + 1, info_client->liste_cartes[indice]);
    printf("\nTemps de réaction : %0.3f\n", temps);
    printf("%s\n------------------------------------------------------------------\n%s", CYAN, BLANC);
}

float calcul_temps_reaction(struct timeval debut, struct timeval fin)
{
    float temps = 0.0;
    float temps_s = fin.tv_sec - debut.tv_sec;
    float temps_ms = fin.tv_usec - debut.tv_usec;
    if (temps_ms < 0)
    {
        temps_s--;
        temps_ms += 1000000;
    }
    temps = temps_s + temps_ms / 1000000.0;
    return temps;
}

int traitement_carte_jouee(InfoClient *info_client, int indice, float temps, int indice_temps)
{
    if (info_client->liste_cartes[indice] != '\0')
    {
        jeu->carte_actuelle = info_client->liste_cartes[indice];
        info_client->liste_cartes[indice] = '\0';
        info_client->liste_temps_reaction[indice_temps] = temps;
        indice_temps++;
        affichage_donnees_client(info_client, indice, temps);
        pthread_cond_signal(&cond);
        return 1;
    }
    return 0;
}
