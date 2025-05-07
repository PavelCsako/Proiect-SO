#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_CMD_LEN 256
#define TEMP_CMD_FILE "monitor_cmd.txt"

pid_t monitor_pid = 0;
volatile sig_atomic_t monitor_ready = 0;
volatile sig_atomic_t monitor_done = 0;

// === semnale de la monitor ===
void handler_ready(int sig) {
    monitor_ready = 1;
}
void handler_done(int sig) {
    monitor_done = 1;
}
void handler_child_exit(int sig) {
    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_pid = 0;
}

void start_monitor() {
    if (monitor_pid != 0) {
        printf("Monitorul rulează deja!\n");
        return;
    }

    monitor_pid = fork();

    if (monitor_pid == -1) {
        perror("Eroare la fork");
        exit(1);
    }

    if (monitor_pid == 0) {
        // === Codul din monitor (proces copil) ===

        // Setează handler pt SIGUSR1
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGUSR1, &sa, NULL);

        while (1) {
            pause(); // așteaptă semnal

            FILE *f = fopen(TEMP_CMD_FILE, "r");
            if (!f) continue;

            char cmd[MAX_CMD_LEN];
            fgets(cmd, sizeof(cmd), f);
            fclose(f);

            if (strcmp(cmd, "exit\n") == 0) {
                usleep(500000); // întârziere simulată
                kill(getppid(), SIGUSR2); // semnal către părinte că s-a terminat
                exit(0);
            }

            printf(">>> Monitorul execută: %s", cmd);
            system(cmd);

            kill(getppid(), SIGUSR1); // semnal către părinte că s-a executat
        }

    } else {
        printf("Monitorul a fost pornit (PID: %d)\n", monitor_pid);
    }
}

void send_cmd_to_monitor(const char *cmdline) {
    if (monitor_pid == 0) {
        printf("Eroare: monitorul nu este pornit.\n");
        return;
    }

    FILE *f = fopen(TEMP_CMD_FILE, "w");
    if (!f) {
        perror("Eroare scriere fișier cmd");
        return;
    }
    fprintf(f, "%s\n", cmdline);
    fclose(f);

    monitor_ready = 0;
    kill(monitor_pid, SIGUSR1);

    while (!monitor_ready) {
        pause();
    }
}

void stop_monitor() {
    if (monitor_pid == 0) {
        printf("Monitorul nu rulează.\n");
        return;
    }

    FILE *f = fopen(TEMP_CMD_FILE, "w");
    fprintf(f, "exit\n");
    fclose(f);

    monitor_done = 0;
    kill(monitor_pid, SIGUSR1);

    while (!monitor_done) {
        pause();
    }

    printf("Monitorul a fost oprit.\n");
    monitor_pid = 0;
}

int main() {
    // === Setăm handler-ele pentru semnale de la monitor ===
    struct sigaction sa_ready, sa_done, sa_chld;
    sa_ready.sa_handler = handler_ready;
    sigemptyset(&sa_ready.sa_mask);
    sa_ready.sa_flags = 0;
    sigaction(SIGUSR1, &sa_ready, NULL);

    sa_done.sa_handler = handler_done;
    sigemptyset(&sa_done.sa_mask);
    sa_done.sa_flags = 0;
    sigaction(SIGUSR2, &sa_done, NULL);

    sa_chld.sa_handler = handler_child_exit;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = 0;
    sigaction(SIGCHLD, &sa_chld, NULL);

    char input[256];

    while (1) {
        printf("\n[treasure_hub] > ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        input[strcspn(input, "\n")] = 0; // elimină newline

        if (strcmp(input, "start_monitor") == 0) {
            start_monitor();
        }
        else if (strncmp(input, "list_hunts", 10) == 0) {
            send_cmd_to_monitor("./treasure_manager --list-hunts");
        }
        else if (strncmp(input, "list_treasures", 14) == 0) {
            char hunt[128];
            printf("Hunt ID: ");
            scanf("%s", hunt); getchar();
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "./treasure_manager --list %s", hunt);
            send_cmd_to_monitor(cmd);
        }
        else if (strncmp(input, "view_treasure", 13) == 0) {
            char hunt[128]; int tid;
            printf("Hunt ID: "); scanf("%s", hunt);
            printf("Treasure ID: "); scanf("%d", &tid); getchar();
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "./treasure_manager --view %s %d", hunt, tid);
            send_cmd_to_monitor(cmd);
        }
        else if (strcmp(input, "stop_monitor") == 0) {
            stop_monitor();
        }
        else if (strcmp(input, "exit") == 0) {
            if (monitor_pid != 0) {
                printf("Monitorul rulează încă. Opriți-l cu 'stop_monitor'!\n");
            } else {
                printf("Ieșire din treasure_hub.\n");
                break;
            }
        }
        else {
            printf("Comandă necunoscută.\n");
        }
    }

    return 0;
}
