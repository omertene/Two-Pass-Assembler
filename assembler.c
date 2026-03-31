/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: assembler.c                                          */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
    This is the main file for the assembler project.
    It has the main function which is the starting point of the program.
    Its job is to read the filenames from the command line and send
    each file to be processed by the assembler stages.
*/
/* ----------------------------------------------------------------------------------------------------------------------- */


#include "pre_processor.h" 
#include "first_pass.h"
#include "second_pass.h"
#include "symbol_table.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Function that handles one file. It runs the whole process for a single assembly file.
Calls the pre-processor, then the first pass, and then the second pass.
It also creates the main data structures and makes sure to free
all the memory at the end. */
void process_file(const char *filename) 
{
    char am_filename[MAX_FILE_NAME_LEN];
    char as_filename[MAX_FILE_NAME_LEN];
    int icf = 0, dcf = 0;
    int second_pass_status, first_pass_status;
    int errors = 0;

    macro_table macros = {0};
    symbol_node *symbol_table = NULL;
    FILE *am_file = NULL;

    cleanup_context ctx;
    ctx.mt = &macros;
    ctx.st = &symbol_table;
    ctx.am_file = NULL;
    ctx.am_filename = am_filename;
    
    /* check if the filename is too long */
    if (strlen(filename) + strlen(".as") >= MAX_FILE_NAME_LEN) 
    {
        printf("ERROR! Input filename '%s' is too long.\n", filename);
        return; 
    }

    /* create ".as", ".am" files */
    strcpy(as_filename, filename);
    strcat(as_filename, ".as");
    strcpy(am_filename, filename);
    strcat(am_filename, ".am");
    
    init_macro_table(&macros, &ctx);
    
    /* first: preprocessor */
    if (run_preprocessor(as_filename, am_filename, &macros, &ctx) == ERROR) 
    {
        /* delete file if process failed, and clean memory */
        remove(am_filename);
        goto cleanup;
    }

    /* pre_processor succeeded */
    
    am_file = fopen(am_filename, "r");
    ctx.am_file = am_file;
    if (!am_file) 
    {
        printf("Error! Could not open file '%s'.\n", am_filename);
        goto cleanup;
    }

    /* start first pass */
    first_pass_status = run_first_pass(am_file, &symbol_table, &icf, &dcf, &macros, &ctx, &errors);
    rewind(am_file);
    second_pass_status = run_second_pass(am_file, filename, symbol_table, icf, dcf, (first_pass_status == SUCCESS), &ctx, &errors);

    if (first_pass_status == SUCCESS && second_pass_status == SUCCESS) 
        printf("Assembly process for %s completed successfully!\n", as_filename);
    else 
        printf("Assembly process for %s failed with %d errors!\n", as_filename, errors);
    

cleanup:
    if (am_file) 
        fclose(am_file);
    
    /* free memory */
    free_symbol_table(symbol_table);
    free_macro_table(&macros);
}

/* Read files from the user and send them to processing */
int main(int argc, char *argv[]) 
{
    int i;
    if (argc < MIN_ARGC) 
    {
        printf("No file were recived, program finished.\n");
        return 0;
    }
    /* start process every file */
    for (i = 1; i < argc; i++) 
        process_file(argv[i]);
    return 0;
}


