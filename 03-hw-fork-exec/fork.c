#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t pid;
    int fd;

    // Open fork-output.txt for writing
    fd = open("fork-output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open failed");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Child process

        // Duplicate the file descriptor to standard output
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 failed");
            exit(EXIT_FAILURE);
        }

        close(fd);  // Close the original file descriptor after duplicating

        if (argc <= 1) {
            printf("No program to exec. Exiting...\n");
            exit(0);
        }

        printf("Running exec of \"%s\"\n", argv[1]);

        // Ensure all output is flushed before calling execve
        fflush(stdout);

        execve(argv[1], &argv[1], NULL);

        // If execve fails
        perror("execve failed");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        // Optionally, wait for the child process to complete
        wait(NULL);
    }

    return 0;
}
