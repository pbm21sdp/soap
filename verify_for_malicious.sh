#!/bin/bash

#VERIFICARE 1 - existenta argumentelor

if test "$#" -eq 0 #verifica daca s-au introdus argumente 
then
    echo "Nu ati introdus destule argumente." #afiseaza in fisierul nostru de comentarii un mesaj de eroare 
    exit 1 #intrerupe executia programului
fi

#VERIFICARE 2 - este fisier

if ! test -f "$1" || test -L "$1" #verifica daca a fost furnizat un fisier spre verificare si nu o legatura simbolica
then
    echo "Nu a fost furnizat un fisier." #afiseaza un mesaj de eroare
    exit 1 #intrerupe executia programului
fi

#daca am ajuns sa rulez un script inseamna ca am un fisier posibil suspicios, care este aici doar pentru ca nu are permisiuni
#ca sa pot sa fac anumite verificari asupra lui, am nevoie sa ii ofer cateva permisiuni 
#ca sa fie mai simplu ii dau toate permisiunile acum si pe parcursul programului fie i le iau pe toate fie ii dau doar drept de read pentru toti posibilii utilizatori - user, group si other

chmod u=rwx,g=rwx,o=rwx "$1" 

#VERIFICARE 3 - dimensiune suspecta

#presupunem ca o dimensiune suspecta pentru fisierul nostru ar fi peste 100MB
#100 MB = 100 000 000 bytes

dim_max=100000000 #variabila cu care vom compara dimensiunea fisierului 

dimensiune_fisier=$(stat -c %s "$1") #variabila care retine dimensiunea fisierului nostru in bytes

#am folosit stat cu argumentul  -c  --format=FORMAT use the specified FORMAT instead of the default; output  a  new line after each use of FORMAT
#am folosit format sequence-ul %s total size, in bytes

if test $dimensiune_fisier -gt $dim_max #verifica daca dimensiunea in bytes a fisierului este mai mare decat cea considerata deja malitioasa de noi
then
    echo -n "$1" #trimite numele fisierului inapoi ca sa stim cine e malitios, -n se asigura ca nu va mai fi \n adaugat la final -> faciliteaza compararea 
    chmod u=r,g=r,o=r "$1" 
    exit 1 #iesi din script cu cod de eroare 
fi

#VERIFICARE 4 - numar de linii, cuvinte si caractere

#variabila care retine numarul de linii
linii_fisier=$(wc -l < $1) #wc folosit cu optiunea -l printeaza nr de linii, < $1 redirecteaza intrarea standard, astfel incat se vor citi linii din fisier

#variabila care retine numarul de cuvinte
cuvinte_fisier=$(wc -w < "$1") #wc folosit cu optiunea -w printeaza nr de cuvinte, < $1 redirecteaza intrarea standard, astfel incat se vor citi linii din fisier

#variabila care retine numarul de caractere
caractere_fisier=$(wc -m < "$1") #wc folosit cu optiunea -m printeaza nr de caractere, < $1 redirecteaza intrarea standard, astfel incat se vor citi linii din fisier

#fisierul nostru ar putea fi suspect daca ar avea valori foarte mari pentru acesti 3 parametri, dar si daca sunt putine linii multe cuvinte/caractere

#VERIFICARE 4.1 - foarte multe linii, caractere si cuvinte

#presupunem ca ar fi suspect ca un fisier sa aiba 10021 linii, 20021 cuvinte si 30021 caractere

linii_max=10021
cuvinte_max=20021
caractere_max=30021

#verificam daca parametrii din fisierul nostru depasesc dimensiunile exagerate

if test $linii_fisier -gt $linii_max && $cuvinte_fisier -gt $cuvinte_max && $caractere_fisier -gt $caractere_max
then
    echo -n "$1" #trimite numele fisierului inapoi ca sa stim cine e malitios, -n se asigura ca nu va mai fi \n adaugat la final -> faciliteaza compararea 
    chmod u=r,g=r,o=r "$1" 
    exit 1
fi

#VERIFICARE 4.2 - foarte putine linii, foarte multe caractere si cuvinte

#presupunem ca ar fi suspect ca un fisier sa aiba sub 5 linii si peste 2021 cuvinte si 4021 caractere

linii_min=5
cuvinte_min=2021
caractere_min=4021

#verificam daca parametrii din fisierul nostru depasesc dimensiunile exagerate

if test $linii_fisier -lt $linii_min && test $cuvinte_fisier -gt $cuvinte_min && test $caractere_fisier -gt $caractere_min
then
    echo -n "$1" #trimite numele fisierului inapoi ca sa stim cine e malitios, -n se asigura ca nu va mai fi \n adaugat la final -> faciliteaza compararea 
    chmod u=r,g=r,o=r "$1" 
    exit 1
fi

#VERIFICARE 4.3 - foarte putine linii, foarte multe cuvinte, foarte putine caractere

#presupunem ca ar fi suspect ca un fisier sa aiba sub 5 linii, sub 121 caractere si peste 2021 cuvinte 

linii_mediu=5
cuvinte_mediu=2021
caractere_mediu=121

#verificam daca parametrii din fisierul nostru depasesc dimensiunile exagerate

if test $linii_fisier -lt $linii_mediu && $cuvinte_fisier -gt $cuvinte_mediu && $caractere_fisier -lt $caractere_mediu
then
    echo -n "$1" #trimite numele fisierului inapoi ca sa stim cine e malitios, -n se asigura ca nu va mai fi \n adaugat la final -> faciliteaza compararea 
    chmod u=r,g=r,o=r "$1" 
    exit 1
fi

#VERIFICARE 5 - cuvinte suspicioase

#folosim grep cu optiunea -c pentru a obtine numarul de linii care corespund cu pattern-ul dat

linii_grep_cuvinte=$(grep -cE 'corrupted|dangerous|risk|attack|malware|malicious|trojan|virus|hacked|hack|atac|periculos|corupt|malitios' "$1") 

#daca numarul de linii obtinut in urma lui grep este mai mare decat 0, inseamna ca s-a gasit o potrivire cu unul din cuvintele suspecte

if test $linii_grep_cuvinte -gt 0
then
    echo -n "$1" #trimite numele fisierului inapoi ca sa stim cine e malitios, -n se asigura ca nu va mai fi \n adaugat la final -> faciliteaza compararea 
    chmod u=r,g=r,o=r "$1" 
    exit 1
fi

#VERIFICARE 6 - caractere non-ASCII

#folosim grep cu optiunea -c pentru a obtine numarul de linii care corespund cu pattern-ul dat

linii_grep_caractere=$(grep --color='auto' -n "[^[:ascii:]]" "$1") #https://www.baeldung.com/linux/find-non-ascii-chars

#daca numarul de linii obtinut in urma lui grep este mai mare decat 0, inseamna ca s-a gasit o potrivire cu unul din cuvintele suspecte

if test $linii_grep_caractere -gt 0
then
    echo -n "$1" #trimite numele fisierului inapoi ca sa stim cine e malitios, -n se asigura ca nu va mai fi \n adaugat la final -> faciliteaza compararea 
    chmod u=r,g=r,o=r "$1" 
    exit 1
fi

#daca pana acum nu s-a incheiat executia scriptului, inseamna ca fisierul a trecut toate testele si este safe

echo -n "safe"
chmod u-rwx,g-rwx,o-rwx "$1" #am determinat ca fisierul nu e malitios, doar nu are permisiuni, deci ii redau lipsa permisiunilor de la inceput
exit 0 #ies din script cu succes 
