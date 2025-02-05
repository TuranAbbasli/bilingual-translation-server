#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

int main(int argc, char *argv[]) {
    // Check input
    if (argc != 2) {
        printf("Usage: %s <value>\n", argv[0]);
        return 1;
    }

    int server_pid = atoi(argv[1]);

    srand(time(NULL));

    for (int i = 0; i < 100; i++) {
        int signal = (rand() % 2) ? SIGUSR1 : SIGUSR2;
        if (kill(server_pid, signal) == -1) {
            perror("Failed to send signal");
            return EXIT_FAILURE;
        }
        printf("Sent signal: %s\n", (signal == SIGUSR1) ? "SIGUSR1" : "SIGUSR2");
        usleep(100000);
    }

    return 0;
}