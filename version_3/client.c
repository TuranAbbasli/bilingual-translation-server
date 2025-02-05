#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

void send_request(const char *word, int signal) {
    int pipe_fd;
    char buffer[100];

    pipe_fd = open("/tmp/word_pipe", O_WRONLY);
    if (pipe_fd < 0) {
        perror("Failed to connect to server");
        return;
    }

    snprintf(buffer, sizeof(buffer), "%d %s", signal, word);
    write(pipe_fd, buffer, strlen(buffer) + 1);
    close(pipe_fd);
}

void receive_response() {
    int pipe_fd;
    char response[256];

    pipe_fd = open("/tmp/word_response_pipe", O_RDONLY); // Open the response pipe
    if (pipe_fd < 0) {
        perror("Failed to open response pipe");
        return;
    }

    read(pipe_fd, response, sizeof(response)); // Read the response from server
    printf("Server response: %s\n", response); // Print server's response
    close(pipe_fd);
}

int main() {
    char word[50];
    int direction;

    while (1) {
        printf("Enter a word (or type '-1' to quit): ");
        scanf("%49s", word);

        // Exit
        if (strcmp(word, "-1") == 0) {
            break;
        }

        printf("Enter direction (1 for EN->FR, 2 for FR->EN): ");
        scanf("%d", &direction);

        send_request(word, direction == 1 ? SIGUSR1 : SIGUSR2);
        receive_response(); // Get the response from the server
    }

    printf("Client exiting.\n");
    return 0;
}