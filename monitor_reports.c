#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

volatile sig_atomic_t keep_running = 1;

void handle_sigint(int sig) {
    char msg[] = "[EXIT] Semnal SIGINT primit. Monitorul se inchide.\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    keep_running = 0;
}

void handle_sigusr1(int sig) {
    char msg[] = "[NOTIFY] Un nou raport a fost adaugat in sistem!\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
    // Faza 3: Verificare daca alt monitor ruleaza deja
    int fd_check = open(".monitor_pid", O_RDONLY);
    if (fd_check >= 0) {
        char buf[32] = {0};
        int bytes = read(fd_check, buf, sizeof(buf) - 1);
        close(fd_check);
        
        if (bytes > 0) {
            char msg[128];
            snprintf(msg, sizeof(msg), "[ERROR] Exista deja un monitor activ cu PID: %d\n", atoi(buf));
            write(STDOUT_FILENO, msg, strlen(msg));
            return 1; // Iese si va inchide automat pipe-ul
        }
    }

    // Configurarea interceptarii semnalelor
    struct sigaction sa_int, sa_usr1;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    // Salvare PID curent
    int fd = open(".monitor_pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        char msg[] = "[ERROR] Nu am putut crea .monitor_pid\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return 1;
    }
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d\n", getpid());
    write(fd, buf, len);
    close(fd);

    char start_msg[128];
    snprintf(start_msg, sizeof(start_msg), "[INFO] Monitor pornit cu PID: %d. Astept semnale...\n", getpid());
    write(STDOUT_FILENO, start_msg, strlen(start_msg));

    // Bucla de asteptare
    while (keep_running) {
        pause(); 
    }

    unlink(".monitor_pid");
    return 0;
}