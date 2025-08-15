#ifndef PARSER_H
#define PARSER_H

typedef struct Parser{
    char *input;
    int position;
    int length;
} Parser;

int parse_shell_command(const char *input);
void skip_whitespace(Parser *parser);
int parse_name(Parser *parser);
int parse_input_redirect(Parser *parser);
int parse_output_redirect(Parser *parser);
int parse_atomic(Parser *parser);
int parse_cmd_group(Parser *parser);
int parse_shell_cmd(Parser *parser);

#endif