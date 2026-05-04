# Documentatie Utilizare AI - Faza 1 si Faza 2
# Proiect: Sisteme de Operare - City Manager

Acest document ofera o transparenta totala asupra modului in care am integrat uneltele de Inteligenta Artificiala (LLM) in dezvoltarea proiectului. AI-ul a fost utilizat strict ca un asistent tehnic si tutor pentru clarificarea apelurilor de sistem UNIX, in timp ce arhitectura, logica de baza si depanarea au fost realizate manual.

---

## FAZA 1: Filtrarea si Procesarea Rapoartelor

* Instrument: Gemini 3 Flash
* Data utilizarii: Aprilie 2026
* Scop: Implementarea parsarii avansate a argumentelor din linia de comanda.

### 1. Interactiunea si Prompt-urile
Pentru sectiunea de filtrare, cel mai mare obstacol a fost transformarea unor argumente textuale introduse de utilizator (de tipul "severity:>=:2") in operatii logice pe structuri de date binare din limbajul C. Am oferit AI-ului definitia structurii mele `Report` si i-am solicitat sa genereze scheletul pentru doua functii de utilitate: `parse_condition` si `match_condition`.

### 2. Analiza si Modificarile Manuale
Codul generat initial a necesitat revizuiri arhitecturale importante pentru a fi functional si sigur in mediul de testare UNIX:
* Gestionarea timpului (64-bit): AI-ul a propus conversia tuturor numerelor cu `atoi()`. Am identificat rapid ca acest lucru ar fi corupt campul `timestamp`, care este de tip `time_t` (pe 64 de biti pe sistemele moderne, pentru a evita problema anului 2038). Am rescris manual acea sectiune folosind `atol()` si cast-uri explicite.
* Securitatea bufferelor (sscanf): Pentru a preveni erorile de tip "buffer overflow" si parsarea gresita a delimitatorilor, am inlocuit formatul standard `%s` din sscanf cu `%[^:]`. Aceasta modificare instruieste compilatorul sa citeasca strict pana la caracterul ':' si sa nu inghita accidental operatorii logici.
* Compararea string-urilor: Am adaugat verificari suplimentare folosind `strcmp` pentru campurile `category` si `inspector_name`.

---

## FAZA 2: Gestiunea Proceselor, Semnale si IPC (Inter-Process Communication)

* Instrument: Gemini 3 Flash
* Data utilizarii: Aprilie 2026
* Scop: Implementarea monitorizarii asincrone si a comenzilor de stergere distructiva.

### 1. Interceptarea Semnalelor si API-ul UNIX Modern
Am consultat AI-ul pentru a intelege diferentele dintre vechiul apel `signal()` si varianta moderna si sigura `sigaction()`. AI-ul a furnizat sablonul pentru structura `sigaction`.
* Implementare manuala: Am integrat functia `sigemptyset()` pentru a ma asigura ca nu exista semnale blocate accidental in timpul executiei handler-ului. De asemenea, am setat manual flag-ul `SA_RESTART` pentru semnalul `SIGUSR1`, prevenind astfel intreruperea functiei `pause()` din bucla principala a monitorului.
* Concepte invatate: AI-ul m-a ajutat sa inteleg conceptul de "async-signal-safety". Am aflat ca functiile din familia `printf` pot cauza deadlock-uri daca sunt chemate din interiorul unui semnal. In consecinta, am modificat manual tot codul de logging din monitor pentru a folosi exclusiv apelul de sistem de nivel scazut `write(STDOUT_FILENO, ...)`.

### 2. Crearea si Stergerea cu Fork si Exec
Pentru implementarea comenzii `remove_district`, am solicitat un exemplu de utilizare a familiilor de functii `exec`.
* Modificari de securitate: AI-ul a sugerat crearea procesului copil (`pid_t pid = fork()`) si utilizarea `execlp("rm", "rm", "-rf", target, NULL)`. Totusi, pentru a preveni executia unor comenzi dezastruoase din greseala (cum ar fi stergerea radacinii `/`), am implementat manual un filtru de securitate in procesul parinte, inainte de `fork`, care verifica prezenta caracterelor de tip slash in `district_id`.
* Sincronizare: Am utilizat apelul de sistem `wait(&status)` in procesul parinte, in tandem cu macro-urile `WIFEXITED` si `WEXITSTATUS`, pentru a ma asigura ca programul principal nu returneaza succes inainte ca `rm -rf` sa fi sters fizic fisierele de pe disc.

### 3. Comunicarea Inter-Procese (Semnale)
Pentru ca `city_manager` sa notifice `monitor_reports`, am cerut sfaturi despre trimiterea asincrona de date. AI-ul a propus utilizarea functiei `kill()`. 
* Modificari: Pe parcursul implementarii, am invatat cum sa tratez erorile de compilare specifice (ex: declararea implicita a functiei `kill`), incluzand manual header-ele necesare (`<signal.h>` si `<sys/wait.h>`). Am conceput logica de citire a PID-ului din fisierul ascuns `.monitor_pid` folosind functiile de baza `open`, `read` si conversia cu `atoi`, tratand corect cazurile in care monitorul nu ruleaza in fundal.

---
**Concluzie:** Utilizarea asistentului LLM in aceste doua etape mi-a accelerat semnificativ procesul de scriere a sintaxei UNIX "boilerplate" si a actionat ca un instrument eficient de studiu. Acest lucru mi-a permis sa aloc timpul salvat catre analiza arhitecturii aplicatiei, tratarea cazurilor limita (edge-cases), securizarea inputului si intelegerea detaliata a modului in care un sistem de operare modern gestioneaza concurenta, descriptorii de fisiere si comunicarea inter-procese. 

In plus, experienta de a rafina si corecta codul propus initial de AI mi-a dezvoltat considerabil acuitatea in procesul de debugging. Am realizat ca, desi Inteligenta Artificiala poate oferi o baza solida de pornire, integrarea finala necesita o intelegere riguroasa a manualelor de sistem (man pages) si o validare critica a fiecarei functii. Astfel, proiectul a reprezentat nu doar o aplicare directa a conceptelor teoretice de la curs, ci si un exercitiu valoros de adaptare a instrumentelor moderne de dezvoltare la rigorile si standardele stricte impuse de programarea la nivel de sistem (systems programming).