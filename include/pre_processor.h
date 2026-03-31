#ifndef PRE_PROCESSOR_H
#define PRE_PROCESSOR_H

#include "data_structures.h"
#include "constants.h"

#include <stdlib.h>


/*
   An enum to represent the result of checking if a line is a "mcro" statement.
   - NOT_MACROSTART: The line is not a macro start statement.
   - INVALID_MACROSTART: The line starts with "mcro" but has an error in it.
   - VALID_MACROSTART: The line is a correct "mcro" statement.
*/
typedef enum 
{
    NOT_MACROSTART, 
    INVALID_MACROSTART, 
    VALID_MACROSTART
} macro_start_status;

/*
   An enum to represent the result of checking if a line is a "mcroend" statement.
   - NOT_MACROEND: The line is not a macro end statement.
   - INVALID_MACROEND: The line starts with "mcroend" but has extra text.
   - VALID_MACROEND: The line is a correct "mcroend" statement.
*/
typedef enum 
{
    NOT_MACROEND, 
    INVALID_MACROEND, 
    VALID_MACROEND
} macro_end_status;

/*
   This is the main function for the pre-processor stage.
   It gets an ".as" file, finds all the macros, expands them,
   and writes the result to a new ".am" file.
   It also reports any errors found during this process.

   - input_file: the name of the file we need to read from.
   - output_file: the name of the new file we will create.
   - macros: a pointer to the macro table where we store the macros.
   - ctx: a pointer to the cleanup struct, for handling memory errors.

   Return SUCCESS if everything is ok, or ERROR if something went wrong.
*/
int run_preprocessor(const char* input_file, const char* output_file, macro_table* macros, cleanup_context* ctx);

/*
   This function helps process a single line inside the main preprocessor loop.
   It checks if we are inside a macro definition, if the line is a macro
   start or end, a macro call, or just a regular line of code.

   - line: the single line of code to process.
   - macros: the macro table
   - output: the file pointer to the .am file
   - in_macro: flag that tells us if we are currently defining a macro.
   - current_macro: a pointer to the macro we are currently adding lines to.
   - errors: a pointer to the error counter.
   - ctx: the cleanup struct for memory errors.
*/
void handle_preproc_line(char* line, macro_table* macros, FILE* output, const char* input_file, int line_num, int* in_macro, macro** current_macro, int* errors, cleanup_context* ctx);

/*
   Checks if a line is a valid "mcro" declaration.
   It validates everything: the 'mcro' keyword, the macro name (not too long,
   not a reserved word, valid characters), and makes sure there is no extra text after.

   - table: the macro table.
   - line: the line to check.
   - name_out: a buffer where the valid macro name will be stored if found.
   - filename: the name of the input file, for error messages.
   - line_num: the current line number, for error messages.

   Return one of the macro_start_status enums (VALID, INVALID, or NOT_MACROSTART).
*/
macro_start_status valid_macro_declaration(macro_table* table, const char* line, char* name_out, const char* filename, int line_num);

/*
   Checks if a line is a valid "mcroend" statement. It also checks
   that there is no extra text after the "mcroend" keyword.

   - line: the line to check.
   - filename: the name of the input file, for error messages.
   - line_num: the current line number, for error messages.

   Return one of the macro_end_status enums (VALID, INVALID, or NOT_MACROEND).
*/
macro_end_status is_macro_end(const char* line, const char* filename, int line_num);

/*
   This function searches the macro table to see if a macro
   with the given name already exists.

   - table: the macro table to search in.
   - name: the macro name to look for.

   Return a pointer to the macro if found, otherwise NULL.
*/
macro* name_already_exists(macro_table* table, const char* name);

/*
   Adds a new macro to the macro table. 
   Reallocates memory if the table is full.

   - table: the macro table to add the new macro to.
   - name: the name of the new macro.
   - ctx: the cleanup struct to handle memory errors.

   Return a pointer to the newly created macro.
*/
macro* add_macro(macro_table* table, const char* name, cleanup_context* ctx);

/*
   Adds a line of code to a macro's content. It reallocates the
   memory for the content string to make it bigger for the new line.

   - m: a pointer to the macro we are adding content to.
   - line: the line of text to add.
   - ctx: the cleanup struct to handle memory errors.
*/
void add_line_to_macro(macro* m, const char* line, cleanup_context* ctx);

/*
   Initializes a new macro table, allocating the starting
   amount of memory for the macros array.

   - t: a pointer to the macro_table struct to initialize.
   - ctx: the cleanup struct to handle memory errors.
*/
void init_macro_table(macro_table* t, cleanup_context* ctx);

/*
   Frees all the memory that was used by the macro table. It loops through
   all the macros and frees their content string, and then frees the
   main macros array itself.

   - t: a pointer to the macro_table to free.
*/
void free_macro_table(macro_table* t);

#endif