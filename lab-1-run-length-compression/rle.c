
// Runlength compression in C
// Usage: ./rle <input file> <output file> <runlength> <mode>
//        Where mode=0 is compress and mode=1 is decompress

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>

// Read up to size bytes, handling partial reads
ssize_t read_full(int fd, void *buffer, size_t size) {
    unsigned char *data = buffer;
    size_t total = 0;

    while (total < size) {
        ssize_t bytes_read = read(fd, data + total, size - total);

        if (bytes_read == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error reading file");
            return -1;
        }

        // End of file
        if (bytes_read == 0) {
            break;
        }

        total += bytes_read;
    }

    return (ssize_t)total;
}

// Write all requested bytes, handling partial writes
int write_all(int fd, const void *buffer, size_t size) {
    const unsigned char *data = buffer;
    size_t total = 0;

    while (total < size) {
        ssize_t written = write(fd, data + total, size - total);

        if (written == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error writing file");
            return -1;
        }

        if (written == 0) {
            fprintf(stderr, "Error: write made no progress\n");
            return -1;
        }

        total += written;
    }

    return 0;
}

// Compress input using run-length encoding
int compress_file(int input_fd, int output_fd, int k) {
    unsigned char *current = malloc((size_t)k);
    unsigned char *next = malloc((size_t)k);

    if (current == NULL || next == NULL) {
        perror("Error allocating memory");
        free(current);
        free(next);
        return -1;
    }

    int result = -1;

    // Read the first pattern
    ssize_t current_size = read_full(input_fd, current, (size_t)k);

    if (current_size == -1) {
        goto cleanup;
    }

    // Empty input file
    if (current_size == 0) {
        result = 0;
        goto cleanup;
    }

    uint8_t count = 1;

    while (1) {
        ssize_t next_size = read_full(input_fd, next, (size_t)k);

        if (next_size == -1) {
            goto cleanup;
        }

        // Check whether the next pattern matches the current one
        if (next_size == current_size &&
            next_size > 0 &&
            memcmp(current, next, (size_t)current_size) == 0 &&
            count < 255) {

            count++;
            continue;
        }

        // Write the binary repetition counter
        if (write_all(output_fd, &count, 1) == -1) {
            goto cleanup;
        }

        // Write the pattern
        if (write_all(output_fd, current,
                      (size_t)current_size) == -1) {
            goto cleanup;
        }

        // End of input file
        if (next_size == 0) {
            break;
        }

        // Start a new run
        memcpy(current, next, (size_t)next_size);
        current_size = next_size;
        count = 1;
    }

    result = 0;

cleanup:
    free(current);
    free(next);
    return result;
}

// Decompress a run-length encoded file
int decompress_file(int input_fd, int output_fd, int k) {
    unsigned char *pattern = malloc((size_t)k);

    if (pattern == NULL) {
        perror("Error allocating memory");
        return -1;
    }

    int result = -1;
    uint8_t count;

    while (1) {
        // Read the one-byte binary repetition counter
        ssize_t bytes_read = read_full(input_fd, &count, 1);

        if (bytes_read == -1) {
            goto cleanup;
        }

        // End of compressed file
        if (bytes_read == 0) {
            break;
        }

        if (count == 0) {
            fprintf(stderr, "Invalid compressed file: zero count\n");
            goto cleanup;
        }

        // Read the pattern following the counter
        bytes_read = read_full(input_fd, pattern, (size_t)k);

        if (bytes_read == -1) {
            goto cleanup;
        }

        if (bytes_read == 0) {
            fprintf(stderr, "Invalid compressed file\n");
            goto cleanup;
        }

        // Write the pattern 'count' times
        for (int i = 0; i < count; i++) {
            if (write_all(output_fd, pattern,
                          (size_t)bytes_read) == -1) {
                goto cleanup;
            }
        }
    }

    result = 0;

cleanup:
    free(pattern);
    return result;
}

int main(int argc, char *argv[]) {

    // Program requires exactly 4 arguments
    if (argc != 5) {
        fprintf(stderr,
                "Usage: %s <input> <output> <length> <mode>\n",
                argv[0]);
        exit(-1);
    }

    // Extract input and output filenames
    char *input_file = argv[1];
    char *output_file = argv[2];

    // Convert compression length from string to integer
    char *end;
    errno = 0;
    long length = strtol(argv[3], &end, 10);

    if (errno != 0 || *end != '\0' ||
        length < 1 || length > INT_MAX) {
        fprintf(stderr, "Invalid compression length\n");
        exit(-1);
    }

    // Convert mode from string to integer
    errno = 0;
    long mode = strtol(argv[4], &end, 10);

    if (errno != 0 || *end != '\0' ||
        (mode != 0 && mode != 1)) {
        fprintf(stderr, "Invalid mode. Use 0 or 1.\n");
        exit(-1);
    }

    // Open input file for reading
    int input_fd = open(input_file, O_RDONLY);

    if (input_fd == -1) {
        perror("Error opening input file");
        exit(-1);
    }

    // Open output file without truncating it yet
    int output_fd = open(output_file,
                         O_WRONLY | O_CREAT,
                         S_IRUSR | S_IWUSR);

    if (output_fd == -1) {
        perror("Error opening output file");

        if (close(input_fd) == -1) {
            perror("Error closing input file");
        }

        exit(-1);
    }

    // Check whether input and output refer to the same file
    struct stat input_stat;
    struct stat output_stat;
    int result = 0;

    if (fstat(input_fd, &input_stat) == -1) {
        perror("Error checking input file");
        result = -1;
    } else if (fstat(output_fd, &output_stat) == -1) {
        perror("Error checking output file");
        result = -1;
    } else if (input_stat.st_dev == output_stat.st_dev &&
               input_stat.st_ino == output_stat.st_ino) {
        fprintf(stderr,
                "Input and output cannot be the same file\n");
        result = -1;
    } else {
        // Clear the output file only after the safety check
        if (ftruncate(output_fd, 0) == -1) {
            perror("Error truncating output file");
            result = -1;
        } else if (lseek(output_fd, 0, SEEK_SET) == -1) {
            perror("Error seeking output file");
            result = -1;
        } else {
            // Perform compression or decompression
            if (mode == 0) {
                result = compress_file(input_fd, output_fd,
                                       (int)length);
            } else {
                result = decompress_file(input_fd, output_fd,
                                         (int)length);
            }
        }
    }

    // Close input file
    if (close(input_fd) == -1) {
        perror("Error closing input file");
        result = -1;
    }

    // Close output file
    if (close(output_fd) == -1) {
        perror("Error closing output file");
        result = -1;
    }

    if (result == -1) {
        exit(-1);
    }

    return 0;
}
