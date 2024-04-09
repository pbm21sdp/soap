/*Subiectul proiectului: monitorizarea modificarilor aparute in directoare de a lungul timpului.
Saptamana 1: Utilizatorul poate specifica directorul pe care doreste sa il monitorizeze in linia de comanda, ca prim argument.
Se vor urmari atat modificarile aparute la nivelul directorului, cat si la nivelul subarborelui. Se va parcurge directorul si intregul
sau subrbore si se face un SNAPSHOT. Acest snapshot poate fi un fisier, care contine toate datele pe care le furnizeaza directorul
si subarborele si care sunt considerate necesare. Facem un director separat (daca vrem) de cel dat ca argument unde adaugam toate snapshot-urile obtinute.
La fiecare director obtin un snapshot diferit.
Saptamana 2: Se va rula programul de mai multe ori. La fiecare rulare comparam snapshot-urile curente cu snapshot-urile de data trecuta.
Daca s-a facut o modificare, inlocuim snapshot-ul vechi cu cel nou.
Se actualizeaza functionalitatea programului astfel incat sa primeasca un numar nespecificat de argumente in linie de comanda (oricate,
dar nu mai mult de 10), cu mentiunea ca niciun argument nu se va repeta. Programul proceseaza doar directoarele si ignora alte tipuri de
argumente. Programul va primi un argument suplimentar care va fi un director de iesire, care va stoca toate snapshot-urile argumentelor
date la intrare. Directorul de iesire va avea in fata lui un -o, ca sa fie evidentiat.

    MAIN SUCCESS SCENARIO-ish pentru saptamana 1

1. primesc un argument in linie de comanda (verific daca am primit sigur doar 1 sau am mai multe)
2. daca nu e director -> nu e eroare, dar dau exit (aici verific daca e director cu S_ISDIR)
3. parcurg directorul si fac un snapshot -> un fisier care se poate afla in directorul respectiv (trebuie sa il creez)
4. cautam in functie de inode, nu de nume, pentru ca e posibil sa fie redenumit fisierul
5. - verificam si size-ul, dar si data ultimei modificari
- rename, edit, delete, move, add -> modificari care pot afecta fisierele
- nume, inode, dimensiune, data unei modificari -> necesitati care trebuie sa fie trecute in snapshot
6. la fiecare rulare, ne extragem datele de care avem nevoie si le adaugam la snapshot

Se va actualiza funcționalitatea programului în așa fel încât acesta să primească un număr nespecificat de argumente în linia de comandă, dar nu mai mult de 10, cu
mențiunea că niciun argument nu se va repeta. Programul va procesa numai directoarele, alte tipuri de argumente vor fi ignorate. Logica de captură a metadatelor se va 
aplica acum tuturor argumentelor primite valide, ceea ce înseamnă că programul va actualiza snapshot-urile pentru toate directoarele specificate de utilizator.
În cazul în care se vor înregistra modificări la nivelul directoarelor, utilizatorul va putea să compare snapshot-ul anterior al directorului specificat cu cel curent. 
În cazul în care există diferențe între cele două snapshot-uri, snapshot-ul vechi va fi actualizat cu noile informații din snapshot-ul curent.
Funcționalitatea codului va fi extinsă astfel încât programul să primească un argument suplimentar, care va reprezenta directorul de ieșire, în care vor fi stocate 
toate snapshot-urile intrărilor din directoarele specificate în linia de comandă. Acest director de ieșire va fi specificat folosind opțiunea '-O'. De exemplu, comanda 
de rulare a programului va fi: '/program_exe -o output input1 input2*/

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

// structura care va retine informatiile despre un regular file
typedef struct fisier
{
    char nume[256];         // numele fisierului
    off_t dimensiune;       // dimensiune totala in bytes
    time_t data_modificare; // momentul ultimei modificari (st_mtim din struct stat)
    ino_t inode;            // numarul inode-ului
} fisier;

// structura care va retine informatiile despre un director
typedef struct director
{
    char nume[256];         // numele directorului
    off_t dimensiune;       // dimensiune totala in bytes
    time_t data_modificare; // momentul ultimei modificari (st_mtim din struct stat)
    ino_t inode;            // numarul inode-ului
} director;

// structura care va retine informatiile despre o legatura simbolica
typedef struct legatura_simbolica
{
    char nume[256];         // numele legaturii simbolice
    off_t dimensiune;       // dimensiune totala in bytes
    time_t data_modificare; // momentul ultimei modificari (st_mtim din struct stat)
    ino_t inode;            // numarul inode-ului
} legatura_simbolica;

