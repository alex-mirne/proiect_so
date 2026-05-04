#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> //pt apeluri de sistem
#include <sys/stat.h>
#include <fcntl.h> //file control
#include <signal.h>
#include <sys/wait.h>

#define MAX_NAME 64
#define MAX_CAT 64
#define MAX_DESC 256

typedef struct Report {
    int report_id;
    char inspector_name[MAX_NAME];
    float lat;
    float lon;
    char category[MAX_CAT];
    int severity; // 1 minor, 2 moderate, 3 critical
    time_t timestamp;
    char description[MAX_DESC];
} Report;

void get_permissions_string(mode_t mode, char *str) {
    strcpy(str, "---------"); 
    if(mode & S_IRUSR) str[0] = 'r';
    if (mode & S_IWUSR) str[1] = 'w';
    if (mode & S_IXUSR) str[2] = 'x';
    if (mode & S_IRGRP) str[3] = 'r';
    if (mode & S_IWGRP) str[4] = 'w';
    if (mode & S_IXGRP) str[5] = 'x';
    if (mode & S_IROTH) str[6] = 'r';
    if (mode & S_IWOTH) str[7] = 'w';
    if (mode & S_IXOTH) str[8] = 'x';
    str[9] = '\0';
}

void log_operation(const char *district_id, const char *role, const char *user, const char *action) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/logged_district", district_id);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        char buffer[512];
        time_t now = time(NULL);
        int len = snprintf(buffer, sizeof(buffer), "%ld | %s | %s | %s\n", 
                           now, role, user, action);
        write(fd, buffer, len); 
        chmod(log_path, 0644); 
        close(fd);
    }
}

// Functie helper pentru faza 2: Notifica monitor_reports
int notify_monitor() {
    int fd = open(".monitor_pid", O_RDONLY);
    if (fd < 0) return -1; // Fisierul nu exista (monitorul nu ruleaza)
    
    char buf[32] = {0};
    int bytes = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    
    if (bytes > 0) {
        int pid = atoi(buf);
        if (pid > 0 && kill(pid, SIGUSR1) == 0) {
            return 0; // Succes
        }
    }
    return -1; // Eroare la trimiterea semnalului
}

void add_report(const char *district_id, const char *role, const char *user_name) {
    if(mkdir(district_id, 0750) == -1) {
        // Folder exists or error
    } else {
        chmod(district_id, 0750);
    }
    
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district_id);

    int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if (fd < 0) {
        perror("Eroare la deschiderea reports.dat");
        return;
    }
    chmod(file_path, 0664);

    Report r;
    memset(&r, 0, sizeof(Report));
    
    r.report_id = (int)time(NULL);
    strncpy(r.inspector_name, user_name, MAX_NAME - 1);
    r.timestamp = time(NULL);
    
    printf("X (GPS Latitude): "); scanf("%f", &r.lat);
    printf("Y (GPS Longitude): "); scanf("%f", &r.lon);
    printf("Category (road/lighting/flooding): "); scanf("%s", r.category);
    printf("Severity level (1/2/3): "); scanf("%d", &r.severity);
    printf("Description: "); 
    getchar(); 
    fgets(r.description, MAX_DESC, stdin);
    r.description[strcspn(r.description, "\n")] = 0;

    if (write(fd, &r, sizeof(Report)) < 0) {
        perror("Eroare la scrierea raportului");
    } else {
        printf("Raport adaugat cu succes in %s/reports.dat\n", district_id);
        log_operation(district_id, role, user_name, "add");
    }

    close(fd);

    // Faza 2: Notificam monitorul si setam mesajul corespunzator pentru log
        char log_msg[256];
        if (notify_monitor() == 0) {
            snprintf(log_msg, sizeof(log_msg), "add (monitor notificat cu succes)");
        } else {
            snprintf(log_msg, sizeof(log_msg), "add (eroare: monitorul nu a putut fi notificat)");
        }
        log_operation(district_id, role, user_name, log_msg);
}

