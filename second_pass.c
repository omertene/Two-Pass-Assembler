/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: second_pass.c                                          */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
    This file contains all the functions for the second pass of the assembler.
    It runs after the first pass is finished and the symbol table is complete.
    Its main job is to go over the code again and translate everything
    into the final binary machine code, because now we can finally know
    the real address of all the labels. It creates the code image and the
    data image, and handles .entry and .extern directives.
*/
/* ----------------------------------------------------------------------------------------------------------------------- */

#include "second_pass.h"
#include "symbol_table.h"
#include "output_generator.h"
#include "utils.h"
#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* Processes the assembly file to complete the machine code generation, by resolving symbol addresses, handling entry directives, and encoding instructions and data into binary images for final output files. */
int run_second_pass(FILE *input_file, const char *filename, symbol_node *symbol_table, int icf, int dcf, int first_pass_succeeded, cleanup_context* ctx, int* errors) 
{
    int second_pass_success = 1; /* flag to indicate if second pass succeeded */
    machine_word code_image[MAX_PROGRAM_SIZE] = {0};
    machine_word data_image[MAX_PROGRAM_SIZE] = {0};
    extern_node *extern_list = NULL;
    int ic = CODE_START_IDX, dc = 0;

    /* handle entries, weather first pass failed or succeeded*/
    if (handle_entry_directives(input_file, symbol_table, errors) == ERROR) 
        second_pass_success = 0; /* error was found, second pass failed */
    
    if(first_pass_succeeded)
    {
        rewind(input_file);
        if (build_code_image(input_file, code_image, &ic, symbol_table, &extern_list, ctx, errors) == ERROR) 
            second_pass_success = 0; /* code image build failed, second pass failed */
            
        rewind(input_file); /* go back to start of file */        
        build_data_image(input_file, data_image, &dc);
        /* if both passes succeeded - generate output files */
        if (second_pass_success) 
            write_output_files(filename, icf, dcf, code_image, data_image, symbol_table, extern_list);
    }    
    /* free memory allocated for extern list */
    while(extern_list) 
    {
        extern_node* temp = extern_list;
        extern_list = extern_list->next;
        free(temp);
    }
    if(first_pass_succeeded && second_pass_success)
        return SUCCESS;
    else
        return ERROR;
}

/* validate all entry directives */
int handle_entry_directives(FILE *input_file, symbol_node *symbol_table, int* errors)
{
    char line[MAX_LINE_LEN];
    int line_number = 0;
    int entry_success = 1;

    /* go back to start of file */
    rewind(input_file);

    /* start reading lines */
    while(fgets(line, sizeof(line), input_file) != NULL) 
    {
        splitted_line splitted = split_line(line); /* split line to label, command, and operands */
        line_number++;

        if (splitted.command && strcmp(splitted.command, ".entry") == 0) /* entry directive line */
        {
            symbol_node* sym;
            if (splitted.operands == NULL) /* error - there is no label '.entry' decalared on */
            {
                printf("Line %d: ERROR! Missing label operand for .entry directive.\n", line_number);
                entry_success = 0;
                (*errors)++; 
                continue; /* continue to next line*/
            }

            sym = find_symbol(symbol_table, splitted.operands); /* make sure the label declared actually exists */
            if (sym)
            {
                if (sym->type.is_external) /* entry label was declared before as extern */
                {
                    printf("Line %d: ERROR! Symbol '%s' cannot be both extern and entry.\n", line_number, splitted.operands);
                    entry_success = 0;
                    (*errors)++; 
                }
                else
                    sym->type.is_entry = 1; /* change symbol 'is entry' flag */
            }
            else /* entry declared of an unexisting label */
            {
                printf("Line %d: ERROR! .entry for undefined symbol '%s'\n", line_number, splitted.operands);
                entry_success = 0;
                (*errors)++;
            }
        }
    }
    if(!entry_success)
        return ERROR;
    else
        return SUCCESS;
}

