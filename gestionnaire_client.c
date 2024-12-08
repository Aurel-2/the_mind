#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "include/server.h"
#include <signal.h>

extern pthread_mutex_t verrou;
extern pthread_cond_t cond;
extern int nb_clients;
extern Jeu *partie;

void *deconnexion_client(InfoClient *info_client)
{
    printf("\nClient %d déconnecté\n", info_client->socket_client);
    close(info_client->socket_client);
    pthread_mutex_lock(&verrou);
    nb_clients--;
    if (nb_clients == 0 && partie->etat != PRET)
    {
        printf("\nAucun joueur en ligne - fermeture du serveur.\n");
        free(info_client);
        pthread_mutex_unlock(&verrou);
        pthread_kill(partie->thread_principal, SIGINT);
    }
    else
    {

        free(info_client);
        pthread_mutex_unlock(&verrou);
    }
    return NULL;
}

void *gestionnaire_client(void *arg)
{
    InfoClient *info_client = (InfoClient *)arg;
    char buffer[10];

    // Réception du pseudo du client
    const ssize_t bytes = recv(info_client->socket_client, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0)
    {
        buffer[bytes] = '\0';
        snprintf(info_client->pseudo, sizeof(info_client->pseudo), "%s", buffer);
    }
    else
    {
        printf("Erreur lors de la réception du nom du client.\n");
        deconnexion_client(info_client);
        return NULL;
    }

    // Attente que le jeu commence lorsque le nombre de clients est insuffisant
    pthread_mutex_lock(&verrou);
    while (nb_clients < MAX_CLIENTS)
    {
        char buffer[BUFFER_SIZE];
        snprintf(buffer, BUFFER_SIZE, "Le jeu est sur le point de commencer...");
        send(info_client->socket_client, buffer, strlen(buffer), 0);
        pthread_cond_wait(&cond, &verrou);
    }
    pthread_mutex_unlock(&verrou);

    // Boucle de jeu
    while (partie->etat != FIN)
    {
        // Si le jeu est en phase de distribution, attente
        pthread_mutex_lock(&verrou);
        while (partie->etat == DISTRIBUTION)
        {
            pthread_cond_wait(&cond, &verrou);
        }
        pthread_mutex_unlock(&verrou);
        while (partie->etat == EN_JEU)
        {
            ssize_t recv_bytes = recv(info_client->socket_client, buffer, BUFFER_SIZE, 0);
            if (recv_bytes <= 0)
            {
                // Déconnexion du client
                if (recv_bytes == 0)
                {
                    deconnexion_client(info_client);
                    return NULL;
                }
                else
                {
                    perror("Erreur de réception");
                }
                break;
            }

            buffer[recv_bytes] = '\0';
            int indice = atoi(buffer);

            // Vérification de l'indice
            if (indice >= 0 && indice < partie->manche)
            {
                pthread_mutex_lock(&verrou);

                // Si la carte est valide et n'a pas encore été jouée
                if (info_client->liste_cartes[indice] != '\0')
                {
                    partie->carte_actuelle = info_client->liste_cartes[indice];
                    info_client->liste_cartes[indice] = '\0';
                    pthread_cond_signal(&cond);
                }
                else
                {
                    // Si la carte a déjà été jouée
                    snprintf(buffer, BUFFER_SIZE, "\nIndice de carte invalide : carte déjà jouée\n");
                    send(info_client->socket_client, buffer, strlen(buffer), 0);
                }

                pthread_mutex_unlock(&verrou);
            }
            else
            {
                // Indice invalide
                snprintf(buffer, BUFFER_SIZE, "\nIndice de carte invalide : hors limites\n");
                send(info_client->socket_client, buffer, strlen(buffer), 0);
            }
        }

        if (partie->etat == MAUVAIS_TOUR || partie->etat == SCORE)
        {
            pthread_mutex_lock(&verrou);
            pthread_cond_wait(&cond, &verrou);
            pthread_mutex_unlock(&verrou);
        }
    }
    deconnexion_client(info_client);
    return NULL;
}
