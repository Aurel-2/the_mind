#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/utils.h"

// Conversion un tableau d'entiers en une chaîne de caractères
char *int_to_string(int *tab, int size) {
    char *resultat = (char *) calloc(size * 4 + 1, sizeof(char));
    if (resultat == NULL) {
        perror("Erreur d'allocation mémoire pour int_to_string");
        return NULL;
    }
    resultat[0] = '\0';
    for (int i = 0; i < size; i++) {
        char conversion_buffer[16];
        sprintf(conversion_buffer, "%d", tab[i]);
        strcat(resultat, conversion_buffer);
        if (i < size - 1) {
            strcat(resultat, " ");
        }
    }

    return resultat;
}

// Calcul la moyenne des valeurs d'un tableau
float moyenne(float tab[], int taille) {
    float somme = 0.0;
    float compteur = 0;
    for (int i = 0; i < taille; i++) {
        if (tab[i] != 0.0) {
            somme += tab[i];
            compteur++;
        }
    }
    return somme / compteur;
}

// Tri un tableau d'entiers en ordre croissant
void tri(int *cartes, int taille) {
    for (int i = 0; i < taille - 1; i++) {
        for (int j = i + 1; j < taille; j++) {
            if (cartes[i] > cartes[j]) {
                int temp = cartes[i];
                cartes[i] = cartes[j];
                cartes[j] = temp;
            }
        }
    }
}

// Conversion une chaîne de caractères en un tableau d'entiers
int string_to_int(char *donnee, int *tableau_int) {
    int j = 0;
    char *fin;
    int taille = strlen(donnee);

    for (int i = 0; i < taille; i++) {
        tableau_int[j] = strtol(&donnee[i], &fin, 10);
        j++;
        i = fin - donnee - 1;
    }
    return j;
}
