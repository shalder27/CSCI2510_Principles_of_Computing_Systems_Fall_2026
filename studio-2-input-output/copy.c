#include <unistd.h>

#define bufferSize 200

int main(void)
{
    char buffer[bufferSize];
    ssize_t bytesRead;

    while (1)
    {
        // Read up to bufferSize bytes from standard input
        bytesRead = read(STDIN_FILENO, buffer, bufferSize);

        // A return value of 0 means we reached EOF
        if (bytesRead == 0)
        {
            break;
        }

        // Write only the number of bytes that were actually read
        write(STDOUT_FILENO, buffer, bytesRead);
    }

    return 0;
}