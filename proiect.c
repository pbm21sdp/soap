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
#include <sys/wait.h>

FILE *fisier_com; // un fisier obisnuit care va retine doar comentariile despre flow-ul executiei, nu are nicio influenta asupra programului

// structura care va retine informatiile despre un regular file
typedef struct fisier
{
    char cale[PATH_MAX];    // calea fisierului
    char nume[256];         // numele fisierului
    off_t dimensiune;       // dimensiune totala in bytes
    time_t data_modificare; // momentul ultimei modificari (st_mtime din struct stat)
    time_t ultima_accesare; // momentul ultimei accesari (st_atime din struct stat)
    ino_t inode;            // numarul inode-ului
} fisier;

// structura care va retine informatiile despre un director
typedef struct director
{
    char cale[PATH_MAX];    // calea directorului
    char nume[256];         // numele directorului
    off_t dimensiune;       // dimensiune totala in bytes
    time_t data_modificare; // momentul ultimei modificari (st_mtime din struct stat)
    time_t ultima_accesare; // momentul ultimei accesari (st_atime din struct stat)
    ino_t inode;            // numarul inode-ului
} director;

// structura care va retine informatiile despre o legatura simbolica
typedef struct legatura_simbolica
{
    char cale[PATH_MAX];    // calea legaturii simbolice
    char nume[256];         // numele legaturii simbolice
    off_t dimensiune;       // dimensiune totala in bytes
    time_t data_modificare; // momentul ultimei modificari (st_mtime din struct stat)
    time_t ultima_accesare; // momentul ultimei accesari (st_atime din struct stat)
    ino_t inode;            // numarul inode-ului
} legatura_simbolica;

void eroare(const char *mesaj) // functie care primeste ca parametru un String si semnaleaza o eroare de executie sau de logica
{
    perror(mesaj); // afiseaza un mesaj de eroare mai simplu
    exit(-1); // termina programul cu un cod de eroare
}

void invalid(FILE *fisier, const char *mesaj) // functie care primeste ca parametru un String si un fisier si semnaleaza o "diferenta" intre cerinta si ceea ce am obtinut, dar nu o eroare
{
    fprintf(fisier, mesaj); // afiseaza un mesaj informativ in fisierul primit ca parametru, care sa ne atentioneze ca nu respectam cerinta
    exit(EXIT_SUCCESS); // opreste executia programului pentru ca nu are rost sa o continuam daca nu facem ce se cere, dar nu cu cod de eroare, ci cu cod 0
}

const char *prelucrare_permisiuni(struct stat st) // functie care creeaza un nou String care retine permisiunile entitatii, primeste ca parametru o structura stat
{
    static char permis[11]; // string permis de 11 caractere declarat static pentru a ramane in memorie cat am nevoie de el, chiar si dupa ce ies din functie
    strcpy(permis, "");     // se initializeaza string-ul la fiecare iteratie cu string-ul gol, practic "resetez" string-ul ca sa retina permisiunile pentru fiecare entitate

    if(st.st_mode & S_IRUSR) // verifica daca user are drept de read
    {
        strcat(permis, "r"); // daca are, concateneaza la string-ul "permis" litera r, pe prima pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de read, concateneaza la string-ul "permis" caracterul "-", pe prima pozitie
    }
    if(st.st_mode & S_IWUSR) // verifica daca user are drept de write
    {
        strcat(permis, "w"); // daca are, concateneaza la string-ul "permis" litera w, pe a doua pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de write, concateneaza la string-ul "permis" caracterul "-", pe a doua pozitie
    }
    if(st.st_mode & S_IXUSR) // verifica daca user are drept de execute
    {
        strcat(permis, "x"); // daca are, concateneaza la string-ul "permis" litera w, pe a treia pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de execute, concateneaza la string-ul "permis" caracterul "-", pe a treia pozitie
    }
    if(st.st_mode & S_IRGRP) // verifica daca grup are drept de read
    {
        strcat(permis, "r"); // daca are, concateneaza la string-ul "permis" litera r, pe a patra pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de read, concateneaza la string-ul "permis" caracterul "-", pe a patra pozitie
    }
    if(st.st_mode & S_IWGRP) // verifica daca grup are drept de write
    {
        strcat(permis, "w"); // daca are, concateneaza la string-ul "permis" litera w, pe a cincea pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de write, concateneaza la string-ul "permis" caracterul "-", pe a cincea pozitie
    }
    if(st.st_mode & S_IXGRP) // verifica daca grup are drept de execute
    {
        strcat(permis, "x"); // daca are, concateneaza la string-ul "permis" litera x, pe a sasea pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de execute, concateneaza la string-ul "permis" caracterul "-", pe a sasea pozitie
    }
    if(st.st_mode & S_IROTH) // verifica daca other are drept de read
    {
        strcat(permis, "r"); // daca are, concateneaza la string-ul "permis" litera r, pe a saptea pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de read, concateneaza la string-ul "permis" caracterul "-", pe a saptea pozitie
    }
    if(st.st_mode & S_IWOTH) // verifica daca other are drept de write
    {
        strcat(permis, "w"); // daca are, concateneaza la string-ul "permis" litera w, pe a opta pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de write, concateneaza la string-ul "permis" caracterul "-", pe a opta pozitie
    }
    if(st.st_mode & S_IXOTH) // verifica daca other are drept de execute
    {
        strcat(permis, "x"); // daca are, concateneaza la string-ul "permis" litera x, pe a noua pozitie
    }
    else
    {
        strcat(permis, "-"); // daca nu are drept de execute, concateneaza la string-ul "permis" caracterul "-", pe a noua pozitie
    }

    return strdup(permis); // returneaza o copie a string-ului
}

