/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: pre_processor.c                                          */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
   This file handles the pre-assembler stage.
   Its main purpose is to scan the source code, find all macro definitions,
   and then expand every macro call by replacing it with the macro's content.
   It also reports errors in the macro definition stage if exist.
   This process creates the ".am" file, which is the input for the first pass.
*/
/* ----------------------------------------------------------------------------------------------------------------------- */

#include "pre_processor.h"
#include "constants.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>


/* Incharge of the pre-assembler process. Reads every line, creates macros table, 
and printing the ".am" file after expanding the macro calls */
int run_preprocessor(const char* input_file, const char* output_file, macro_table* macros, cleanup_context* ctx) 
{
    FILE *input, *output;
    char line[MAX_LINE_LEN + 1]; /* +1 for '\0' */
    int line_num = 0, errors = 0; /* will be used for error messages */
    macro* current_macro = NULL;
    int in_macro = 0; /* flag to indicate weather the current line read is inside macro definition or not */

    /* check if files were opened correctly */
    if (!(input = fopen(input_file, "r"))) 
    { 
        printf("ERROR: Cannot open file '%s'\n", input_file); 
        return ERROR; 
    }
    if (!(output = fopen(output_file, "w")))
    { 
        printf("ERROR: Cannot create file '%s'\n", output_file);
        fclose(input); /* input file was opened - close it */
        return ERROR; 
    }
    /* start read lines from the input file */
    while (fgets(line, sizeof(line), input)) 
    {
        char *semicolon; 
        line_num++; 
        /* check if line exceeds its maximum allowed length */
        if (line_too_long(input, line))
        {
            printf("File %s, Line %d: ERROR! Line is too long, maximum characters allowed is %d\n", input_file, line_num, MAX_LINE_LEN-1); /* -1 for '\0' */
            errors++;
            continue;
        }
        /* check if line is a comment line */
        semicolon = strchr(line, ';');
        if (line[0] == ';') 
            continue; /* skip comment line */
        if (semicolon != NULL && strstr(line, ".string") == NULL) 
        {
            /* semicolon must appear at the begginig of the line, unless it's inside a string */
            printf("File %s, Line %d: ERROR! Invalid comment placement.\n", input_file, line_num);
            errors++;
            continue;
        }
        
        clean_line(line); /* remove leading and trailing whitespaces */
        if (is_empty(line)) 
            continue; /* skip empty line */

        /* send the line to this function for further handling */
        handle_preproc_line(line, macros, output, input_file, line_num, &in_macro, &current_macro, &errors, ctx);
    }
    /* close files when done using them */
    fclose(input);
    fclose(output);
    if (errors > 0) /* if mistakes were found - report to user and return for main function there were errors */
    {
        printf("Pre processor has failed, found %d errors!\n", errors);
        return ERROR;
    }
    else 
        return SUCCESS;
}

/* Handle every sort of line accordingly. Check Start/end of macro, add macro or macro line if needed, prints to output file if needed */
void handle_preproc_line(char* line, macro_table* macros, FILE* output, const char* input_file, int line_num, 
                         int* in_macro, macro** current_macro, int* errors, cleanup_context* ctx)
{
    if (*in_macro) /* We are currently inside a macro definition */
    { 
        macro_end_status end_stat = is_macro_end(line, input_file, line_num); /* first check if line ends the macro definition */
        if (end_stat == VALID_MACROEND) 
        {
            *in_macro = 0; /* reset inside macro flag */
            *current_macro = NULL; /* finished with current macro */
        } 
        else if (end_stat == INVALID_MACROEND) 
            (*errors)++;
        
        else  /* end stat = NOT_MACROEND: it's a line inside the macro, so add it */
            add_line_to_macro(*current_macro, line, ctx);
    } 
    else  /* We are not inside a macro definition */
    {  
        char macro_name[MAX_MACRO_NAME_LEN];
        /* check if line starts a new macro definition */
        macro_start_status start_stat = valid_macro_declaration(macros, line, macro_name, input_file, line_num); 

        if (start_stat == VALID_MACROSTART) 
        {
            *current_macro = add_macro(macros, macro_name, ctx); /* add new macro to table */
            *in_macro = 1; /* update inside macro flag */
        } 
        else if (start_stat == INVALID_MACROSTART) 
            (*errors)++;
        else  /* start stat = NOT_MACROSTART: it's a regular line or a macro call */
        {
            char first_word[MAX_LINE_LEN];
            macro* m;
            sscanf(line, "%s", first_word); /* get first word of the line */
            m = name_already_exists(macros, first_word); /* check if first word is macro name, meaning the line calls for macro */
            if (m) 
            { /* it's a macro call - print its content to output file */
                if(strlen(first_word) == strlen(line)) 
                {
                    if (m->content)
                        fputs(m->content, output);
                } 
                else 
                {   /* check if there is extra text after macro call */
                    printf("File %s, Line %d: ERROR! Extra text after macro call '%s'.\n", input_file, line_num, first_word);
                    (*errors)++;
                }    
            }
            /* it's an ordinary line */ 
            else 
                fprintf(output, "%s\n", line);
        }
    }
}

