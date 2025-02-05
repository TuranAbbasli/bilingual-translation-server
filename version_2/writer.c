#include "decleration.h"

void load_dictionary_to_queue(int msgid, const char *folder) {
    struct dirent *entry;
    DIR *dp = opendir(folder);
    if (!dp) {
        perror("Failed to open dictionary folder");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dp)) != NULL) {
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", folder, entry->d_name);

        if (entry->d_type == DT_REG) {
            FILE *file = fopen(filepath, "r");
            if (!file) continue;

            char line[256];
            while (fgets(line, sizeof(line), file)) {
                char *english = strtok(line, ";");
                char *french = strtok(NULL, "\n");
                if (english && french) {
                    Message msg;
                    msg.mtype = 1; // Message type
                    strncpy(msg.english, english, 50);
                    strncpy(msg.french, french, 50);
                    msg.direction = 1; // English-to-French
                    msgsnd(msgid, &msg, sizeof(Message) - sizeof(long), 0);

                    msg.mtype = 2; // Message type for French-to-English
                    strncpy(msg.english, french, 50);
                    strncpy(msg.french, english, 50);
                    msg.direction = 2; // French-to-English
                    msgsnd(msgid, &msg, sizeof(Message) - sizeof(long), 0);

                    printf("Sent: %s -> %s\n", english, french);
                }
            }
            fclose(file);
        }
    }
    closedir(dp);
}


// Monitor folder to check for updates
void *folder_monitor_thread(void *arg) {
    int msgid = *(int *)arg;
    time_t last_mtime = 0;

    while (1) {
        struct stat folder_stat;
        if (stat(FOLDER_PATH, &folder_stat) == -1) {
            perror("stat");
            exit(EXIT_FAILURE);
        }

        // Check if the folder's modification time has changed
        if (folder_stat.st_mtime != last_mtime) {
            printf("\nFolder modification detected!\n");
            load_dictionary_to_queue(msgid, FOLDER_PATH);
            printf("Dictionary loaded into queue!\n");
            last_mtime = folder_stat.st_mtime; // Update the last modification time
        }

        sleep(5); // Sleep interval for periodic checks
    }
    return NULL;
}

int main() {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Failed to create message queue");
        exit(EXIT_FAILURE);
    }

    pthread_t monitor_thread;
    pthread_create(&monitor_thread, NULL, folder_monitor_thread, &msgid);

    printf("Writer running and monitoring folder: %s\n", FOLDER_PATH);
    pthread_join(monitor_thread, NULL);

    return 0;
}