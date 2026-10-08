#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define bufferSize 200

int main(int argc, char *argv[])
{
    char buffer[bufferSize];
    ssize_t bytesRead;

    // Ensure a filename was provided
    if (argc != 2)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    // Open the file in read-only mode
    int fd = open(argv[1], O_RDONLY);
if (fd == -1)
{
    perror("Error opening file");
    return -1;
}

    // Read from the opened file
    while ((bytesRead = read(fd, buffer, bufferSize)) > 0)
    {
        // Print the contents to standard output
        write(STDOUT_FILENO, buffer, bytesRead);
    }

    // Close the file
    close(fd);

    return 0;
}