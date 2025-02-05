#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_WORDS 1000
#define FOLDER_PATH "./dictionary"

#define SLEEP_INTERVAL 10

typedef struct {
    char english[50];
    char french[50];
} WordPair;

WordPair dictionary[MAX_WORDS];
int word_count = 0;

// Function to load values in all files in the given folder
void load_dictionary(const char *folder) {
    struct dirent *entry;
    DIR *dp = opendir(folder);

    // verify the folder
    if (!dp) {
        perror("Failed to open dictionary folder");
        printf("Check existance of the folder: %s\n", FOLDER_PATH);
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dp)) != NULL) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", folder, entry->d_name);

        if (entry->d_type == DT_REG) {
            FILE *file = fopen(filepath, "r");
            if (!file) continue;

            char line[256];
            while (fgets(line, sizeof(line), file) && word_count < MAX_WORDS) {
                char *english = strtok(line, ";");
                char *french = strtok(NULL, "\n");
                if (english && french) {
                    strncpy(dictionary[word_count].english, english, 50);
                    strncpy(dictionary[word_count].french, french, 50);
                    word_count++;
                }
            }
            fclose(file);
        }
    }
    closedir(dp);
}

void handle_signal(int signal) {
    if (word_count == 0) {
        printf("No words in dictionary!\n");
        return;
    }

    int idx = rand() % word_count;
    if (signal == SIGUSR1) {
        printf("English: %s -> French: %s\n", dictionary[idx].english, dictionary[idx].french);
    } else if (signal == SIGUSR2) {
        printf("French: %s -> English: %s\n", dictionary[idx].french, dictionary[idx].english);
    }
}

void monitor_folder(const char *folder) {
    time_t last_mtime = 0;

    while (1) {
        struct stat dir_stat;
        if (stat(folder, &dir_stat) == -1) {
            perror("stat");
            return;
        }

        // Check if the modification time has changed
        if (dir_stat.st_mtime != last_mtime) {
            printf("\n\nFolder modification detected!\n");
            load_dictionary(folder);
            printf("Dictionary updated!!\n\n");
            last_mtime = dir_stat.st_mtime; // Update the last modification time
        }

        sleep(SLEEP_INTERVAL);
    }
}

int main() {
    srand(time(NULL));
    load_dictionary(FOLDER_PATH);

    signal(SIGUSR1, handle_signal);
    signal(SIGUSR2, handle_signal);

    printf("Server is running. PID: %d", getpid());
    while (1) {
        monitor_folder(FOLDER_PATH);
        pause(); // Wait for signals
    }

    return 0;
}