/* Checks if line defines a new macro, and weather it's a valid definition */
macro_start_status valid_macro_declaration(macro_table* table, const char* line, char* name_out, const char* filename, int line_num) 
{
    const char* name_start; /* will point to start of macro name */
    size_t name_len = 0;
    int i; 
    /* first check if line starts with 'mcro' */
    if (strncmp(line, MCRO_STR, MCRO_LEN) != 0) 
        return NOT_MACROSTART;

    /* move pointer after 'mcro' */
    line += MCRO_LEN;

    /* line didn't end, and there is no whitespace after 'mcro' */
    if (*line && !isspace((unsigned char)*line)) 
    {
        printf("File %s, Line %d: ERROR! Space required after 'mcro'\n", filename, line_num);
        return INVALID_MACROSTART;
    }
    /* skip white spaces between 'mcro' and it's name */
    while (isspace((unsigned char)*line)) 
        line++;
        
    /* reached end of line - no name was declared */
    if (!*line) 
    {
        printf("File %s, Line %d: ERROR! Macro name is missing\n", filename, line_num);
        return INVALID_MACROSTART;
    }

    /* line points to start of macro's name */
    name_start = line;

    /* read the name to calculate its length */
    while (*line && !isspace((unsigned char)*line)) 
    {
        line++;
        name_len++;
    }
    /* check if name length is valid */
    if (name_len > MAX_MACRO_NAME_LEN)
    {
        printf("File %s, Line %d: ERROR! Macro name is too long. Max length allowed is %d characters.\n",filename, line_num, MAX_MACRO_NAME_LEN-1);
        return INVALID_MACROSTART;
    }

    /* copy the name to the output buffer - add null terminatior at the end */
    strncpy(name_out, name_start, name_len);
    name_out[name_len] = '\0';
    
    /* validate all characters of the macro name */
    if (!isalpha((unsigned char)name_out[0]))
    {
        printf("File %s, Line %d: ERROR! Macro name must start with a letter.\n", filename, line_num);
        return INVALID_MACROSTART;
    }
    for (i = 1; i < name_len; i++)
    {
        if (!isalnum((unsigned char)name_out[i]) && name_out[i] != '_')
        {
            printf("File %s, Line %d: ERROR! Macro name '%s' contains invalid character '%c'.\n", filename, line_num, name_out, name_out[i]);
            return INVALID_MACROSTART;
        }
    }
    
    /* check if name was used before */
    if(name_already_exists(table, name_out))
    {
        printf("File %s, Line %d: ERROR! Macro name '%s' is already defined\n", filename, line_num, name_out);
        return INVALID_MACROSTART;
    }
    /* check if name is an assembly reserved word */
    if (is_reserved_word(name_out)) 
    {
        printf("File %s, Line %d: ERROR! Macro name '%s' is a reserved word\n", filename, line_num, name_out);
        return INVALID_MACROSTART;
    }
    /* skip whitespaces after the name */
    while (isspace((unsigned char)*line)) 
        line++;

    /* didn't reach end of line - extra test after the name */
    if (*line) 
    {
        printf("File %s, Line %d: ERROR! Extra text after macro name\n", filename, line_num);
        return INVALID_MACROSTART;
    }
    return VALID_MACROSTART; /* valid start and name */
}