const char *indentare(int nivel_indentare) // functie care returneaza un String care va reprezenta nivelul indentarii pentru a crea o ierarhie, care primeste ca parametru un intreg reprezentand nivelul indentarii pe care ne aflam
{
    static char indentare[1000]; // string indentare de maxim 1000 caractere declarat static pentru a ramane in memorie cat am nevoie de el, chiar si dupa ce ies din functie
    strcpy(indentare, ""); // se initializeaza string-ul la fiecare iteratie cu string-ul gol, practic "resetez" string-ul ca sa retina nivelul indentarii pe care ne aflam

    int i; // variabila care va fi folosita pe post de iterator

    for(i = 0; i < nivel_indentare; i++) // bucla care se executa de atatea ori cate indica nivelul indentarii, practic construind un string care este " " * nivel_indentare
    {
        strcat(indentare, "\t"); // concateneaza la fiecare iteratie cate un tab string-ului indentare
    }

    return indentare; // returneaza string-ul construit indentare
}

void parcurgere_recursiva(const char *director, int snapi, int nivel_indentare) // primeste ca parametru numele unei entitati, descriptorul pentru fisierul de snapshot si nivelul curent al indentarii
{
    // pentru parcurgerea arborelui se deschide directorul cu functia opendir

    DIR *subdir = opendir(director); // daca subdir este NULL inseamna ca ceva nu a mers cu opendir si am intampinat o eroare

    char cale[PATH_MAX]; // folosit pentru a retine fiecare cale nou construita

    if(subdir == NULL) // opendir returneaza NULL daca a aparut o eroare
    {
        eroare("Directorul pe care il voiam curent nu a putut fi deschis."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    fprintf(fisier_com, "Directorul a putut fi deschis: %s\n", director); // mesaj afisat in fisier pentru a urmari flow-ul executiei

    /* se citeste pe rand cate o intrare din director, apeland functia readdir. 
    fiecare apel al acestei functii va returna un pointer la o structura struct dirent in care se vor gasi informatii despre intrarea in 
    director citita. 
    intrarile vor fi parcurse una dupa alta pana cand se ajunge la ultima inregistrare. 
    in momentul in care nu mai exista inregistrari in directorul citit, functia readdir va returna NULL. 
    singura informatie care poate fi extrasa (conform POSIX) din structura dirent este numele intrarii in director. 
    toate celelalte informatii despre intrarea citita se pot afla apeland in continuare functiile stat, fstat sau lstat.*/

    // inainte de orice citire am nevoie de zona de memorie care va stoca toate informatiile, adica struct dirent

    struct dirent *dst; // dst de la director struct nu de la destination

    // trebuie sa apelez readdir pana cand ajung la ultima inregistrare din director, dupa care primesc ca return value NULL cand am epuizat inregistrarile
    // readdir returneaza un pointer la o structura struct dirent, deci dst al meu va fi de fapt readdir

    while ((dst = readdir(subdir)) != NULL) // parcugrem cat timp exista inregistrari in director
    {
        if(strcmp(dst->d_name, ".") == 0 || strcmp(dst->d_name, "..") == 0) // verifica daca entitatea curenta este directorul dat ca argument sau "parintele" acestuia
        {
            continue; // sare peste acea iteratie pentru a preveni bucla infinita
        }

        snprintf(cale, sizeof(cale), "%s/%s", director, dst->d_name); // snprintf este folosit pentru a construi calea fiecarei entitati din subarbore (preferat in locul lui sprintf pentru ca snprintf previne depasirea bufferului)
        fprintf(fisier_com, "Calea entitatii curente este: %s\n", cale); // mesaj afisat in fisier pentru a urmari flow-ul executiei

        struct stat st; // structura care va retine atributele entitatii

        if(lstat(cale, &st) == -1) // daca lstat returneaza -1 inseamna ca a aparut o eroare
        {
            eroare("Eroare la determinarea informatiilor despre entitatea curenta, ceva s-a intamplat cu lstat."); // apelam functia eroare si precizam ce mai exact nu a mers
        }

        if(S_ISDIR(st.st_mode) != 0) // verifica daca entitatea curenta este un director
        {
            struct director d;                            // definim o structura care retine informatii despre un director
            strncpy(d.nume, dst->d_name, sizeof(d.nume)); // copiaza in campul nume din struct director numele preluat din struct dirent
            strncpy(d.cale, cale, sizeof(d.cale));        // copiaza in campul cale din struct director calea concatenata mai sus
            d.dimensiune = st.st_size;                    // atribuie campului dimensiune din struct director valoarea preluata din st.st_size
            d.data_modificare = st.st_mtime;              // atribuie campului data_modificare din struct director timestamp-ul preluat din st.st_mtime
            d.ultima_accesare = st.st_atime;              // atribuie campului ultima_accesare din struct director timestamp-ul preluat din st.st_atime
            d.inode = st.st_ino;                          // atribuie campului inode din struct director valoarea preluata din st.st_ino
            const char *p = prelucrare_permisiuni(st);          // atribuie unui pointer p de tip char valoarea returnata de functia prelucrare_permisiuni, dand ca parametru struct stat-ul definit mai sus

            char buffer[5000]; // buffer care va retine informatia formatata
            char *modificare_director = ctime(&d.data_modificare); // atribuie unui pointer modificare_director de tip char timestamp-ul obtinut prin prelucrarea cu ctime a valorii din campul data_modificare
            char *accesare_director = ctime(&d.ultima_accesare); // atribuie unui pointer accesare_director de tip char timestamp-ul obtinut prin prelucrarea cu ctime a valorii din campul ultima_accesare
            const char *indent = indentare(nivel_indentare); // atribuie unui pointer indent de tip char indentarea obtinuta in urma executiei functiei "indentare"
            snprintf(buffer, sizeof(buffer), "%sDirector\n%sCale: %s\n%sNume: %s\n%sDimensiune: %ld bytes\n%sModificare: %s%sUltima accesare: %s%sInode: %ld\n%sPermisiuni: %s\n\n", indent, indent, d.cale, indent, d.nume, indent, d.dimensiune, indent, modificare_director, indent, accesare_director, indent, d.inode, indent, p); // folosesc snprintf ca sa obtin un string formatat asa cum vreau eu

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer)); // scrie in fisier informatia formatata
            if(written_bytes == -1) // daca am obtinut -1 inseamna ca apelul write() s-a terminat cu eroare
            {
                eroare("Eroare la scrierea informatiilor despre director in snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
            }

            parcurgere_recursiva(cale, snapi, nivel_indentare + 1); // daca este director, se apeleaza recursiv functia pentru a parcurge directorul gasit
        }
        else if(S_ISREG(st.st_mode) != 0) // verifica daca entitatea curenta este un fisier
        {
            struct fisier fis;                                // definim o structura care retine informatii despre un fisier regular
            strncpy(fis.nume, dst->d_name, sizeof(fis.nume)); // copiaza in campul nume din struct fisier numele preluat din struct dirent
            strncpy(fis.cale, cale, sizeof(fis.cale));        // copiaza in campul cale din struct fisier calea concatenata mai sus
            fis.dimensiune = st.st_size;                      // atribuie campului dimensiune din struct fisier valoarea preluata din st.st_size
            fis.data_modificare = st.st_mtime;                // atribuie campului data_modificare din struct fisier timestamp-ul preluat din st.st_mtime
            fis.ultima_accesare = st.st_atime;                // atribuie campului ultima_accesare din struct fisier timestamp-ul preluat din st.st_atime
            fis.inode = st.st_ino;                            // atribuie campului inode din struct fisier valoarea preluata din st.st_ino
            const char *p_fis = prelucrare_permisiuni(st);    // atribuie unui pointer p_fis de tip char valoarea returnata de functia prelucrare_permisiuni, dand ca parametru struct stat-ul definit mai sus

            char buffer[5000]; // buffer care va retine informatia formatata
            char *modificare_fisier = ctime(&fis.data_modificare); // atribuie unui pointer modificare_fisier de tip char timestamp-ul obtinut prin prelucrarea cu ctime a valorii din campul data_modificare
            char *accesare_fisier = ctime(&fis.ultima_accesare); // atribuie unui pointer accesare_fisier de tip char timestamp-ul obtinut prin prelucrarea cu ctime a valorii din campul ultima_accesare
            const char *indent_fis = indentare(nivel_indentare); // atribuie unui pointer indent_fis de tip char indentarea obtinuta in urma executiei functiei "indentare"
            snprintf(buffer, sizeof(buffer), "%sFisier\n%sCale: %s\n%sNume: %s\n%sDimensiune: %ld bytes\n%sModificare: %s%sUltima accesare: %s%sInode: %ld\n%sPermisiuni: %s\n\n", indent_fis, indent_fis, fis.cale, indent_fis, fis.nume, indent_fis, fis.dimensiune, indent_fis, modificare_fisier, indent_fis, accesare_fisier, indent_fis, fis.inode, indent_fis, p_fis); // folosesc snprintf ca sa obtin un string formatat asa cum vreau eu

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer)); // scrie in fisier informatia formatata
            if(written_bytes == -1) // daca am obtinut -1 inseamna ca apelul write() s-a terminat cu eroare
            {
                eroare("Eroare la scrierea informatiilor despre fisier in snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
            }
        }
        else if(S_ISLNK(st.st_mode) != 0) // verifica daca entitatea curenta este o legatura simbolica
        {
            struct legatura_simbolica leg;                    // definim o structura care retine informatii despre o legatura simbolica
            strncpy(leg.nume, dst->d_name, sizeof(leg.nume)); // copiaza in campul nume din struct legatura numele preluat din struct dirent
            strncpy(leg.cale, cale, sizeof(leg.cale));        // copiaza in campul cale din struct legatura calea concatenata mai sus
            leg.dimensiune = st.st_size;                      // atribuie campului dimensiune din struct legatura valoarea preluata din st.st_size
            leg.data_modificare = st.st_mtime;                // atribuie campului data_modificare din struct legatura timestamp-ul preluat din st.st_mtime
            leg.ultima_accesare = st.st_atime;                // atribuie campului ultima_accesare din struct legatura timestamp-ul preluat din st.st_atime
            leg.inode = st.st_ino;                            // atribuie campului inode din struct legatura valoarea preluata din st.st_ino
            const char *p_leg = prelucrare_permisiuni(st);    // atribuie unui pointer p_leg de tip char valoarea returnata de functia prelucrare_permisiuni, dand ca parametru struct stat-ul definit mai sus

            char buffer[5000]; // buffer care va retine informatia formatata
            char *modificare_legatura = ctime(&leg.data_modificare); // atribuie unui pointer modificare_legatura de tip char timestamp-ul obtinut prin prelucrarea cu ctime a valorii din campul data_modificare
            char *accesare_legatura = ctime(&leg.ultima_accesare); // atribuie unui pointer accesare_legatura de tip char timestamp-ul obtinut prin prelucrarea cu ctime a valorii din campul ultima_accesare
            const char *indent_leg = indentare(nivel_indentare); // atribuie unui pointer indent_leg de tip char indentarea obtinuta in urma executiei functiei "indentare"
            snprintf(buffer, sizeof(buffer), "%sLegatura simbolica\n%sCale: %s\n%sNume: %s\n%sDimensiune: %ld bytes\n%sModificare: %s%sUltima accesare: %s%sInode: %ld\n%sPermisiuni: %s\n\n", indent_leg, indent_leg, leg.cale, indent_leg, leg.nume, indent_leg, leg.dimensiune, indent_leg, modificare_legatura, indent_leg, accesare_legatura, indent_leg, leg.inode, indent_leg, p_leg); // folosesc snprintf ca sa obtin un string formatat asa cum vreau eu

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer)); // scrie in fisier informatia formatata
            if(written_bytes == -1) // daca am obtinut -1 inseamna ca apelul write() s-a terminat cu eroare
            {
                eroare("Eroare la scrierea informatiilor despre legatura simbolica in fisierul snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
            }
        }
    }

    if(closedir(subdir) == -1) // inchide directorul si verifica daca s-a inchis corect
    {
        eroare("Eroare la inchiderea directorului."); // apelam functia eroare si precizam ce mai exact nu a mers
    }
}

