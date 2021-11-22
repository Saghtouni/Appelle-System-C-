

// ==== include ===================================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

// =============================define============================================================
#define SRC 1
#define DST 2



//=============================Structure sparse_header============================================
typedef struct sparse_header {
        uint32_t magic;         /* 0xed26ff3a */
        uint16_t major_version; /* (0x1) - reject images with higher major versions */
        uint16_t minor_version; /* (0x0) - allow images with higer minor versions */
        uint16_t file_hdr_sz;   /* 28 bytes for first revision of the file format */
        uint16_t chunk_hdr_sz;  /* 12 bytes for first revision of the file format */
        uint32_t blk_sz;        /* block size in bytes, must be a multiple of 4 (4096) */
        uint32_t total_blks;    /* total blocks in the non-sparse output image */
        uint32_t total_chunks;  /* total chunks in the sparse input image */
        uint32_t image_checksum;/* CRC32 checksum of the original data, counting "don't care" */
                                /* as 0. Standard 802.3 polynomial, use a Public Domain */
                                /* table implementation */
    } sparse_header_t;


//=============================Structure chunk_header==============================================
    typedef struct chunk_header {
        uint16_t chunk_type;    /* 0xCAC1 -> raw; 0xCAC2 -> fill; 0xCAC3 -> don't care */
        uint16_t reserved1;
        uint32_t chunk_sz;      /* in blocks in output image */
        uint32_t total_sz;      /* in bytes of chunk input file including chunk header and data */
    } chunk_header_t;


//=============================Structure blocTest=================================================
typedef struct blocTest
{
    unsigned char tab[1024]; // tableau pour stocker chaque bloc 
    int nbrBloc; // nombre de bloc dans chauqe chunks 
    int testBloc; // 1 bloc plein ou 0 bloc vide pour chaque bloc 
} blocTest;

//===========Affichage des bytes dans le consol===================================================
void affichage(int size, unsigned char *bytesFile, int n) 
{
	int i;

	for(i = n; i < size; i++) {
		printf("%0X", bytesFile[i] );

		if( (i % 4) == 0) 
			printf(" ");

		if( (i % 16) == 0) 
			printf("\n");
	}

	printf("\n\n 1/2 bloc \n\n");
}

//==Deviser chque bloc sur 2 par 4 bytes pour comparer si c'est un bloc Raw ou Fill=========================
void halfBloc(unsigned char *bytesFile, unsigned char *halfBytes, int byteStart) 
{

	int n = byteStart;
	int x = 0;
	int i;

	for(i = 0; i < 1024/2; i++){

		halfBytes[x] = bytesFile[n];
		x++;
		n++;

		if((n % 4) == 0) 
			n += 4;
	}

}

//======Comparer les deux 1/2 tableaux du bloc, si il est fill test est à 1 et si il est raw test est à 0===
int testBloc(unsigned char *halfBytes, unsigned char *halfBytes1) {

	int test = 0;
	int i;

	for(i = 0; i < 1024/2; i++) {
		if(halfBytes[i] != halfBytes1[i])
			test = 1;
	}

	return test;
}

