#include <stdio.h>
#include <stdlib.h>

char *reverseString(char *input)
{
    int length = 0;

    // Find the length of the input string
    while (*(input + length) != '\0')
    {
        length++;
    }

    // Allocate enough memory for the characters + '\0'
    char *output = (char *)malloc(length + 1);

    // Copy characters in reverse order
    for (int i = 0; i < length; i++)
    {
        *(output + i) = *(input + length - 1 - i);
    }

    // Add the null terminator
    *(output + length) = '\0';

    return output;
}

int main(void)
{
    char *messagePtr = "HELLOWORLD!";

    char *reversedMessage = reverseString(messagePtr);

    printf("Reversed string: %s\n", reversedMessage);

    free(reversedMessage);

    return 0;
}