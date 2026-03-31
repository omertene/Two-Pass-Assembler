/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: output_generator.c                                          */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
    This file is for the last stage of the assembler.
    Its main function is to write_output_files, only after both
    passes were successful. It takes the final code and data images and
    the symbol table, and writes all the output files (.ob, .ent, .ext).
    It also has helper functions to convert numbers to the special base 4
    that is required.
*/
/* ----------------------------------------------------------------------------------------------------------------------- */


#include "output_generator.h"
#include "constants.h"
#include "second_pass.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* Converts a number to its special base 4 form*/
void convert_to_base4(unsigned int n, char *base4_str, int digits) 
{
    const char base4_chars[] = {'a', 'b', 'c', 'd'};
    int i;
    base4_str[digits] = '\0';
    /* dividing by 4 until reach zero, each iteration save the division remainder */
    for (i = digits - 1; i >= 0; i--) 
    {
        base4_str[i] = base4_chars[n % BASE_4];
        n /= BASE_4;
    }
}

/* Creates out put files - .ob, and if needed .ent and .ext */
void write_output_files(const char *filename, int icf, int dcf, machine_word code_image[], machine_word data_image[], symbol_node *symbol_table, extern_node *extern_list) 
{
    FILE *fp;
    char output_filename[MAX_FILE_NAME_LEN];
    char base4_addr[ADDRESS_WORD_LENGTH+1], base4_word[MACHINE_WORD_LENGTH+1];
    int i, entries_found = 0;
    symbol_node *curr_sym;
    extern_node *ext_curr;

    /* loop through symbol table and mark the entries one */
    for (curr_sym = symbol_table; curr_sym; curr_sym = curr_sym->next) 
    {
        if (curr_sym->type.is_entry)
            entries_found = 1;
    }

    /* create the .ob file */
    sprintf(output_filename, "%s.ob", filename);
    fp = fopen(output_filename, "w");
    if(!fp)
    {
        printf("ERROR! Can not open .ob file\n");
        return;
    }

    /* print number of codes and data words at the beginning of the file (unpadded with 'a') */
    convert_to_base4_unpadded(icf - CODE_START_IDX, base4_word);
    fprintf(fp, "\t%s ", base4_word);
    convert_to_base4_unpadded(dcf, base4_word);
    fprintf(fp, "%s\n", base4_word);


    /*print all the code words - first address and then the code itself (padded) */
    for (i = 0; i < icf - CODE_START_IDX; i++) 
    {
        unsigned int final_word = (code_image[i].bits) | code_image[i].are;

        if (code_image[i].are == ARE_EXTERNAL) 
            final_word = ARE_EXTERNAL;
        else
            final_word = code_image[i].bits | code_image[i].are;
        
        convert_to_base4(i + CODE_START_IDX, base4_addr, ADDRESS_WORD_LENGTH);
        fprintf(fp, "%s\t", base4_addr);
        convert_to_base4(final_word, base4_word, MACHINE_WORD_LENGTH);
        fprintf(fp, "%s\n", base4_word);
    }
    /* print all the data words - first address and then the data itself (padded with 'a') */
    for (i = 0; i < dcf; i++) 
    {
        convert_to_base4(i + icf, base4_addr, ADDRESS_WORD_LENGTH);
        fprintf(fp, "%s\t", base4_addr);
        convert_to_base4(data_image[i].bits, base4_word, MACHINE_WORD_LENGTH);
        fprintf(fp, "%s\n", base4_word);
    }
    fclose(fp);

    /* if there are entries, create the entry file */
    if (entries_found) 
    {
        sprintf(output_filename, "%s.ent", filename);
        fp = fopen(output_filename, "w");
        if(!fp)
        {
            printf("ERROR! Can not open .ent file\n");
            return;
        }

        /* loop through all symbols */
        for (curr_sym = symbol_table; curr_sym; curr_sym = curr_sym->next) 
        {
            /* if symbol is entry type - print its name and next to it its based 4 conversion */
            if (curr_sym->type.is_entry) 
            {
                convert_to_base4(curr_sym->address, base4_addr, ADDRESS_WORD_LENGTH);
                fprintf(fp, "%s\t%s\n", curr_sym->name, base4_addr);
            }
        }
        fclose(fp);
    }

    /* if extern were found - create the extern file */
    if (extern_list) 
    {
        sprintf(output_filename, "%s.ext", filename);
        fp = fopen(output_filename, "w");
        if(!fp)
        {
            printf("ERROR! Can not open .ext file\n");
            return;
        }

        /* loop through all the externs */
        for (ext_curr = extern_list; ext_curr; ext_curr = ext_curr->next) 
        {
            /* print extern names and next to it its base 4 conversion */
            convert_to_base4(ext_curr->address, base4_addr, ADDRESS_WORD_LENGTH);
            fprintf(fp, "%s\t%s\n", ext_curr->name, base4_addr);
        }
        fclose(fp);
    }
}






/* Converts a number to its special base 4 form without padding with 'a' */
void convert_to_base4_unpadded(unsigned int n, char *base4_str) 
{
    const char base4_chars[] = {'a', 'b', 'c', 'd'};
    char temp_str[TEMP_STR_BUFFER]; /* buffer for reversed string */
    int i = 0, j;

    /* Hhndle the case of 0 explicitly */
    if (n == 0) {
        base4_str[0] = 'a';
        base4_str[1] = '\0';
        return;
    }

    /* Convert to base 4 in reverse order */
    while (n > 0) {
        temp_str[i++] = base4_chars[n % BASE_4];
        n /= BASE_4;
    }

    /* reverse the temporary string to get the correct order */
    for (j = 0; j < i; j++) {
        base4_str[j] = temp_str[i - 1 - j];
    }
    base4_str[i] = '\0'; /* add '\0' */
}