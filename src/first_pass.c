/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: first_pass.c                                          */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
   This file handles the first pass of the assembler.
   Its main job is to read the ".am" file after the preprocessor,
   build the symbol table with all the labels, and find out the final
   IC and DC values. It also does a lot of error checking on the lines
   to make sure the syntax is correct, before the second pass starts. 
*/
/* ----------------------------------------------------------------------------------------------------------------------- */
#include "first_pass.h"
#include "symbol_table.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


/* Incharge of the first pass, reads every line, detecting its type, saves label if exist, updates data and image tables */
int run_first_pass(FILE *input_file, symbol_node **symbol_table, int *icf, int *dcf, macro_table* mac_table, cleanup_context* ctx, int* errors) 
{
    char line[MAX_LINE_LEN+1];
    int line_num = 0; /* will be used for error messages */
    int ic = CODE_START_IDX; /* first address of code */
    int dc = 0;    
    int success = 1; /* flag to indicate success or failure */
    
    *errors = 0;
    /* read every line from the file */
    while (fgets(line, sizeof(line), input_file) != NULL) 
    {
        splitted_line splitted;
        line_num++;
        
        /* distinguish between label, opcode, and operands */
        splitted = split_line(line);

        /* send the line to this function for further handling */
        handle_first_pass_line(splitted, symbol_table, &ic, &dc, line_num, mac_table, &success, errors, ctx);
    }
    /* too many word for memory */
    if ((ic + dc) > MAX_MEMORY) 
    {
        printf("ERROR! Program size exceeds available memory. Final address would be %d, but maximum is %d.\n", (ic + dc - 1), MAX_MEMORY-1); 
        (*errors)++;
        success = 0;
    }

    /* update the final dc and ic */
    *icf = ic;
    *dcf = dc;
    update_data_symbol_addresses(*symbol_table, *icf); /* update symbol addresses with the final correct address */
    if (success) 
        return SUCCESS;
    else
        return ERROR;
}

/* Processes a single line during the first pass */
void handle_first_pass_line(splitted_line splitted, symbol_node **symbol_table, int *ic, int *dc, int line_num, macro_table* mac_table, int *success, int *errors, cleanup_context* ctx)
{
    /* ingore empty lines and comments - saftey check */
    if (splitted.label == NULL && splitted.command == NULL)
        return;

    /* in case there were errors while splitting the line to parts */
    if (splitted.is_error) 
    {
        if (splitted.is_error == LABEL_SPACE_BEFORE_COLON)  
            printf("Line %d: ERROR! Whitespace between label ('%s') and colon is forbidden.\n", line_num, splitted.label);

        else if (splitted.is_error == LABEL_NO_SPACE_AFTER_COLON)  
            printf("Line %d: ERROR! Missing whitespace after the colon for label '%s'.\n", line_num, splitted.label);
        
        (*errors)++;
        *success = 0;
        return;
    }

    /* process label if defined */
    if (splitted.label != NULL) 
        process_label_definition(splitted, symbol_table, *ic, *dc, line_num, mac_table, success, errors, ctx);
    
    /* process commend part of the line */
    if (splitted.command) 
    {
        if (splitted.command[0] == '.') /* it's a directive */
        { 
            directive_type dir_type = get_directive_type(splitted.command); /* find which directive and process it accordingly */
            switch (dir_type)
            {
                case DATA_DIRECTIVE:
                    if(process_data_dir(splitted.operands, dc, line_num) == ERROR) 
                    { 
                        (*errors)++; 
                        *success = 0; 
                    } break;
                case STRING_DIRECTIVE:
                    if (process_string_dir(splitted.operands, dc, line_num) == ERROR) 
                    { 
                        (*errors)++; 
                        *success = 0; 
                    } break;
                case MAT_DIRECTIVE:
                    if(process_mat_dir(splitted.operands, dc, line_num) == ERROR) 
                    { 
                        (*errors)++; 
                        *success = 0; 
                    } break;
                case EXTERN_DIRECTIVE:
                    if (splitted.operands) 
                    {
                        symbol_type sym_type = {0, 0, 1, 0};
                        if(add_symbol(symbol_table, splitted.operands, 0, sym_type, line_num, mac_table, ctx) == ERROR) /* invalid label after .extern */
                        { 
                            (*errors)++; 
                            *success = 0; 
                        }
                    } 
                    else /* no label after '.extern' - print error  */
                    {
                        printf("Line %d: ERROR! Missing label after .extern directive.\n", line_num);
                        (*errors)++; 
                        *success = 0;
                    } break;
                case UNKNOWN_DIRECTIVE:
                {
                    printf("Line %d: ERROR! Unknown directive '%s'\n", line_num, splitted.command);
                    (*errors)++; 
                    *success = 0;
                } break;
                case ENTRY_DIRECTIVE: /* .entry is currently ignored */
                    break;
            }
            
        } 
        else  /* it's an instruction */
        {    
            int words = calculate_instruction_words(splitted, line_num); /* process the instrtuction */
            if (words < 0) /* found an error */
            {
                 (*errors)++;
                 *success = 0;
            } 
            else 
                (*ic) += words;
        }
    }
    else if (splitted.label != NULL) /* label exists without a command after */
    {
        printf("Line %d: ERROR! Missing command after label '%s'.\n", line_num, splitted.label);
        (*errors)++;
        *success = 0;
    }
}

