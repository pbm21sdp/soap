#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

typedef struct fisier{
    char nume[256];         
    off_t dimensiune;       
    time_t data_modificare; 
    ino_t inode;            
} fisier;

typedef struct director{
    char nume[256];         
    off_t dimensiune;       
    time_t data_modificare; 
    ino_t inode;            
} director;

typedef struct legatura_simbolica{
    char nume[256];         
    off_t dimensiune;       
    time_t data_modificare; 
    ino_t inode;           
} legatura_simbolica;

void eroare(const char *mesaj) 
{
    perror(mesaj); 
    exit(-1);      
}

void invalid(const char *mesaj) 
{
    printf(mesaj);      
    exit(EXIT_SUCCESS); 
}

void parcurgere_recursiva(const char *directorcale, int snapi) 
{
    DIR *subdir = opendir(directorcale);

    char cale[PATH_MAX];

    if (subdir == NULL)
    {
        eroare("Directorul pe care il voiam curent nu a putut fi deschis.");
    }

    struct dirent *dst; 

    while ((dst = readdir(subdir)) != NULL)
    {
        sprintf(cale, "%s/%s", directorcale, dst->d_name);

        struct stat st;

        if (lstat(cale, &st) == -1) 
        {
            eroare("Eroare la determinarea informatiilor despre entitatea curenta, ceva s-a intamplat cu lstat."); // apelam functia eroare si precizam ce mai exact nu a mers
        }

        if (S_ISDIR(st.st_mode) != 0) 
        {
            struct director d; 
            strcpy(d.nume, dst->d_name);
            d.dimensiune = st.st_size;
            d.data_modificare = st.st_mtime;
            d.inode = st.st_ino;

            char buffer[sizeof(d)];                                                                
            sprintf(buffer, "%s %ld %ld %ld\n", d.nume, d.dimensiune, d.data_modificare, d.inode); 

            ssize_t written_bytes = write(snapi, buffer, sizeof(buffer));
            if (written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre director in snapshot.");
            }

            parcurgere_recursiva(cale, snapi); 
        }                                             
        else if (S_ISREG(st.st_mode) != 0)            
        {
            struct fisier fis; 
            strcpy(fis.nume, dst->d_name);
            fis.dimensiune = st.st_size;
            fis.data_modificare = st.st_mtime;
            fis.inode = st.st_ino;

            char buffer[sizeof(fis)];                                                                      
            sprintf(buffer, "%s %ld %ld %ld\n", fis.nume, fis.dimensiune, fis.data_modificare, fis.inode); 

            ssize_t written_bytes = write(snapi, buffer, sizeof(buffer));
            if (written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre fisier in snapshot.");
            }
        }
        else if (S_ISLNK(st.st_mode) != 0) 
        {
            struct legatura_simbolica leg; 
            strcpy(leg.nume, dst->d_name);
            leg.dimensiune = st.st_size;
            leg.data_modificare = st.st_mtime;
            leg.inode = st.st_ino;

            char buffer[sizeof(leg)];                                                                      
            sprintf(buffer, "%s %ld %ld %ld\n", leg.nume, leg.dimensiune, leg.data_modificare, leg.inode); 

            ssize_t written_bytes = write(snapi, buffer, sizeof(buffer));
            if (written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre legatura simbolica in fisierul snapshot.");
            }
        }
    }
    closedir(subdir);
}

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
    {
        invalid("Numarul de argumente difera de cel asteptat.");
    }

    const char *director = argv[1]; 
    struct stat st; 

    printf("Directorul introdus: %s\n", director);

    if (lstat(director, &st) == -1) 
    {
        eroare("Eroare la determinarea informatiilor despre director, ceva s-a intamplat cu lstat."); 
    }

    if (S_ISDIR(st.st_mode) == 0) 
    {
        invalid("Argumentul introdus nu este un director."); 
    }

    DIR *direct = opendir(director); 

    if (direct == NULL)
    {
        eroare("Eroare la deschiderea directorului, s-a intamplat ceva cu opendir."); 
    }

    const char *nume_snapshot = "snap.txt";           
    mode_t mod = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; 

    int snapi = open(nume_snapshot, O_WRONLY | O_CREAT | O_TRUNC, mod); 
    if (snapi == -1)
    {
        eroare("Eroare la crearea si/sau deschiderea fisierului de snapshot."); 
    }

    parcurgere_recursiva(director, snapi);

    if (closedir(direct) == -1)
    {
        eroare("Eroare la inchiderea directorului."); 
    }

    if (close(snapi) == -1)
    {
        eroare("Eroare la inchiderea snapshot-ului."); 
    }

    return 0;
}