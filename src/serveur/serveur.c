#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#include "../../include/serveur.h"
#include "../../include/utils.h"

// Déclaration de la structure de données Jeu et du thread pour la logique du jeu
Jeu *jeu;
pthread_t jeu_logique;

// Libère la mémoire allouée pour la liste des joueurs
void free_liste_joueurs()
{
    for (int i = 0; i < jeu->max_clients; i++)
    {
        if (jeu->liste_joueurs[i] != NULL)
        {
            free(jeu->liste_joueurs[i]);
        }
    }
    free(jeu->liste_joueurs);
}

int main(int argc, char const *argv[])
{
    // Affiche le titre du serveur
    printf("%s*****%s SERVEUR - THE MIND %s******\n%s", ROUGE, VERT, ROUGE, BLANC);

    // Initialise le générateur de nombres aléatoires
    srand(time(NULL));

    // Définit les gestionnaires de signaux pour SIGINT et SIGTERM
    signal(SIGINT, gestion_signal);
    signal(SIGTERM, gestion_signal);

    jeu = malloc(sizeof(Jeu));
    if (jeu == NULL)
    {
        perror("Erreur d'allocation mémoire pour jeu");
        exit(1);
    }

    // Demande à l'utilisateur de saisir le nombre maximum de clients
    int valide = 0; // Variable pour vérifier la validité de l'entrée
    while (!valide)
    {
        printf("%s\nVeuillez saisir un nombre maximum entre 1 et 8 : \n%s", VERT, BLANC);
        scanf("%d", &jeu->max_clients);
        // Vérification de la validité de l'entrée
        if (jeu->max_clients >= 1 && jeu->max_clients <= 8)
        {
            valide = 1;
        }
        else
        {
            printf("Erreur : veuillez saisir un nombre valide entre 1 et 8.\n");
        }
    }

    // Initialise les propriétés du jeu
    jeu->vies = 2;
    jeu->tour = 0;
    jeu->manche = 1;
    jeu->carte_actuelle = 0;
    jeu->nb_clients = 0;
    jeu->etat = PRET;
    jeu->processus_pid = getpid();

    // Initialise les verrous et les conditions
    pthread_mutex_init(&jeu->verrou_jeu, NULL);
    pthread_cond_init(&jeu->cond_jeu, NULL);
    pthread_mutex_init(&jeu->verrou_serveur, NULL);
    pthread_cond_init(&jeu->cond_serveur, NULL);

    // Crée le deck de 100 cartes
    for (int i = 0; i < 100; i++)
    {
        jeu->deck[i] = i + 1;
    }

    // Alloue de la mémoire pour la liste des joueurs
    jeu->liste_joueurs = malloc(sizeof(InfoClient *) * jeu->max_clients);
    for (int i = 0; i < jeu->max_clients; i++)
    {
        jeu->liste_joueurs[i] = malloc(sizeof(InfoClient));
    }

    // Initialise les structures de socket
    struct sockaddr_in adresse_serveur, adresse_client;
    socklen_t addr_len = sizeof(adresse_client);
    const int port = atoi(argv[1]);
    const int socket_serveur = socket(AF_INET, SOCK_STREAM, 0);
    jeu->socket_serveur = socket_serveur;

    // Vérifie si la création du socket a échoué
    if (socket_serveur < 0)
    {
        perror("Erreur: Socket.");
        free_liste_joueurs();
        free(jeu);
        exit(1);
    }

    // Configure l'adresse du serveur
    memset(&adresse_serveur, 0, sizeof(adresse_serveur));
    adresse_serveur.sin_family = AF_INET;
    adresse_serveur.sin_port = htons(port);
    adresse_serveur.sin_addr.s_addr = htonl(INADDR_ANY);

    // Lie le socket à l'adresse du serveur
    if (bind(socket_serveur, (struct sockaddr *)&adresse_serveur, sizeof(adresse_serveur)) < 0)
    {
        perror("Erreur: Binding.");
        close(socket_serveur);
        free_liste_joueurs();
        free(jeu);
        exit(1);
    }

    // Crée le thread pour la logique du jeu
    if (pthread_create(&jeu_logique, NULL, logique_jeu, (void *)jeu) != 0)
    {
        perror("Erreur lors de la création de logique_jeu");
        close(socket_serveur);
        free_liste_joueurs();
        free(jeu);
        exit(1);
    }

    // Écoute les connexions entrantes
    listen(socket_serveur, 5);
    printf("%s\nServeur en attente sur le port : %d\n%s", JAUNE, port, BLANC);

    // Boucle pour accepter les nouveaux clients jusqu'à ce que le nombre maximum soit atteint
    while (jeu->nb_clients < jeu->max_clients)
    {
        InfoClient *nouveau_client = malloc(sizeof(InfoClient));

        // Accepte une nouvelle connexion cliente
        nouveau_client->socket_client = accept(socket_serveur, (struct sockaddr *)&nouveau_client->client_addr, &addr_len);
        if (nouveau_client->socket_client < 0)
        {
            perror("Erreur d'acceptation de la connexion");
            free(nouveau_client);
            continue;
        }

        // Verrouille le jeu pour protéger l'accès à la liste des joueurs
        pthread_mutex_lock(&jeu->verrou_jeu);

        // Vérifie si le nombre maximum de clients est atteint
        if (jeu->nb_clients >= jeu->max_clients)
        {
            printf("%s\nNombre maximum de clients atteint.\nDéconnexion du client.\n%s", ROUGE, BLANC);
            close(nouveau_client->socket_client);
            free(nouveau_client);
            pthread_mutex_unlock(&jeu->verrou_jeu);
            continue;
        }

        // Ajoute le nouveau client à la liste des joueurs
        jeu->liste_joueurs[jeu->nb_clients] = nouveau_client;
        printf("\nNouveau client connecté.\nNombre de clients : %d/%d\n", jeu->nb_clients + 1, jeu->max_clients);
        jeu->nb_clients++;

        // Déverrouille le jeu
        pthread_mutex_unlock(&jeu->verrou_jeu);

        // Crée un thread pour gérer le client
        pthread_t thread_client;
        if (pthread_create(&thread_client, NULL, gestionnaire_client, (void *)nouveau_client) != 0)
        {
            perror("Erreur création de thread_client");
            close(nouveau_client->socket_client);
            free(nouveau_client);
            jeu->nb_clients--;
            continue;
        }
        pthread_detach(thread_client);

        // Si le nombre maximum de clients est atteint et que le jeu est prêt, le jeu commence
        if (jeu->nb_clients == jeu->max_clients && jeu->etat == PRET)
        {
            printf("%s\nLe jeu commence.\n%s", VERT, BLANC);
            pthread_mutex_lock(&jeu->verrou_jeu);
            jeu->etat = DISTRIBUTION;
            pthread_mutex_unlock(&jeu->verrou_jeu);

            pthread_cond_broadcast(&jeu->cond_jeu);
        }
    }

    // Attente que le jeu soit terminé
    pthread_mutex_lock(&jeu->verrou_serveur);
    while (jeu->etat != FIN)
    {
        pthread_cond_wait(&jeu->cond_serveur, &jeu->verrou_serveur);
    }
    pthread_mutex_unlock(&jeu->verrou_serveur);

    // Nettoyage du jeu
    nettoyage();
    return 0;
}

