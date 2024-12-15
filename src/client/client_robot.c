#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define SERVEUR_ADDR "127.0.0.1"

pthread_mutex_t verrou = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int en_connexion = 1;
char buffer[BUFFER_SIZE];
char donnee[BUFFER_SIZE];
int tableau_int[12];
int tableau_rempli = 1;

int string_to_int(char *donnee, int *tableau_int)
{
    int j = 0;
    char *fin;
    int taille = strlen(donnee);

    for (int i = 0; i < taille; i++)
    {
        tableau_int[j] = strtol(&donnee[i], &fin, 10);
        j++;
        i = fin - donnee - 1;
    }
    return j;
}

void *reception_msg(void *arg)
{
    int serveur_socket = *((int *)arg);
    ssize_t taille_reponse;
    while ((taille_reponse = recv(serveur_socket, donnee, sizeof(donnee) - 1, 0)) > 0)
    {
        donnee[taille_reponse] = '\0';
        if (strcmp(donnee, "q") == 0)
        {
            pthread_mutex_lock(&verrou);
            en_connexion = 0;
            pthread_mutex_unlock(&verrou);
            pthread_cond_signal(&cond);
            printf("Déconnexion demandée par le serveur\n");
            return NULL;
        }
        printf("Données reçues :%s\n", donnee);
        pthread_cond_signal(&cond);
        tableau_rempli = 1;
    }
    if (taille_reponse == 0)
    {
        printf("Fermeture du client\n");
    }
    else if (taille_reponse < 0)
    {
        perror("Erreur de réception");
    }

    pthread_mutex_lock(&verrou);
    en_connexion = 0;
    pthread_mutex_unlock(&verrou);

    return NULL;
}

int main(int argc, char const *argv[])
{
    printf("Je suis un robot!\n");
    int serveur_socket;
    const int port = atoi(argv[1]);
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
        pthread_mutex_lock(&verrou);
        pthread_cond_wait(&cond, &verrou);
        pthread_mutex_unlock(&verrou);
        pthread_mutex_lock(&verrou);
        int taille_int = string_to_int(donnee, tableau_int);
        tableau_rempli = 1;
        while (1)
        {
            int indice = rand() % taille_int;
            while (tableau_int[indice] == 0)
            {
                indice = rand() % taille_int;
            }

            int valeur = tableau_int[indice];
            printf("Indice %d: %d\n", indice, valeur);
            if (valeur >= 1 && valeur <= 25)
            {
                printf("Sommeil de 2 secondes...\n");
                sleep(2);
            }
            else if (valeur > 25 && valeur <= 50)
            {
                printf("Sommeil de 4 secondes...\n");
                sleep(4);
            }
            else if (valeur > 50 && valeur <= 75)
            {
                printf("Sommeil de 6 secondes...\n");
                sleep(6);
            }
            else
            {
                sleep(10);
            }

            tableau_int[indice] = 0;
            indice++;
            snprintf(buffer, BUFFER_SIZE, "%d", indice);
            send(serveur_socket, buffer, strlen(buffer), 0);
            for (int i = 0; i < taille_int; i++)
            {
                if (tableau_int[i] != 0)
                {
                    tableau_rempli = 0;
                    break;
                }
            }

            if (tableau_rempli)
            {
                printf("Tableau rempli de zéros, arrêt de l'envoi\n");
                break;
            }
        }
        pthread_mutex_unlock(&verrou);
    }

    pthread_cancel(thread_reception);
    pthread_join(thread_reception, NULL);
    pthread_mutex_destroy(&verrou);
    close(serveur_socket);

    return 0;
}