// Functie noua Faza 2: Stergerea unui district intreg cu rm -rf
void remove_district(const char *district_id, const char *role) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Eroare: Doar managerul poate sterge un district complet!\n");
        return;
    }

    // Masura de siguranta minora ca sa nu stergem "/" sau ".." accidental
    if (strchr(district_id, '/') != NULL || strcmp(district_id, ".") == 0 || strcmp(district_id, "..") == 0) {
        fprintf(stderr, "Eroare: Nume de district invalid.\n");
        return;
    }

    // Stergem link-ul simbolic
    char link_name[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);
    unlink(link_name);

    pid_t pid = fork();
    
    if (pid < 0) {
        perror("Eroare la fork");
    } else if (pid == 0) {
        execlp("rm", "rm", "-rf", district_id, NULL);
        perror("Eroare la executia comenzii rm");
        exit(1);
    } else {
        // Suntem in procesul parinte. Asteptam sa se termine comanda de stergere
        int status;
        wait(&status);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("Districtul '%s' si link-ul simbolic au fost sterse cu succes.\n", district_id);
        } else {
            printf("A aparut o eroare la stergerea districtulu6i.\n");
        }
    }
}

void list_reports(const char *district_id, const char *role, const char *user) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district_id);

    struct stat st;
    if (stat(file_path, &st) < 0) {
        perror("Eroare: Nu s-a putut accesa reports.dat");
        return;
    }

    char perms[11];
    get_permissions_string(st.st_mode, perms);
    
    printf("File: %s\n", file_path);
    printf("Permissions: %s\n", perms);
    printf("Size: %lld bytes\n", (long long)st.st_size);
    printf("Last modified: %s", ctime(&st.st_mtime));
    printf("-------------------------------------------\n");

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        printf("[%d] Inspector: %s | Cat: %s | Sev: %d\n", 
                r.report_id, r.inspector_name, r.category, r.severity);
        printf("    Coords: %.4f, %.4f | Time: %s", 
                r.lat, r.lon, ctime(&r.timestamp));
        printf("    Description: %s\n", r.description);
        printf("    ---------------------------------------\n");
    }
    close(fd);
    log_operation(district_id, role, user, "list");
}

void view_report(const char *district_id, const char *role, const char *user, int target_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    int found = 0;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.report_id == target_id) {
            printf("\n=== DETALII RAPORT %d ===\n", r.report_id);
            printf("Inspector:  %s\n", r.inspector_name);
            printf("Locatie:    Lat: %.4f, Lon: %.4f\n", r.lat, r.lon);
            printf("Categorie:  %s\n", r.category);
            printf("Severitate: %d\n", r.severity);
            printf("Data:       %s", ctime(&r.timestamp));
            printf("Descriere:  %s\n", r.description);
            found = 1;
            break;
        }
    }
    close(fd);
    if (found) log_operation(district_id, role, user, "view");
}

void remove_report(const char *district_id, const char *role, const char *user, int target_id) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Eroare: Doar managerul poate sterge rapoarte!\n");
        return;
    }

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district_id);

    int fd = open(file_path, O_RDWR);
    if (fd < 0) return;

    Report r;
    off_t pos_to_delete = -1;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        if (r.report_id == target_id) {
            pos_to_delete = lseek(fd, 0, SEEK_CUR) - sizeof(Report);
            break;
        }
    }

    if (pos_to_delete != -1) {
        Report next_r;
        off_t current_read_pos = pos_to_delete + sizeof(Report);
        off_t current_write_pos = pos_to_delete;

        while (pread(fd, &next_r, sizeof(Report), current_read_pos) == sizeof(Report)) {
            pwrite(fd, &next_r, sizeof(Report), current_write_pos);
            current_read_pos += sizeof(Report);
            current_write_pos += sizeof(Report);
        }

        struct stat st;
        fstat(fd, &st);
        ftruncate(fd, st.st_size - sizeof(Report));
        printf("Raportul %d a fost sters.\n", target_id);
        log_operation(district_id, role, user, "remove_report");
    }
    close(fd);
}

void update_threshold(const char *district_id, const char *role, const char *user, int new_threshold) {
    if (strcmp(role, "manager") != 0) {
        fprintf(stderr, "Eroare: Manager role only.\n");
        return;
    }

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/district.cfg", district_id);

    struct stat st;
    if (stat(file_path, &st) == 0) {
        if ((st.st_mode & 0777) != 0640) {
            fprintf(stderr, "Diagnostic: Permisiuni incorecte (se astepta 640).\n");
            return;
        }
    }

    int fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    if (fd < 0) return;

    char buffer[32];
    int len = snprintf(buffer, sizeof(buffer), "%d\n", new_threshold);
    write(fd, buffer, len);
    chmod(file_path, 0640);
    close(fd);
    log_operation(district_id, role, user, "update_threshold");
}

