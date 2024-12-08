#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/utils.h"

char * tab_to_string(int *tab, int size)
{
    char *resultat = (char *)calloc(size * 4 + 1, sizeof(char));
    if (resultat == NULL)
    {
        perror("Erreur d'allocation mémoire pour tab_to_string");
        return NULL;
    }
    resultat[0] = '\0';
    for (int i = 0; i < size; i++)
    {
        char conversion_buffer[16];
        sprintf(conversion_buffer, "%d", tab[i]);
        strcat(resultat, conversion_buffer);
        if (i < size - 1)
        {
            strcat(resultat, " ");
        }
    }

    return resultat;
}