/* adds extern label to the extern list */
void add_to_extern_list(const char *name, int address, extern_node **extern_list, cleanup_context* ctx)
{
    /* allocate memory for new extern */
    extern_node *ext_node = (extern_node*)malloc(sizeof(extern_node));
    if (!ext_node) 
        handle_error(ctx, "Memory allocation for extern node"); /* free all allocated memory so far and exit program */
    
    /* copy extern's name and add it to the end of the list */
    strcpy(ext_node->name, name); 
    ext_node->address = address;
    ext_node->next = *extern_list;
    *extern_list = ext_node;
}

/* helps to encode one-operand instructions */
int encode_one_operand(splitted_line splitted, machine_word* code_image, int line_num, int* ic, symbol_node* symbol_table, extern_node** extern_list, cleanup_context* ctx)
{

    /* get the addressing mode of the single operand. */
    addressing_types dest_mode = get_addressing_mode(splitted.operands);
    int words_generated = 1, count = 0;

    /* update  main instruction word that was already created. beacuse destination mode bits are at positions 2 and 3, 
    shift the mode number left by 2 to place it correctly.*/
    code_image[(*ic) - CODE_START_IDX].bits |= (dest_mode << DEST_OPERAND_SHIFT);
    
     /* call the function to create the extra machine words needed for this operand. */
    if (encode_operand(splitted.operands, 0, &code_image[(*ic) - CODE_START_IDX + 1], &count, symbol_table, extern_list, (*ic) + 1, ctx, line_num) == ERROR) 
    {
        /* symbol was not found */
        printf("Line %d: ERROR! Undefined symbol used as operand: '%s'\n", line_num, trim_whitespace(splitted.operands));
        return ERROR;
    }
    /* update the ic by the number of words  created. */
    words_generated += count;
    (*ic) += words_generated;
    return SUCCESS; 
}

/* Helps to encode two-operand instructions */
int encode_two_operands(splitted_line splitted, machine_word* code_image, int line_num, int* ic, symbol_node* symbol_table, extern_node** extern_list, cleanup_context* ctx)
{
    char temp_operands[MAX_LINE_LEN];
    char *comma, *source_op, *dest_op;
    addressing_types source_mode, dest_mode;
    int words_generated = 1;
    int count1 = 0, count2 = 0;

    /* split sting to cource and destination */
    strcpy(temp_operands, splitted.operands);
    comma = strchr(temp_operands, ','); /* find the comma which seperates them */
    *comma = '\0';
    source_op = temp_operands;
    dest_op = comma + 1;
    /* get addressing mode for both operands */
    source_mode = get_addressing_mode(source_op);
    dest_mode = get_addressing_mode(dest_op);
    /* update first machine word with the mode bits, using the shifts defined. */
    code_image[(*ic) - CODE_START_IDX].bits |= (source_mode << SOURCE_OPERAND_SHIFT) | (dest_mode << DEST_OPERAND_SHIFT);
    
    /* in case both are register - encode to single word */
    if (source_mode == REGISTER && dest_mode == REGISTER) 
    {
        code_image[(*ic) - CODE_START_IDX + 1].bits = (atoi(trim_whitespace(source_op) + 1) << SOURCE_REGISTER_SHIFT) | (atoi(trim_whitespace(dest_op) + 1) << DEST_OPERAND_SHIFT);
        code_image[(*ic) - CODE_START_IDX + 1].are = ARE_ABSOLUTE;
        words_generated++;
    } 
    else 
    {
        if (encode_operand(source_op, 1, &code_image[(*ic) - CODE_START_IDX + 1], &count1, symbol_table, extern_list, (*ic) + 1, ctx, line_num) == ERROR) 
        {
            printf("Line %d: ERROR! Undefined symbol used as source operand: '%s'\n", line_num, trim_whitespace(source_op));
            return ERROR;
        }
        if (encode_operand(dest_op, 0, &code_image[(*ic) - CODE_START_IDX + 1 + count1], &count2, symbol_table, extern_list, (*ic) + 1 + count1, ctx, line_num) == ERROR) 
        {
            printf("Line %d: ERROR! Undefined symbol used as destination operand: '%s'\n", line_num, trim_whitespace(dest_op));
            return ERROR;
        }
        /* total words generated is  sum of words from both operands. */ 
        words_generated += count1 + count2;
    }
    /* update ic */
    (*ic) += words_generated;
    return SUCCESS; 
}

