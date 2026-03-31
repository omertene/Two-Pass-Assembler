#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "data_structures.h"

#include <stdio.h>

/*
   An enum for the A,R,E (Absolute, Relocatable, External) field
   in a machine word.
*/
enum 
{ 
     ARE_ABSOLUTE, 
     ARE_EXTERNAL, 
     ARE_RELOCATABLE 
};

/*
   The main function for the second pass. It runs after the first pass is complete.
   Its job is to resolve all symbol addresses, encode the instructions into
   the final machine code, and then call the functions to write the output files.

   - input_file: the ".am" file to read.
   - filename: the original base name of the file, for creating output files.
   - symbol_table: the completed symbol table from the first pass.
   - icf: the final instruction counter value from the first pass.
   - dcf: the final data counter value from the first pass.
   - first_pass_succeeded: a flag that tells us if the first pass was successful.
   - ctx: the cleanup struct for handling memory errors.

   Return SUCCESS if this pass also succeeded, otherwise ERROR.
*/
int run_second_pass(FILE *input_file, const char *filename, symbol_node *symbol_table, int icf, int dcf, int first_pass_succeeded, cleanup_context* ctx, int* errors);

/*
   This function reads the file and finds all ".entry" directives.
   It marks the corresponding symbols in the symbol table as 'entry' points,
   so they will be written to the .ent file. It also checks for errors.

   - input_file: the ".am" file to read.
   - symbol_table: the symbol table to search and update.

   Return SUCCESS if all entry directives were valid, otherwise ERROR.
*/
int handle_entry_directives(FILE *input_file, symbol_node *symbol_table, int* errors);

/*
   A helper function that adds a new node to the externals linked list.
   This list keeps track of every time an external symbol is used in the
   code, and at which memory address it was used.

   - name: the name of the external symbol that was used.
   - address: the memory address where the symbol was used.
   - extern_list: a pointer to the head of the externals list.
   - ctx: the cleanup struct for handling memory errors.
*/
void add_to_extern_list(const char *name, int address, extern_node **extern_list, cleanup_context* ctx);

/*
   A helper for build_code_image that handles the full encoding of a
   one-operand instruction, including its extra machine words.

   - splitted: the struct containing the parts of the line.
   - code_image: the array where the machine code is stored.
   - line_num: the current line number for error messages.
   - ic: a pointer to the current instruction counter.
   - symbol_table: the completed symbol table.
   - extern_list: a pointer to the externals list.
   - ctx: the cleanup struct.

   Return SUCCESS if encoding worked, otherwise ERROR.
*/
int encode_one_operand(splitted_line splitted, machine_word* code_image, int line_num, int* ic, symbol_node* symbol_table, extern_node** extern_list, cleanup_context* ctx);

/*
   A helper for build_code_image that handles the full encoding of a
   two-operand instruction, including its extra machine words.

   - splitted: the struct containing the parts of the line.
   - code_image: the array where the machine code is stored.
   - line_num: the current line number for error messages.
   - ic: a pointer to the current instruction counter.
   - symbol_table: the completed symbol table.
   - extern_list: a pointer to the externals list.
   - ctx: the cleanup struct.

   Return SUCCESS if encoding worked, otherwise ERROR.
*/
int encode_two_operands(splitted_line splitted, machine_word* code_image, int line_num, int* ic, symbol_node* symbol_table, extern_node** extern_list, cleanup_context* ctx);

/*
   A helper function that takes a single operand string 
   and converts it into its final binary machine word.

   - operand: the operand string.
   - is_source: a flag to know if this is a source or destination operand.
   - words_arr: a pointer to the location in the code_image to write the new words to.
   - word_count: a pointer to an int, to return how many words were created.
   - symbol_table: the symbol tableע
   - extern_list: the externals list, to add to if the symbol is external.
   - current_ic: the current memory address, needed for the externals list.
   - ctx: the cleanup struct.

   Return SUCCESS if encoding worked, otherwise ERROR (if symbol not found).
*/
int encode_operand(char *operand, int is_source, machine_word *words_arr, int *word_count, symbol_node *symbol_table, extern_node **extern_list, int current_ic, cleanup_context* ctx, int line_num);

/*
   This function builds the final code_image array. It reads the file,
   ignores directives, and translates each instruction line into its
   binary representation using the completed symbol table.

   - input_file: the ".am" file to read.
   - code_image: the array to fill with the binary code.
   - ic: a pointer to the instruction counter, to use as an index.
   - symbol_table: the completed symbol table, to look up labels.
   - extern_list: a pointer to the externals list, to add to it.
   - ctx: the cleanup struct for memory errors.

   Return SUCCESS if the whole image was built without errors, otherwise ERROR.
*/
int build_code_image(FILE *input_file, machine_word code_image[], int *ic, symbol_node *symbol_table, extern_node **extern_list, cleanup_context* ctx, int* errors);

/*
   This function builds the final data_image array. It reads the file again,
   finds all the .data, .string, and .mat lines, and copies their
   binary values into the array.

   - input_file: the ".am" file to read.
   - data_image: the array to fill with the binary data.
   - dc: a pointer to the data counter, used as an index for the array.
*/
void build_data_image(FILE *input_file, machine_word data_image[], int *dc);


#endif