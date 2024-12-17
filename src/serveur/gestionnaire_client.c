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
#include "../../include/utils.h"

extern Jeu *jeu;

// Gère les communications avec un client spécifique
void *gestionnaire_client(void *p_client)
{
    InfoClient *info_client = p_client;
    int est_vivant = 1; // Indique si le client est toujours connecté
    char pseudo[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];
    struct timeval debut;
    struct timeval fin;

    pthread_mutex_lock(&jeu->verrou_jeu);
    info_client->liste_temps_reaction = calloc(100, sizeof(float));
    if (info_client->liste_temps_reaction == NULL)
    {
        perror("Erreur d'allocation mémoire.");
        pthread_mutex_unlock(&jeu->verrou_jeu);
        return NULL;
    }
    pthread_mutex_unlock(&jeu->verrou_jeu);

    // Réception du pseudo du client
    ssize_t reponse = recv(info_client->socket_client, pseudo, sizeof(pseudo) - 1, 0);
    if (reponse <= 0)
    {
        perror("Erreur lors de la réception du pseudo.");
        deconnexion_client(info_client);
        return NULL;
    }
    pseudo[reponse] = '\0';
    pthread_mutex_lock(&jeu->verrou_jeu);
    snprintf(info_client->pseudo, sizeof(info_client->pseudo), "%s", pseudo);
    info_client->robot = (strcmp(info_client->pseudo, "robot") == 0) ? 1 : 0;
    pthread_mutex_unlock(&jeu->verrou_jeu);

    pthread_mutex_lock(&jeu->verrou_jeu);
    while (jeu->etat == PRET)
    {
        if (info_client->robot == 0)
        {
            snprintf(buffer, BUFFER_SIZE, "%s\nLe jeu est sur le point de commencer...\n%s", VERT, BLANC);
            if (send(info_client->socket_client, buffer, strlen(buffer), 0) == -1)
            {
                perror("Erreur lors de l'envoi du message du début du jeu.");
                pthread_mutex_unlock(&jeu->verrou_jeu);
                deconnexion_client(info_client);
                return NULL;
            }
        }
        pthread_cond_wait(&jeu->cond_jeu, &jeu->verrou_jeu);
    }
    pthread_mutex_unlock(&jeu->verrou_jeu);

    // Boucle principale pour gérer les communications pendant le jeu
    while (jeu->etat != FIN && est_vivant)
    {
        pthread_mutex_lock(&jeu->verrou_jeu);
        while (jeu->etat == DISTRIBUTION)
        {
            pthread_cond_wait(&jeu->cond_jeu, &jeu->verrou_jeu);
        }
        pthread_mutex_unlock(&jeu->verrou_jeu);

        while (jeu->etat == EN_JEU)
        {
            gettimeofday(&debut, NULL);
            reponse = recv(info_client->socket_client, buffer, BUFFER_SIZE, 0);
            gettimeofday(&fin, NULL);
            if (reponse <= 0)
            {
                est_vivant = 0;
                break;
            }
            buffer[reponse] = '\0';

            // Traitement de l'indice de carte reçu du client
            int indice = atoi(buffer) - 1;
            if (indice >= 0 && indice < jeu->manche)
            {
                float temps = calcul_temps_reaction(debut, fin);
                pthread_mutex_lock(&jeu->verrou_jeu);
                if (traitement_carte_jouee(info_client, indice, temps, 0) == 0)
                {
                    if (info_client->robot == 0)
                    {
                        snprintf(buffer, BUFFER_SIZE, "Indice de carte invalide : carte déjà jouée\n");
                        if (send(info_client->socket_client, buffer, strlen(buffer), 0) == -1)
                        {
                            perror("Erreur lors de l'envoi du message de carte déjà jouée.");
                            pthread_mutex_unlock(&jeu->verrou_jeu);
                            est_vivant = 0;
                            break;
                        }
                    }
                }
                pthread_mutex_unlock(&jeu->verrou_jeu);
            }
            else
            {
                pthread_mutex_lock(&jeu->verrou_jeu);
                if (info_client->robot == 0)
                {
                    snprintf(buffer, BUFFER_SIZE, "Indice de carte invalide : hors limites\n");
                    if (send(info_client->socket_client, buffer, strlen(buffer), 0) == -1)
                    {
                        perror("Erreur lors de l'envoi du message de carte invalide.");
                        pthread_mutex_unlock(&jeu->verrou_jeu);
                        est_vivant = 0;
                        break;
                    }
                }
                pthread_mutex_unlock(&jeu->verrou_jeu);
            }
        }
    }

    deconnexion_client(info_client);
    return NULL;
}

// Gère la déconnexion d'un client
void deconnexion_client(InfoClient *info_client)
{
    pthread_mutex_lock(&jeu->verrou_jeu);

    free(info_client->liste_cartes);
    free(info_client->liste_temps_reaction);

    jeu->nb_clients--;

    // Si plus aucun client n'est connecté, ferme le serveur
    if (jeu->nb_clients == 0 && jeu->etat != PRET)
    {
        printf("Aucun joueur en ligne - fermeture du serveur.\n");
        close(info_client->socket_client);
        pthread_cond_signal(&jeu->cond_serveur);
        pthread_cond_broadcast(&jeu->cond_jeu);
    }
    else
    {
        envoi_message(jeu, "Un joueur s'est déconnecté, la partie est finie!\n");
        pthread_mutex_unlock(&jeu->verrou_jeu);
        close(info_client->socket_client);
        jeu->etat = FIN;
        pthread_cond_broadcast(&jeu->cond_jeu);
    }
}

// Affiche les données du client
void affichage_donnees_client(InfoClient *info_client, int indice, float temps)
{
    printf("%s\n-------------------------- Joueur %s ---------------------------\n%s", CYAN, info_client->pseudo, BLANC);
    printf("\nManche actuelle : %d, Indice reçu : %d, Carte correspondante : %d\n", jeu->manche, indice + 1, info_client->liste_cartes[indice]);
    printf("\nTemps de réaction : %0.3f\n", temps);
    printf("%s\n------------------------------------------------------------------\n%s", CYAN, BLANC);
}

// Calcule le temps de réaction du client
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

// Traite la carte jouée par le client
int traitement_carte_jouee(InfoClient *info_client, int indice, float temps, int indice_temps)
{
    if (info_client->liste_cartes[indice] != '\0')
    {
        affichage_donnees_client(info_client, indice, temps);
        jeu->carte_actuelle = info_client->liste_cartes[indice];
        info_client->liste_cartes[indice] = '\0';
        info_client->liste_temps_reaction[indice_temps] = temps;
        indice_temps++;
        pthread_cond_signal(&jeu->cond_jeu);
        return 1;
    }
    return 0;
}
