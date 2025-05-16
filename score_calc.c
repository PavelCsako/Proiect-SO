#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define USERNAME_LEN 50
#define CLUE_LEN 100
#define MAX_USERS 100

typedef struct {
    int ID;
    char username[USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[CLUE_LEN];
    int value;
} Treasure;

typedef struct {
    char username[USERNAME_LEN];
    int total_score;
} UserScore;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "%s/treasures.dat", argv[1]);

    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("Nu se poate deschide fișierul cu comori");
        return 1;
    }

    UserScore users[MAX_USERS];
    int user_count = 0;

    Treasure tr;
    while (read(fd, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i].username, tr.username) == 0) {
                users[i].total_score += tr.value;
                found = 1;
                break;
            }
        }
        if (!found && user_count < MAX_USERS) {
            strncpy(users[user_count].username, tr.username, USERNAME_LEN);
            users[user_count].total_score = tr.value;
            user_count++;
        }
    }

    close(fd);

    for (int i = 0; i < user_count; i++) {
        printf("%s: %d\n", users[i].username, users[i].total_score);
    }

    return 0;
}