/* Gets a directive and return which specific directive it is */
directive_type get_directive_type(const char *command) 
{
    if (!command) 
        return UNKNOWN_DIRECTIVE;
    if (strcmp(command, ".data") == 0) 
        return DATA_DIRECTIVE;
    if (strcmp(command, ".string") == 0) 
        return STRING_DIRECTIVE;
    if (strcmp(command, ".mat") == 0) 
        return MAT_DIRECTIVE;
    if (strcmp(command, ".entry") == 0) 
        return ENTRY_DIRECTIVE;
    if (strcmp(command, ".extern") == 0) 
        return EXTERN_DIRECTIVE;

    /* none of the above */
    return UNKNOWN_DIRECTIVE;
}

/* Processes '.data', updates dc, prints error if needed */
int process_data_dir(const char *operands, int *dc, int line_num)
{
    const char *p;
    int count = 0; /* counter of valid numbers */
    long num_val;
    char *endptr; /* for 'strtol' - pointer to end of number */
    
    /* no operands - empty .data declaration*/
    if (operands == NULL)
    {
        printf("Line %d: ERROR! Missing numbers after .data\n", line_num);
        return ERROR;
    }
    p = operands;

    /* skip whitespaces */
    while (isspace((unsigned char)*p))
        p++;
    /* still an empty declaration */
    if (operands == NULL)
    {
        printf("Line %d: ERROR! Missing numbers after .data\n", line_num);
        return ERROR;
    } 

    /* loop through operands string */
    while (*p != '\0')
    {   
        /* expect to see a number */
        if (*p == '+' || *p == '-')     /* optional sign - skip it */
            p++;

        /* must have at least one digit after the sign */
        if (!isdigit((unsigned char)*p))
        {
            printf("Line %d: ERROR! Expected a number after .data instead of '%c'\n", line_num, *p);
            return ERROR;
        }

        num_val = strtol(p, &endptr, DECIMAL_BASE);    /* get the first number's value */
        /* make sure value number is in valid range */
        if (num_val < MIN_10_BIT_VAL || num_val > MAX_10_BIT_VAL)
        {
            printf("Line %d: ERROR! Number %ld is out of range (%d to %d).\n", line_num, num_val, MIN_10_BIT_VAL, MAX_10_BIT_VAL);
            return ERROR;
        }

        /* skip the number */
        while(isdigit((unsigned char)*p)) 
            p++;
            
        /* valid number found, increase count */
        count++; 

        /* expect comma or end of line */
        while(isspace((unsigned char)*p)) /* skip whitespaces after the number */
            p++;

        /* finished reading the numbers */
        if (*p == '\0') 
            break;

        if (*p == ',') /* found a comma */
        {
            p++; /* skip comma */
            while(isspace((unsigned char)*p)) /* skip whitespaces after comma */
                p++;

            /* line ends with comma - error */
            if (*p == '\0')
            {
                printf("Line %d: ERROR! Trailing comma at the end of .data declaration.\n", line_num);
                return ERROR;
            }
        }
        else
        {
            /* not the end and not a comma - it's an error */
            printf("Line %d: ERROR! Missing comma or there is invalid character between numbers.\n", line_num);
            return ERROR;
        }
    }

    /* proces completed successfully - add operands number to count */
    *dc += count;
    return SUCCESS;
}

