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
#include <time.h>

typedef struct fisier
{
    char cale[PATH_MAX];
    char nume[256];
    off_t dimensiune;
    time_t data_modificare;
    time_t ultima_accesare;
    ino_t inode;
} fisier;

typedef struct director
{
    char cale[PATH_MAX];
    char nume[256];
    off_t dimensiune; 
    time_t data_modificare;
    time_t ultima_accesare;
    ino_t inode;
} director;

typedef struct legatura_simbolica
{
    char cale[PATH_MAX];
    char nume[256];
    off_t dimensiune;
    time_t data_modificare;
    time_t ultima_accesare;
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

const char *prelucrare_permisiuni(struct stat st)
{
    static char permis[11];
    strcpy(permis, "");

    if(st.st_mode & S_IRUSR)
    {
        strcat(permis, "r");
    }
    else
    {
        strcat(permis, "-");
    }
    if(st.st_mode & S_IWUSR)
    {
        strcat(permis, "w");
    }
    else
    {
        strcat(permis, "-");
    }
    if(st.st_mode & S_IXUSR)
    {
        strcat(permis, "x");
    }
    else
    {
        strcat(permis, "-");
    }
    if(st.st_mode & S_IRGRP)
    {
        strcat(permis, "r");
    }
    else
    {
        strcat(permis, "-");
    }
    if(st.st_mode & S_IWGRP)
    {
        strcat(permis, "w");
    }
    else
    {
        strcat(permis, "-");
    }
    if(st.st_mode & S_IXGRP)
    {
        strcat(permis, "x");
    }
    else
    {
        strcat(permis, "-");
    }
    if(st.st_mode & S_IROTH)
    {
        strcat(permis, "r");
    }
    else
    {
        strcat(permis, "-");
    }
    if(st.st_mode & S_IWOTH)
    {
        strcat(permis, "w");
    }
    else
    {
        strcat(permis, "-");
    }
    if(st.st_mode & S_IXOTH)
    {
        strcat(permis, "x");
    }
    else
    {
        strcat(permis, "-");
    }

    return strdup(permis);
}

void parcurgere_recursiva(const char *director, int snapi)
{
    DIR *subdir = opendir(director);

    char cale[PATH_MAX];

    if(subdir == NULL)
    {
        eroare("Directorul pe care il voiam curent nu a putut fi deschis.");
    }

    printf("Directorul a putut fi deschis: %s\n", director);

    struct dirent *dst;

    while((dst = readdir(subdir)) != NULL)
    {
        if(strcmp(dst->d_name, ".") == 0 || strcmp(dst->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(cale, sizeof(cale), "%s/%s", director, dst->d_name);
        printf("Calea entitatii curente este: %s\n", cale);

        struct stat st;

        if(lstat(cale, &st) == -1)
        {
            eroare("Eroare la determinarea informatiilor despre entitatea curenta, ceva s-a intamplat cu lstat."); 
        }

        if(S_ISDIR(st.st_mode) != 0)
        {
            struct director d;
            strncpy(d.nume, dst->d_name, sizeof(d.nume));
            strncpy(d.cale, cale, sizeof(d.cale));
            d.dimensiune = st.st_size;
            d.data_modificare = st.st_mtime;
            d.ultima_accesare = st.st_atime;
            d.inode = st.st_ino;
            char *p = prelucrare_permisiuni(st);

            char buffer[5000];
            char *modificare_director = ctime(&d.data_modificare);
            char *accesare_director = ctime(&d.ultima_accesare);
            snprintf(buffer, sizeof(buffer), "Director\nCale: %s\nNume: %s\nDimensiune: %ld bytes\nModificare: %sUltima accesare: %sInode: %ld\nPermisiuni: %s\n\n", d.cale, d.nume, d.dimensiune, modificare_director, accesare_director, d.inode, p);

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer));
            if(written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre director in snapshot.");
            }

            parcurgere_recursiva(cale, snapi);
        }
        else if(S_ISREG(st.st_mode) != 0)
        {
            struct fisier fis;
            strncpy(fis.nume, dst->d_name, sizeof(fis.nume));
            strncpy(fis.cale, cale, sizeof(fis.cale));
            fis.dimensiune = st.st_size;
            fis.data_modificare = st.st_mtime;
            fis.ultima_accesare = st.st_atime;
            fis.inode = st.st_ino;
            const char *p_fis = prelucrare_permisiuni(st);

            char buffer[5000];
            char *modificare_fisier = ctime(&fis.data_modificare);
            char *accesare_fisier = ctime(&fis.ultima_accesare);
            snprintf(buffer, sizeof(buffer), "Fisier\nCale: %s\nNume: %s\nDimensiune: %ld bytes\nModificare: %sUltima accesare: %sInode: %ld\nPermisiuni: %s\n\n", fis.cale, fis.nume, fis.dimensiune, modificare_fisier, accesare_fisier, fis.inode, p_fis);

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer));
            if(written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre fisier in snapshot.");
            }
        }
        else if(S_ISLNK(st.st_mode) != 0)
        {
            struct legatura_simbolica leg;
            strncpy(leg.nume, dst->d_name, sizeof(leg.nume));
            strncpy(leg.cale, cale, sizeof(leg.cale));
            leg.dimensiune = st.st_size;
            leg.data_modificare = st.st_mtime;
            leg.ultima_accesare = st.st_atime;
            leg.inode = st.st_ino;
            const char *p_leg = prelucrare_permisiuni(st);

            char buffer[5000];
            char *modificare_legatura = ctime(&leg.data_modificare);
            char *accesare_legatura = ctime(&leg.ultima_accesare);
            snprintf(buffer, sizeof(buffer), "Legatura simbolica\nCale: %s\nNume: %s\nDimensiune: %ld bytes\nModificare: %sUltima accesare: %sInode: %ld\nPermisiuni: %s\n\n", leg.cale, leg.nume, leg.dimensiune, modificare_legatura, accesare_legatura, leg.inode, p_leg);

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer));
            if(written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre legatura simbolica in fisierul snapshot.");
            }
        }
    }

    if(closedir(subdir) == -1)
    {
        eroare("Eroare la inchiderea directorului.");
    }
}

