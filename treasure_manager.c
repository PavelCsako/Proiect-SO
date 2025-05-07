#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>

#define USERNAME_LEN 50
#define CLUE_LEN 100
#define PATH_LEN 256

typedef struct {
    int ID;
    char username[USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[CLUE_LEN];
    int value;
} Treasure;

void log_action(const char *hunt_id, const char *action) {
    char log_path[PATH_LEN];
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);

    int fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Eroare la deschiderea fișierului de log");
        return;
    }

    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // eliminăm newline

    dprintf(fd, "[%s] %s\n", timestamp, action);
    close(fd);

    char symlink_path[PATH_LEN];
    snprintf(symlink_path, sizeof(symlink_path), "logged_hunt-%s", hunt_id);
    if (access(symlink_path, F_OK) == -1) {
        symlink(log_path, symlink_path);
    }
}

void add_treasure(const char *hunt_id) {
    mkdir(hunt_id, 0777);

    char file_path[PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    Treasure tr;
    int id_check, exists = 0;

    printf("ID: ");
    scanf("%d", &id_check);

    int fd = open(file_path, O_RDONLY);
    if (fd != -1) {
        Treasure temp;
        while (read(fd, &temp, sizeof(Treasure)) == sizeof(Treasure)) {
            if (temp.ID == id_check) {
                exists = 1;
                break;
            }
        }
        close(fd);
    }

    if (exists) {
        printf("Eroare: ID-ul %d există deja.\n", id_check);
        return;
    }

    tr.ID = id_check;
    printf("Username: "); scanf("%s", tr.username);
    printf("Latitude: "); scanf("%f", &tr.latitude);
    printf("Longitude: "); scanf("%f", &tr.longitude);

    getchar();  
    printf("Clue: ");
    fgets(tr.clue, CLUE_LEN, stdin);
    size_t len = strlen(tr.clue);
    if (len > 0 && tr.clue[len - 1] == '\n') {
        tr.clue[len - 1] = '\0';
    }

    printf("Value: "); scanf("%d", &tr.value);

    fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("Eroare la scriere");
        return;
    }

    write(fd, &tr, sizeof(Treasure));
    close(fd);

    printf("Comoară adăugată cu succes în %s!\n", hunt_id);

    char msg[100];
    snprintf(msg, sizeof(msg), "Adăugare comoară ID %d", tr.ID);
    log_action(hunt_id, msg);
}

void list_treasures(const char *hunt_id) {
    char file_path[PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    struct stat st;
    if (stat(file_path, &st) == -1) {
        perror("Fișier inexistent");
        return;
    }

    printf("Hunt: %s\nDimensiune: %ld bytes\nUltima modificare: %s", hunt_id, st.st_size, ctime(&st.st_mtime));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschidere");
        return;
    }

    Treasure tr;
    while (read(fd, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
        printf("ID: %d, User: %s, GPS: %.2f, %.2f, Clue: %s, Value: %d\n",
               tr.ID, tr.username, tr.latitude, tr.longitude, tr.clue, tr.value);
    }

    close(fd);
    log_action(hunt_id, "Listare comori");
}

void view_treasure(const char *hunt_id, int treasure_id) {
    char file_path[PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschidere");
        return;
    }

    Treasure tr;
    int found = 0;
    while (read(fd, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
        if (tr.ID == treasure_id) {
            found = 1;
            printf("=== Comoară găsită ===\n");
            printf("ID: %d\nUser: %s\nGPS: %.2f, %.2f\nClue: %s\nValoare: %d\n",
                   tr.ID, tr.username, tr.latitude, tr.longitude, tr.clue, tr.value);
            break;
        }
    }

    close(fd);

    if (!found) {
        printf("Comoara cu ID %d nu a fost găsită.\n", treasure_id);
    } else {
        char msg[100];
        snprintf(msg, sizeof(msg), "Vizualizat comoara ID %d", treasure_id);
        log_action(hunt_id, msg);
    }
}

void remove_treasure(const char *hunt_id, int treasure_id) {
    char file_path[PATH_LEN];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    char temp_path[] = "temp.dat";
    int in_fd = open(file_path, O_RDONLY);
    int out_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (in_fd == -1 || out_fd == -1) {
        perror("Eroare la fișiere");
        return;
    }

    Treasure tr;
    int found = 0;
    while (read(in_fd, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
        if (tr.ID == treasure_id) {
            found = 1;
        } else {
            write(out_fd, &tr, sizeof(Treasure));
        }
    }

    close(in_fd);
    close(out_fd);

    if (found) {
        rename(temp_path, file_path);
        printf("Comoara cu ID %d a fost ștearsă.\n", treasure_id);
        char msg[100];
        snprintf(msg, sizeof(msg), "Șters comoara ID %d", treasure_id);
        log_action(hunt_id, msg);
    } else {
        remove(temp_path);
        printf("Comoara nu a fost găsită.\n");
    }
}

void remove_hunt(const char *hunt_id) {
    char path[PATH_LEN];

    snprintf(path, sizeof(path), "%s/treasures.dat", hunt_id);
    remove(path);
    snprintf(path, sizeof(path), "%s/logged_hunt", hunt_id);
    remove(path);
    snprintf(path, sizeof(path), "logged_hunt-%s", hunt_id);
    remove(path);
    rmdir(hunt_id);

    printf("Vânătoarea %s a fost ștearsă.\n", hunt_id);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Utilizare: %s --comandă <hunt_id> [id_comoară]\n", argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    const char *hunt_id = argv[2];

    if (strcmp(cmd, "--add") == 0) {
        add_treasure(hunt_id);
    } else if (strcmp(cmd, "--list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(cmd, "--view") == 0 && argc >= 4) {
        view_treasure(hunt_id, atoi(argv[3]));
    } else if (strcmp(cmd, "--remove_treasure") == 0 && argc >= 4) {
        remove_treasure(hunt_id, atoi(argv[3]));
    } else if (strcmp(cmd, "--remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } else {
        printf("Comandă invalidă sau argumente insuficiente.\n");
    }

    return 0;
}
