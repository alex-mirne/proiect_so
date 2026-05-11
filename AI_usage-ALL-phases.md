# Documentatie Utilizare AI - Faze Complete (1, 2 si 3)
# Proiect: Sisteme de Operare - City Manager

Acest document ofera o transparenta totala asupra modului in care am integrat uneltele de Inteligenta Artificiala (LLM) in dezvoltarea proiectului. AI-ul a fost utilizat strict ca un asistent tehnic pentru clarificarea apelurilor de sistem UNIX (Pipes, Fork, Exec, Semnale).

---

## FAZA 1: Filtrarea, Procesarea Rapoartelor si Fisiere Binare
* Instrument: Gemini 3 Flash
* Scop: Implementarea parsarii argumentelor si manipularea avansata a datelor binare pe disc.

Am oferit AI-ului definitia structurii `Report` si am cerut sabloane pentru functiile `parse_condition` si `match_condition`. AI-ul a propus initial conversia numerica cu `atoi()`, insa am modificat codul manual folosind `atol()` pentru campul `timestamp` (time_t) pentru a asigura compatibilitatea pe sisteme pe 64-biti. Pentru stergerea din fisiere binare, am refuzat sablonul AI care rescria tot fisierul si am implementat propriul algoritm bazat pe `pread/pwrite` si apelul de sistem `ftruncate`.

---

## FAZA 2: Gestiunea Proceselor, Semnale si IPC (Inter-Process Communication)
* Instrument: Gemini 3 Flash
* Scop: Implementarea monitorizarii asincrone cu `sigaction` si procese copil.

Pentru implementarea comezii `remove_district` si interceptarea notificarilor in background, am consultat AI-ul referitor la bunele practici UNIX. Am inlocuit functia nesigura `signal()` cu `sigaction()`, setand explicit flag-ul `SA_RESTART`. 
De asemenea, AI-ul a ajutat la intelegerea prevenirii proceselor Zombie. Prin flag-ul POSIX `SA_NOCLDWAIT`, am implementat logica prin care executia `rm -rf` din interiorul lui `execlp` devine asincrona in mod absolut, sistemul de operare avand responsabilitatea de a curata resursele.

---

## FAZA 3: Pipes, Redirects si Interfata Interactiva CLI
* Instrument: Gemini 3 Flash
* Scop: Construirea `city_hub` pentru rularea concurenta a proceselor auxiliare si redirectionarea output-ului.

### 1. Interfata Interactiva si Parsarea
Pentru a construi interfata CLI, am folosit ca inspiratie sabloanele AI pentru parsarea de siruri cu `strtok` si preluarea de linii cu `fgets`. A trebuit sa ajustez logica pentru a integra rularea comenzilor native alaturi de print-uri si afisarea constanta a promptului `city_hub> `.

### 2. Redirectionarea datelor (dup2 si Pipes)
Am apelat la asistenta AI pentru ordinea corecta a inchiderii descriptorilor de fisiere in lucrul cu apelul `pipe()`. 
* Implementare `hub_mon`: Pentru comanda `start_monitor`, am realizat o arhitectura "double fork". Am clonat `city_hub` generand `hub_mon`, care la randul lui creeaza un pipe si genereaza `monitor_reports`. Prin utilizarea apelului `dup2(fd[1], STDOUT_FILENO)` am conectat stdout-ul noului monitor la capatul de scriere al pipe-ului, `hub_mon` functionand pe post de proxy asincron.
* Implementare multi-pipe: Pentru `calculate_scores`, AI-ul a clarificat cum pot fi generate pipe-uri intr-un vector inainte de un fork. Am implementat iterarea pe baza districtelor trimise ca argument, lansand programul executabil separat `scorer`. Fiecare `scorer` isi scrie rezultatele, iar parintele (`city_hub`) citeste blocant fiecare pipe pe rand, formand un raport concatenat fara ca string-urile proceselor concurente sa se intrepatrunda pe ecran (Race Condition la nivel de afisare stdout evitat cu succes).

---
**Concluzie Generala:** Pe parcursul tuturor celor trei etape, asistentul LLM m-a scutit de navigarea manuala indelungata prin paginile de manual ("man pages"). Acest sprijin m-a lasat sa dedic timpul dezvoltarii arhitecturii, optimizarii algoritmice pe blocuri de date binare, tratarii edge-case-urilor de securitate si prevenirii scurgerilor de memorie/file descriptori in ecosistemul de programare C.