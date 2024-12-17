#ifndef UTILS_H
#define UTILS_H
#define BLANC "\033[0m"
#define ROUGE "\033[31m"
#define VERT "\033[32m"
#define JAUNE "\033[33m"
#define BLEU "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"

char *int_to_string(int *tab, int size);

int string_to_int(char *donnee, int *tableau_int);

float moyenne(float tab[], int taille);

void tri(int *cartes, int taille);

#endif // UTILS_H
