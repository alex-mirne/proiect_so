# Documentatie Utilizare AI - Etapa 1

Pentru realizarea primei etape a proiectului de Sisteme de Operare, am utilizat asistenta Inteligentei Artificiale exclusiv pentru implementarea logicii de filtrare a rapoartelor. 

## 1. Instrumentul utilizat
* Model: Gemini 3 Flash 
* Data: Aprilie 2026 

## 2. Prompt-uri utilizate
Am folosit AI-ul pentru a genera doua functii specifice, oferindu-i detaliile despre structura mea "Report":

* Pentru parsare: I-am cerut sa creeze functia "parse_condition" care sa poata sparge un string de tipul "camp:operator:valoare" in trei variabile separate, folosind caracterul ":" ca separator.
* Pentru potrivire: I-am cerut sa genereze functia "match_condition" care sa compare datele dintr-un raport cu o conditie primita (de exemplu, daca severitatea este mai mare sau egala cu o anumita valoare). Am specificat ca trebuie sa suporte atat comparari numerice, cat si de text pentru campurile severity, category, inspector si timestamp.

## 3. Modificari si evaluare critica
Dupa ce am primit codul initial, am facut cateva ajustari manuale importante pentru ca programul sa functioneze corect in mediul UNIX:

* Corectia tipurilor: AI-ul a propus initial "atoi" pentru toate numerele, dar am modificat manual partea de "timestamp" sa foloseasca "atol", pentru a nu avea probleme cu formatul time_t pe 64 de biti.
* Optimizarea scanarii: Am modificat formatul functiei sscanf pentru a fi mai robust, asigurandu-ma ca separatorii nu sunt inclusi accidental in valorile extrase.
* Verificarea string-urilor: Am confirmat ca pentru campurile de text se foloseste strcmp(), evitand erorile de comparare directa a pointerilor.

## 4. Ce am invatat
Acest proces m-a ajutat sa inteleg mai bine cum sa procesez argumente complexe din linia de comanda si, mai ales, cum sa fac legatura intre datele introduse de utilizator ca text si structurile binare salvate pe disc. Am invatat, de asemenea, sa folosesc sscanf intr-un mod mai avansat pentru a extrage sub-stringuri fara a strica buffer-ul original.