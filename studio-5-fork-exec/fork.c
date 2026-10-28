
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    // Arguments for the child program
    char* child_argv[] = {"ls", "-l", NULL};
    char* child_prog = child_argv[0];

    // Create a child process
    pid_t child_PID = fork();

    if (child_PID < 0) {
        // Fork failed
        perror("fork");
        return 1;
    }
    else if (child_PID == 0) {
        // Child executes the ls -l command
        execvp(child_prog, child_argv);

        // Runs only if execvp fails
        perror("execvp");
        return 1;
    }
    else {
        // Parent waits for child to finish
        waitpid(child_PID, NULL, 0);

        // Parent prints its PID
        printf("Parent process PID: %ld\n", (long)getpid());
    }

    return 0;
}
