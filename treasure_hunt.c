#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<errno.h>

#define UMAX 50
#define CMAX 100

typedef struct {
  int ID;
  char username[UMAX];
  float latitude;
  float longitude;
  char clue[CMAX];
  int value;
} Treasure;

void add_treasure(const char *hunt_id)
{  
  char dir_path[100];
  snprintf(dir_path, sizeof(dir_path), "%s", hunt_id);

  mkdir(dir_path, 0777);

  char file_path[150];
  snprintf(file_path, sizeof(file_path), "%s/treasures.dat", dir_path);

  
  int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
  if (fd == -1) {
    perror("Erorr opening file");
    return;
  }
  
  Treasure tr;
  printf("Enter treasure id: "); scanf("%d", &tr.ID);
  printf("Enter username: "); scanf("%s", tr.username);
  printf("Enter latitude: "); scanf("%f", &tr.latitude);
  printf("Enter longitude: "); scanf("%f", &tr.longitude);
  printf("Enter clue: "); scanf("%s", tr.clue);
  printf("Enter value: "); scanf("%d", &tr.value);
  
  write(fd, &tr, sizeof(Treasure));
  close(fd);

  printf("Treasure added successfully in %s hunt!\n", hunt_id);
}

void list_treasures(const char *hunt_id) {

  char file_path[150];
  snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

  int fd = open(file_path, O_RDONLY);
  if (fd < 0) {
    perror("Erorr opening file");
    return;
  }

  struct stat st;
  if (stat(file_path, &st) == 0) {
    printf("Hunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size);
    //printf("Last modified: %d\n", ctime(&st.st_mtime));
  }

  Treasure tr;
  while (read(fd, &tr, sizeof(Treasure)) == sizeof(Treasure)) {
    printf("ID: %d, User: %s, GPS: (%.2f, %.2f), Clue: %s, Value: %d\n",
	   tr.ID, tr.username, tr.latitude,
	   tr.longitude, tr.clue, tr.value);
  }
  close(fd);
  
}

void remove_treasure(const char *hunt_id, int treasure_id) {
  char file_path[150];
  snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);

  int fd = open(file_path, O_RDONLY);
  if (fd == -1) {
    perror("Eroare la deschiderea fișierului");
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
    if (temp.ID != treasure_id) {
      fwrite(&temp, sizeof(Treasure), 1, tmp_file);
    } else {
      found = 1;
    }
  }
  close(fd);
  fclose(tmp_file);

  if (!found) {
    printf("Comoara cu ID-ul %d nu a fost găsită.\n", treasure_id);
    remove("temp.dat");
    return;
  }

  rename("temp.dat", file_path);
  printf("Comoara cu ID-ul %d a fost ștearsă.\n", treasure_id);
}

void remove_hunt(const char *hunt_id) {
  char file_path[150];
  snprintf(file_path, sizeof(file_path), "%s/treasures.dat", hunt_id);
  char log_path[150];
  snprintf(log_path, sizeof(log_path), "%s/logged_hunt", hunt_id);

  remove(file_path);
  remove(log_path);
  rmdir(hunt_id);

  printf("Vânătoarea %s a fost ștearsă.\n", hunt_id);
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
  } else if (strcmp(argv[1], "--remove_treasure") == 0) {
    if (argc < 4) {
      printf("Specificați ID-ul comorii de șters!\n");
      return 1;
    }
    remove_treasure(argv[2], atoi(argv[3]));
  } else if (strcmp(argv[1], "--remove_hunt") == 0) {
    remove_hunt(argv[2]);
  } else {
    printf("Comandă necunoscută!\n");
  }

  return 0;
}