void manage_symlink(const char *district_id) {
    char link_name[256];
    char target_path[256];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);
    snprintf(target_path, sizeof(target_path), "%s/reports.dat", district_id);

    struct stat st;
    if (lstat(link_name, &st) == 0) {
        if (S_ISLNK(st.st_mode)) unlink(link_name);
    }
    symlink(target_path, link_name);
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    if (sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3) return 1;
    return 0;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (strcmp(field, "severity") == 0) {
        int val = atoi(value);
        if (strcmp(op, "==") == 0) return r->severity == val;
        if (strcmp(op, "!=") == 0) return r->severity != val;
        if (strcmp(op, "<") == 0)  return r->severity < val;
        if (strcmp(op, "<=") == 0) return r->severity <= val;
        if (strcmp(op, ">") == 0)  return r->severity > val;
        if (strcmp(op, ">=") == 0) return r->severity >= val;
    } 
    else if (strcmp(field, "timestamp") == 0) {
        long val = atol(value);
        if (strcmp(op, "==") == 0) return r->timestamp == (time_t)val;
        if (strcmp(op, "!=") == 0) return r->timestamp != (time_t)val;
        if (strcmp(op, "<") == 0)  return r->timestamp < (time_t)val;
        if (strcmp(op, "<=") == 0) return r->timestamp <= (time_t)val;
        if (strcmp(op, ">") == 0)  return r->timestamp > (time_t)val;
        if (strcmp(op, ">=") == 0) return r->timestamp >= (time_t)val;
    } 
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->category, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->category, value) != 0;
    } 
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r->inspector_name, value) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r->inspector_name, value) != 0;
    }
    return 0;
}

void filter_reports(const char *district_id, const char *role, const char *user, int argc, char *argv[], int start_idx) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district_id);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int all_match = 1;
        for (int i = start_idx; i < argc; i++) {
            char f[64], o[10], v[128];
            if (parse_condition(argv[i], f, o, v)) {
                if (!match_condition(&r, f, o, v)) {
                    all_match = 0;
                    break;
                }
            }
        }
        if (all_match) {
            printf("[%d] %s - %s (Sev: %d)\n", r.report_id, r.category, r.inspector_name, r.severity);
        }
    }
    close(fd);
    log_operation(district_id, role, user, "filter");
}

int main(int argc, char *argv[]) {
    char *role = NULL, *user = NULL, *district = NULL, *cmd = NULL;
    int arg_idx_after_district = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--role") == 0 && i + 1 < argc) role = argv[++i];
        else if (strcmp(argv[i], "--user") == 0 && i + 1 < argc) user = argv[++i];
        else if (cmd == NULL) cmd = argv[i];
        else if (district == NULL) {
            district = argv[i];
            arg_idx_after_district = i + 1;
        }
    }

    if (!role || !user || !cmd || !district) {
        printf("Usage: ./city_manager --role <role> --user <user> <command> <district_id> [extra_args]\n");
        return 1;
    }

    if (strcmp(cmd, "remove_district") == 0) {
        remove_district(district, role);
        return 0; // Oprim executia aici pt ca am sters districtul
    }

    manage_symlink(district);

    if (strcmp(cmd, "add") == 0) {
        add_report(district, role, user);
    } 
    else if (strcmp(cmd, "list") == 0) {
        list_reports(district, role, user);
    } 
    else if (strcmp(cmd, "view") == 0) {
        if (arg_idx_after_district < argc) view_report(district, role, user, atoi(argv[arg_idx_after_district]));
    } 
    else if (strcmp(cmd, "remove_report") == 0) {
        if (arg_idx_after_district < argc) remove_report(district, role, user, atoi(argv[arg_idx_after_district]));
    } 
    else if (strcmp(cmd, "update_threshold") == 0) {
        if (arg_idx_after_district < argc) update_threshold(district, role, user, atoi(argv[arg_idx_after_district]));
    } 
    else if (strcmp(cmd, "filter") == 0) {
        filter_reports(district, role, user, argc, argv, arg_idx_after_district);
    }
    return 0;
}