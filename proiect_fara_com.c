#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/sysmacros.h>

#define PATH_MAX 4096
FILE *fisier_com;
const char *izolare_global;

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

void invalid(FILE *fisier, const char *mesaj)
{
    fprintf(fisier, mesaj);
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

    strcat(permis, " ");

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

    strcat(permis, " ");

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

int lipsa_permisiuni(const char *permis)
{
    int lipsa = 1;

    if(permis == NULL) 
    {
        eroare("S-a incercat verificarea unui String NULL.");
    }

    if(strcmp(permis, "--- --- ---") != 0)
    {
        lipsa = 0;
    }

    return lipsa;
}

void izolare_fisier(const char *cale, char *nume_fisier) 
{
    char cale_izolare[PATH_MAX];

    snprintf(cale_izolare, sizeof(cale_izolare), "./%s/%s", izolare_global, nume_fisier);

    if (rename(cale, cale_izolare) == -1) 
    {
        eroare("Fisierul nu a putut fi mutat in directorul de izolare.");
    }

    chmod(cale_izolare, 000);
}

void fisier_suspect(const char *cale, char *nume_fisier, int *nr_fisiere_malitioase) 
{
    int pfd[2];
    if (pipe(pfd) == -1) 
    {
        eroare("Nu s-a putut crea pipe-ul");
    }

    pid_t spid = fork();

    if(spid == -1)
    {
        eroare("Operatia nu s-a putut efectua 1.");
    }
    if (spid == 0) 
    {
        close(pfd[0]); 
        int d = dup2(pfd[1], STDOUT_FILENO); 

        if(d == -1)
        {
            eroare("Dup2 a esuat.");
        }

        close(pfd[1]);
        close(pfd[0]);
        execlp("./verify_for_malicious.sh", "./verify_for_malicious.sh", cale, NULL);
        eroare("Nu s-a putut deschide scriptul sau exec a dat fail.");
    } 
    else 
    {
        int status;
        pid_t w;

        close(pfd[1]);

        do
        {
            w = wait(&status);

            if(w == -1)
            {   
                if(errno != ECHILD)
                {
                    eroare("Operatia nu s-a putut efectua 2.");
                }
                break;
            }

            if (WIFEXITED(status) == 1) 
            {
                fprintf(fisier_com, " ");
                char buffer[1024];
                read(pfd[0], buffer, sizeof(buffer));

                if(strstr(buffer, "safe"))
                {
                    buffer[4] = 0;
                }

                if (strstr(buffer, "safe") == NULL) 
                {
                    (*nr_fisiere_malitioase)++;
                    izolare_fisier(cale, nume_fisier);
                }

                close(pfd[0]);
                wait(NULL); 
            }
            else{
                fprintf(fisier_com, " ");
            }

        } while(21);
    }
}

const char *indentare(int nivel_indentare)
{
    static char indentare[1000];
    strcpy(indentare, ""); 

    int i;

    for(i = 0; i < nivel_indentare; i++)
    {
        strcat(indentare, "\t");
    }

    return indentare;
}

void parcurgere_recursiva(const char *director, int snapi, int nivel_indentare, int *nr_fisiere_malitioase)
{
    DIR *subdir = opendir(director);

    char cale[PATH_MAX];

    if(subdir == NULL)
    {
        eroare("Directorul pe care il voiam curent nu a putut fi deschis.");
    }

    fprintf(fisier_com, "Directorul a putut fi deschis: %s\n", director);

    struct dirent *dst;

    while((dst = readdir(subdir)) != NULL)
    {
        if(strcmp(dst->d_name, ".") == 0 || strcmp(dst->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(cale, sizeof(cale), "%s/%s", director, dst->d_name);
        fprintf(fisier_com, "Calea entitatii curente este: %s\n", cale);

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
            const char *p = prelucrare_permisiuni(st);

            char buffer[5000];
            char *modificare_director = ctime(&d.data_modificare);
            char *accesare_director = ctime(&d.ultima_accesare);
            const char *indent = indentare(nivel_indentare);
            snprintf(buffer, sizeof(buffer), "%sDirector\n%sCale: %s\n%sNume: %s\n%sDimensiune: %ld bytes\n%sModificare: %s%sUltima accesare: %s%sInode: %ld\n%sPermisiuni: %s\n\n", indent, indent, d.cale, indent, d.nume, indent, d.dimensiune, indent, modificare_director, indent, accesare_director, indent, d.inode, indent, p);

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer));
            if(written_bytes == -1)
            {
                eroare("Eroare la scrierea informatiilor despre director in snapshot.");
            }

            parcurgere_recursiva(cale, snapi, nivel_indentare + 1, nr_fisiere_malitioase);
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

            if(lipsa_permisiuni(p_fis)) 
            {
                fisier_suspect(fis.cale, fis.nume, nr_fisiere_malitioase);
            }

            char buffer[5000];
            char *modificare_fisier = ctime(&fis.data_modificare);
            char *accesare_fisier = ctime(&fis.ultima_accesare);
            const char *indent_fis = indentare(nivel_indentare);
            snprintf(buffer, sizeof(buffer), "%sFisier\n%sCale: %s\n%sNume: %s\n%sDimensiune: %ld bytes\n%sModificare: %s%sUltima accesare: %s%sInode: %ld\n%sPermisiuni: %s\n\n", indent_fis, indent_fis, fis.cale, indent_fis, fis.nume, indent_fis, fis.dimensiune, indent_fis, modificare_fisier, indent_fis, accesare_fisier, indent_fis, fis.inode, indent_fis, p_fis);

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
            const char *indent_leg = indentare(nivel_indentare);
            snprintf(buffer, sizeof(buffer), "%sLegatura simbolica\n%sCale: %s\n%sNume: %s\n%sDimensiune: %ld bytes\n%sModificare: %s%sUltima accesare: %s%sInode: %ld\n%sPermisiuni: %s\n\n", indent_leg, indent_leg, leg.cale, indent_leg, leg.nume, indent_leg, leg.dimensiune, indent_leg, modificare_legatura, indent_leg, accesare_legatura, indent_leg, leg.inode, indent_leg, p_leg);

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

int compara_snapi(const char *snapshot_vechi, const char *snapshot_nou)
{
    int snap_vechi = open(snapshot_vechi, O_RDONLY, S_IRUSR | S_IROTH | S_IRGRP);

    if(snap_vechi == -1)
    {
        eroare("Fisierul vechi de snapshot nu a putut fi deschis.");
    }

    int snap_nou = open(snapshot_nou, O_RDONLY, S_IRUSR | S_IROTH | S_IRGRP);

    if(snap_nou == -1)
    {
        eroare("Fisierul nou de snapshot nu a putut fi deschis.");
    }

    char linie_veche[5000];
    char linie_noua[5000];
    ssize_t bytes_cititi_vechi, bytes_cititi_nou;

    while((bytes_cititi_vechi = read(snap_vechi, linie_veche, sizeof(linie_veche))) > 0 && (bytes_cititi_nou = read(snap_nou, linie_noua, sizeof(linie_noua))) > 0)
    {
        if(bytes_cititi_vechi != bytes_cititi_nou || memcmp(linie_veche, linie_noua, bytes_cititi_vechi) != 0)
        {
            if((close(snap_nou) == -1) || (close(snap_vechi) == -1))
            {
                eroare("Eroare la inchiderea snapshot-urilor.");
            }
            return 1;
        }
    }

    if(bytes_cititi_vechi == -1 || bytes_cititi_nou == -1)
    {
        eroare("Eroare la citirea din fisierele de snapshot.");
    }

    if(close(snap_vechi) == -1)
    {
        eroare("Eroare la inchiderea snapshot-ului vechi.");
    }

    if(close(snap_nou) == -1)
    {
        eroare("Eroare la inchiderea snapshot-ului nou.");
    }

    return 0;
}

int main(int argc, char *argv[])
{
    fisier_com = fopen("flow_executie.txt", "w");

    if(fisier_com == NULL)
    {
        printf("Eroare la deschiderea fisierului care urmareste flow-ul executiei.\n");
    }

    if((argc < 5) || (argc > 15))
    {
        eroare("Numarul de argumente difera de cel asteptat.");
    }

    int i;
    int j;
    int index_output = -1;
    int index_izolare = -1;

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

    fprintf(fisier_com, "Nu exista argumente repetitive.\n");

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

    for(i = 1; i < argc; i++)
    {
        if((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "-S") == 0))
        {
            if((i + 1) >= argc)
            {
                eroare("Nu exista si directorul de izolare printre argumente.");
            }

            index_izolare = i + 1;
            break;
        }
    }

    if(index_izolare == -1)
    {
        eroare("Nu s-a precizat optiunea -s, deci nu se poate determina directorul de izolare.");
    }

    const char *output = argv[index_output];
    const char *izolare = argv[index_izolare];
    izolare_global = argv[index_izolare];

    fprintf(fisier_com, "Indexul lui -o este %d.\n", (index_output - 1));
    fprintf(fisier_com, "Indexul directorului de output este %d.\n", index_output);
    fprintf(fisier_com, "Indexul lui -s este %d.\n", (index_izolare - 1));
    fprintf(fisier_com, "Indexul directorului de izolare este %d.\n", index_izolare);

    struct stat out;

    if(lstat(output, &out) == -1)
    {
        eroare("Eroare la determinarea informatiilor despre directorul de output, ceva s-a intamplat cu lstat.");
    }

    if(S_ISDIR(out.st_mode) == 0)
    {
        invalid(fisier_com, "Nu a fost dat ca argument un director pentru output, este dat un fisier sau o legatura simbolica.");
    }

    struct stat izo;

    if(lstat(izolare, &izo) == -1)
    {
        eroare("Eroare la determinarea informatiilor despre directorul de izolare, ceva s-a intamplat cu lstat.");
    }

    if(S_ISDIR(izo.st_mode) == 0)
    {
        invalid(fisier_com, "Nu a fost dat ca argument un director pentru izolare, este dat un fisier sau o legatura simbolica.");
    }

    int status;
    pid_t cpid, w;

    for(i = 1; i < argc; i++)
    {
        if((i == index_output) || (i == index_output - 1) || (i == index_izolare) || (i == index_izolare - 1))
        {
            continue;
        }

        cpid = fork();

        if(cpid == -1)
        {
            eroare("Operatia nu s-a putut efectua.");
        }
        else if(cpid == 0)
        {
            fprintf(fisier_com, "PID-ul copilului este %d\n", (int)(getpid()));

            int nr_fisiere_malitioase = 0;

            const char *director = argv[i];
            struct stat st;
            struct director dir;

            fprintf(fisier_com, "Directorul introdus: %s\n", director);

            if(lstat(director, &st) == -1)
            {
                eroare("Eroare la determinarea informatiilor despre director, ceva s-a intamplat cu lstat.");
            }

            if(S_ISDIR(st.st_mode) == 0)
            {
                fprintf(fisier_com, "Argumentul cu numarul %d nu este un director, deci nu va fi procesat.\n", i);
                continue;
            }

            char nume_snapshot[PATH_MAX];
            char nume_snapshot_temporar[PATH_MAX];
            char nume_snapshot_vechi[PATH_MAX];

            snprintf(nume_snapshot, sizeof(nume_snapshot), "%s/snapshot_%s.txt", output, director);
            snprintf(nume_snapshot_temporar, sizeof(nume_snapshot_temporar), "%s/snapshot_%s_temp.txt", output, director);
            snprintf(nume_snapshot_vechi, sizeof(nume_snapshot_vechi), "%s/snapshot_%s_old.txt", output, director);

            int snap = open(nume_snapshot, O_RDONLY);

            if(snap < 0)
            {
                snap = open(nume_snapshot, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP);

                if(snap == -1)
                {
                    eroare("Fisierul nu a putut fi deschis/creat.");
                }

                fprintf(fisier_com, "Fisierul de snapshot a fost creat cu succes: %s.\n", nume_snapshot);
            }
            else
            {
                snap = open(nume_snapshot_temporar, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP);

                if(snap == -1)
                {
                    eroare("Fisierul nu a putut fi deschis/creat.");
                }

                fprintf(fisier_com, "Fisierul temporar a fost creat cu succes: %s.\n", nume_snapshot_temporar);
            }

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

            parcurgere_recursiva(director, snap, 1, &nr_fisiere_malitioase);

            if(close(snap) == -1)
            {
                eroare("Eroare la inchiderea snapshot-ului.");
            }

            snap = open(nume_snapshot_temporar, O_RDONLY);

            if(snap >= 0)
            { 
                if(close(snap) == -1)
                {
                    eroare("Eroare la inchiderea fisierului temporar.");
                }

                if(compara_snapi(nume_snapshot, nume_snapshot_temporar) == 1)
                {
                    if(rename(nume_snapshot, nume_snapshot_vechi) != 0)
                    {
                        eroare("Nu s-a putut redenumi fisierul curent ca cel vechi.");
                    }

                    if(rename(nume_snapshot_temporar, nume_snapshot) != 0)
                    {
                        eroare("Nu s-a putut redenumi fisierul temporar ca cel curent.");
                    }

                    fprintf(fisier_com, "Au fost detectate modificari: %s a fost redenumit %s\n", nume_snapshot, nume_snapshot_vechi);
                }
                else
                {
                    if(unlink(nume_snapshot_temporar) == -1)
                    {
                        eroare("Nu s-a putut sterge snapshot-ul temporar.");
                    }

                    fprintf(fisier_com, "Nu a aparut nici o modificare in directorul %s.\n", director);
                }
            }
            else
            {
                fprintf(fisier_com, "Snapshot-ul temporar nu exista, deoarece e prima rulare.\n");
            }

            exit(nr_fisiere_malitioase);
        }
    }

    int process_number = 1;

    do
    {
        w = wait(&status);

        if(w == -1)
        {   
            if(errno != ECHILD)
            {
                eroare("Operatia nu s-a putut efectua.");
            }
            break;
        }

        if(WIFEXITED(status)) 
        {
            fprintf(fisier_com, "S-au detectat %d fisiere malitioase.\n", WEXITSTATUS(status));
            fprintf(fisier_com, "Child process %d terminated with PID %d\n", process_number++, w);
        } 

    } while(21);

    fclose(fisier_com);

    return 0;
}