void eroare(const char *mesaj) // functie care primeste ca parametru un String si semnaleaza o eroare de executie sau de logica
{
    perror(mesaj); // afiseaza un mesaj de eroare mai simplu
    exit(-1); // termina programul cu un cod de eroare
}

void invalid(const char *mesaj) // functie care primeste ca parametru un String si semnaleaza o "diferenta" intre cerinta si ceea ce am obtinut, dar nu o eroare
{
    printf(mesaj); // afiseaza un mesaj informativ care sa ne atentioneze ca nu respectam cerinta
    exit(EXIT_SUCCESS); // opreste executia programului pentru ca nu are rost sa o continuam daca nu facem ce se cere, dar nu cu cod de eroare, ci cu cod 0
}

void parcurgere_recursiva(const char *director, int snapi) // primeste ca parametru numele unei entitati si descriptorul pentru fisierul de snapshot
{
    // 1. pt parcurgerea arborelui se deschide directorul cu functia opendir

    DIR *subdir = opendir(director); // daca subdir este NULL inseamna ca ceva nu a mers cu opendir si am intampinat o eroare

    char cale[PATH_MAX]; // folosit pentru a retine fiecare cale nou construita

    if (subdir == NULL) // opendir returneaza NULL daca a aparut o eroare
    {
        eroare("Directorul pe care il voiam curent nu a putut fi deschis."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    printf("Directorul a putut fi deschis: %s\n", director); // mesaj afisat in terminal pentru a urmari flow-ul executiei

    /*2. Se citeste pe rand cate o intrare din director, apeland functia readdir. Fiecare apel al acestei functii va returna un pointer la
    o structura struct dirent in care se vor gasi informatii despre intrarea in director citita. Intrarile vor fi parcurse una dupa alta
    pana cand se ajunge la ultima inregistrare. In momentul in care nu mai exista inregistrari in directorul citit, functia readdir va
    returna NULL. Singura informatie care poate fi extrasa (conform POSIX) din structura dirent este numele intrarii in director. Toate
    celelalte informatii despre intrarea citita se pot afla apeland in continuare functiile stat, fstat sau lstat.*/

    // inainte de orice citire am nevoie de zona de memorie care va stoca toate informatiile, adica struct dirent

    struct dirent *dst; // dst de la director struct nu de la destination

    // trebuie sa apelez readdir pana cand ajung la ultima inregistrare din director, dupa care primesc ca return value NULL cand am epuizat inregistrarile
    // readdir returneaza un pointer la o structura struct dirent, deci dst al meu va fi de fapt readdir

    while ((dst = readdir(subdir)) != NULL) // parcugrem cat timp exista inregistrari in director
    {
        if (strcmp(dst->d_name, ".") == 0 || strcmp(dst->d_name, "..") == 0) // verifica daca entitatea curenta este directorul dat ca argument sau "parintele" acestuia
        {
            continue; // sare peste acea iteratie pentru a preveni bucla infinita
        }

        snprintf(cale, sizeof(cale), "%s/%s", director, dst->d_name); // snprintf este folosit pentru a construi calea fiecarei entitati din subarbore (preferat in locul lui sprintf pentru ca snprintf previne depasirea bufferului)
        printf("Calea entitatii curente este: %s\n", cale); // mesaj afisat in terminal pentru a urmari flow-ul executiei

        struct stat st; // structura care va retine atributele entitatii

        if (lstat(cale, &st) == -1) // daca lstat returneaza -1 inseamna ca a aparut o eroare
        {
            eroare("Eroare la determinarea informatiilor despre entitatea curenta, ceva s-a intamplat cu lstat."); // apelam functia eroare si precizam ce mai exact nu a mers
        }

        if (S_ISDIR(st.st_mode) != 0) // verifica daca entitatea curenta este un director
        {
            struct director d; // definim o structura care retine informatii despre un director
            strncpy(d.nume, dst->d_name, sizeof(d.nume));
            d.dimensiune = st.st_size;
            d.data_modificare = st.st_mtime;
            d.inode = st.st_ino;

            char buffer[5000]; // buffer care va retine informatia formatata
            snprintf(buffer, sizeof(buffer), "Nume: %s Dimensiune: %ld Modificare: %ld Inode: %ld\n", d.nume, d.dimensiune, d.data_modificare, d.inode); // folosesc snprintf ca sa obtin un string formatat asa cum vreau eu

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer)); // scrie in fisier informatia formatata
            if (written_bytes == -1) // daca am obtinut -1 inseamna ca apelul write() s-a terminat cu eroare
            {
                eroare("Eroare la scrierea informatiilor despre director in snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
            }

            parcurgere_recursiva(cale, snapi); // daca este director, se apeleaza recursiv functia pentru a parcurge directorul gasit
        }
        else if (S_ISREG(st.st_mode) != 0) // verifica daca entitatea curenta este un fisier
        {
            struct fisier fis; // definim o structura care retine informatii despre un fisier regular
            strncpy(fis.nume, dst->d_name, sizeof(fis.nume));
            fis.dimensiune = st.st_size;
            fis.data_modificare = st.st_mtime;
            fis.inode = st.st_ino;

            char buffer[5000]; // buffer care va retine informatia formatata
            snprintf(buffer, sizeof(buffer), "Nume: %s Dimensiune: %ld Modificare: %ld Inode: %ld\n", fis.nume, fis.dimensiune, fis.data_modificare, fis.inode); // folosesc snprintf ca sa obtin un string formatat asa cum vreau eu

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer)); // scrie in fisier informatia formatata
            if (written_bytes == -1) // daca am obtinut -1 inseamna ca apelul write() s-a terminat cu eroare
            {
                eroare("Eroare la scrierea informatiilor despre fisier in snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
            }
        }
        else if (S_ISLNK(st.st_mode) != 0) // verifica daca entitatea curenta este o legatura simbolica
        {
            struct legatura_simbolica leg; // definim o structura care retine informatii despre o legatura simbolica
            strncpy(leg.nume, dst->d_name, sizeof(leg.nume));
            leg.dimensiune = st.st_size;
            leg.data_modificare = st.st_mtime;
            leg.inode = st.st_ino;

            char buffer[5000]; // buffer care va retine informatia formatata                                                                                                                                  
            snprintf(buffer, sizeof(buffer), "Nume: %s Dimensiune: %ld Modificare: %ld Inode: %ld\n", leg.nume, leg.dimensiune, leg.data_modificare, leg.inode); // folosesc snprintf ca sa obtin un string formatat asa cum vreau eu

            ssize_t written_bytes = write(snapi, buffer, strlen(buffer)); // scrie in fisier informatia formatata
            if (written_bytes == -1) // daca am obtinut -1 inseamna ca apelul write() s-a terminat cu eroare
            {
                eroare("Eroare la scrierea informatiilor despre legatura simbolica in fisierul snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
            }
        }
    }

    if (closedir(subdir) == -1) // inchide directorul si verifica daca s-a inchis corect 
    {
        eroare("Eroare la inchiderea directorului."); // apelam functia eroare si precizam ce mai exact nu a mers
    }
}

int main(int argc, char *argv[]) // arguments count(argc) specifica nr argumentelor din linie de comanda, incluzand numele programului executat 
// arguments vector(argv) este un vector de String-uri care contine cate un argument pe fiecare pozitie incepand cu 0, unde se afla numele executabilului
// pot avea maxim 10 argumente? cred nu prea stiu daca 10 cu tot cu ./prog sau nu
{
    // se asteapta 10 nume de directoare date ca argumente, deci argc trebuie sa fie minim 3 (pentru ./prog, -o si output) si maxim 10? cred

    if (argc < 3 || argc > 10) // verificam daca argc este diferit mai mic decat 3 sau mai mare decat 10 si depaseste limitele impuse de noi
    {
        // daca nr de argumente nu este 1 inseamna ca nu s-a respectat cerinta
        // acesta este un caz de eroare, deci putem apela functia creata eroare anterior cu un mesaj care sa ne anunte ca am introdus un nr diferit de argumente
        invalid("Numarul de argumente difera de cel asteptat."); // apelam functia invalid si precizam ce mai exact nu a potrivit
    }

    int i; // variabila care va fi folosita pe post de iterator 
    int index_output = -1; // variabila care va retine pozitia directorului de output intre cele 10 argumente

    for(i = 1; i < argc; i++) // iteram prin argumente pentru a gasi "-o"
    {
        if ((strcmp(argv[i], "-o") == 0) || (strcmp(argv[i], "-O") == 0)) // comparam fiecare argument (argv[i]) cu "-o" sau "-O" pentru a gasi indexul
        {
            if (i + 1 >= argc) // daca l-am gasit dar depasim numarul posibil de argumente e clar ca "-o" este ultimul si nu mai exista si director de output
            {
                eroare("Nu exista si directorul de output printre argumente."); // apelam functia eroare si precizam ce mai exact nu a mers
            }

            index_output = i + 1; // indexul directorului de output este cu 1 mai mare decat cel al lui "-o", pentru ca succede acest indicator
            break; // daca am gasit indexul cautat intrerupem iteratia 
        }
    }

    if (index_output == -1) // daca indexul a ramas tot la valoarea data de noi, inseamna ca nu s-a gasit nici -o nici directorul
    {
        eroare("Nu s-a precizat optiunea -o, deci nu se poate determina directorul de output."); // apelam functia eroare si precizam ce mai exact nu a mers 
    }

    const char *output = argv[index_output]; // pointer in care se stocheaza numele directorului de output

    printf("Indexul lui -o este %d.\n", (index_output - 1)); // mesaj afisat in terminal pentru a urmari flow-ul executiei
    printf("Indexul directorului de output este %d.\n", index_output); // mesaj afisat in terminal pentru a urmari flow-ul executiei

    struct stat out; // structura care contine metadata directorului de output

    if(lstat(output, &out) == -1) // daca lstat returneaza -1 inseamna ca a aparut o eroare
    {
         eroare("Eroare la determinarea informatiilor despre directorul de output, ceva s-a intamplat cu lstat."); // apelam functia eroare si precizam ce mai exact nu a mers
    }

    if(S_ISDIR(out.st_mode) == 0) // daca valoarea returnata este 0 inseamna ca nu am primit ca argument un director
    {
        invalid("Nu a fost dat ca argument un director pentru output, este dat un fisier sau o legatura simbolica."); // faptul ca nu am primit ca arg un director nu e o eroare, doar nu face ce vreau eu -> doar invalid, nu eroare
    }

    for(i = 1; i < argc; i++) // iteram prin argumente 
    {
        if((i == index_output) || (i == index_output - 1)) // daca ne aflam pe pozitia lui "-o" sau a directorului de output dam skip, pentru ca nu vrem sa afisam informatii despre acestea 
        {
            continue;
        }

        const char *director = argv[i]; // pointer in care se stocheaza numele directorului argument curent
        struct stat st; // structura care contine metadata
        struct director dir; // structura care va retine informatiile despre directorul argument 

        printf("Directorul introdus: %s\n", director); // mesaj afisat in terminal pentru a urmari flow-ul executiei

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
            printf("Argumentul cu numarul %d nu este un director, deci nu va fi procesat.\n", i); // // mesaj afisat in terminal pentru a urmari flow-ul executiei
            continue; // daca nu am primit un director ca argument dam skip, dar nu oprim executia
        }

        char nume_snapshot[PATH_MAX]; // sir de caractere care va stoca pe rand numele fisierelor de snapshot
        snprintf(nume_snapshot, sizeof(nume_snapshot), "%s/snapshot_%s.txt", output, director); // snprintf construieste calea fiecarui fisier de snapshot, indicand ca se va crea in output (output/) si cui apartine(_director.txt), pentru a le deosebi

        int snap = open(nume_snapshot, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP); // deschide fisierul snap.txt numai pentru scriere (O_WRONLY), il creeaza daca nu exista deja (O_CREAT) si daca fisierul exista, continutul lui este sters (O_TRUNC); daca exista, adauga la sfarsitul sau (O_APPEND)
        // permisiunile pentru acest fisier snap sunt: read si write pt user, doar read pt group si other
        // am combinat constante folosind operatorul sau ('|'), pentru a seta mai multi biti
        // in caz de succes, open returneaza un descriptor de fisier
        // in caz de eroare, returneaza -1

        if (snap == -1)
        {
            eroare("Eroare la crearea si/sau deschiderea fisierului de snapshot."); // ori nu s-a creat, ori nu s-a deschis, cert e ca n-a mers
        }

        printf("Fisierul a fost creat cu succes: %s\n", nume_snapshot); // mesaj afisat in terminal pentru a urmari flow-ul executiei

        // stocam informatiile directorului argument 

        dir.dimensiune = st.st_size;
        dir.data_modificare = st.st_mtime;
        dir.inode = st.st_ino;

        char buffer[5000]; // buffer care va retine informatia formatata                 
        snprintf(buffer, sizeof(buffer), "Nume: %s Dimensiune: %ld Modificare: %ld Inode: %ld\n", director, dir.dimensiune, dir.data_modificare, dir.inode); // folosesc snprintf ca sa obtin un string formatat asa cum vreau eu

        ssize_t written_bytes = write(snap, buffer, strlen(buffer)); // scrie in fisier informatia formatata
        if(written_bytes == -1) // daca am obtinut -1 inseamna ca apelul write() s-a terminat cu eroare
        {
            eroare("Eroare la scrierea informatiilor despre director in snapshot."); // apelam functia eroare si precizam ce mai exact nu a mers
        }

        // apelez functia mea recursiva care parcurge subarborele
        parcurgere_recursiva(director, snap);

        // inchid fisierul de snapshot pentru ca am adaugat tot ce era nevoie in el
        // functia close returneaza 0 in caz de succes, -1 in caz de eroare

        if (close(snap) == -1)
        {
            eroare("Eroare la inchiderea snapshot-ului."); // alta self-explanatory
        }
    }

    return 0;
}