#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include "../../include/utils.h"

#define BUFFER_SIZE 1024
#define SERVEUR_ADDR "127.0.0.1"

pthread_mutex_t verrou_jeu = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_jeu = PTHREAD_COND_INITIALIZER;
int en_connexion = 1;
char donnee[BUFFER_SIZE];
int tableau_int[12];
int tableau_rempli = 1;
int nouvelle_donnee = 0;
int indice;

void *reception_msg(void *arg)
{
    int serveur_socket = *((int *)arg);
    ssize_t taille_reponse;
    while ((taille_reponse = recv(serveur_socket, donnee, sizeof(donnee) - 1, 0)) > 0)
    {
        donnee[taille_reponse] = '\0';
        if (strcmp(donnee, "q") == 0)
        {
            pthread_mutex_lock(&verrou_jeu);
            en_connexion = 0;
            pthread_mutex_unlock(&verrou_jeu);
            pthread_cond_signal(&cond_jeu);
            printf("Déconnexion demandée par le serveur\n");
            return NULL;
        }
        // On indique au thread principal qu'il y a eu des nouvelles données et on réinitialise les valeurs
        printf("\nDonnées reçues : %s\n", donnee);
        nouvelle_donnee = 1;
        indice = 0;
        pthread_cond_signal(&cond_jeu);
        if (taille_reponse == 0)
        {
            printf("\nFermeture du client\n");
            break;
        }
        else if (taille_reponse < 0)
        {
            perror("Erreur de réception");
        }
    }

    pthread_mutex_lock(&verrou_jeu);
    en_connexion = 0;
    pthread_mutex_unlock(&verrou_jeu);

    return NULL;
}
void gestion_signal(int signal)
{
    printf("%s\nProblème de pipe\n%s", ROUGE, BLANC);
    pthread_mutex_lock(&verrou_jeu);
    en_connexion = 0;
    pthread_mutex_unlock(&verrou_jeu);
    pthread_cond_signal(&cond_jeu);
}
int main(int argc, char const *argv[])
{
    printf("\nJe suis un robot!\n");
    signal(SIGPIPE, gestion_signal);
    char buffer[BUFFER_SIZE];
    int serveur_socket;
    int port = atoi(argv[1]);
    struct sockaddr_in adresse_serveur;
    if ((serveur_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Problème lors de la création du socket");
        exit(1);
    }
    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(port);
    adresse_serveur.sin_addr.s_addr = inet_addr(SERVEUR_ADDR);
    if (connect(serveur_socket, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0)
    {
        perror("La connexion a échoué");
        close(serveur_socket);
        exit(1);
    }
    send(serveur_socket, "robot", strlen("robot"), 0);
    pthread_t thread_reception;
    pthread_create(&thread_reception, NULL, reception_msg, &serveur_socket);
    srand(time(NULL));
    while (en_connexion)
    {
        pthread_mutex_lock(&verrou_jeu);
        while (!nouvelle_donnee && en_connexion)
        {
            printf("%s\nEn attente de nouvelles données...\n%s", JAUNE, BLANC);
            pthread_cond_wait(&cond_jeu, &verrou_jeu);
        }
        if (!en_connexion)
        {
            pthread_mutex_unlock(&verrou_jeu);
            break;
        }
        printf("%s\nNouvelles données disponibles\n%s", VERT, BLANC);
        int taille_int = string_to_int(donnee, tableau_int);
        tableau_rempli = 1;
        nouvelle_donnee = 0;
        pthread_mutex_unlock(&verrou_jeu);

        while (1)
        {
            pthread_mutex_lock(&verrou_jeu);
            int valeur = tableau_int[indice];
            if (tableau_int[indice] == 0)
            {
                printf("%s\nLe tableau comporte une erreur.\n%s", ROUGE, BLANC);
                break;
            }
            
            printf("Indice %d: %d\n", indice, valeur);
            pthread_mutex_unlock(&verrou_jeu);
            // Choix du temps de sommeil par rapport à la valeur de la carte
            int sommeil = 0;
            if (valeur >= 1 && valeur <= 25)
            {
                sommeil = 2;
            }
            else if (valeur > 25 && valeur <= 50)
            {
                sommeil = 4;
            }
            else if (valeur > 50 && valeur <= 75)
            {
                sommeil = 6;
            }
            else
            {
                sommeil = 8;
            }
            // Simulation du sommeil
            for (int i = 0; i < sommeil; i++)
            {
                printf("%s\nDors depuis %d seconde...\n%s", JAUNE, i, BLANC);
                sleep(1);
                pthread_mutex_lock(&verrou_jeu);

                if (nouvelle_donnee)
                {
                    // On arrête le sommeil s'il y a de nouvelles données
                    printf("%s\nNouvelles données reçues pendant le sommeil.\n%s", VERT, BLANC);
                    pthread_mutex_unlock(&verrou_jeu);

                    break;
                }
                pthread_mutex_unlock(&verrou_jeu);
            }
            // On met à 0 la valeur dans notre tableau
            tableau_int[indice] = 0;
            indice++;
            snprintf(buffer, BUFFER_SIZE, "%d", indice);
            if (send(serveur_socket, buffer, strlen(buffer), 0) < 0)
            {
                break;
            }

            int tableau_rempli_temp = 1;
            for (int i = 0; i < taille_int; i++)
            {
                if (tableau_int[i] != 0)
                {
                    tableau_rempli_temp = 0;
                    break;
                }
            }
            // On retourne en attente quand le tableau est vite
            if (tableau_rempli_temp)
            {
                printf("%s\nTableau rempli de zéros, arrêt de l'envoi\n%s", JAUNE, BLANC);
                pthread_mutex_unlock(&verrou_jeu);
                break;
            }
            pthread_mutex_unlock(&verrou_jeu);
        }
    }
    printf("\nTerminaison.\n");
    pthread_cancel(thread_reception);
    pthread_join(thread_reception, NULL);
    pthread_mutex_destroy(&verrou_jeu);
    close(serveur_socket);
    return 0;
}