/* encodes a single assembly operand into one or more binary machine words, handling different addressing modes and resolving symbols according to the symbol table." */
int encode_operand(char *operand, int is_source, machine_word *words_arr, int *word_count,
     symbol_node *symbol_table, extern_node **extern_list, int current_ic, cleanup_context* ctx, int line_num) 
{
    /* clean line from whitespaces */
    char *clean_op = trim_whitespace(operand);
    addressing_types mode = get_addressing_mode(clean_op);
    symbol_node *sym;
    *word_count = 0;

    /* get the number after the '#', keep the last 8 bits using a mask,
           and shift it left to make space for the ARE bits. */
    if (mode == IMMEDIATE) 
    {   
        /* get the number after the '#', keep the last 8 bits using a mask,
           and shift it left to make space for the ARE bits. */
        words_arr[0].bits = (atoi(clean_op + 1) & IMMEDIATE_MASK) << ARE_SHIFT;
        words_arr[0].are = ARE_ABSOLUTE;
        *word_count = 1;
    }
    
    else if (mode == DIRECT) 
    {
        /* check if symbol exists */
        sym = find_symbol(symbol_table, clean_op);
        if (!sym)
            return ERROR;

        /* if symbol is extern - add to extern list, the bits are zeros */
        if (sym->type.is_external) 
        {
            add_to_extern_list(sym->name, current_ic, extern_list, ctx);
            words_arr[0].bits = 0; /* initial address for extern is 0 */
            words_arr[0].are = ARE_EXTERNAL;
        }
        /* local symbol, encode the address */ 
        else 
        {
            words_arr[0].bits = (sym->address << ARE_SHIFT);
            words_arr[0].are = ARE_RELOCATABLE;
        }
        *word_count = 1;

    }
    else if (mode == MATRIX)
    {
        char mat_name[MAX_LABEL_LEN];
        int reg_row, reg_col;

        /* break string into the matrix name and the register numbers. */
        if (sscanf(clean_op, "%[^[][r%d][r%d]", mat_name, &reg_row, &reg_col) != MATRIX_OPERAND_PARTS)
            return ERROR; 

        /* encode matrix address like normal address */
        sym = find_symbol(symbol_table, mat_name);
        if (!sym) 
        return ERROR;

        if (sym->type.is_external) 
        {
        add_to_extern_list(sym->name, current_ic, extern_list, ctx);
            words_arr[0].bits = 0;
            words_arr[0].are = ARE_EXTERNAL;
        } 
        else 
        {
            words_arr[0].bits = (sym->address << ARE_SHIFT);
            words_arr[0].are = ARE_RELOCATABLE;
        }

        /* second Word, shift the row register and the column register to their correct bits. */
        words_arr[1].bits = (reg_row << MATRIX_ROW_REG_SHIFT) | (reg_col << MATRIX_COL_REG_SHIFT);
        words_arr[1].are = ARE_ABSOLUTE;
        
        *word_count = MATRIX_OPERAND_WORDS; /* This operand created 2 machine words */
    }



    
     else if (mode == REGISTER) 
     {
         /* check if this is a source or destination operand to know how much to shift */
         if (is_source) 
             /* if it's a source operand, shift its number to the left by 6 bits */
             words_arr[0].bits = (atoi(clean_op + 1) << SINGLE_SOURCE_REG_SHIFT);
         else 
             /* if it's a destination operand, shift its number to the left by 2 bits */
             words_arr[0].bits = (atoi(clean_op + 1) << SINGLE_DEST_REG_SHIFT);
         words_arr[0].are = ARE_ABSOLUTE;
         *word_count = 1;
     }
     return SUCCESS;
}

