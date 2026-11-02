
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    int fd[2];
    pid_t pid1, pid2;

    // Create pipe before forking
    if (pipe(fd) == -1) {
        perror("pipe");
        return 1;
    }

    // First child: Program 1
    pid1 = fork();

    if (pid1 == -1) {
        perror("fork");
        return 1;
    }

    if (pid1 == 0) {
        // Close unused read end
        close(fd[0]);

        // Redirect stdout to pipe write end
        if (dup2(fd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }

        close(fd[1]);

        char *args[] = {"print", NULL};
        execvp("./print", args);

        perror("execvp print");
        exit(1);
    }

    // Second child: Program 2
    pid2 = fork();

    if (pid2 == -1) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        waitpid(pid1, NULL, 0);
        return 1;
    }

    if (pid2 == 0) {
        // Close unused write end
        close(fd[1]);

        // Redirect stdin to pipe read end
        if (dup2(fd[0], STDIN_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }

        close(fd[0]);

        char *args[] = {"input", NULL};
        execvp("./input", args);

        perror("execvp input");
        exit(1);
    }

    // Parent does not use the pipe
    close(fd[0]);
    close(fd[1]);

    // Wait for both children
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}