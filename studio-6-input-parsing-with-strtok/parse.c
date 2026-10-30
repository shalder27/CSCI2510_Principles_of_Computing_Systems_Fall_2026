
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    char buffer[1024];

    int max_args = 15;
    int max_argv_size = max_args + 2;
    char *cmd;
    char *my_argv[max_argv_size];

    // Exercise 1: Read user input
    printf("Enter a command: ");
    fflush(stdout);

    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        return 1;
    }

    // Exercise 6: Remove trailing newline
    buffer[strcspn(buffer, "\n")] = '\0';

    // Exercise 8: Extract command
    cmd = strtok(buffer, " \t");

    if (cmd == NULL) {
        printf("No command entered.\n");
        return 1;
    }

    // Exercise 9: Build argv array
    my_argv[0] = cmd;

    int i = 1;
    char *res;

    while (i <= max_args &&
           (res = strtok(NULL, " \t")) != NULL) {
        my_argv[i] = res;
        i++;
    }

    // NULL-terminate the argument array
    my_argv[i] = NULL;

    // Exercise 10: Execute command
    execvp(cmd, my_argv);

    // Only reached if execvp fails
    perror("execvp failed");
    return 1;
}
