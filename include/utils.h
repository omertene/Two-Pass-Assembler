#ifndef UTILS_H
#define UTILS_H

#include "data_structures.h"

#include <stdio.h>

/*
   A struct that holds all the  information about a single assembly command.
   - name: the command name string.
   - opcode: the command's opcode number.
   - expected_operands: how many operands the command should get.
   - valid_source_modes: an array of flags for allowed source addressing modes.
   - valid_dest_modes: an array of flags for allowed destination addressing modes.
*/
typedef struct 
{ 
    const char *name; 
    int opcode; 
    int expected_operands;
    int valid_source_modes[ADDRESING_TYPES_NUM]; 
    int valid_dest_modes[ADDRESING_TYPES_NUM]; 
} command_details;

/* Checks if a line contains only whitespaces. */
int is_empty(const char* line);

/* Removes leading and trailing whitespace from a string, and also the '\n' at the end. */
void clean_line(char* line);

/* Checks if a line is longer than the max length, and consumes the rest of it. */
int line_too_long(FILE*fp,const char*line);

/* Checks if a given word is an assembly reserved keyword. */
int is_reserved_word(const char*word);

/* Checks if command is a directive */
int is_directive(const char*word);

/* Function that gets a command name and returns a pointer to its command_details struct. */
const command_details* get_command_info(const char *command_name);

/* A helper that gets a string and returns a pointer to the first non-whitespace character. */
char* trim_whitespace(char *str);

/* Function that takes a line and splits it into: label, command, and operands. */
splitted_line split_line(char *line);

/* Gets an operand string and returns its addressing mode type from the addressing_types enum. */
addressing_types get_addressing_mode(char *operand_str);

/* The central function for memory errors. It cleans up all memory and exits the program. */
void handle_error(const cleanup_context* ctx, const char* error_message);

#endif