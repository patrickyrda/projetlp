#include <defines.h>
#include <string.h>
#include <stdio.h>

/*!
 * @brief concat_path concatenates suffix to prefix into result
 * It checks if prefix ends by / and adds this token if necessary
 * It also checks that result will fit into PATH_SIZE length
 * @param result the result of the concatenation
 * @param prefix the first part of the resulting path
 * @param suffix the second part of the resulting path
 * @return a pointer to the resulting path, NULL when concatenation failed
 */
char *concat_path(char *result, char *prefix, char *suffix) {
    // Vérifie si les entrées ne sont pas NULL
    if (!prefix || !suffix || !result) {
        return NULL;
    }

    // Calcul de la longueur nécessaire pour le résultat, +1 pour la barre oblique éventuelle, +1 pour '\0'
    size_t needed_length = strlen(prefix) + strlen(suffix) + 2;

    // Vérifie si la longueur dépasse PATH_SIZE
    if (needed_length > PATH_SIZE) {
        fprintf(stderr, "Erreur: chemin concaténé trop long.\n");
        return NULL;
    }

    // Copie le préfixe dans le résultat
    strcpy(result, prefix);

    // Ajoute une barre oblique si nécessaire
    if (prefix[strlen(prefix) - 1] != '/') {
        strcat(result, "/");
    }

    // Concatène le suffixe
    strcat(result, suffix);

    return result;
}
