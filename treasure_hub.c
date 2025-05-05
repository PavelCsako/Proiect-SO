#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

#define CMD_FILE "cmd.txt"
#define ARG_FILE "args.txt"

pid_t monitor_pid = -1;
int monitor_running = 0;

void sigchld_handler(int sig) {
    int status;
    pid_t pid = waitpid(monitor_pid, &status, WNOHANG);
    if (pid > 0) {
        printf("Monitorul s-a încheiat cu status %d\n", status);
        monitor_running = 0;
        monitor_pid = -1;
    }
}

void setup_sigchld_handler() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

void send_command(const char *cmd, const char *args) {
    int fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open CMD_FILE");
        return;
    }
    write(fd, cmd, strlen(cmd));
    close(fd);

    if (args) {
        fd = open(ARG_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            perror("open ARG_FILE");
            return;
        }
        write(fd, args, strlen(args));
        close(fd);
    }

    if (kill(monitor_pid, SIGUSR1) == -1) {
        perror("kill");
    }
}

int main() {
    char input[256];
    setup_sigchld_handler();

    while (1) {
        printf(">> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        input[strcspn(input, "\n")] = 0; // Elimină newline

        if (strncmp(input, "start_monitor", 13) == 0) {
            if (monitor_running) {
                printf("Monitorul este deja pornit.\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == -1) {
                perror("fork");
                continue;
            }

            if (monitor_pid == 0) {
                execl("./treasure_monitor", "treasure_monitor", NULL);
                perror("execl");
                exit(EXIT_FAILURE);
            } else {
                monitor_running = 1;
                printf("Monitorul a fost pornit cu PID %d.\n", monitor_pid);
            }
        } else if (strncmp(input, "list_hunts", 10) == 0) {
            if (!monitor_running) {
                printf("Monitorul nu este pornit.\n");
                continue;
            }
            send_command("list_hunts", NULL);
        } else if (strncmp(input, "list_treasures", 14) == 0) {
            if (!monitor_running) {
                printf("Monitorul nu este pornit.\n");
                continue;
            }
            char *args = input + 15;
            send_command("list_treasures", args);
        } else if (strncmp(input, "view_treasure", 13) == 0) {
            if (!monitor_running) {
                printf("Monitorul nu este pornit.\n");
                continue;
            }
            char *args = input + 14;
            send_command("view_treasure", args);
        } else if (strncmp(input, "stop_monitor", 12) == 0) {
            if (!monitor_running) {
                printf("Monitorul nu este pornit.\n");
                continue;
            }
            send_command("stop_monitor", NULL);
        } else if (strncmp(input, "exit", 4) == 0) {
            if (monitor_running) {
                printf("Monitorul este încă activ. Opriți-l înainte de a ieși.\n");
                continue;
            }
            printf("Ieșire din program.\n");
            break;
        } else {
            printf("Comandă necunoscută.\n");
        }
    }

    return 0;
}