/* Checks if line is end of macro, and weather it is closed properly */
macro_end_status is_macro_end(const char* line, const char* filename, int line_num) 
{   
    /* check if line start with 'mcroend' */
    if(strncmp(line, MCROEND_STR, MCROEND_LEN) != 0) 
        return NOT_MACROEND;

    /* move the pointer after 'mcroend' */
    line += MCROEND_LEN;

    /* skip whitespaces after 'mcroend' */
    while (isspace((unsigned char)*line))
        line++;

    /* if didn't reach end of line - extra text */
    if(*line)
    {
        printf("File %s, Line %d: ERROR! Extra text after 'macroend'\n", filename, line_num);
        return INVALID_MACROEND;
    }
    /* valid macroend declaration */
    return VALID_MACROEND;
}

/* Checks if macro name was used before */
macro* name_already_exists(macro_table* table, const char* name) 
{
    size_t i;

    /*if it is the first macro - it's not used*/ 
    if (!table || !table->macros_arr) 
        return NULL;
    /* loop through all the names */
    for (i = 0; i < table->used; i++) 
    {
        if (strcmp(table->macros_arr[i].name, name) == 0) 
            return &table->macros_arr[i];
    }
    /* didn't find the name */
    return NULL;
}

/* Adds macro to the table and returns its pointer */
macro* add_macro(macro_table* table, const char* name, cleanup_context* ctx) 
{
    /* if macros array is full - reallocate memory */
    if (table->used == table->max) 
    {
        macro* p; 
        /* double the capacity */
        table->max *= GROWTH_FACTOR;
        p = realloc(table->macros_arr, table->max * sizeof(macro));
        if (!p) 
            handle_error(ctx, "Memory allocation for macros array"); /* free all allocated memory and exit the program */
        table->macros_arr = p;
    }

    /* add the new macro to the end of the macro array  */
    strcpy(table->macros_arr[table->used].name, name);
    table->macros_arr[table->used].content = NULL;      /* no content yet */
    table->macros_arr[table->used].content_size = 0;
    table->macros_arr[table->used].line_count = 0;
    
    return &table->macros_arr[table->used++];           /* return the new macro pointer and add one to 'used' */
}

/* Adds a new line to macro content */
void add_line_to_macro(macro* m, const char* line, cleanup_context* ctx) 
{
    size_t line_len = strlen(line);
    size_t old_size = m->content_size;
    char* new_content;

    /* reallocating memory in content for the new line */
    m->content_size += line_len + 1; /* +1 for newline character */
    /* reallocate memory for the new line */
    new_content = realloc(m->content, m->content_size);
    if (!new_content) 
        handle_error(ctx, "Memory allocation for macro content"); /* free all allocated memory and exit the program */
       
    /* add new line with '\n' and increase the line count by one */
    m->content = new_content;
    memcpy(m->content + old_size, line, line_len);
    m->content[m->content_size - 1] = '\n';
    m->line_count++;
}

/* Initializes macro table and allocates memory for it */
void init_macro_table(macro_table* t, cleanup_context* ctx) 
{
    t->used = 0; /* no macros yet */
    t->max = INITIAL_MACRO_CAPACITY;
    t->macros_arr = malloc(t->max * sizeof(macro)); /* allocate initial memory */
    if (!t->macros_arr) 
        handle_error(ctx, "Memory allocation failed for macro table"); /* free all allocated memory and exit the program */
}

/* Frees allocated memory for macro table and for each saved macro(content) */
void free_macro_table(macro_table* t) 
{
    size_t i;
    if (!t) 
        return;
    for (i = 0; i < t->used; i++)
         free(t->macros_arr[i].content); /* free all macros contents */
    free(t->macros_arr); /* free the macros array itself */
    /* reset the fields */
    t->macros_arr = NULL;
    t->used = 0;
    t->max = 0;
}


