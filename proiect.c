/*Subiectul proiectului: monitorizarea modificarilor aparute in directoare de a lungul timpului.
Saptamana 1: Utilizatorul poate specifica directorul pe care doreste sa il monitorizeze in linia de comanda, ca prim argument. 
Se vor urmari atat modificarile aparute la nivelul directorului, cat si la nivelul subarborelui. Se va parcurge directorul si intregul 
sau subrbore si se face un SNAPSHOT. Acest snapshot poate fi un fisier, care contine toate datele pe care le furnizeaza directorul
si subarborele si care sunt considerate necesare.

    MAIN SUCCESS SCENARIO-ish

1. primesc un argument in linie de comanda (verific daca am primit sigur doar 1 sau am mai multe)
2. daca nu e director -> nu e eroare, dar dau exit (aici verific daca e director cu S_ISDIR)
3. parcurg directorul si fac un snapshot -> un fisier care se poate afla in directorul respectiv (trebuie sa il creez)
4. cautam in functie de inode, nu de nume, pentru ca e posibil sa fie redenumit fisierul
5. - verificam si size-ul, dar si data ultimei modificari
- rename, edit, delete, move, add -> modificari care pot afecta fisierele
- nume, inode, dimensiune, data unei modificari -> necesitati care trebuie sa fie trecute in snapshot
6. la fiecare rulare, ne extragem datele de care avem nevoie si le adaugam la snapshot*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h> 

void eroare(const char *mesaj) //functie care primeste ca parametru un String si semnaleaza o eroare de executie sau de logica 
{
    perror(mesaj); //afiseaza un mesaj de eroare mai simplu
    exit(-1); //termina programul cu un cod de eroare 
} 

void invalid(const char *mesaj) //functie care primeste ca parametru un String si semnaleaza o "diferenta" intre cerinta si ceea ce am obtinut, dar nu o eroare
{
    printf(mesaj); //afiseaza un mesaj informativ care sa ne atentioneze ca nu respectam cerinta
    exit(EXIT_SUCCESS); //opreste executia programului pentru ca nu are rost sa o continuam daca nu facem ce se cere, dar nu cu cod de eroare, ci cu cod 0
}

void parcurgere_recursiva(const char *director, int snapi) //primeste ca parametru calea spre un anumit director si descriptorul pt fisierul de snapshot
{
    //1. pt parcurgerea arborelui se deschide directorul cu functia opendir

    DIR *subdir = opendir(director); //daca subdir este NULL inseamna ca ceva nu a mers cu opendir si am intampinat o eroare

    if(subdir == NULL)
    {
        eroare("Directorul pe care il voiam curent nu a putut fi deschis.");
    }

    /*2. se citeste pe rand cate o intrare din director, apeland functia readdir, Fiecare apel al acestei functii va returna un pointer la
    o structura struct dirent in care se vor gasi informatii despre intrarea in director citita. Intrarile vor fi parcurse una dupa alta 
    pana cand se ajunge la ultima inregistrare. In momentul in care nu mai exista inregistrari in directorul citit, functia readdir va 
    returna NULL. Singura informatie care poate fi extrasa (conform POSIX) din structura dirent este numele intrarii in director. Toate 
    celelalte informatii despre intrarea citita se pot afla apeland in continuare functiile stat, fstat sau lstat.*/

    //inainte de orice citire am nevoie de zona de memorie care va stoca toate informatiile, adica struct dirent 

    struct dirent *dst; //dst de la director struct nu de la destination

    //trebuie sa apelez readdir pana cand ajung la ultima inregistrare din director, dupa care primesc ca return value NULL cand am epuizat inregistrarile
    //readdir returneaza un pointer la o structura struct dirent, deci dst al meu va fi de fapt readdir

    while(1)
    {
        dst = readdir(subdir); //la fiecare iteratie completez structura cu date despre obiectul analizat la acel moment

        if(dst == NULL) //daca avem NULL ca return value inseamna ca nu mai sunt inregistrari in director 
        {
            break; //s-a incheiat parcurgerea directorului, deci putem iesi din loop-ul infinit
        }
    }
}

