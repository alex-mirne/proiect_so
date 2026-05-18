#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT 256
#define MAX_ARGS 20

// Pentru a preveni procesele zombie atunci cand copiii mor in background
void setup_sigchld() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDWAIT; 
    sigaction(SIGCHLD, &sa, NULL);
}

void start_monitor() {
    pid_t hub_mon_pid = fork();
    if (hub_mon_pid < 0) {
        perror("Eroare fork hub_mon");
        return;
    }
    
    if (hub_mon_pid == 0) {
        // --- PROCESUL HUB_MON ---
        int pipe_fd[2];
        if (pipe(pipe_fd) < 0) {
            perror("Eroare creare pipe");
            exit(1);
        }

        pid_t monitor_pid = fork();
        if (monitor_pid < 0) {
            perror("Eroare fork monitor_reports");
            exit(1);
        }

        if (monitor_pid == 0) {
            // monitor_reports
            close(pipe_fd[0]); // Inchidem capatul de citire (nu avem nevoie de el)
            
            // Legam iesirea standard la capatul de scriere al pipe-ului
            dup2(pipe_fd[1], STDOUT_FILENO); 
            close(pipe_fd[1]); // Putem inchide originalul dupa clonare
            
            execlp("./monitor_reports", "monitor_reports", NULL);
            perror("Eroare la pornirea monitor_reports");
            exit(1);
        } else {
            // hub_mon
            close(pipe_fd[1]); // Inchidem capatul de scriere (doar citim de la monitor)
            
            char buffer[256];
            int bytes_read;
            
            // Citim din pipe cat timp monitorul e activ (sau pana se opreste brusc)
            while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytes_read] = '\0';
                // Prefixam output-ul pentru a se distinge in hub
                printf("\n[HUB_MON_LOG] %scity_hub> ", buffer);
                fflush(stdout); // Fortam afisarea pe ecran
            }
            
            // Daca read() returneaza 0, pipe-ul a fost rupt (monitorul a murit / a dat exit)
            printf("\n[HUB_MON_LOG] Atentie: Procesul monitor a fost oprit!\ncity_hub> ");
            fflush(stdout);
            
            close(pipe_fd[0]);
            exit(0); // hub_mon isi termina treaba si el
        }
    } else {
        // --- PARINTELE (city_hub) ---
        printf("-> Procesul hub_mon a fost lansat in fundal (PID: %d).\n", hub_mon_pid);
    }
}

void calculate_scores(int count, char *districts[]) {
    if (count == 0) {
        printf("-> Eroare: Trebuie specificat cel putin un district.\n");
        return;
    }

    int pipes[count][2];
    pid_t pids[count];

    // Pornim cate un scorer pentru fiecare district simultan
    for (int i = 0; i < count; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("Eroare pipe");
            continue;
        }

        pids[i] = fork();
        if (pids[i] < 0) {
            perror("Eroare fork scorer");
            continue;
        }

        if (pids[i] == 0) {
            // --- SCORER ---
            close(pipes[i][0]); // Inchidem citirea
            dup2(pipes[i][1], STDOUT_FILENO); // Redirectionam stdout catre pipe
            close(pipes[i][1]);

            execlp("./scorer", "scorer", districts[i], NULL);
            perror("Eroare exec scorer");
            exit(1);
        } else {
            // --- PARINTELE ---
            close(pipes[i][1]); // Parintele doar citeste, nu scrie
        }
    }

    // Asteptam datele de la pipe-uri si generam raportul combinat
    printf("\n============== RAPORT GLOBAL WORKLOAD ==============\n");
    for (int i = 0; i < count; i++) {
        char buffer[512];
        int bytes_read;
        while ((bytes_read = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        close(pipes[i][0]);
    }
    printf("====================================================\n");
}

int main() {
    setup_sigchld();
    char input[MAX_INPUT];

    printf("======================================\n");
    printf("   City Hub - Interactive Interface   \n");
    printf("======================================\n");
    printf("Comenzi suportate:\n");
    printf("  - start_monitor\n");
    printf("  - calculate_scores <district1> <district2> ...\n");
    printf("  - exit\n\n");

    // Bucla interactiva CLI
    while (1) {
        printf("city_hub> ");
        if (fgets(input, sizeof(input), stdin) == NULL) break;

        // Curatam newline-ul
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;

        // Parsam input-ul de la tastatura
        char *args[MAX_ARGS];
        int argc = 0;
        char *token = strtok(input, " ");
        while (token != NULL && argc < MAX_ARGS) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }

        if (strcmp(args[0], "exit") == 0) {
            break;
        } else if (strcmp(args[0], "start_monitor") == 0) {
            start_monitor();
        } else if (strcmp(args[0], "calculate_scores") == 0) {
            calculate_scores(argc - 1, &args[1]);
        } else {
            printf("-> Comanda necunoscuta: %s\n", args[0]);
        }
    }

    printf("Se inchide City Hub.\n");
    return 0;
}