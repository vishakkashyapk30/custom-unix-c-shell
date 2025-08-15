#include "../include/shell.h"
#include "../include/input.h"

char* read_input(void) 
{
    char *input = malloc(MAX_INPUT_SIZE);
    if (input == NULL) 
    {
        perror("malloc");
        return NULL;
    }
    
    if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL)
    {
        free(input);
        return NULL;
    }
    
    size_t len = strlen(input);
    if (len > 0 && input[len - 1] == '\n')
        input[len - 1] = '\0';
    
    return input;
}