int main(int argc, char *argv[]) //arguments count(argc) specifica nr argumentelor din linie de comanda, incluzand numele programului executat -> 1 parametru dat -> argc = 2 (un fel de $# din script)
//arguments vector(argv) este un vector de String-uri care contine cate un argument pe fiecare pozitie incepand cu 0, unde se afla numele executabilului
//-> numele directorului meu va fi pe pozitia 1, deci argv[1]
{
    //se asteapta un nume de director dat ca argument, deci argc trebuie sa fie 2 

    if(argc != 2) //verificam daca argc este diferit de 2
    {
        //daca nr de argumente nu este 1 inseamna ca nu s-a respectat cerinta
        //acesta este un caz de eroare, deci putem apela functia creata eroare anterior cu un mesaj care sa ne anunte ca am introdus un nr diferit de argumente 
        invalid("Numarul de argumente difera de cel asteptat.");
    }

    const char *director = argv[1]; //pointer in care se stocheaza calea directorului dat ca argument

    struct stat st; //structura care contine metadata 

    //apelul lstat returneaza 0 in caz de succes, sau -1 in caz de eroare 
    //pentru a fi folosit necesita includerea fisierului header sys/stat.header

    //folosim apelul lstat pentru directorul primit ca parametru si pasam ca parametru 2 zona de memorie unde vrem sa pastram toate informatiile, adica st
    //lstat este mai useful pentru ca daca este aplicata unei legaturi simbolice, informatiile returnate se vor referi la legatura, si nu la fisierul indicat

    if(lstat(director, &st) == -1) //daca lstat returneaza -1 inseamna ca a aparut o eroare
    {
        eroare("Eroare la determinarea informatiilor despre director, ceva s-a intamplat cu lstat."); //apelam functia eroare si precizam ce mai exact nu a mers
    }

    //daca nu s-a oprit din executie programul inseamna ca apelul lstat a reusit, deci am obtinut informatii despre director
    //prin intermediul campului st_mode din st, putem verifica daca intr-adevar am primit un director ca parametru sau daca avem altceva
    //folosim macro-ul S_ISDIR(m) ca sa verificam daca tipul fisierului primit este director
    //in cazul nostru m este st.st_mode
    
    if(S_ISDIR(st.st_mode) != 0) //daca valoarea returnata nu este 0 inseamna ca nu am primit ca argument un director
    {
        invalid("Argumentul introdus nu este un director."); //faptul ca nu am primit ca arg un director nu e o eroare, doar nu face ce vreau eu -> doar invalid, nu eroare
    }

    //daca nu s-a oprit din executie programul inseamna ca am primit ca argument un director, deci pana acum respectam cerinta
    //urmatorul pas este sa deschidem acest director folosind functia opendir cu director ca parametru
    //opendir returneaza NULL in caz de eroare sau o referinta la o structura dedicata de tip DIR in caz de succes
    //definim noi un pointer la o structura de tip DIR, care va prelua valoarea returnata de opendir 

    DIR *direct = opendir(director); //multi dir asa ca am pus direct ca sa le mai deosebesc intre ele 

    //daca direct este NULL inseamna ca ceva nu a mers cu opendir si am intampinat o eroare

    if(direct == NULL)
    {
        eroare("Eroare la deschiderea directorului, s-a intamplat ceva cu opendir."); //clar n-a mers opendir deci nu pot sa deschid directorul si sa ma uit prin el
    }

    //daca pana acuma programul nu s-a oprit din executie inseamna ca am respectat cerintele si n-am dat de erori
    //pana aici ar trebui sa am un singur arg in linia de comanda adica directorul pe care vreau sa il verific 
    //ar trebui sa am o confirmare ca am putut sa aflu metadata cu lstat 
    //ar trebui sa am o confirmare ca este director 
    //ar trebui sa am o confirmare ca pot sa il si deschid 

    //urmeaza sa fac o functie care sa parcurga in mod recursiv directorul si sa adune date despre orice are in el
    //am nevoie de fisierul snapshot 
    //ca sa il creez si deschid folosesc functia creat (const char *pathname, mode_t mode);

    const char *nume_snapshot = "snap.txt"; //numele fisierului snapshot este snap.txt
    mode_t mod = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; //intr-o variabila de tip mode_t stochez permisiunile pt acest fisier snap si anume: read si write pt user, doar read pt group si other
    //am combinat constante folosind operatorul sau ('|'), pentru a seta mai multi biti 

    //in caz de succes, creat returneaza un descriptor de fisier
    //in caz de eroare, returneaza -1

    int snapi = creat(nume_snapshot, mod);
    if(snapi == -1)
    {
        eroare("Eroare la crearea si/sau deschiderea fisierului de snapshot."); //ori nu s-a creat, ori nu s-a deschis, cert e ca n-a mers
    }

    //apelez functia mea recursiva care parcurge subarborele 
    
    //inchid directorul dupa ce am ajuns la finalul programului
    //closedir returneaza 0 in caz de succes, -1 in caz de eroare

    if(closedir(direct) == -1)
    {
        eroare("Eroare la inchiderea directorului."); //self-explanatory
    }

    //inchid si fisierul de snapshot pentru ca am adaugat tot ce era nevoie in el
    //functia close returneaza 0 in caz de succes, -1 in caz de eroare

    if(close(nume_snapshot) == -1)
    {
        eroare("Eroare la inchiderea snapshot-ului."); //alta self-explanatory
    }

    return 0;
}