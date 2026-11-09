//SLUSH - The SLU SHELL
//
//This assignment reads user input, and executes that input as a sequence of
//programs to execute.
//
//Program execution commands have the form:
//
//prog_n [args] ( ... prog_3 [args] ( prog_2 [args] ( prog_1 [args]
//
//For example, "grep slush ( sort -r ( ls -l -a" returns a reverse sorted list
//of all files in the current directory that contain the string "slush" in
//
//See the lab writeup for more details and other requirements.

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>

#define MAX_LINE 256
#define MAX_COMMANDS 32
#define MAX_ARGS 16

typedef struct {
    char *argv[MAX_ARGS + 1];
} Command;

static volatile sig_atomic_t interrupted = 0;

void handle_sigint(int sig)
{
    (void)sig;
    interrupted = 1;

    ssize_t result = write(STDOUT_FILENO, "\n", 1);
    (void)result;
}

/* Display the current directory (extra credit) */
void print_prompt(void)
{
    char cwd[4096];
    char *home = getenv("HOME");

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        printf("slush> ");
    } else if (home != NULL &&
               strncmp(cwd, home, strlen(home)) == 0 &&
               (cwd[strlen(home)] == '/' ||
                cwd[strlen(home)] == '\0')) {

        char *relative = cwd + strlen(home);

        if (*relative == '/')
            relative++;

        if (*relative == '\0')
            printf("slush|~> ");
        else
            printf("slush|%s> ", relative);

    } else {
        printf("slush|%s> ", cwd);
    }

    fflush(stdout);
}

/* Parse a single command into arguments */
int parse_command(char *text, Command *cmd)
{
    int argc = 0;
    char *saveptr = NULL;
    char *token = strtok_r(text, " \t\r\n", &saveptr);

    while (token != NULL) {
        if (argc >= MAX_ARGS) {
            fprintf(stderr, "Too many arguments\n");
            return -1;
        }

        cmd->argv[argc++] = token;
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }

    cmd->argv[argc] = NULL;

    if (argc == 0) {
        fprintf(stderr, "Invalid null command\n");
        return -1;
    }

    return argc;
}

/* Parse the backward pipeline syntax */
int parse_line(char *line, Command commands[])
{
    int count = 0;
    char *start = line;
    char *p = line;

    while (1) {
        if (*p == '(' || *p == '\0') {
            char end = *p;
            *p = '\0';

            if (count >= MAX_COMMANDS) {
                fprintf(stderr, "Too many commands\n");
                return -1;
            }

            if (parse_command(start, &commands[count]) < 0)
                return -1;

            count++;

            if (end == '\0')
                break;

            start = p + 1;
        }

        p++;
    }

    return count;
}

/* Run all programs in a backward pipeline */
void execute_pipeline(Command commands[], int count)
{
    pid_t pids[MAX_COMMANDS];
    int previous_read = -1;
    int started = 0;

    /*
     * Example:
     * more ( sort ( ps aux
     *
     * commands[0] = more
     * commands[1] = sort
     * commands[2] = ps aux
     *
     * Execute from right to left.
     */

    for (int i = count - 1; i >= 0; i--) {
        int fd[2] = {-1, -1};

        if (i > 0) {
            if (pipe(fd) == -1) {
                perror("pipe");
                break;
            }
        }

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");

            if (fd[0] != -1)
                close(fd[0]);
            if (fd[1] != -1)
                close(fd[1]);

            break;
        }

        if (pid == 0) {
            /*
             * Children use the default SIGINT behavior.
             * Ctrl+C should terminate running commands.
             */
            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGINT, &sa, NULL);

            /* Input comes from the previous command */
            if (previous_read != -1) {
                if (dup2(previous_read, STDIN_FILENO) == -1) {
                    perror("dup2");
                    _exit(1);
                }
            }

            /* Output goes to the command on the left */
            if (i > 0) {
                if (dup2(fd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    _exit(1);
                }
            }

            if (previous_read != -1)
                close(previous_read);

            if (fd[0] != -1)
                close(fd[0]);

            if (fd[1] != -1)
                close(fd[1]);

            execvp(commands[i].argv[0], commands[i].argv);

            if (errno == ENOENT || errno == EACCES) {
                fprintf(stderr, "%s: Not found\n",
                        commands[i].argv[0]);
            } else {
                perror(commands[i].argv[0]);
            }

            _exit(127);
        }

        pids[started++] = pid;

        if (previous_read != -1)
            close(previous_read);

        if (fd[1] != -1)
            close(fd[1]);

        previous_read = fd[0];
    }

    if (previous_read != -1)
        close(previous_read);

    /* Wait for every child, including interrupted ones */
    for (int i = 0; i < started; i++) {
        int status;

        while (waitpid(pids[i], &status, 0) == -1) {
            if (errno == EINTR)
                continue;

            perror("waitpid");
            break;
        }
    }
}

/* Built-in cd command */
void change_directory(Command *cmd)
{
    if (cmd->argv[1] == NULL) {
        char *home = getenv("HOME");

        if (home == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }

        if (chdir(home) == -1)
            perror("cd");

    } else if (cmd->argv[2] != NULL) {
        fprintf(stderr, "cd: too many arguments\n");

    } else {
        if (chdir(cmd->argv[1]) == -1)
            perror("cd");
    }
}

int main(void)
{
    char line[MAX_LINE + 2];
    Command commands[MAX_COMMANDS];

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    while (1) {
        interrupted = 0;
        print_prompt();

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (feof(stdin)) {
                printf("\n");
                break;
            }

            if (ferror(stdin) && errno == EINTR) {
                clearerr(stdin);
                continue;
            }

            if (interrupted) {
                clearerr(stdin);
                continue;
            }

            perror("fgets");
            break;
        }

        /* Reject input longer than MAX_LINE */
        if (strchr(line, '\n') == NULL &&
            !feof(stdin) &&
            strlen(line) > MAX_LINE) {

            fprintf(stderr, "Input line too long\n");

            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;

            continue;
        }

        /* Ignore empty lines */
        if (strspn(line, " \t\r\n") == strlen(line))
            continue;

        memset(commands, 0, sizeof(commands));

        int count = parse_line(line, commands);

        if (count < 0)
            continue;

        /* cd must be executed by the shell itself */
        if (strcmp(commands[0].argv[0], "cd") == 0) {
            if (count != 1) {
                fprintf(stderr,
                        "cd: cannot be used in a pipeline\n");
            } else {
                change_directory(&commands[0]);
            }

            continue;
        }

        execute_pipeline(commands, count);
    }

    return 0;
}


