#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

#define UMAX 50
#define CMAX 100
#define PATHMAX 256

typedef struct {
  int ID;
  char username[UMAX];
  float latitude;
  float longitude;
  char clue[CMAX];
  int value;
} Treasure;

void log_action(const char *hunt_id, const char *action) {
    char log_path[PATHMAX];
    snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);

    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("Eroare la deschiderea fișierului de log");
        return;
    }

    time_t now = time(NULL);
    char time_str[64];
    snprintf(time_str, sizeof(time_str), "%s", ctime(&now));
    time_str[strlen(time_str) - 1] = '\0'; 

    dprintf(log_fd, "[%s] %s\n", time_str, action);
    close(log_fd);

    char symlink_name[PATHMAX];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    if (access(symlink_name, F_OK) == -1) {
        symlink(log_path, symlink_name);
    }
}

void add_treasure(const char *hunt_id)
{  
  mkdir(hunt_id, 0777);

  char file_path[PATHMAX];
  snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
  
  int fd_check = open(file_path, O_RDONLY);
  int id_to_check;

  if (fd_check != -1) {
        Treasure existing;
        printf("ID: "); scanf("%d", &id_to_check);

        while (read(fd_check, &existing, sizeof(Treasure)) == sizeof(Treasure)) {
            if (existing.ID == id_to_check) {
                printf("Eroare: ID-ul %d există deja în această vânătoare!\n", id_to_check);
                close(fd_check);
                return;
            }
        }
        close(fd_check);
    } else {
        printf("ID: "); scanf("%d", &id_to_check);
    }
  
  Treasure tr;
  tr.ID = id_to_check;
  printf("Enter username: "); scanf("%s", tr.username);
  printf("Enter latitude: "); scanf("%f", &tr.latitude);
  printf("Enter longitude: "); scanf("%f", &tr.longitude);
  printf("Enter clue: "); scanf("%s", tr.clue);
  printf("Enter value: "); scanf("%d", &tr.value);

  int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd == -1) {
    perror("Erorr opening file");
    return;
  }
  
  write(fd, &tr, sizeof(Treasure));
  close(fd);

  printf("Treasure added successfully in %s hunt!\n", hunt_id);

  char log_msg[200];
  snprintf(log_msg, sizeof(log_msg), "Adăugat comoara ID %d", tr.ID);
  log_action(hunt_id, log_msg);
}

void list_treasures(const char *hunt_id) {

  char file_path[PATHMAX];
  snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

  struct stat st;
  if (stat(file_path, &st) == -1) {
    perror("Nu se poate accesa fișierul de comori");
    return;
  }

  printf("Hunt: %s\n", hunt_id);
  printf("Dimensiune fișier: %ld bytes\n", st.st_size);
  printf("Ultima modificare: %s", ctime(&st.st_mtime));

  int fd = open(file_path, O_RDONLY);
  if (fd < 0) {
    perror("Erorr opening file");
    return;
  }
  
  Treasure tr;
  while (read(fd, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
    printf("ID: %d, User: %s, GPS: (%.2f, %.2f), Clue: %s, Value: %d\n",
	   tr.ID, tr.username, tr.latitude,
	   tr.longitude, tr.clue, tr.value);
  }
  close(fd);

  log_action(hunt_id, "Listare comori");
}

void view_treasure(const char *hunt_id, int treasure_id) {
    char file_path[PATHMAX];
    snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Nu se poate deschide fișierul");
        return;
    }

    Treasure tr;
    int found = 0;
    while (read(fd, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
        if (tr.ID == treasure_id) {
            printf("=== Comoară găsită ===\n");
            printf("ID: %d\nUser: %s\nCoordonate: %.2f, %.2f\nClue: %s\nValoare: %d\n",
                   tr.ID, tr.username, tr.latitude, tr.longitude, tr.clue, tr.value);
            found = 1;
            break;
        }
    }
    close(fd);

    if (!found) {
        printf("Comoara cu ID %d nu a fost găsită.\n", treasure_id);
    } else {
        char log_msg[200];
        snprintf(log_msg, sizeof(log_msg), "Vizualizat comoara ID %d", treasure_id);
        log_action(hunt_id, log_msg);
    }
}

void remove_treasure(const char *hunt_id, int treasure_id) {
  char file_path[PATHMAX];
  snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
  char temp_path[PATHMAX] = "temp.dat";
  
  int fd = open(file_path, O_RDONLY);
  if (fd == -1) {
    perror("Eroare la deschiderea fișierului");
    return;
  }

  int temp_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (temp_fd == -1) {
        perror("Eroare la fișierul temporar");
        close(fd);
        return;
    }
    
  Treasure temp;
  FILE *tmp_file = fopen("temp.dat", "wb");
  if (!tmp_file) {
    perror("Eroare la crearea fișierului temporar");
    close(fd);
    return;
  }

  int found = 0;
  while (read(fd, &temp, sizeof(Treasure)) == sizeof(Treasure)) {
    if (temp.ID == treasure_id) {
      found = 1;
      continue;
    }
    write(temp_fd, &temp, sizeof(Treasure));
  }
  
  close(fd);
  fclose(tmp_file);

  if (found) {
    rename(temp_path, file_path);
    printf("Comoara cu ID %d a fost ștearsă.\n", treasure_id);

    char log_msg[200];
    snprintf(log_msg, sizeof(log_msg), "Șters comoara ID %d", treasure_id);
    log_action(hunt_id, log_msg);
  } else {
    printf("Comoara nu a fost găsită.\n");
    remove(temp_path);
  }
}

void remove_hunt(const char *hunt_id) {
  char file_path[PATHMAX], log_path[PATHMAX], symlink_path[PATHMAX];
  snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
  snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);
  snprintf(symlink_path, sizeof(symlink_path), "logged_hunt-%s", hunt_id);

  remove(file_path);
  remove(log_path);
  remove(symlink_path);
  rmdir(hunt_id);

  printf("Vânătoarea %s a fost ștearsă.\n", hunt_id);
}
 
void list_all_hunts() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Nu pot deschide directorul curent");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0) {

            char filepath[512];
            snprintf(filepath, sizeof(filepath), "%s/treasures.dat", entry->d_name);

            FILE *f = fopen(filepath, "rb");
            if (!f) continue;

            int count = 0;
            Treasure tr;
            while (fread(&tr, sizeof(Treasure), 1, f) == 1) {
                count++;
            }
            fclose(f);

            printf("Hunt: %s - %d comori\n", entry->d_name, count);
        }
    }
    closedir(dir);
}
 
int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("Utilizare: %s --comanda <hunt_id> [opțiuni]\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "--add") == 0) {
    add_treasure(argv[2]);
  } else if (strcmp(argv[1], "--list") == 0) {
    list_treasures(argv[2]);
  } else if (strcmp(argv[1], "--view") == 0) {
    if (argc < 4) {
      printf("Specificați ID-ul comorii!\n");
      return 1;
    }
    view_treasure(argv[2], atoi(argv[3]));
  } else if (strcmp(argv[1], "--remove_treasure") == 0) {
    if (argc < 4) {
      printf("Specificați ID-ul comorii!\n");
      return 1;
    }
    remove_treasure(argv[2], atoi(argv[3]));
  } else if (strcmp(argv[1], "--remove_hunt") == 0) {
    remove_hunt(argv[2]);
  } else if (strcmp(argv[1], "--list-hunts") == 0) {
    list_all_hunts();
  }else {
    printf("Comandă necunoscută.\n");
  }

  return 0;
}