/* Encodes instructions and builds the code image */
int build_code_image(FILE *input_file, machine_word code_image[], int *ic, symbol_node *symbol_table, extern_node **extern_list, cleanup_context* ctx, int* errors)
{
    char line[MAX_LINE_LEN];
    int line_num = 0;
    int lines_ok = 1;
    rewind(input_file);

    /* start reading lines */
    while(fgets(line, sizeof(line), input_file) != NULL) 
    {
        const command_details *cmd_info;
        splitted_line splitted;
        line_num++;
        splitted = split_line(line); /* split line to label,command, and operands */
        

        if (!splitted.command || splitted.command[0] == '.') /* directive line - continue to next line */
            continue;

        cmd_info = get_command_info(splitted.command); /* find which specific command it is */
        if (!cmd_info) 
            continue;
        
        /* create first machine word which contains the opcode, 
        and the addressing modes will be added by the helpers. */
        code_image[(*ic) - CODE_START_IDX].bits = (cmd_info->opcode << OPCODE_SHIFT); 
        code_image[(*ic) - CODE_START_IDX].are = ARE_ABSOLUTE;
        
        if (cmd_info->expected_operands == TWO_OPERANDS) 
        {
            if (encode_two_operands(splitted, code_image, line_num, ic, symbol_table, extern_list,ctx) == ERROR)
            {
                (*errors)++;
                lines_ok = 0;
            }
                
        } 
        else if (cmd_info->expected_operands == ONE_OPERAND) 
        {
            if (encode_one_operand(splitted, code_image, line_num, ic, symbol_table, extern_list,ctx) == ERROR)
            {
                (*errors)++;
                lines_ok = 0;
            } 
        } 
        else  /* zero operands */
            (*ic)++;
        
    }

    if (lines_ok)
        return SUCCESS;
    else
        return ERROR;
}

/* Function incharge of building the data image */
void build_data_image(FILE *input_file, machine_word data_image[], int *dc)
{
    char line[MAX_LINE_LEN+1];
    rewind(input_file);

    /* start reading lines */
    while(fgets(line, sizeof(line), input_file) != NULL) 
    {
        splitted_line splitted = split_line(line); /* split for label, command, and operands */
        if (splitted.command && splitted.operands) 
        {
            if (strcmp(splitted.command, ".data") == 0) 
            {
                char *token = strtok(splitted.operands, ", \t\n");
                while (token) 
                {
                    /* convert each number token to integer and store its 10 bits. */
                    data_image[(*dc)++].bits = atoi(token) & TEN_BIT_MASK;
                    token = strtok(NULL, ", \t\n");
                }
            } 
            else if (strcmp(splitted.command, ".string") == 0) 
            {
                /* find first quote to know where string starts */
                char *p = strchr(splitted.operands, '"');
                if (p) 
                {
                    p++;
                    while (*p != '"' && *p) /* copy string into data image. */
                        data_image[(*dc)++].bits = *p++;
                    data_image[(*dc)++].bits = '\0';
                }
            }
             else if (strcmp(splitted.command, ".mat") == 0)
             {
                char temp_operands[MAX_LINE_LEN];
                char *p;
                char *endptr;
                int r = 0, c = 0, i, size;

                strcpy(temp_operands, splitted.operands);
                p = temp_operands;

                /* find the dimensions */
                p = trim_whitespace(p);
                if (*p == '[') 
                    p++;
                p = trim_whitespace(p);
                r = strtol(p, &endptr, DECIMAL_BASE);
                p = trim_whitespace(endptr);
                if (*p == ']') 
                    p++;
                p = trim_whitespace(p);
                if (*p == '[') 
                    p++;
                p = trim_whitespace(p);
                c = strtol(p, &endptr, DECIMAL_BASE);
                p = trim_whitespace(endptr);
                if (*p == ']') p++;
                
                size = r * c;

                p = trim_whitespace(p); /* find the start of the numbers list */
                for(i = 0; i < size; i++)
                {
                    if (*p != '\0')
                    {
                        data_image[(*dc)++].bits = strtol(p, &endptr, DECIMAL_BASE) & TEN_BIT_MASK;
                        p = endptr;
                        p = trim_whitespace(p);
                        if (*p == ',') p++;
                    }
                    else /* no more numbers, fill the rest with zeros. */
                        data_image[(*dc)++].bits = 0;
                    
                }
            }
        }
    }
}