//=====================main()===================================================================
int main (int argc, char *argv[])
{

//--------------------Variable-------------------------------------------------------------------
	struct stat fileStat;
	int fd_in;
	int fd_out;
	unsigned char *bytesFile = NULL;
	unsigned char *halfBytes = NULL;
	unsigned char *halfBytes1 = NULL;
	ssize_t  size_file;
	ssize_t  bloc_size;
	int i, j;
	int nbrchunks = 1;
	int test;
	int nbrBlocFill = 0;
    int nbrBlocRaw = 0;
    int size = 0;
  

	if(argc != 3)  
       exit(1);
 
//--------------Récuperation taille de fichier .img -----------------------------------------------

    if(stat(argv[SRC],&fileStat) < 0)    
        exit(1);

	size_file = fileStat.st_size;
//---------Tester si le fichier est un fichier image------------------------------------------------
	if(size_file % 4 != 0){
		exit(1);
	}

//--------------Réservation memoire pour les trois tableaux et p ------------------------------------

	blocTest *p[(size_file/1024 + 1)];
	for(i = 0; i <(size_file/1024 + 1); i++){
		p[i] = malloc (sizeof(blocTest));
	} 

	if(p == NULL){
		exit(1);
	}

	bytesFile = malloc(1024 * sizeof(unsigned char));
	if(bytesFile == NULL)
		exit(1);

	halfBytes = malloc(1024/2 * sizeof(unsigned char));
	if(halfBytes == NULL)
		exit(1);

	halfBytes1 = malloc(1024/2 * sizeof(unsigned char));
	if(halfBytes1 == NULL)
		exit(1);

//---------------Ouverture de fichier source -------------------------------------------------------
	fd_in = open(argv[SRC], O_RDONLY);
	if(fd_in < 0) {
		perror("open");
		exit(1);
	}
//---------------Création de fichier destination --------------------------------------------------
	fd_out = creat(argv[DST], O_RDWR);
	 if(fd_out < 0) {
		perror("creat");
		exit(1);
	}

//-------------Parcourir le ficher source chaque 1024 Bytes------------------------------------------

	for(bloc_size = 1024; bloc_size <= size_file; bloc_size += 1024) {

//--------------Récuperer les 1024 Bytes-------------------------------------------------------------
		if(read(fd_in, bytesFile, 1024) < 0) {
			perror("read");
			exit(1);
		}

//--------------Diviser le bloc sur 2 ---------------------------------------------------------------

		halfBloc(bytesFile, halfBytes, 0);
		halfBloc(bytesFile, halfBytes1, 4);
//-----------Ecriture les deux fichiers de la moitié de fichier source ------------------------------
		if(write(fd_out, halfBytes, 1024/2) < 0){
			perror("write");
			exit(1);
		}

//-----------Tester le bloc--------------------------------------------------------------------------
		for(i = 0; i < 1024/2; i++) {
			if(testBloc(halfBytes, halfBytes1) == 1){
				test = 1;
				
			}
			else {
				test = 0;
				
			}
		}

//-----------remplir la structure blocTest -------------------- -------------------------------------
		if(test == 1){
				nbrBlocRaw = 0;
				nbrBlocFill++;
				p[size]->testBloc = test;
				p[size]->nbrBloc = nbrBlocFill ;

		}
		else {
				nbrBlocFill = 0;
				nbrBlocRaw++;
				p[size]->testBloc = test;
				p[size]->nbrBloc = nbrBlocRaw;
		}
		
		for(i = 0; i < 1024; i++) {
            	p[size]->tab[i] = bytesFile[i];
        	}
		
		size++;

	}
//----------parcourir le nombre des blocs pour trouver le nombre de chunks---------------------------
	for(i = 1; i <= size_file/1024; i++) {
		if(p[i-1]->testBloc != p[i]->testBloc){
			nbrchunks++;
		}	
	}

//----------Remplir l'entête de fichier sparse et la créer-------------------------------------------------------
	sparse_header_t s = {0xed26ff3a,
		 0x1, 
		 0x0,
		 28,
		 12,
		 1024,
		 size_file/1024, 
		 nbrchunks,
		 0
	};

	if(write(fd_out, &s, 28) < 0){
			perror("write1");
			exit(1);
		}

    p[size_file/1024]->testBloc = -1;

//---------parcourir le nombre des blocs pour Remplir l'entête de chunks et la créer avec les blocs qui convient---
	for(i = 1; i <= size_file/1024; i++) {
		if(p[i-1]->testBloc != p[i]->testBloc) {

			if(p[i-1]->testBloc == 0) {

				chunk_header_t c = {0xCAC2,
									0,
									p[i-1]->nbrBloc,
									4 + 12
								   };

				if(write(fd_out, &c, 12) < 0){
					perror("write1");
					exit(1);
				}

				if(write(fd_out, p[i-1]->tab, 4) < 0){
					perror("write1");
					exit(1);
				}					

			}

			if(p[i-1]->testBloc == 1) {

				chunk_header_t c = {0xCAC1,
									0,
									p[i-1]->nbrBloc,
									(p[i-1]->nbrBloc * 1024) + 12
								   };

				if(write(fd_out, &c, 12) < 0){
					perror("write1");
					exit(1);
				}

				for(j = i-p[i-1]->nbrBloc; j < i; j++) {

					if(write(fd_out, p[j]->tab, 1024) < 0){
						perror("write1");
						exit(1);
					}

				}

			}
	
		}	

	}

// ---- fermeture des fichiers -----------------------------------------------------------------------

	if(close(fd_in) < 0){
        perror("fermeture du fichier source (décompressé).");
        exit(1);
    }

    if(close(fd_out) < 0){
        perror("fermeture du fichier de destination1.");
        exit(1);
    }

// ---- libérer les trois tableaux et p ----------------------------------------------------------------

    /*for(i = 0; i <(size_file/1024 + 1); i++){
		free(p[i]);
	} */
    //free(p);
	free(bytesFile);
	free(halfBytes);
	free(halfBytes1);

	return 0;
	
}