/* Processes '.string', updates dc, prints error if needed */
int process_string_dir(const char *operands, int *dc, int line_num)
{
    char temp_operands[MAX_LINE_LEN + 1]; /* buffer for operands copy - for safty */
    char *trimmed_operands; /* pointer to the end of opernads - before whitespaces */
    int len, i;

    if (operands == NULL || operands[0] == '\0') /* no string was found */
    {
        printf("Line %d: ERROR! Missing string operand after '.string'.\n", line_num);
        return ERROR;
    }

    /* copy the operands to the array */
    strcpy(temp_operands, operands);
    trimmed_operands = trim_whitespace(temp_operands);
    len = strlen(trimmed_operands);

    /* string has to be at least two characters - first and last characters are double quotes */
    if (len < QUOTES_NUM || trimmed_operands[0] != '"' || trimmed_operands[len - 1] != '"')
    {
        printf("Line %d: ERROR! String operand must be properly enclosed in double quotes.\n", line_num);
        return ERROR;
    }
    /* string framing is ok - check the characters between the double quotes */
    for (i = 1; i < len - 1; i++) 
    {
        /* check if current character is within the valid range allowed */
        if (trimmed_operands[i] < ASCII_MIN_PRINT || trimmed_operands[i] > ASCII_MAX_PRINT) 
        {
            printf("Line %d: Error! Invalid character in string. Only printable ASCII (%d-%d) is allowed.\n", line_num, ASCII_MIN_PRINT, ASCII_MAX_PRINT);
            return ERROR;
        }
    }
    /* string is ok - add each character to dc. */
    *dc += (len - QUOTES_NUM) + 1; /* -2 for the tow double quotes, +1 for '\0' */ 

    return SUCCESS;
}

/* Processes '.mat', updates dc, prints error if needed */
int process_mat_dir(const char *operands, int *dc, int line_num)
{
    long rows = 0, cols = 0;
    int size = 0, count = 0;
    const char *p;
    char *endptr; /* for 'strtol' */
    long num_val;

    /* no operands - empty '.mat' declaration */
    if (operands == NULL || *operands == '\0') 
    {
        printf("Line %d: ERROR! Missing matrix definition after '.mat' directive.\n", line_num);
        return ERROR;
    }
    p = operands;
    while (isspace((unsigned char)*p)) /* skip whitespaces */
        p++;

     /* call this function to handle checking matrix dimentions */
     if (parse_mat_dimensions(&p, &rows, &cols, line_num) == ERROR) 
        return ERROR;

    size = (int)rows * (int)cols; /* total numbers of values for the matrix */
    /* whitespace must come after the dimentions */
    if (*p != '\0' && !isspace((unsigned char)*p)) 
    {
        printf("Line %d: ERROR! Missing whitespace between matrix dimensions and initializer list.\n", line_num);
        return ERROR;
    }
    while (isspace((unsigned char)*p)) 
        p++;

    if (*p == '\0') /* matrix with no values - intialized with zeros */
    {
        *dc += size;
        return SUCCESS;
    }

    while (*p != '\0') 
    {
        if (count >= size) /* check if matrix was initialized with too many values */
        {
            printf("Line %d: ERROR! Too many initializers for matrix of size %ldx%ld.\n", line_num, rows, cols);
            return ERROR;
        }
        num_val = strtol(p, &endptr, DECIMAL_BASE); /* get number */
        if (p == endptr) 
        {
            printf("Line %d: ERROR! Invalid value in initializer list.\n", line_num);
            return ERROR;
        }
        if (num_val < MIN_10_BIT_VAL || num_val > MAX_10_BIT_VAL) 
        {
            printf("Line %d: ERROR! Matrix initializer value %ld is out of range (%d to %d).\n", line_num, num_val, MIN_10_BIT_VAL, MAX_10_BIT_VAL);
            return ERROR;
        }
        count++; /* valid number was added */
        p = endptr;
        while (isspace((unsigned char)*p)) 
            p++;
        if (*p == ',') /* expect another number */
        {
            p++;
            while (isspace((unsigned char)*p)) 
                p++;
            if (*p == '\0') /* line ends without a number after comma */
            {
                printf("Line %d: ERROR! Trailing comma in initializer list.\n", line_num);
                return ERROR;
            }
        } 
        else if (*p != '\0') 
        {
            printf("Line %d: ERROR! Missing comma or invalid text between numbers in initializer list.\n", line_num);
            return ERROR;
        }
    }
    
    *dc += size; /* valid matrix declaration - update dc */
    return SUCCESS;
}

