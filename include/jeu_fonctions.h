
#ifndef JEU_FONCTIONS_H
#define JEU_FONCTIONS_H

#include "serveur.h"

void melange_cartes(Jeu *jeu);

void distribution(Jeu *jeu);

void nouvelle_manche(Jeu *jeu);

void insertion_stats(Jeu *jeu);

void envoi_stats(Jeu *jeu);

void message_etat(Jeu *jeu, InfoClient *client, char *message);

void envoi_etat(Jeu *jeu, char *message);

void envoi_message(Jeu *jeu, char *message);

void message_robot(Jeu* jeu, InfoClient *client);

#endif // JEU_FONCTIONS_H
