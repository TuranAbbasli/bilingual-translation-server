#include "decleration.h"

void *translation_reader_thread(void *arg) {
    int msgid = *(int *)arg;

    // Attach shared memory
    int shmid = shmget(SHM_KEY, sizeof(WordPair) * MAX_WORDS, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Failed to create shared memory");
        exit(EXIT_FAILURE);
    }
    WordPair *shared_dict = shmat(shmid, NULL, 0);
    if (shared_dict == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(EXIT_FAILURE);
    }

    int shared_word_count = 0;

    while (1) {
        Message msg;
        if (msgrcv(msgid, &msg, sizeof(Message) - sizeof(long), 0, 0) != -1) {
            if (shared_word_count < MAX_WORDS) {
                strncpy(shared_dict[shared_word_count].english, msg.english, 50);
                strncpy(shared_dict[shared_word_count].french, msg.french, 50);
                shared_word_count++;
                printf("Stored: %s -> %s\n", msg.english, msg.french);
            } else {
                printf("Shared dictionary is full!\n");
            }
        }
    }

    shmdt(shared_dict);
    return NULL;
}

int main() {
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("Failed to create message queue");
        exit(EXIT_FAILURE);
    }

    pthread_t reader_thread;
    pthread_create(&reader_thread, NULL, translation_reader_thread, &msgid);

    printf("Reader running and processing translations...\n");
    pthread_join(reader_thread, NULL);

    return 0;
}
