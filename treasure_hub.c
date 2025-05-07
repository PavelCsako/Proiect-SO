#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX_CMD 256
#define TEMP_FILE "monitor_cmd.txt"
#define USERNAME_LEN 50
#define CLUE_LEN 100

typedef struct {
    int ID;
    char username[USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[CLUE_LEN];
    int value;
} Treasure;

pid_t monitor_pid = -1;
int monitor_running = 0;

volatile sig_atomic_t got_signal = 0;

void handle_sigusr1(int sig) {
    got_signal = 1;
}

void write_command_to_file(const char *cmd) {
    int fd = open(TEMP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, cmd, strlen(cmd));
        close(fd);
    }
}

void run_monitor() {
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    while (1) {
        while (!got_signal) pause();
        got_signal = 0;

        char cmd[MAX_CMD] = {0};
        int fd = open(TEMP_FILE, O_RDONLY);
        if (fd >= 0) {
            read(fd, cmd, sizeof(cmd));
            close(fd);
        }

        if (strncmp(cmd, "list_hunts", 10) == 0) {
            DIR *d = opendir(".");
            if (!d) { perror("opendir"); continue; }
            struct dirent *entry;
            while ((entry = readdir(d)) != NULL) {
                if (entry->d_type == DT_DIR && strncmp(entry->d_name, "Hunt", 4) == 0) {
                    char path[256];
                    int len = snprintf(path, sizeof(path), "%s/treasures.dat", entry->d_name);
                    if (len >= sizeof(path)) {
                        fprintf(stderr, "Path prea lung, ignorat: %s\n", entry->d_name);
                        continue;
                    }
                    int count = 0, fd2 = open(path, O_RDONLY);
                    if (fd2 >= 0) {
                        Treasure tr;
                        while (read(fd2, &tr, sizeof(Treasure)) == sizeof(Treasure))
                            count++;
                        close(fd2);
                    }
                    printf("Hunt: %s, Comori: %d\n", entry->d_name, count);
                }
            }
            closedir(d);

        } else if (strncmp(cmd, "list_treasures", 14) == 0) {
            char hunt[100];
            sscanf(cmd, "list_treasures %s", hunt);
            char path[256];
            snprintf(path, sizeof(path), "%s/treasures.dat", hunt);
            int fd2 = open(path, O_RDONLY);
            if (fd2 < 0) {
                printf("Vânătoarea %s nu există.\n", hunt);
                continue;
            }
            Treasure tr;
            while (read(fd2, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
                printf("ID: %d, User: %s, GPS: %.2f, %.2f, Clue: %s, Value: %d\n",
                       tr.ID, tr.username, tr.latitude, tr.longitude, tr.clue, tr.value);
            }
            close(fd2);

        } else if (strncmp(cmd, "view_treasure", 13) == 0) {
            char hunt[100];
            int tid;
            sscanf(cmd, "view_treasure %s %d", hunt, &tid);
            char path[256];
            snprintf(path, sizeof(path), "%s/treasures.dat", hunt);
            int fd2 = open(path, O_RDONLY);
            if (fd2 < 0) {
                printf("Vânătoarea %s nu există.\n", hunt);
                continue;
            }
            Treasure tr;
            int found = 0;
            while (read(fd2, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
                if (tr.ID == tid) {
                    found = 1;
                    printf("ID: %d, User: %s, GPS: %.2f, %.2f, Clue: %s, Value: %d\n",
                           tr.ID, tr.username, tr.latitude, tr.longitude, tr.clue, tr.value);
                    break;
                }
            }
            if (!found)
                printf("Comoara %d nu a fost găsită în %s.\n", tid, hunt);
            close(fd2);

        } else if (strncmp(cmd, "stop_monitor", 12) == 0) {
            printf("[monitor] Se închide...\n");
            usleep(1000000);
            exit(0);
        }
    }
}

int main() {
    char input[MAX_CMD];

    while (1) {
        printf(">>> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_running) {
                printf("Monitorul este deja pornit.\n");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                run_monitor();
                exit(0);
            } else if (pid > 0) {
                monitor_pid = pid;
                monitor_running = 1;
                printf("Monitor pornit (PID: %d)\n", monitor_pid);
            } else {
                perror("fork");
            }

        } else if (strncmp(input, "list_hunts", 10) == 0 ||
                   strncmp(input, "list_treasures", 14) == 0 ||
                   strncmp(input, "view_treasure", 13) == 0) {

            if (!monitor_running || monitor_pid <= 0) {
                printf("Monitorul nu este activ. Folosește 'start_monitor' mai întâi.\n");
                continue;
            }

            write_command_to_file(input);
            kill(monitor_pid, SIGUSR1);
            usleep(300000);

        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_running) {
                printf("Monitorul nu este activ.\n");
                continue;
            }

            write_command_to_file("stop_monitor");
            kill(monitor_pid, SIGUSR1);
            waitpid(monitor_pid, NULL, 0);
            monitor_running = 0;
            printf("[monitor] Procesul s-a încheiat.\n");

        } else if (strcmp(input, "exit") == 0) {
            if (monitor_running) {
                printf("Nu poți ieși până nu oprești monitorul.\n");
                continue;
            }
            break;

        } else {
            printf("Comandă necunoscută.\n");
        }
    }

    return 0;
}