/* Checks if matrix dimention in '.mat' are valid */
int parse_mat_dimensions(const char **operands, long *rows_out, long *cols_out, int line_num)
{
    const char *p = *operands;
    char *endptr;

    /* read dimentions - expect a opening bracket */
                     /* rows */
    if (*p != '[') 
    {
        printf("Line %d: ERROR! Invalid matrix definition format. Expected '[' at the beginning.\n", line_num);
        return ERROR;
    }
    p++;
    while (isspace((unsigned char)*p)) /* skip whitespaces after '[' */
        p++;
    
    *rows_out = strtol(p, &endptr, DECIMAL_BASE); /* get rows dimention */
    if (p == endptr) 
    {
        printf("Line %d: ERROR! Matrix row dimension must be an integer.\n", line_num);
        return ERROR;
    }
    if (*rows_out <= 0) 
    {
        printf("Line %d: ERROR! Matrix row dimension must be a positive integer, but got %ld.\n", line_num, *rows_out);
        return ERROR;
    }
    p = endptr;
    while (isspace((unsigned char)*p)) 
        p++;
    
    if (*p != ']') /* finished reading number - expect closing bracket */
    {
        printf("Line %d: ERROR! Invalid matrix definition format. Expected ']' after row dimension.\n", line_num);
        return ERROR;
    }
    p++;
                /* cols */

    if (*p != '[') /* columns opening bracket must be next to rows closing bracket */
    {
        printf("Line %d: ERROR! Invalid matrix definition format. Expected '[' right after rows closing bracket and before column dimension.\n", line_num);
        return ERROR;
    }
    p++;
    while (isspace((unsigned char)*p)) /* skip whitespaces after '[' */
        p++;

    *cols_out = strtol(p, &endptr, DECIMAL_BASE); /* get columns number */
    if (p == endptr) 
    {
        printf("Line %d: ERROR! Matrix column dimension must be an integer.\n", line_num);
        return ERROR;
    }
    if (*cols_out <= 0) 
    {
        printf("Line %d: ERROR! Matrix column dimension must be a positive integer.\n", line_num);
        return ERROR;
    }
    p = endptr;
    while (isspace((unsigned char)*p)) 
        p++;
    
    if (*p != ']') 
    {
        printf("Line %d: ERROR! Invalid matrix definition format. Expected ']' after column dimension.\n", line_num);
        return ERROR;
    }
    p++;

    *operands = p; /* update the original pointer to point after the dimensions */
    return SUCCESS;
}

