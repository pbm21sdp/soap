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

typedef struct fisier
{
    char nume[256];
    off_t dimensiune;
    time_t data_modificare;
    ino_t inode;
} fisier;

typedef struct director
{
    char nume[256];
    off_t dimensiune;
    time_t data_modificare;
    ino_t inode;
} director;

typedef struct legatura_simbolica
{
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

void parcurgere_recursiva(const char *director, int snapi)
{
    DIR *subdir = opendir(director);

    char cale[PATH_MAX];

    if (subdir == NULL)
    {
        eroare("Directorul pe care il voiam curent nu a putut fi deschis.");
    }

    printf("Directorul a putut fi deschis: %s\n", director);

    struct dirent *dst;

    while ((dst = readdir(subdir)) != NULL)
    {
        if (strcmp(dst->d_name, ".") == 0 || strcmp(dst->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(cale, sizeof(cale), "%s/%s", director, dst->d_name);
        printf("Calea entitatii curente este: %s\n", cale);

        struct stat st;

        if (lstat(cale, &st) == -1)
        {
            eroare("Eroare la determinarea informatiilor despre entitatea curenta, ceva s-a intamplat cu lstat."); // apelam functia eroare si precizam ce mai exact nu a mers
        }

        if (S_ISDIR(st.st_mode) != 0)
        {
            struct director d;
            strncpy(d.nume, dst->d_name, sizeof(d.nume));
            d.dimensiune = st.st_size;
            d.data_modificare = st.st_mtime;
            d.inode = st.st_ino;

            char buffer[5000];
            snprintf(buffer, sizeof(buffer), "Nume: %s Dimensiune: %ld Modificare: %ld Inode: %ld\n", d.nume, d.dimensiune, d.data_modificare, d.inode);

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer));
            if (written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre director in snapshot.");
            }

            parcurgere_recursiva(cale, snapi);
        }
        else if (S_ISREG(st.st_mode) != 0)
        {
            struct fisier fis;
            strncpy(fis.nume, dst->d_name, sizeof(fis.nume));
            fis.dimensiune = st.st_size;
            fis.data_modificare = st.st_mtime;
            fis.inode = st.st_ino;

            char buffer[5000];
            snprintf(buffer, sizeof(buffer), "Nume: %s Dimensiune: %ld Modificare: %ld Inode: %ld\n", fis.nume, fis.dimensiune, fis.data_modificare, fis.inode);

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer));
            if (written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre fisier in snapshot.");
            }
        }
        else if (S_ISLNK(st.st_mode) != 0)
        {
            struct legatura_simbolica leg;
            strncpy(leg.nume, dst->d_name, sizeof(leg.nume));
            leg.dimensiune = st.st_size;
            leg.data_modificare = st.st_mtime;
            leg.inode = st.st_ino;

            char buffer[5000];
            snprintf(buffer, sizeof(buffer), "Nume: %s Dimensiune: %ld Modificare: %ld Inode: %ld\n", leg.nume, leg.dimensiune, leg.data_modificare, leg.inode);

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer));
            if (written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre legatura simbolica in fisierul snapshot.");
            }
        }
    }

    if (closedir(subdir) == -1)
    {
        eroare("Eroare la inchiderea directorului.");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        invalid("Numarul de argumente difera de cel asteptat.");
    }

    const char *director = argv[1];
    struct stat st;
    struct director dir;

    printf("Directorul introdus: %s\n", director);

    if (lstat(director, &st) == -1)
    {
        eroare("Eroare la determinarea informatiilor despre director, ceva s-a intamplat cu lstat.");
    }

    if (S_ISDIR(st.st_mode) == 0)
    {
        invalid("Argumentul introdus nu este un director.");
    }

    int snap = open("snapshot2.txt", O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP);
    if (snap == -1)
    {
        eroare("Fisierul nu a putut fi deschis/creat.\n");
    }

    printf("Fisierul a fost creat cu succes: snapshot2.txt\n");

    dir.dimensiune = st.st_size;
    dir.data_modificare = st.st_mtime;
    dir.inode = st.st_ino;

    char buffer[5000];
    snprintf(buffer, sizeof(buffer), "Nume: %s Dimensiune: %ld Modificare: %ld Inode: %ld\n", director, dir.dimensiune, dir.data_modificare, dir.inode);

    ssize_t written_bytes = write(snap, buffer, strlen(buffer));
    if (written_bytes == -1)
    {
        eroare("Eroare la scrierea informatiilor despre director in snapshot.");
    }

    parcurgere_recursiva(director, snap);

    if (close(snap) == -1)
    {
        eroare("Eroare la inchiderea snapshot-ului.");
    }

    return 0;
}