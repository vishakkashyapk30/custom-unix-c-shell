#include "../include/shell.h"
#include "../include/prompt.h"
#include "../include/input.h"
#include "../include/parser.h"

int main() {
    initialize_shell_info();
    char *input;
    
    while (1) 
    {
        display_prompt();
        input = read_input();
        if (input == NULL) 
        {
            printf("\n");
            break;
        }
        
        // Parse the input using A.3 parser
        if (!parse_shell_command(input)) {
            printf("Invalid Syntax!\n");
        }
        // If valid, do nothing for now as per A.3 requirements
        
        free(input);
    }
    free(home_directory);
    free(username);
    free(system_name);
    
    return 0;
}