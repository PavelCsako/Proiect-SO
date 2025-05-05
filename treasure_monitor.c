#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#define CMD_FILE "cmd.txt"
#define ARG_FILE "args.txt"

volatile sig_atomic_t command_received = 0;

void handle_sigusr1(int sig) {
    command_received = 1;
}

void execute_command() {
    char cmd[100];
    FILE *f = fopen(CMD_FILE, "r");
    if (!f) {
        perror("fopen CMD_FILE");
        return;
    }
    fgets(cmd, sizeof(cmd), f);
    fclose(f);

    cmd[strcspn(cmd, "\n")] = 0;

    if (strcmp(cmd, "list_hunts") == 0) {
        system("./treasure_manager --comanda list");
    } else if (strcmp(cmd, "list_treasures") == 0) {
        char args[100];
        FILE *fa = fopen(ARG_FILE, "r");
        if (!fa) {
            perror("fopen ARG_FILE");
            return;
        }
        fgets(args, sizeof(args), fa);
        fclose(fa);
        args[strcspn(args, "\n")] = 0;

        char command[256];
        snprintf(command, sizeof(command),
                 "./treasure_manager --comanda list_treasures --hunt_id %s", args);
        system(command);
    } else if (strcmp(cmd, "view_treasure") == 0) {
        char args[100];
        FILE *fa = fopen(ARG_FILE, "r");
        if (!fa) {
            perror("fopen ARG_FILE");
            return;
        }
        fgets(args, sizeof(args), fa);
        fclose(fa);
        args[strcspn(args, "\n")] = 0;

        char hunt_id[50];
        int treasure_id;
        sscanf(args, "%s %d", hunt_id, &treasure_id);

        char command[256];
        snprintf(command, sizeof(command),
                 "./treasure_manager --comanda view_treasure --hunt_id %s --treasure_id %d",
                 hunt_id, treasure_id);
        system(command);
    } else if (strcmp(cmd, "stop_monitor") == 0) {
        printf("Monitorul se oprește...\n");
        usleep(2000000);
        exit(0);
    } else {
        printf("Comandă necunoscută: %s\n", cmd);
    }
}

int main() {
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("Monitorul a pornit. Așteaptă comenzi...\n");

    while (1) {
        pause(); // Așteaptă semnal
        if (command_received) {
            execute_command();
            command_received = 0;
        }
    }

    return 0;
}
