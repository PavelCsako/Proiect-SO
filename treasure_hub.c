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

pid_t monitor_pid = -1; //PID-ul procesului monitor
int monitor_running = 0; //flag care spune dacă monitorul e activ
volatile sig_atomic_t got_signal = 0; // folosit de sigaction pentru a semnala că s-a primit SIGUSR1

void handle_sigusr1(int sig) {
  got_signal = 1;
}
//Când monitorul primește SIGUSR1, își dă seama că trebuie să verifice fișierul monitor_cmd.txt
//Este handler-ul pentru semnalul SIGUSR1

void write_command_to_file(const char *cmd) { //aici scriu comanda intr-un fișier temporar pt a fi citita de monitor
  int fd = open(TEMP_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd >= 0) {
    write(fd, cmd, strlen(cmd));
    close(fd);
  }
}

// Procesul monitor – execută comenzi primite prin semnal
void run_monitor() {
  struct sigaction sa;
  sa.sa_handler = handle_sigusr1;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);//aici setez handler-ul

  while (1) {
    while (!got_signal) pause();// aici astept un semnal
    got_signal = 0; //Intră într-o buclă infinită și așteaptă comenzi. pause() oprește execuția până primste un semnal

    char cmd[MAX_CMD] = {0};
    int fd = open(TEMP_FILE, O_RDONLY);
    if (fd >= 0) {
      read(fd, cmd, sizeof(cmd));
      close(fd);
    } //După ce primește semnalul, citește comanda din fișier si o executa
    if (strncmp(cmd, "list_treasures", 14) == 0) {
      char hunt[100];
      sscanf(cmd, "list_treasures %s", hunt);
      pid_t pid = fork();
      if (pid == 0) {
	execlp("./treasure_manager", "treasure_manager", "--list", hunt, NULL);
	perror("exec list_treasures");
	exit(1);
      } else {
	waitpid(pid, NULL, 0);
      }

    } else if (strncmp(cmd, "view_treasure", 13) == 0) {
      char hunt[100], id[10];
      sscanf(cmd, "view_treasure %s %s", hunt, id);
      pid_t pid = fork();
      if (pid == 0) {
	execlp("./treasure_manager", "treasure_manager", "--view", hunt, id, NULL);
	perror("exec view_treasure");
	exit(1);
      } else {
	waitpid(pid, NULL, 0);
      }

    } else if (strncmp(cmd, "list_hunts", 10) == 0) {
      DIR *d = opendir(".");
      if (!d) {
	perror("opendir");
	exit(1);
      }
      struct dirent *entry;
      while ((entry = readdir(d)) != NULL) {
	if (entry->d_type == DT_DIR && strncmp(entry->d_name, "Hunt", 4) == 0) {
	  printf(">>> %s\n", entry->d_name);
	  pid_t pid = fork();
	  if (pid == 0) {
	    execlp("./treasure_manager", "treasure_manager", "--list", entry->d_name, NULL);
	    perror("exec list_hunt dir");
	    exit(1);
	  } else {
	    waitpid(pid, NULL, 0);
	  }
	}
      }
      closedir(d);

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
    input[strcspn(input, "\n")] = 0; // citeste comanda ce o dau

    if (strcmp(input, "start_monitor") == 0) {
      if (monitor_running) {
	printf("Monitorul este deja pornit.\n");
	continue;
      }

      pid_t pid = fork(); 
      if (pid == 0) {
	run_monitor(); //copilu
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
      // Scrie comanda în fișier și trimite semnalul pentru a o activa
      usleep(300000);

    } else if (strcmp(input, "calculate_score") == 0) {
      DIR *d = opendir("."); // deschide directorul curent
      if (!d) {
        perror("opendir");
        continue;
      }

      struct dirent *entry;
      while ((entry = readdir(d)) != NULL) { // parcurge toate fisierele
        if (entry->d_type == DT_DIR && strncmp(entry->d_name, "Hunt", 4) == 0) {
	  int pipefd[2]; // pt fiecare director valid creeaza un pipe
	  if (pipe(pipefd) == -1) {
	    perror("pipe");
	    continue;
	  }

	  pid_t pid = fork(); //face fork
	  if (pid == -1) {
	    perror("fork");
	    continue;
	  }

	  if (pid == 0) {
	    // copil: redirectează stdout în pipe și execută score_calc
	    close(pipefd[0]); // închide capătul de citire al pipe uluui
	    dup2(pipefd[1], STDOUT_FILENO); //redirectioneaza stdout catre pipe
	    close(pipefd[1]); // pipefd[1] = scriere
	    execlp("./score_calc", "score_calc", entry->d_name, NULL); //înlocuiește procesul curent cu un program extern si ruleazaa score_calc
	    perror("exec score_calc"); // doar dacă eșuează
	    exit(1);
	  } else {
	    // părinte: citește din pipe
	    close(pipefd[1]); // închide capătul de scriere

	    char buffer[512];
	    int n;
	    printf("Scoruri pentru %s:\n", entry->d_name);
	    while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) { //citeste outputul din pipe
	      buffer[n] = '\0';
	      printf("%s", buffer); // afiseaza scorurile
	    }
	    close(pipefd[0]);// pipefd[0] = citire

	    waitpid(pid, NULL, 0); // așteaptă copilul
	    printf("\n");
	  }
        }
      }

      closedir(d);
    }
    else if (strcmp(input, "stop_monitor") == 0) {
      if (!monitor_running) {
	printf("Monitorul nu este activ.\n");
	continue;
      }

      write_command_to_file("stop_monitor");
      kill(monitor_pid, SIGUSR1);
      waitpid(monitor_pid, NULL, 0);
      monitor_running = 0;
      // Trimite comanda de oprire și așteaptă să moară monitorul.
      printf("[monitor] Procesul s-a încheiat.\n");

    } else if (strcmp(input, "exit") == 0) {
      if (monitor_running) {
	printf("Nu poți ieși până nu oprești monitorul.\n");
	continue;
      }
      break;
      // Nu permite ieșirea până ce procesul monitor a fost oprit corect
    } else {
      printf("Comandă necunoscută.\n");
    }
  }

  return 0;
}
