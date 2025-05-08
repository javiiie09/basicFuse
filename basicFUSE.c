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
 *  ./basicFUSE proverbiosycatares.txt punto_montaje
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

#define UMBRAL 256  // Umbral en bytes para BIG/little

/*
 *  Para usar los datos pasados a FUSE usar en las funciones:
 * 
	struct structura_mis_datos *mis_datos= (struct structura_mis_datos *) fuse_get_context()->private_data;
 * 
 * */

/***********************************
 * */
static int mi_getattr(const char *path, struct stat *stbuf)
{	struct structura_mis_datos *mis_datos= (struct structura_mis_datos *) fuse_get_context()->private_data;
	/* completar */
	int i;
    int res = 0;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0 || strcmp(path, "/BIG") == 0 || strcmp(path, "/little") == 0) {
        // Directorio raíz, BIG o little
        stbuf->st_mode  = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        /*stbuf->st_uid   = mis_datos->st_uid;
        stbuf->st_gid   = mis_datos->st_gid;
        stbuf->st_atime  = mis_datos->st_atime;
        stbuf->st_mtime  = mis_datos->st_mtime;
        stbuf->st_ctime  = mis_datos->st_ctime;*/
        stbuf->st_size  = 4096/*1024*/;
        stbuf->st_blocks= 2;

	 } else if ((i= buscar_fichero(path, mis_datos)) >= 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		
		stbuf->st_size = strlen(mis_datos->contenido_ficheros[i]);
		stbuf->st_blocks = stbuf->st_size/512 + (stbuf->st_size%512)? 1 : 0;
	} else
		res = -ENOENT;

	stbuf->st_uid = mis_datos->st_uid;
	stbuf->st_gid = mis_datos->st_gid;
	
	stbuf->st_atime = mis_datos->st_atime;
	stbuf->st_mtime = mis_datos->st_mtime;
	stbuf->st_ctime = mis_datos->st_ctime;

    return res;
	/*
	
	int i;
	
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0 || strcmp(path, "/BIG") == 0 || strcmp(path, "/little") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		stbuf->st_uid = mis_datos->st_uid;
		stbuf->st_gid = mis_datos->st_gid;
		
		stbuf->st_atime = mis_datos->st_atime;
		stbuf->st_mtime = mis_datos->st_mtime;
		stbuf->st_ctime = mis_datos->st_ctime;
		stbuf->st_size = 1024;
		stbuf->st_blocks = 2;
	
	} else if (strncmp(path, "/BIG/", 5) == 0 || strncmp(path, "/little/", 8) == 0) {
		// Ruta dentro de BIG o little
		const char *nombre = strchr(path + 1, '/') + 1;
		i = buscar_fichero(nombre, mis_datos);
		
		if (i < 0)
			res = -ENOENT;
		else if ((strncmp(path, "/BIG/", 5) == 0 && strlen(mis_datos->contenido_ficheros[i]) <= UMBRAL) ||
		         (strncmp(path, "/little/", 8) == 0 && strlen(mis_datos->contenido_ficheros[i]) > UMBRAL))
			res = -ENOENT;
		
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		
		stbuf->st_uid = mis_datos->st_uid;
		stbuf->st_gid = mis_datos->st_gid;
		
		stbuf->st_atime = mis_datos->st_atime;
		stbuf->st_mtime = mis_datos->st_mtime;
		stbuf->st_ctime = mis_datos->st_ctime;
		
		stbuf->st_size = strlen(mis_datos->contenido_ficheros[i]);
		stbuf->st_blocks = stbuf->st_size/512 + (stbuf->st_size%512)? 1 : 0;

	} else if ((i= buscar_fichero(path, mis_datos)) >= 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		
		stbuf->st_uid = mis_datos->st_uid;
		stbuf->st_gid = mis_datos->st_gid;
		
		stbuf->st_atime = mis_datos->st_atime;
		stbuf->st_mtime = mis_datos->st_mtime;
		stbuf->st_ctime = mis_datos->st_ctime;
		
		stbuf->st_size = strlen(mis_datos->contenido_ficheros[i]);
		stbuf->st_blocks = stbuf->st_size/512 + (stbuf->st_size%512)? 1 : 0;
	} else
		res = -ENOENT;

	return res;*/
}

