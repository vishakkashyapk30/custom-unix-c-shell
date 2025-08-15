#include "../include/shell.h"
#include "../include/parser.h"

void skip_whitespace(Parser *parser) {
    while (parser->position < parser->length) {
        char c = parser->input[parser->position];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            parser->position++;
        } else {
            break;
        }
    }
}

int parse_name(Parser *parser) {
    skip_whitespace(parser);
    
    int start = parser->position;
    
    // name -> r"[^|&><;]+"
    while (parser->position < parser->length) {
        char c = parser->input[parser->position];
        if (c != '|' && c != '&' && c != '>' && c != '<' && c != ';' && 
            c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            parser->position++;
        } else {
            break;
        }
    }
    
    return parser->position > start; // Return 1 if we consumed any characters.
}

int parse_input_redirect(Parser *parser) {
    int saved_position = parser->position;
    skip_whitespace(parser);
    
    if (parser->position >= parser->length || parser->input[parser->position] != '<') {
        parser->position = saved_position;
        return 0;
    }
    
    parser->position++; // consume '<'
    
    // An input redirect must be followed by a name.
    if (!parse_name(parser)) {
        parser->position = saved_position;
        return 0;
    }
    
    return 1;
}

int parse_output_redirect(Parser *parser) {
    int saved_position = parser->position;
    skip_whitespace(parser);
    
    if (parser->position >= parser->length || parser->input[parser->position] != '>') {
        parser->position = saved_position;
        return 0;
    }
    
    parser->position++; // consume first '>'
    
    // Check for '>>' (append).
    if (parser->position < parser->length && parser->input[parser->position] == '>') {
        parser->position++; // consume second '>'
    }
    
    // An output redirect must be followed by a name.
    if (!parse_name(parser)) {
        parser->position = saved_position;
        return 0;
    }
    
    return 1;
}

int parse_atomic(Parser *parser) {
    // atomic -> name (name | input | output)*
    
    // An atomic command must start with a name.
    if (!parse_name(parser)) {
        return 0;
    }
    
    // After the first name, it can be followed by any number of
    // other names (arguments), input redirects, or output redirects.
    while (1) {
        int saved_position = parser->position;
        
        if (parse_input_redirect(parser) || parse_output_redirect(parser) || parse_name(parser)) {
            continue; // Successfully parsed one more element.
        }
        
        // If we can't parse any more valid elements, restore the position and stop.
        parser->position = saved_position;
        break;
    }
    
    return 1;
}

int parse_cmd_group(Parser *parser) {
    // cmd_group -> atomic (\| atomic)*
    
    // A command group must start with an atomic command.
    if (!parse_atomic(parser)) {
        return 0;
    }
    
    // It can then be followed by zero or more pipes, each followed by an atomic command.
    while (1) {
        int saved_position = parser->position;
        skip_whitespace(parser);
        
        if (parser->position < parser->length && parser->input[parser->position] == '|') {
            // Make sure it's a single '|' and not '||'.
            if (parser->position + 1 < parser->length && parser->input[parser->position + 1] == '|') {
                parser->position = saved_position;
                break; // This is '||', which is not a pipe separator.
            }
            
            parser->position++; // consume '|'
            
            // A pipe must be followed by another valid atomic command.
            if (!parse_atomic(parser)) {
                return 0; // Invalid syntax: pipe not followed by a command.
            }
        } else {
            // No more pipes found.
            parser->position = saved_position;
            break;
        }
    }
    
    return 1;
}

int parse_shell_cmd(Parser *parser) {
    // Updated grammar: shell_cmd -> cmd_group ((; | & | &&) cmd_group)* &?
    
    if (!parse_cmd_group(parser)) {
        return 0;
    }
    
    while (1) {
        int saved_position = parser->position;
        skip_whitespace(parser);

        if (parser->position < parser->length) {
            char c = parser->input[parser->position];
            
            if (c == ';') {
                parser->position++; // consume ';'
                // ';' MUST be followed by another command group
                if (!parse_cmd_group(parser)) {
                    return 0;
                }
            } else if (c == '&') {
                parser->position++; // consume first '&'
                
                if (parser->position < parser->length && parser->input[parser->position] == '&') {
                    parser->position++; // consume second '&' for '&&'
                    // '&&' MUST be followed by another command group
                    if (!parse_cmd_group(parser)) {
                        return 0;
                    }
                } else {
                    // Single '&' - try to parse another command group
                    if (!parse_cmd_group(parser)) {
                        // No command group follows, so this '&' is trailing
                        parser->position = saved_position;
                        break;
                    }
                }
            } else {
                // No valid separator found
                parser->position = saved_position;
                break;
            }
        } else {
            // End of input
            parser->position = saved_position;
            break;
        }
    }

    // Optional trailing '&' for backgrounding the last command.
    int saved_position = parser->position;
    skip_whitespace(parser);
    if (parser->position < parser->length && parser->input[parser->position] == '&') {
        // Make sure it's not the start of '&&'
        if (parser->position + 1 < parser->length && parser->input[parser->position + 1] == '&') {
            parser->position = saved_position;
        } else {
            parser->position++; // Consume the trailing '&'
        }
    } else {
        parser->position = saved_position;
    }

    return 1;
}

int parse_shell_command(const char *input) {
    if (input == NULL || strlen(input) == 0) {
        return 1; // Empty input is considered valid.
    }
    
    Parser parser;
    parser.input = (char *)input;
    parser.position = 0;
    parser.length = strlen(input);
    
    if (!parse_shell_cmd(&parser)) {
        return 0;
    }
    
    // Check that we've consumed the entire input
    skip_whitespace(&parser);
    if (parser.position < parser.length) {
        return 0; // Invalid syntax: trailing characters found.
    }
    
    return 1;
}