// Fonction de nettoyage à la fin du jeu
void nettoyage()
{
    printf("\n%sNettoyage du serveur en cours...\n%s", CYAN, BLANC);

    pthread_cond_destroy(&jeu->cond_serveur);
    pthread_mutex_destroy(&jeu->verrou_serveur);
    pthread_mutex_lock(&jeu->verrou_jeu);
    free_liste_joueurs();
    free(jeu->cartes_jouee);
    free(jeu->carte_bon_ordre);
    if (jeu->socket_serveur > 0)
    {
        close(jeu->socket_serveur);
    }
    free(jeu);
    pthread_mutex_unlock(&jeu->verrou_jeu);
    pthread_cond_destroy(&jeu->cond_jeu);
    pthread_mutex_destroy(&jeu->verrou_jeu);
    pthread_join(jeu_logique, NULL);
    printf("\n%sTerminé!\n%s", CYAN, BLANC);
}

// Gestion des signaux pour interruption du jeu
void gestion_signal(int signal)
{
    printf("\n%sLe jeu a été interrompu par un signal !\n%s", ROUGE, BLANC);
    if (jeu != NULL)
    {
        jeu->etat = FIN;
        pthread_cond_broadcast(&jeu->cond_jeu);
        nettoyage();
        exit(0);
    }
}