/***********************************
 * */

static int mi_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	struct structura_mis_datos *mis_datos= (struct structura_mis_datos *) fuse_get_context()->private_data;
		
	/* completar */
	int i;
	
	//(void) offset;
	//(void) fi;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if(strcmp(path, "/") == 0){
		filler(buf, "BIG", NULL, 0);
		filler(buf, "little", NULL, 0);
		
		for (i=0; i< mis_datos->numero_ficheros; i++)
		{
			if (filler(buf, mis_datos->nombre_ficheros[i], NULL, 0) != 0)
				return -ENOMEM;
		}
		
	} else if(strcmp(path, "/BIG") == 0) {
		for (i=0; i< mis_datos->numero_ficheros; i++)
		{
			if (strlen(mis_datos->contenido_ficheros[i]) > UMBRAL)
				if (filler(buf, mis_datos->nombre_ficheros[i], NULL, 0) != 0)
					return -ENOMEM;
		}
		
	} else if(strcmp(path, "/little") == 0) {
		for (i=0; i< mis_datos->numero_ficheros; i++)
		{
			if (strlen(mis_datos->contenido_ficheros[i]) <= UMBRAL)
				if (filler(buf, mis_datos->nombre_ficheros[i], NULL, 0) != 0)
					return -ENOMEM;
		}
		
	} else{
		return -ENOENT;
	}
		
	
	return 0;
}

/***********************************
 * */
static int mi_open(const char *path, struct fuse_file_info *fi)
{
	struct structura_mis_datos *mis_datos= (struct structura_mis_datos *) fuse_get_context()->private_data;

	/*/*const char *nombre_fichero = strchr(path, '/');
	if (nombre_fichero == NULL)
		return -ENOENT;
	int i = buscar_fichero(path, mis_datos);

	if(i == -1)
		return -ENOENT;
	
	fi->flags |= O_RDONLY;
	fi->fh = i;guardamos el índice del fichero en el handle*/

	int i = buscar_fichero(path, mis_datos);

	if(i < 0){
		char new_path[256];

		if (strncmp(path, "/BIG/", 5) == 0) {
			snprintf(new_path, sizeof(new_path), "%s", path + 5);
			if(i < 0 || strlen(mis_datos->contenido_ficheros[i]) <= UMBRAL)
				return -ENOENT;
		} else if (strncmp(path, "/little/", 8) == 0) {
			snprintf(new_path, sizeof(new_path), "%s", path + 8);
			if(i < 0 || strlen(mis_datos->contenido_ficheros[i]) > UMBRAL)
				return -ENOENT;
		} else
			return -ENOENT;
		
		i = buscar_fichero(new_path, mis_datos);
		if (i < 0)
			return -ENOENT;
	}

	fi->flags |= O_RDONLY; // Set the file handle to read-only
	fi->fh = i; // Save the file index in the handle

	return 0;
}


/***********************************
 * */
static int mi_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	struct structura_mis_datos *mis_datos= (struct structura_mis_datos *) fuse_get_context()->private_data;
	
	int i = fi->fh;
	const char *data = mis_datos->contenido_ficheros[i];
	size_t len = strlen(data);

	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, data + offset, size);
	} else
		size = 0;

	return size;
}


/***********************************
 * operaciones FUSE
 * */
static struct fuse_operations basic_oper = {
	.getattr	= mi_getattr,
	.readdir	= mi_readdir,
	.open		= mi_open,
	.read		= mi_read,
};


/***********************************
 * */
int main(int argc, char *argv[])
{
	struct structura_mis_datos *mis_datos;
	
	mis_datos=malloc(sizeof(struct structura_mis_datos));
	
	// análisis parámetros de entrada
	if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) error_parametros();

	mis_datos->fichero_inicial = strdup(argv[argc-2]); // fichero donde están los capítulos
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    leer_fichero(mis_datos);
    
/*    int i;
    for(i=0; i<mis_datos->numero_ficheros; i++)
    {
		printf("----->  %s\n", mis_datos->nombre_ficheros[i]);
		printf("%s",mis_datos->contenido_ficheros[i]);
	}
	exit(0);
*/
	
	return fuse_main(argc, argv, &basic_oper, mis_datos);
}
