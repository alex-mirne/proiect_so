#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

// Variabila globala pentru a opri bucla cand primim SIGINT
volatile sig_atomic_t keep_running = 1;


void handle_sigint(int sig) {
    char msg[] = "\n[Monitor] Semnal SIGINT primit. Se inchide monitorul...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;
}

void handle_sigusr1(int sig) {
    char msg[] = "[Monitor] Notificare: Un nou raport a fost adaugat in sistem!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main() {
    struct sigaction sa_int, sa_usr1;

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    sigaction(SIGINT, &sa_int, NULL);

    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART; // Pentru a nu intrerupe functii blocante
    sigaction(SIGUSR1, &sa_usr1, NULL);

    int fd = open(".monitor_pid", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Eroare la crearea .monitor_pid");
        return 1;
    }
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "%d\n", getpid());
    write(fd, buf, len);
    close(fd);

    printf("[Monitor] Pornit cu PID-ul: %d. Astept semnale...\n", getpid());

    while (keep_running) {
        pause(); 
    }

    unlink(".monitor_pid");
    printf("[Monitor] Fisierul .monitor_pid a fost sters. La revedere!\n");

    return 0;
}