/* Calculate the number of words an instruction will add to code */
int calculate_instruction_words(splitted_line splitted, int line_num) 
{
    const command_details *cmd_dets = get_command_info(splitted.command); /* get the specific command */
    
    if (!cmd_dets) 
    {
        printf("Line %d: ERROR! Unknown instruction '%s'.\n", line_num, splitted.command);
        return ERROR;
    }

    if (cmd_dets->expected_operands == NO_OPERANDS) 
    {
        if (splitted.operands != NULL) /* there is an unexpected operand */
        {
            printf("Line %d: ERROR! Instruction '%s' does not take any operands.\n", line_num, splitted.command);
            return ERROR;
        }
        return SUCCESS;
    }
    else if (cmd_dets->expected_operands == ONE_OPERAND) /* call for the function which handles one operand instructions */
        return calc_one_operand_words(splitted, cmd_dets, line_num);

    else if (cmd_dets->expected_operands == TWO_OPERANDS) /* call for the function which handles two operands instructions */
        return calc_two_operands_words(splitted, cmd_dets, line_num);
    
    return ERROR; /* should not be reached */
}

/* Calculate words for one-operand instructions */
int calc_one_operand_words(splitted_line splitted, const command_details *cmd_dets, int line_num)
{
    char op[MAX_LINE_LEN], extra[MAX_LINE_LEN];
    addressing_types dest_mode; /* one opeand is always the destination  */

    if (splitted.operands == NULL) /* no operands were found */
    {
        printf("Line %d: ERROR! Missing operand for instruction '%s'.\n", line_num, cmd_dets->name);
        return ERROR;
    }
    if (sscanf(splitted.operands, "%s %s", op, extra) != ONE_OPERAND) 
    {
        printf("Line %d: ERROR! Extra text after operand for instruction '%s'.\n", line_num, cmd_dets->name);
        return ERROR;
    }
    dest_mode = get_addressing_mode(op); /* check if destination addressing type is valid */
    if (dest_mode < 0 || !cmd_dets->valid_dest_modes[dest_mode]) 
    {
        printf("Line %d: ERROR! Invalid destination addressing mode for '%s'.\n", line_num, cmd_dets->name);
        return ERROR;
    }
    if (dest_mode == IMMEDIATE) /* immediate mode - only 8 bits for representation */
    {     
        long num_val = strtol(op + 1, NULL, DECIMAL_BASE); /* get number and check if it's in range */
        if (num_val < MIN_8_BIT_VAL || num_val > MAX_8_BIT_VAL) 
        {
            printf("Line %d: ERROR! Immediate value %ld is out of 8-bit range (%d to %d).\n", line_num, num_val, MIN_8_BIT_VAL, MAX_8_BIT_VAL);
            return ERROR;
        }
    }
    if (dest_mode == MATRIX && is_valid_matrix_syntax(op, line_num, "destination") == ERROR) 
        return ERROR;

    /* return the words for operand, +1 for the opcode itself */
    if(dest_mode == MATRIX)
        return TWO_OPERANDS + 1;
    else
        return ONE_OPERAND + 1;
}

