#ifndef FIRST_PASS_H
#define FIRST_PASS_H

#include "data_structures.h"
#include "utils.h"

#include <stdio.h>



/*
   This is the main function that runs the first pass.
   It reads the ".am" file line by line, builds the symbol table,
   finds errors, and calculates the final IC and DC.

   - input_file: a pointer to the ".am" file to read from.
   - symbol_table: a pointer to the head of the symbol table, which will be built here.
   - icf: a pointer to an int where the final Instruction Counter will be stored.
   - dcf: a pointer to an int where the final Data Counter will be stored.
   - mac_table: a pointer to the macro table, to check for name conflicts.
   - ctx: the cleanup struct to handle memory errors.
   - errors: a pointer to an int that will count the errors found.

   Return SUCCESS if the pass finished without errors, otherwise ERROR.
*/
int run_first_pass(FILE *input_file, symbol_node **symbol_table, int *icf, int *dcf, macro_table* mac_table, cleanup_context* ctx, int* errors);

/*
   Function gets one split line and decides what to do with it. It
   sends the line to the correct processing function based 
   on its content.
   
   - splitted: the struct containing the parts of the line.
   - symbol_table: pointer to the symbol table, to add labels to it.
   - ic: pointer to the current Instruction Counter.
   - dc: pointer to the current Data Counter.
   - line_num: the current line number for error messages.
   - mac_table: pointer to the macro table.
   - success: a pointer to a flag that tracks if the pass is still successful.
   - errors: a pointer to the error counter.
   - ctx: the cleanup struct to handle memory errors.
*/
void handle_first_pass_line(splitted_line splitted, symbol_node **symbol_table, int *ic, int *dc, int line_num, macro_table* mac_table, int *success, int *errors, cleanup_context* ctx);

/*
   Function gets a command string that starts
   with a dot and returns its type from the directive_type enum.

   - command: the string to check.

   Return the enum value for the directive, or UNKNOWN_DIRECTIVE if it's not a valid one.
*/
directive_type get_directive_type(const char *command);

/*
   Handles a .data directive. It splits the numbers, validates them,
   and updates the Data Counter  with the number of words needed.

   - operands: a string containing the numbers for the .data directive.
   - dc: a pointer to the Data Counter to update.
   - line_num: the current line number for error messages.

   Return SUCCESS if the line is valid, otherwise ERROR.
*/
int process_data_dir(const char *operands, int *dc, int line_num);

/*
   Handles a .string directive. It validates the string format
   and updates the DC with its length including null terminator.

   - operands: a string containing the string literal, including quotes.
   - dc: a pointer to the Data Counter to update.
   - line_num: the current line number for error messages.

   Return SUCCESS if the line is valid, otherwise ERROR.
*/
int process_string_dir(const char *operands, int *dc, int line_num);

/*
   Handles a .mat directive. It calls a helper to get the [rows][cols] part,
   then splits the initializer numbers and updates the DC.

   - operands: a string containing the matrix definition.
   - dc: a pointer to the Data Counter to update.
   - line_num: the current line number for error messages.

   Return SUCCESS if the line is valid, otherwise ERROR.
*/
int process_mat_dir(const char *operands, int *dc, int line_num);

/*
   A helper for process_mat_dir. Its only job is to parse and validate
   the [rows][cols] part of the .mat line.

   - operands: a pointer to a pointer to the string, so we can advance it past the dimensions.
   - rows_out: a pointer to store the number of rows found.
   - cols_out: a pointer to store the number of columns found.
   - line_num: the current line number for error messages.

   Return SUCCESS if the dimensions are valid, otherwise ERROR.
*/
int parse_mat_dimensions(const char **operands, long *rows_out, long *cols_out, int line_num);

/*
   This is the main function for handling instructions in the first pass.
   Its job is to calculate how many memory words an instruction will need.
   It calls helpers for 1-operand and 2-operand instructions.

   - splitted: the struct containing the parts of the instruction line.
   - line_num: the current line number for error messages.

   Return the number of words the instruction needs, or ERROR if there's a syntax error.
*/
int calculate_instruction_words(splitted_line splitted, int line_num);

/*
   A helper for calculate_instruction_words that handles instructions with
   exactly one operand. It validates the operand and returns the word count.

   - splitted: the struct with the line parts.
   - cmd_dets: a pointer to the details of the command being processed.
   - line_num: the current line number.

   Return the number of words, or ERROR on failure.
*/
int calc_one_operand_words(splitted_line splitted, const command_details *cmd_dets, int line_num);

/*
   A helper for calculate_instruction_words that handles instructions with
   exactly two operands. It validates the operands and returns the word count.

   - splitted: the struct with the line parts.
   - cmd_dets: a pointer to the details of the command being processed.
   - line_num: the current line number.

   Return the number of words, or ERROR on failure.
*/
int calc_two_operands_words(splitted_line splitted, const command_details *cmd_dets, int line_num);

/*
   This function is called when a label is found. It decides if it's
   a code or data label and calls add_symbol to add it to the table.

   - splitted: the struct with the line parts, including the label and command.
   - symbol_table: pointer to the symbol table to add the label to.
   - ic: the current Instruction Counter.
   - dc: the current Data Counter.
   - line_num: the current line number.
   - mac_table: pointer to the macro table for validation.
   - success: pointer to the success flag of the pass.
   - errors: pointer to the error counter.
   - ctx: the cleanup struct.
*/
void process_label_definition(splitted_line splitted, symbol_node **symbol_table, int ic, int dc, int line_num, macro_table* mac_table, int *success, int *errors, cleanup_context* ctx);

/*
   A helper that checks if a matrix operand 
   is written with the correct syntax.

   - operand: the operand string to check.
   - line_num: the current line number.
   - operand_type: a string ("source" or "destination") for printing clear error messages.

   Return SUCCESS if the syntax is valid, otherwise ERROR.
*/
int is_valid_matrix_syntax(const char* operand, int line_num, const char* operand_type);

#endif
