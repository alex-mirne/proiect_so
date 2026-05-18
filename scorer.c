#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_NAME 64
#define MAX_CAT 64
#define MAX_DESC 256

typedef struct Report {
    int report_id;
    char inspector_name[MAX_NAME];
    float lat;
    float lon;
    char category[MAX_CAT];
    int severity; 
    time_t timestamp;
    char description[MAX_DESC];
} Report;

// Structura simpla pentru a tine evidenta scorurilor
typedef struct {
    char name[MAX_NAME];
    int score;
} InspectorScore;

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    const char *district = argv[1];

    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/reports.dat", district);

    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        printf("[%s] Lipsesc datele (reports.dat inexistent).\n", district);
        return 0; // Iesire curata ca sa nu crape pipe-ul in hub
    }

    InspectorScore scores[100];
    int num_inspectors = 0;

    Report r;
    // Citim structura cu structura din fisierul binar
    while (read(fd, &r, sizeof(Report)) == sizeof(Report)) {
        int found = 0;
        for (int i = 0; i < num_inspectors; i++) {
            if (strcmp(scores[i].name, r.inspector_name) == 0) {
                scores[i].score += r.severity; // Adunam la scorul existent
                found = 1;
                break;
            }
        }
        // Daca e un inspector nou, il adaugam in lista
        if (!found && num_inspectors < 100) {
            strcpy(scores[num_inspectors].name, r.inspector_name);
            scores[num_inspectors].score = r.severity;
            num_inspectors++;
        }
    }
    close(fd);

    // Tot ce printam aici va fi capturat de pipe-ul din city_hub
    printf("--- Punctaje Workload: %s ---\n", district);
    for (int i = 0; i < num_inspectors; i++) {
        printf(" -> Inspector %s: %d puncte\n", scores[i].name, scores[i].score);
    }
    if (num_inspectors == 0) {
        printf(" -> Niciun raport gasit.\n");
    }
    
    return 0;
}