/* Calculate words for two-operand instructions */
int calc_two_operands_words(splitted_line splitted, const command_details *cmd_dets, int line_num)
{
    char src_op[MAX_LINE_LEN], dest_op[MAX_LINE_LEN], extra[MAX_LINE_LEN];
    addressing_types source_mode, dest_mode;
    int words = 0;

    if (splitted.operands == NULL) /* no operands were found */
    {
        printf("Line %d: ERROR! Missing operands for instruction '%s'.\n", line_num, cmd_dets->name);
        return ERROR;
    }
    if (sscanf(splitted.operands, " %[^, \t] , %s %s", src_op, dest_op, extra) != TWO_OPERANDS) 
    {
        printf("Line %d: ERROR! Invalid format or extra text for instruction '%s'. Expected: op1, op2\n", line_num, cmd_dets->name);
        return ERROR;
    }
    if (strchr(dest_op, ',') != NULL) /* no comma should be found after the second operand */
    {
        printf("Line %d: ERROR! Too many operands for instruction '%s'.\n", line_num, cmd_dets->name);
        return ERROR;
    }

    /* get source and destination addressing types */
    source_mode = get_addressing_mode(src_op);
    dest_mode = get_addressing_mode(dest_op);

    if (source_mode == IMMEDIATE) /* immediate mode - only 8 bits for representation */ 
    {
        long num_val = strtol(src_op + 1, NULL, DECIMAL_BASE);
        if (num_val < MIN_8_BIT_VAL || num_val > MAX_8_BIT_VAL) 
        {
            printf("Line %d: ERROR! Immediate source value %ld is out of 8-bit range (%d to %d).\n", line_num, num_val, MIN_8_BIT_VAL, MAX_8_BIT_VAL);
            return ERROR;
        }
    }
    if (dest_mode == IMMEDIATE && strcmp(cmd_dets->name, "cmp") != 0) /* cmp is the only two operand instruction which can get immediate addresing in its destination */
    {
        printf("Line %d: ERROR! Immediate addressing mode is not valid for a destination operand in '%s'.\n", line_num, cmd_dets->name);
        return ERROR;
    }
    if (source_mode < 0 || !cmd_dets->valid_source_modes[source_mode]) 
    {
        printf("Line %d: ERROR! Invalid source addressing mode for '%s'.\n", line_num, cmd_dets->name);
        return ERROR; 
    }
    if (dest_mode < 0 || !cmd_dets->valid_dest_modes[dest_mode]) 
    { 
        printf("Line %d: ERROR! Invalid destination addressing mode for '%s'.\n", line_num, cmd_dets->name);
        return ERROR; 
    }
    if (source_mode == MATRIX && is_valid_matrix_syntax(src_op, line_num, "source") == ERROR) 
        return ERROR;

    if (dest_mode == MATRIX && is_valid_matrix_syntax(dest_op, line_num, "destination") == ERROR) 
        return ERROR;

    words = 1; /* כor the instruction itself */
    if (source_mode == REGISTER && dest_mode == REGISTER) /* two registers require only one word */
        words += 1;
    else /* update words - add operands number */
    {
        if(source_mode == MATRIX)
            words += TWO_OPERANDS;
        else
            words += ONE_OPERAND;
            
        if(dest_mode == MATRIX)
            words += TWO_OPERANDS;
        else
            words += ONE_OPERAND;
    }
    return words;
}

/* Checks if matrix addressing type was written correctly */
int is_valid_matrix_syntax(const char* operand, int line_num, const char* operand_type)
{
    int r1 = -1, r2 = -1, chars_consumed = 0;

    /* strict valid syntax: must match LABEL[r#][r#] with no extra text - only registers are valid inside the brackets */
    if (sscanf(operand, "%*[^[][r%d][r%d]%n", &r1, &r2, &chars_consumed) == NUM_OF_MAT_DIMENTIONS && operand[chars_consumed] == '\0') 
    {
        if (r1 >= FIRST_REGISTER_NUM && r1 <= LAST_REGISTER_NUM && r2 >= FIRST_REGISTER_NUM && r2 <= LAST_REGISTER_NUM)
            return SUCCESS; /* Valid syntax and valid registers */
        else 
        {
            printf("Line %d: ERROR! Invalid register number in %s operand '%s'.\n", line_num, operand_type, operand);
            return ERROR;
        }
    }
    
    printf("Line %d: ERROR! Invalid matrix syntax for %s operand '%s'.\n", line_num, operand_type, operand);
    return ERROR;
}

/* Determents the label's type, and adds it to the symbol table if it's valid */
void process_label_definition(splitted_line splitted, symbol_node **symbol_table, int ic, int dc, int line_num, macro_table* mac_table, int *success, int *errors, cleanup_context* ctx)
{
    /* initate symbol type as null */
    symbol_type sym_type = {0, 0, 0, 0};

    /* if before the command there is a dot - it's a data directive label */
    if (splitted.command && splitted.command[0] == '.')
    {
        sym_type.is_data = 1;
        if (add_symbol(symbol_table, splitted.label, dc, sym_type, line_num, mac_table, ctx) == ERROR)
        {
            (*errors)++;
            *success = 0;
        }
    }
    else /* if not directive then it is a code symbol */
    {
        sym_type.is_code = 1;
        if (add_symbol(symbol_table, splitted.label, ic, sym_type, line_num, mac_table, ctx) == ERROR)
        {
            (*errors)++;
            *success = 0;
        }
    }
}








