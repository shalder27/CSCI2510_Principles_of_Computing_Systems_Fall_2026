// Samannita Halder
// September 28, 2026
// Prints a Hello World message using the Linux write() system call

#include <unistd.h>

int main(int argc, char* argv[]) {

    write(STDOUT_FILENO, "Hello, world!\n", 14);

    return 0;
}