int main(int argc, char *argv[])
{
    if((argc < 3) || (argc > 13))
    {
        invalid("Numarul de argumente difera de cel asteptat.");
    }

    int i;
    int j;
    int index_output = -1;

    for(i = 1; i < argc; i++)
    {
        for(j = 1; j < i; j++)
	    {
            if(strcmp(argv[i], argv[j]) == 0)
	        {
                eroare("Unele argumente se repeta.");
	        }
	    }
    }
    
    printf("Nu există argumente repetitive.\n");

    for(i = 1; i < argc; i++)
    {
        if((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "-O") == 0))
        {
            if((i + 1) >= argc)
            {
                eroare("Nu exista si directorul de output printre argumente.");
            }

            index_output = i + 1;
            break;
        }
    }

    if(index_output == -1)
    {
        eroare("Nu s-a precizat optiunea -o, deci nu se poate determina directorul de output.");
    }

    const char *output = argv[index_output];

    printf("Indexul lui -o este %d.\n", (index_output - 1));
    printf("Indexul directorului de output este %d.\n", index_output);

    struct stat out;

    if(lstat(output, &out) == -1)
    {
         eroare("Eroare la determinarea informatiilor despre directorul de output, ceva s-a intamplat cu lstat.");
    }

    if(S_ISDIR(out.st_mode) == 0)
    {
        invalid("Nu a fost dat ca argument un director pentru output, este dat un fisier sau o legatura simbolica.");
    }

    for(i = 1; i < argc; i++)
    {
        if((i == index_output) || (i == index_output - 1))
        {
            continue;
        }

        const char *director = argv[i];
        struct stat st;
        struct director dir;

        printf("Directorul introdus: %s\n", director);

        if(lstat(director, &st) == -1)
        {
            eroare("Eroare la determinarea informatiilor despre director, ceva s-a intamplat cu lstat.");
        }

        if(S_ISDIR(st.st_mode) == 0)
        {
            printf("Argumentul cu numarul %d nu este un director, deci nu va fi procesat.\n", i);
            continue;
        }

        char nume_snapshot[PATH_MAX];
        snprintf(nume_snapshot, sizeof(nume_snapshot), "%s/snapshot_%s.txt", output, director);

        int snap = open(nume_snapshot, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP);
        if(snap == -1)
        {
            eroare("Fisierul nu a putut fi deschis/creat.");
        }

        printf("Fisierul a fost creat cu succes: %s\n", nume_snapshot);

        dir.dimensiune = st.st_size;
        dir.data_modificare = st.st_mtime;
        dir.ultima_accesare = st.st_atime;
        dir.inode = st.st_ino;
        const char *p_init = prelucrare_permisiuni(st);

        char buffer[5000];
        char *modificare_initial = ctime(&dir.data_modificare);
        char *accesare_initial = ctime(&dir.ultima_accesare);
        snprintf(buffer, sizeof(buffer), "Director initial\nNume: %s\nDimensiune: %ld bytes\nModificare: %sUltima accesare: %sInode: %ld\nPermisiuni: %s\n\n", director, dir.dimensiune, modificare_initial, accesare_initial, dir.inode, p_init);

        ssize_t written_bytes = write(snap, buffer, strlen(buffer));
        if(written_bytes == -1)
        {
            eroare("Eroare la scrierea informatiilor despre director in snapshot.");
        }

        parcurgere_recursiva(director, snap);

        if(close(snap) == -1)
        {
            eroare("Eroare la inchiderea snapshot-ului.");
        }
    }

    return 0;
}