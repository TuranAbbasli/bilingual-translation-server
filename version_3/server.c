#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/shm.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_WORDS 1000
#define DICTIONARY_PATH "./dictionary"
#define SHM_KEY 1234
#define PIPE_NAME "/tmp/word_response_pipe"

typedef struct {
    char english[50];
    char french[50];
} WordPair;

WordPair *dictionary;
int word_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Function to load dictionary into shared memory
void load_dictionary() {
    DIR *dir;
    struct dirent *entry;
    FILE *file;
    char filepath[512], line[100];

    // Reset word count
    word_count = 0;

    if ((dir = opendir(DICTIONARY_PATH)) != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue; // Skip hidden files
            snprintf(filepath, sizeof(filepath), "%s/%s", DICTIONARY_PATH, entry->d_name);

            file = fopen(filepath, "r");
            if (file) {
                while (fgets(line, sizeof(line), file)) {
                    sscanf(line, "%49[^;];%49s", dictionary[word_count].english, dictionary[word_count].french);
                    word_count++;
                    if (word_count >= MAX_WORDS) break;
                }
                fclose(file);
            }
        }
        closedir(dir);
    }
    printf("Count of words in the source folder: %d\n", word_count);
}

// Function to send response to client via pipe
void send_response(const char *response) {
    int pipe_fd = open(PIPE_NAME, O_WRONLY);
    if (pipe_fd < 0) {
        perror("Failed to open pipe for response");
        return;
    }
    write(pipe_fd, response, strlen(response) + 1);
    close(pipe_fd);
}

// Function to translate a word based on the signal
void translate(const char *word, int signal) {
    char response[256];

    pthread_mutex_lock(&mutex);
    printf("\nRequest recieved!\n");
    printf("Translate: %s\n", word);
    // If the word found in the shared memory
    for (int i = 0; i < word_count; i++) {
        if (signal == SIGUSR1 && strcmp(dictionary[i].english, word) == 0) {
            snprintf(response, sizeof(response), "Translation: %s -> %s", word, dictionary[i].french);
            send_response(response);
            printf("Response sent: %s\n", dictionary[i].french);
            pthread_mutex_unlock(&mutex);
            return;
        }
        if (signal == SIGUSR2 && strcmp(dictionary[i].french, word) == 0) {
            snprintf(response, sizeof(response), "Translation: %s -> %s", word, dictionary[i].english);
            send_response(response);
            printf("Response sent: %s\n", dictionary[i].english);
            pthread_mutex_unlock(&mutex);
            return;
        }
    }
    // If word not found in the shared memory
    snprintf(response, sizeof(response), "Word '%s' not found. Reloading dictionary...\n", word);
    printf("The word not found! Updating shared memory...\n");
    send_response(response);

    load_dictionary();
    pthread_mutex_unlock(&mutex);
}

// Client request handler
void *client_handler(void *arg) {
    int pipe_fd;
    char buffer[100];
    int signal;

    mkfifo("/tmp/word_pipe", 0666);
    mkfifo(PIPE_NAME, 0666);  // Named pipe for sending response back to client

    while (1) {
        pipe_fd = open("/tmp/word_pipe", O_RDONLY);
        read(pipe_fd, buffer, sizeof(buffer));
        sscanf(buffer, "%d %s", &signal, buffer);
        translate(buffer, signal);
        close(pipe_fd);
    }
}

// File monitor to detect new dictionary files
void *file_monitor(void *arg) {
    struct stat dir_stat;
    time_t last_mtime = 0;

    while (1) {
        // Check every 5 seconds
        stat(DICTIONARY_PATH, &dir_stat);
        if (dir_stat.st_mtime != last_mtime) {
            last_mtime = dir_stat.st_mtime;
            printf("Source folder has been updated. Reloading dictionary...\n");

            pthread_mutex_lock(&mutex);
            load_dictionary();
            printf("Dictionary reload finished.\n");
            pthread_mutex_unlock(&mutex);

        }
        sleep(10);
    }
}

int main() {
    int shm_id;
    pthread_t client_thread, monitor_thread;

    shm_id = shmget(SHM_KEY, MAX_WORDS * sizeof(WordPair), IPC_CREAT | 0666);
    dictionary = (WordPair *)shmat(shm_id, NULL, 0);

    printf("Server started. Waiting for client requests...\n");

    pthread_create(&client_thread, NULL, client_handler, NULL);
    pthread_create(&monitor_thread, NULL, file_monitor, NULL);

    pthread_join(client_thread, NULL);
    pthread_join(monitor_thread, NULL);

    shmdt(dictionary);
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}
