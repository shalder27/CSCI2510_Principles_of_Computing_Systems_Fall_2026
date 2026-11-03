
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

// Custom signal handler for SIGINT
void handle_sigint(int signum) {
    const char msg[] = "Parent is ignoring SIGINT!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(int argc, char* argv[]) {
    pid_t ret;

    // Parent handles SIGINT without terminating
    signal(SIGINT, handle_sigint);

    printf("Forking sleeper...\n");
    fflush(stdout);

    ret = fork();

    if (ret == -1) {
        perror("Could not fork");
        exit(EXIT_FAILURE);
    }

    if (ret == 0) {
        // Child executes sleeper
        char* cmd = "./sleeper";
        char* myargv[] = {"sleeper", NULL};

        execvp(cmd, myargv);

        perror("Error calling execvp");
        exit(EXIT_FAILURE);
    }

    // Parent waits for child
    printf("Waiting for sleeper %d...\n", ret);
    fflush(stdout);

    waitpid(ret, NULL, 0);

    printf("Parent finished waiting and returned successfully!\n");

    return 0;
}
