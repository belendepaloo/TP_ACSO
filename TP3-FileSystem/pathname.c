
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/**
 * TODO
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (pathname == NULL || pathname[0] != '/') {
        return -1; // Ruta inválida
    }

    int inumber = 1; // Comenzar desde el inodo raíz

    // Crear una copia mutable del path
    char path_copy[strlen(pathname) + 1];
    strcpy(path_copy, pathname);

    // Tokenizar el path por '/'
    char *token = strtok(path_copy, "/");
    while (token != NULL) {
        struct direntv6 dirEnt;
        if (directory_findname(fs, token, inumber, &dirEnt) < 0) {
            return -1; // Componente no encontrado
        }
        inumber = dirEnt.d_inumber;
        token = strtok(NULL, "/");
    }

    return inumber;
}