int compara_snapi(const char *snapshot_vechi, const char *snapshot_nou) // functie de comparare a snapshot-urilor care primeste ca parametrii doua nume de snapshot-uri, unul pentru cel anterior si unul pentru cel curent
{
    int snap_vechi = open(snapshot_vechi, O_RDONLY, S_IRUSR | S_IROTH | S_IRGRP); // deschide fisierul anterior de snapshot numai pentru citire (O_RDONLY), iar permisiunile pentru acest fisier sunt read pentru user, other si grup

    // in caz de succes, open returneaza un descriptor de fisier
    if(snap_vechi == -1) // in caz de eroare returneaza -1
    {
        eroare("Fisierul vechi de snapshot nu a putut fi deschis."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    int snap_nou = open(snapshot_nou, O_RDONLY, S_IRUSR | S_IROTH | S_IRGRP); // deschide fisierul curent de snapshot numai pentru citire (O_RDONLY), iar permisiunile pentru acest fisier sunt read pentru user, other si grup

    // in caz de succes, open returneaza un descriptor de fisier
    if(snap_nou == -1) // in caz de eroare returneaza -1
    {
        eroare("Fisierul nou de snapshot nu a putut fi deschis."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    char buffer_linie_snap_vechi[5000]; // buffer care va retine linia citita din snapshot-ul anterior
    char buffer_linie_snap_nou[5000]; // buffer care va retine linia citita din snapshot-ul curent
    ssize_t bytes_cititi_vechi, bytes_cititi_nou; // variabile care retin numarul de bytes cititi din fisierele de snapshot

    while ((bytes_cititi_vechi = read(snap_vechi, buffer_linie_snap_vechi, sizeof(buffer_linie_snap_vechi))) > 0 && (bytes_cititi_nou = read(snap_nou, buffer_linie_snap_nou, sizeof(buffer_linie_snap_nou))) > 0) // cat timp se citesc linii din fisierele de snapshot, se verifica linie cu linie si se compara continutul
    {
        if(bytes_cititi_vechi != bytes_cititi_nou || memcmp(buffer_linie_snap_vechi, buffer_linie_snap_nou, bytes_cititi_vechi) != 0) //  daca numarul de bytes cititi sau continutul lor difera, snapshot-urile sunt diferite
        {
            if((close(snap_nou) == -1) || (close(snap_vechi) == -1)) // inchide fisierele de snapshot si verifica daca s-au inchis corect
            {
                eroare("Eroare la inchiderea snapshot-urilor."); // apelam functia eroare si precizam ce mai exact nu a mers
            }
            return 1; // acest 1 va indica faptul ca snapshot-urile sunt diferite
        }
    }

    if(bytes_cititi_vechi == -1 || bytes_cititi_nou == -1) // daca am obtinut -1 inseamna ca apelul read() s-a terminat cu eroare
    {
        eroare("Eroare la citirea din fisierele de snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    if(close(snap_vechi) == -1) // inchide fisierul anterior de snapshot si verifica daca s-a inchis corect
    {
        eroare("Eroare la inchiderea snapshot-ului vechi."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    if(close(snap_nou) == -1) // inchide fisierul curent de snapshot si verifica daca s-a inchis corect
    {
        eroare("Eroare la inchiderea snapshot-ului nou."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    return 0; // acest 0 va indica faptul ca snapshot-urile sunt identice
}

int main(int argc, char *argv[]) // arguments count(argc) specifica nr argumentelor din linie de comanda, incluzand numele programului executat
// arguments vector(argv) este un vector de String-uri care contine cate un argument pe fiecare pozitie incepand cu 0, unde se afla numele executabilului
// pot avea maxim 10 argumente date ca parametru, 13 cu tot cu ./prog, -o si output
{
    fisier_com = fopen("flow_executie.txt", "w"); // deschid fisierul care va contine comentariile despre flow-ul executiei in modul "write" 

    if(fisier_com == NULL) // verific daca s-a deschis cu succes fisierul, daca nu, va returna NULL
    {
        printf("Eroare la deschiderea fisierului care urmareste flow-ul executiei.\n"); // afisez in terminal acest mesaj ca sa imi indice ca nu am fisier
    }

    // se asteapta 10 nume de directoare date ca argumente, deci argc trebuie sa fie minim 3 (pentru ./prog, -o si output) si maxim 13 

    if((argc < 3) || (argc > 13)) // verificam daca argc este diferit mai mic decat 3 sau mai mare decat 13 si depaseste limitele impuse de noi
    {
        // acesta este un caz de eroare, deci putem apela functia creata eroare anterior cu un mesaj care sa ne anunte ca am introdus un nr diferit de argumente
        eroare("Numarul de argumente difera de cel asteptat."); // apelam functia invalid si precizam ce mai exact nu a potrivit
    }

    int i; // variabila care va fi folosita pe post de iterator
    int j; // variabila care va fi folosita pe post de iterator
    int index_output = -1; // variabila care va retine pozitia directorului de output intre cele 10 argumente
    int index_izolare = -1; // variabila care va retine pozitia directorului de izolare intre cele 10 argumente

    for(i = 1; i < argc; i++) // iteram prin argumente in bucla exterioara
    {
        for(j = 1; j < i; j++) // iteram prin argumentele deja prelucrate in bucla interioara
        {
            if(strcmp(argv[i], argv[j]) == 0) // comparam argumentul curent cu cele anterioare
            {
                eroare("Unele argumente se repeta."); // apelam functie eroare si precizam ce mai exact nu a mers
            }
        }
    }

    fprintf(fisier_com, "Nu există argumente repetitive.\n"); // mesaj afisat in fisier pentru a urmari flow-ul executiei

    for(i = 1; i < argc; i++) // iteram prin argumente pentru a gasi "-o"
    {
        if((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "-O") == 0)) // comparam fiecare argument (argv[i]) cu "-o" sau "-O" pentru a gasi indexul
        {
            if((i + 1) >= argc) // daca l-am gasit dar depasim numarul posibil de argumente e clar ca "-o" este ultimul si nu mai exista si director de output
            {
                eroare("Nu exista si directorul de output printre argumente."); // apelam functia eroare si precizam ce mai exact nu a mers
            }

            index_output = i + 1; // indexul directorului de output este cu 1 mai mare decat cel al lui "-o", pentru ca succede acest indicator
            break; // daca am gasit indexul cautat intrerupem iteratia
        }
    }

    if(index_output == -1) // daca indexul a ramas tot la valoarea data de noi, inseamna ca nu s-a gasit nici -o, nici directorul
    {
        eroare("Nu s-a precizat optiunea -o, deci nu se poate determina directorul de output."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    for(i = 1; i < argc; i++) // iteram prin argumente pentru a gasi "-s"
    {
        if((strcmp(argv[i], "-s") == 0) || (strcmp(argv[i], "-S") == 0)) // comparam fiecare argument (argv[i]) cu "-s" sau "-S" pentru a gasi indexul
        {
            if((i + 1) >= argc) // daca l-am gasit dar depasim numarul posibil de argumente e clar ca "-s" este ultimul si nu mai exista si director de output
            {
                eroare("Nu exista si directorul de izolare printre argumente."); // apelam functia eroare si precizam ce mai exact nu a mers
            }

            index_izolare = i + 1; // indexul directorului de izolare este cu 1 mai mare decat cel al lui "-s", pentru ca succede acest indicator
            break; // daca am gasit indexul cautat intrerupem iteratia
        }
    }

    if(index_izolare == -1) // daca indexul a ramas tot la valoarea data de noi, inseamna ca nu s-a gasit nici -s, nici directorul
    {
        eroare("Nu s-a precizat optiunea -s, deci nu se poate determina directorul de izolare."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    const char *output = argv[index_output]; // pointer in care se stocheaza numele directorului de output
    const char *izolare = argv[index_izolare]; // pointer in care se stocheaza numele directorului de izolare 

    fprintf(fisier_com, "Indexul lui -o este %d.\n", (index_output - 1)); // mesaj afisat in fisier pentru a urmari flow-ul executiei
    fprintf(fisier_com, "Indexul directorului de output este %d.\n", index_output); // mesaj afisat in fisier pentru a urmari flow-ul executiei
    fprintf(fisier_com, "Indexul lui -s este %d.\n", (index_izolare - 1)); // mesaj afisat in fisier pentru a urmari flow-ul executiei
    fprintf(fisier_com, "Indexul directorului de izolare este %d.\n", index_izolare); // mesaj afisat in fisier pentru a urmari flow-ul executiei

    struct stat out; // structura care contine metadata directorului de output

    if(lstat(output, &out) == -1) // daca lstat returneaza -1 inseamna ca a aparut o eroare
    {
        eroare("Eroare la determinarea informatiilor despre directorul de output, ceva s-a intamplat cu lstat."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    if(S_ISDIR(out.st_mode) == 0) // daca valoarea returnata este 0 inseamna ca nu am primit ca argument un director
    {
        invalid(fisier_com, "Nu a fost dat ca argument un director pentru output, este dat un fisier sau o legatura simbolica."); // faptul ca nu am primit ca arg un director nu e o eroare, doar nu face ce vreau eu -> doar invalid, nu eroare
    }

    struct stat izo; // structura care contine metadata directorului de izolare

    if(lstat(izolare, &izo) == -1) // daca lstat returneaza -1 inseamna ca a aparut o eroare
    {
        eroare("Eroare la determinarea informatiilor despre directorul de izolare, ceva s-a intamplat cu lstat."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    if(S_ISDIR(izo.st_mode) == 0) // daca valoarea returnata este 0 inseamna ca nu am primit ca argument un director
    {
        invalid(fisier_com, "Nu a fost dat ca argument un director pentru izolare, este dat un fisier sau o legatura simbolica."); // faptul ca nu am primit ca arg un director nu e o eroare, doar nu face ce vreau eu -> doar invalid, nu eroare
    }

    for(i = 1; i < argc; i++) // iteram prin argumente
    {
        if((i == index_output) || (i == index_output - 1) || (i == index_izolare) || (i == index_izolare - 1)) // daca ne aflam pe pozitia lui "-o", "-s", a directorului de izolare sau a directorului de output dam skip, pentru ca nu vrem sa afisam informatii despre acestea
        {
            continue; // trecem la urmatoarea iteratie
        }

        const char *director = argv[i]; // pointer in care se stocheaza numele directorului argument curent
        struct stat st; // structura care contine metadata
        struct director dir; // structura care va retine informatiile despre directorul argument

        fprintf(fisier_com, "Directorul introdus: %s\n", director); // mesaj afisat in fisier pentru a urmari flow-ul executiei

        // apelul lstat returneaza 0 in caz de succes, sau -1 in caz de eroare
        // pentru a fi folosit necesita includerea fisierului header sys/stat.header
        // folosim apelul lstat pentru directorul primit ca parametru si pasam ca parametru 2 zona de memorie unde vrem sa pastram toate informatiile, adica st
        // lstat este mai useful pentru ca daca este aplicata unei legaturi simbolice, informatiile returnate se vor referi la legatura, si nu la fisierul indicat

        if(lstat(director, &st) == -1) // daca lstat returneaza -1 inseamna ca a aparut o eroare
        {
            eroare("Eroare la determinarea informatiilor despre director, ceva s-a intamplat cu lstat."); // apelam functia eroare si precizam ce mai exact nu a mers
        }

        // daca nu s-a oprit din executie programul inseamna ca apelul lstat a reusit, deci am obtinut informatii despre director
        // prin intermediul campului st_mode din st, putem verifica daca intr-adevar am primit un director ca parametru sau daca avem altceva
        // folosim macro-ul S_ISDIR(m) ca sa verificam daca tipul fisierului primit este director
        // in cazul nostru m este st.st_mode

        if(S_ISDIR(st.st_mode) == 0) // daca valoarea returnata este 0 inseamna ca nu am primit ca argument un director
        {
            fprintf(fisier_com, "Argumentul cu numarul %d nu este un director, deci nu va fi procesat.\n", i); // mesaj afisat in fisier pentru a urmari flow-ul executiei
            continue; // daca nu am primit un director ca argument dam skip, dar nu oprim executia
        }

        char nume_snapshot[PATH_MAX]; // sir de caractere care va stoca pe rand numele fisierelor de snapshot
        char nume_snapshot_temporar[PATH_MAX]; // sir de caractere care va stoca pe rand numele fisierelor temporare de snapshot
        char nume_snapshot_vechi[PATH_MAX]; // sir de caractere care va stoca pe rand numele fisierelor anterioare de snapshot

        snprintf(nume_snapshot, sizeof(nume_snapshot), "%s/snapshot_%s.txt", output, director); // snprintf construieste calea fiecarui fisier de snapshot, indicand ca se va crea in output (output/) si cui apartine(_director.txt), pentru a le deosebi
        snprintf(nume_snapshot_temporar, sizeof(nume_snapshot_temporar), "%s/snapshot_%s_temp.txt", output, director); // snprintf construieste calea fiecarui fisier temporar de snapshot, indicand ca se va crea in output (output/) si cui apartine(_director.txt), pentru a le deosebi
        snprintf(nume_snapshot_vechi, sizeof(nume_snapshot_vechi), "%s/snapshot_%s_old.txt", output, director); // snprintf construieste calea fiecarui fisier anterior de snapshot, indicand ca se va crea in output (output/) si cui apartine(_director.txt), pentru a le deosebi

        int snap = open(nume_snapshot, O_RDONLY); // deschide fisierul curent de snapshot numai pentru citire (O_RDONLY)

        if(snap < 0) // daca open a returnat o valoare negativa inseamna ca fisierul meu curent de snapshot e inexistent, deci ori sunt la prima iteratie, ori pana acum au fost erori
        {
            snap = open(nume_snapshot, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP); // deschide fisierul curent de snapshot numai pentru scriere (O_WRONLY), il creeaza daca nu exista deja (O_CREAT) si daca fisierul exista, continutul lui este sters (O_TRUNC); daca exista, adauga la sfarsitul sau (O_APPEND)
            // permisiunile pentru acest fisier snap sunt: read si write pentru user, doar read pentru group si other
            // am combinat constante folosind operatorul sau ('|'), pentru a seta mai multi biti
            // in caz de succes, open returneaza un descriptor de fisier

            if(snap == -1) // in caz de eroare returneaza -1
            {
                eroare("Fisierul nu a putut fi deschis/creat."); // apelam functia eroare si precizam ce mai exact nu a mers
            }

            fprintf(fisier_com, "Fisierul de snapshot a fost creat cu succes: %s.\n", nume_snapshot); // mesaj afisat in fisier pentru a urmari flow-ul executiei
        }
        else // daca open a returnat o valoare nenegativa inseamna ca aveam deja un fisier de snapshot din rulari anterioare
        {
            snap = open(nume_snapshot_temporar, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP); // deschide fisierul temporar de snapshot numai pentru scriere (O_WRONLY), il creeaza daca nu exista deja (O_CREAT) si daca fisierul exista, continutul lui este sters (O_TRUNC); daca exista, adauga la sfarsitul sau (O_APPEND)
            // permisiunile pentru acest fisier snap sunt: read si write pt user, doar read pt group si other
            // am combinat constante folosind operatorul sau ('|'), pentru a seta mai multi biti
            // in caz de succes, open returneaza un descriptor de fisier

            if(snap == -1) // in caz de eroare returneaza -1
            {
                eroare("Fisierul nu a putut fi deschis/creat."); // apelam functia eroare si precizam ce mai exact nu a mers
            }

            fprintf(fisier_com, "Fisierul temporar a fost creat cu succes: %s.\n", nume_snapshot_temporar); // mesaj afisat in fisier pentru a urmari flow-ul executiei
        }

        // stocam informatiile directorului argument

        dir.dimensiune = st.st_size;                    // atribuie campului dimensiune din struct director valoarea preluata din st.st_size
        dir.data_modificare = st.st_mtime;              // atribuie campului data_modificare din struct director timestamp-ul preluat din st.st_mtime
        dir.ultima_accesare = st.st_atime;              // atribuie campului ultima_accesare din struct director timestamp-ul preluat din st.st_atime
        dir.inode = st.st_ino;                          // atribuie campului inode din struct director valoarea preluata din st.st_ino
        const char *p_init = prelucrare_permisiuni(st); // atribuie unui pointer p_init de tip char valoarea returnata de functia prelucrare_permisiuni, dand ca parametru struct stat-ul definit mai sus

        char buffer[5000]; // buffer care va retine informatia formatata
        char *modificare_initial = ctime(&dir.data_modificare); // atribuie unui pointer modificare_initial de tip char timestamp-ul obtinut prin prelucrarea cu ctime a valorii din campul data_modificare
        char *accesare_initial = ctime(&dir.ultima_accesare); // atribuie unui pointer accesare_initial de tip char timestamp-ul obtinut prin prelucrarea cu ctime a valorii din campul ultima_accesare
        snprintf(buffer, sizeof(buffer), "Director initial\nNume: %s\nDimensiune: %ld bytes\nModificare: %sUltima accesare: %sInode: %ld\nPermisiuni: %s\n\n", director, dir.dimensiune, modificare_initial, accesare_initial, dir.inode, p_init); // folosesc snprintf ca sa obtin un string formatat asa cum vreau eu

        ssize_t written_bytes = write(snap, buffer, strlen(buffer)); // scrie in fisier informatia formatata
        if(written_bytes == -1) // daca am obtinut -1 inseamna ca apelul write() s-a terminat cu eroare
        {
            eroare("Eroare la scrierea informatiilor despre director in snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
        }

        // apelez functia mea recursiva care parcurge subarborele
        parcurgere_recursiva(director, snap, 1);

        // inchid fisierul de snapshot pentru ca am adaugat tot ce era nevoie in el
        // functia close returneaza 0 in caz de succes, -1 in caz de eroare

        if(close(snap) == -1)
        {
            eroare("Eroare la inchiderea snapshot-ului."); // alta self-explanatory
        }

        snap = open(nume_snapshot_temporar, O_RDONLY); // deschide fisierul temporar de snapshot numai pentru citire (O_RDONLY)

        if(snap >= 0) // daca open a returnat o valoare nenegativa, inseamna ca exista un fisier temporar de snapshot, deci clar si un snapshot anterior
        { 
            if(close(snap) == -1) // inchid fisierul temporar de snapshot pentru ca am determinat daca exista sau nu si tin cont ca functia close returneaza 0 in caz de succes, -1 in caz de eroare
            {
                eroare("Eroare la inchiderea fisierului temporar."); // apelam functia eroare si precizam ce mai exact nu a mers
            }

            if(compara_snapi(nume_snapshot, nume_snapshot_temporar) == 1) // apelez functia de comparare a snapshot-urilor si verific daca returneaza 1, adica daca imi spune ca snapshot-urile sunt diferite
            {
                if(rename(nume_snapshot, nume_snapshot_vechi) != 0) // folosesc un principiu ca al functiei void swap, unde fisierul curent devine "vechi"
                {
                    eroare("Nu s-a putut redenumi fisierul curent ca cel vechi."); // apelam functia eroare si precizam ce mai exact nu a mers
                }

                if(rename(nume_snapshot_temporar, nume_snapshot) != 0) // fisierul temporar devine cel curent
                {
                    eroare("Nu s-a putut redenumi fisierul temporar ca cel curent."); // apelam functia eroare si precizam ce mai exact nu a mers
                }

                fprintf(fisier_com, "Au fost detectate modificari: %s a fost redenumit %s\n", nume_snapshot, nume_snapshot_vechi); // mesaj afisat in fisier pentru a urmari flow-ul executiei
            }
            else // daca functia mea de comparare a returnat 0, inseamna ca fisierele mele sunt identice si nu mai am nevoie de snapshot-ul temporar 
            {
                if(unlink(nume_snapshot_temporar) == -1) // incerc sa sterg fisierul temporar si verific daca stergerea s-a produs cu succes, daca nu, va returna -1
                {
                    eroare("Nu s-a putut sterge snapshot-ul temporar."); // apelam functia eroare si precizam ce mai exact nu a mers
                }

                fprintf(fisier_com, "Nu a aparut nici o modificare in directorul %s.\n", director); // mesaj afisat in fisier pentru a urmari flow-ul executiei
            }
        }
        else // daca open a returnat o valoare negativa inseamna ca nu s-a putut deschide fisierul temporar, deci fie este inexistent si ma aflu la prima rulare, fie au aparut erori
        {
            fprintf(fisier_com, "Snapshot-ul temporar nu exista, deoarece e prima rulare.\n"); // mesaj afisat in fisier pentru a urmari flow-ul executiei
        }
    }

    fclose(fisier_com); // inchid fisierul in care am scris comentariile 

    return 0;
}