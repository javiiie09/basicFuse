/*
 * FUSE: Filesystem in Userspace
 * DSO 2014
 * Ejemplo para montar un libro de poesía como sistema de ficheros
 * Cada capítulo del libro será un fichero diferente
 *
 * Compilarlo con make
 *
 * uso:  basicFUSE [opciones FUSE] fichero_inicial punto_de_montaje
 *
 *  ./basicFUSE proverbiosycantares.txt punto_montaje
 *
 */

#define FUSE_USE_VERSION 26

#include "basicFUSE_lib.h"
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

#define UMBRAL 256  // Umbral en bytes para BIG/little

/***********************************
 * Obtiene el índice de un capítulo según la ruta (raíz, BIG o little)
 ***********************************/
static int obtener_indice(const char *path, struct structura_mis_datos *mis_datos) {
    int idx = -1;
    char subpath[PATH_MAX];

    if (strncmp(path, "/BIG/", 5) == 0) {
        // Ruta dentro de BIG
        const char *nombre = path + 5;
        snprintf(subpath, PATH_MAX, "/%s", nombre);
        idx = buscar_fichero(subpath, mis_datos);
        if (idx < 0 || strlen(mis_datos->contenido_ficheros[idx]) <= UMBRAL)
            idx = -1;
    } else if (strncmp(path, "/little/", 8) == 0) {
        // Ruta dentro de little
        const char *nombre = path + 8;
        snprintf(subpath, PATH_MAX, "/%s", nombre);
        idx = buscar_fichero(subpath, mis_datos);
        if (idx < 0 || strlen(mis_datos->contenido_ficheros[idx]) > UMBRAL)
            idx = -1;
    } else {
        // Fichero en raíz
        idx = buscar_fichero(path, mis_datos);
    }
    return idx;
}

/***********************************
 * get atributos de ficheros y directorios
 ***********************************/
static int mi_getattr(const char *path, struct stat *stbuf) {
    struct structura_mis_datos *mis_datos = 
        (struct structura_mis_datos *) fuse_get_context()->private_data;
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        // Directorio raíz
        stbuf->st_mode  = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_uid   = mis_datos->st_uid;
        stbuf->st_gid   = mis_datos->st_gid;
        stbuf->st_atim  = mis_datos->st_atim;
        stbuf->st_mtim  = mis_datos->st_mtim;
        stbuf->st_ctim  = mis_datos->st_ctim;
        stbuf->st_size  = 1024;
        stbuf->st_blocks= 2;
    } else if (strcmp(path, "/BIG") == 0 || strcmp(path, "/little") == 0) {
        // Directorios BIG y little
        stbuf->st_mode  = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        stbuf->st_uid   = mis_datos->st_uid;
        stbuf->st_gid   = mis_datos->st_gid;
        stbuf->st_atim  = mis_datos->st_atim;
        stbuf->st_mtim  = mis_datos->st_mtim;
        stbuf->st_ctim  = mis_datos->st_ctim;
        stbuf->st_size  = 1024;
        stbuf->st_blocks= 2;
    } else {
        // Ficheros virtuales
        int idx = obtener_indice(path, mis_datos);
        if (idx >= 0) {
            size_t tam = strlen(mis_datos->contenido_ficheros[idx]);
            stbuf->st_mode  = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_uid   = mis_datos->st_uid;
            stbuf->st_gid   = mis_datos->st_gid;
            stbuf->st_atim  = mis_datos->st_atim;
            stbuf->st_mtim  = mis_datos->st_mtim;
            stbuf->st_ctim  = mis_datos->st_ctim;
            stbuf->st_size  = tam;
            stbuf->st_blocks= (tam / 512) + (tam % 512 ? 1 : 0);
        } else {
            res = -ENOENT;
        }
    }
    return res;
}

/***********************************
 * leer contenido de directorio
 ***********************************/
static int mi_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                      off_t offset, struct fuse_file_info *fi) {
    struct structura_mis_datos *mis_datos = 
        (struct structura_mis_datos *) fuse_get_context()->private_data;
    (void) offset;
    (void) fi;
    

    if (strcmp(path, "/") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "BIG", NULL, 0);
        filler(buf, "little", NULL, 0);
        for (int i = 0; i < mis_datos->numero_ficheros; i++) {
            filler(buf, mis_datos->nombre_ficheros[i], NULL, 0);
        }
    } else if (strcmp(path, "/BIG") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        for (int i = 0; i < mis_datos->numero_ficheros; i++) {
            if (strlen(mis_datos->contenido_ficheros[i]) > UMBRAL)
                filler(buf, mis_datos->nombre_ficheros[i], NULL, 0);
        }
    } else if (strcmp(path, "/little") == 0) {
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        for (int i = 0; i < mis_datos->numero_ficheros; i++) {
            if (strlen(mis_datos->contenido_ficheros[i]) <= UMBRAL)
                filler(buf, mis_datos->nombre_ficheros[i], NULL, 0);
        }
    } else {
        return -ENOENT;
    }
    return 0;
}

/***********************************
 * abrir un fichero (sólo lectura)
 ***********************************/
static int mi_open(const char *path, struct fuse_file_info *fi) {
    struct structura_mis_datos *mis_datos = 
        (struct structura_mis_datos *) fuse_get_context()->private_data;
    if ((fi->flags & O_ACCMODE) != O_RDONLY)
        return -EACCES;
    int idx = obtener_indice(path, mis_datos);
    if (idx < 0)
        return -ENOENT;
    fi->fh = idx;
    return 0;
}

/***********************************
 * leer datos de un fichero
 ***********************************/
static int mi_read(const char *path, char *buf, size_t size, off_t offset,
                   struct fuse_file_info *fi) {
    struct structura_mis_datos *mis_datos = 
        (struct structura_mis_datos *) fuse_get_context()->private_data;
    int idx = fi->fh;
    const char *data = mis_datos->contenido_ficheros[idx];
    size_t len = strlen(data);
    if (offset >= len)
        return 0;
    if (offset + size > len)
        size = len - offset;
    memcpy(buf, data + offset, size);
    return size;
}

/***********************************
 * operaciones FUSE
 ***********************************/
static struct fuse_operations basic_oper = {
    .getattr = mi_getattr,
    .readdir = mi_readdir,
    .open    = mi_open,
    .read    = mi_read,
};

/***********************************
 * función principal
 ***********************************/
int main(int argc, char *argv[]) {
    struct structura_mis_datos *mis_datos = malloc(sizeof(*mis_datos));
    // análisis parámetros de entrada
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
        error_parametros();

    mis_datos->fichero_inicial = strdup(argv[argc-2]);
    argv[argc-2] = argv[argc-1]; argv[argc-1] = NULL; argc--;
    leer_fichero(mis_datos);
    return fuse_main(argc, argv, &basic_oper, mis_datos);
}
