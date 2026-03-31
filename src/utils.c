/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: utils.c                                          */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
   This is the utilities functions file. 
   It contains many different helper functions,
   that are used all over the project to do different types of tasks.
   Instead of writing the same code in many places, I put it here.
*/
/* ----------------------------------------------------------------------------------------------------------------------- */

#include "utils.h"
#include "pre_processor.h"
#include "symbol_table.h"
#include "first_pass.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Checks if line contains only white spaces */
int is_empty(const char* line)
{
    while(isspace((unsigned char)*line))
        line++;

    if(*line == '\0')
        return 1;
    else
        return 0;
}

/* Removes new line and carrige from the line. Trims leading spaces and ending spaces */
void clean_line(char* line)
{
    char* s = line,*e;
    /* erase new line char */
    if(strchr(s,'\n'))
        *strchr(s,'\n') = '\0';

    /* erase carrige char */
    if(strchr(s,'\r'))
        *strchr(s,'\r') = '\0';

    /* skip leading spaces */
    while(isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);

    /* ignore ending spaces */
    while(e>s&&isspace((unsigned char)*(e-1)))
        e--;
    *e = '\0';

    /* remove the clean line to the start */
    if(s!=line)
        memmove(line,s,strlen(s)+1);
}

/* Checks if line exceeds its maximum allowed length - and consumes the rest */
int line_too_long(FILE*fp,const char*line)
{
    int c;
    /* if there is a new line char, line is ok */
    if(strchr(line,'\n') != NULL)
        return 0;

    c = fgetc(fp);
    /* if there isn't a new line char, it can be the end of file or error */
    if(c == EOF)
        return 0;

    ungetc(c, fp); 
    /* line too long - consumes the rest */
    while ((c = fgetc(fp)) != '\n' && c != EOF);
    return 1;
}

/* Checks if a word is an assmebly reserved word - commands, direvtives, registers, ect. */
int is_reserved_word(const char*word)
{   
    const char *r[]= {"r0","r1","r2","r3","r4","r5","r6","r7", /* registers */
                    "mov","cmp","add","sub","not","clr","lea", /* instructions */
                    "inc","dec","jmp","bne","red","prn","jsr","rts",
                    "stop","data","string","mat","entry", "extern", /* directives */
                    "mcro","mcroend",NULL};

    int i=0;
    /* loop through all reservesd words */
    while(r[i])
    {
        if(strcmp(word,r[i++])==0)
            return 1;
    }
    return 0;
}

/* Checks if line is a directive - starts with a dot */
int is_directive(const char*word)
{
    if(word != NULL && *word == '.') 
        return 1;
    else 
        return 0;
}

/*
   Gets a command name and looks for it in the main command table. 
   If found, returns pointer to a struct that holds all the information about it 
*/
const command_details* get_command_info(const char *command_name) 
{
    /*
    The database of all assembly commands.
    This is a static array, so it's only created once. Each line defines
    one command with all its properties:
   {command_name, opcode, num_of_operands, {valid_source_modes}, {valid_destination_modes}}
    */

    /* 
    Note: I chose to define the opcodes and operand counts
    as literal numbers here because this table is the only use
    for them. The rest of the program gets these values by calling this function,
    so the numbers are not seen around the code.
    It felt cleaner than creating 16+ new defines in the constants file. 
    */
    static const command_details command_lookup_table[] = 
    {
        /* this array contains all instructions info:*/
        {"mov",  0,  2, {1,1,1,1}, {0,1,1,1}},
        {"cmp",  1,  2, {1,1,1,1}, {1,1,1,1}},
        {"add",  2,  2, {1,1,1,1}, {0,1,1,1}},
        {"sub",  3,  2, {1,1,1,1}, {0,1,1,1}},
        {"lea",  4,  2, {0,1,1,0}, {0,1,1,1}}, 
        {"clr",  5,  1, {0,0,0,0}, {0,1,1,1}},
        {"not",  6,  1, {0,0,0,0}, {0,1,1,1}},
        {"inc",  7,  1, {0,0,0,0}, {0,1,1,1}},
        {"dec",  8,  1, {0,0,0,0}, {0,1,1,1}},
        {"jmp",  9,  1, {0,0,0,0}, {0,1,1,1}}, 
        {"bne",  10, 1, {0,0,0,0}, {0,1,1,1}}, 
        {"jsr",  11, 1, {0,0,0,0}, {0,1,1,1}}, 
        {"red",  12, 1, {0,0,0,0}, {0,1,1,1}}, 
        {"prn",  13, 1, {0,0,0,0}, {1,1,1,1}}, 
        {"rts",  14, 0, {0,0,0,0}, {0,0,0,0}},
        {"stop", 15, 0, {0,0,0,0}, {0,0,0,0}},
        {NULL, -1, -1, {0,0,0,0}, {0,0,0,0}} 
    };
       

    int i = 0;
    if (!command_name)
        return NULL;

    while (command_lookup_table[i].name != NULL) 
    {
        if (strcmp(command_lookup_table[i].name, command_name) == 0) 
            return &command_lookup_table[i];
        i++;
    }
    return NULL;
}

/* Gets a string and returns pointer to the nuw cut array - doesn't change the original address like "clean line" */
char* trim_whitespace(char *str) 
{
    char *end;
    if (!str)
        return NULL;
    /* skip leading spaces */
    while(isspace((unsigned char)*str)) 
        str++;
    /* string empty after spaces */
    if(*str == 0) 
        return str;

    end = str + strlen(str) - 1;
    /* trim trailing white spaces */
    while(end > str && isspace((unsigned char)*end)) 
        end--;
    *(end+1) = 0;

    return str;
}

/* Splits the line to label, opcode and operands. Then, extracts it to the splitted line struct */
splitted_line split_line(char *line) 
{
    splitted_line result = {NULL, NULL, NULL, NO_SPLIT_ERROR};
    char *start = line;
    char *end;
    char* colon;
    char *first_space;
    char *newline_char;
    
    /* remove newline char from the end of the line*/
    newline_char = strchr(line, '\n');
    if (newline_char)
        *newline_char = '\0';
    /* remove leading and trailing whitespaces */
    start = trim_whitespace(start);
    
    /* line is empty - done*/
    if (*start == '\0')
        return result;

    /* handle the label if exists */
    colon = strchr(start, ':');
    first_space = strchr(start, ' ');
   /*  space between label name and its colon is illegal */
    if (colon != NULL && first_space != NULL && first_space < colon) 
    {
        *first_space = '\0'; 
        result.label = start;
        result.is_error = LABEL_SPACE_BEFORE_COLON; 
        return result;
    }

    if (colon != NULL) 
    {
        char next_char = *(colon + 1);
       /*  no whitespace after the colon is illegal */
        if (next_char != '\0' && !isspace((unsigned char)next_char)) 
        {
            *colon = '\0';
            result.label = start;
            result.is_error = LABEL_NO_SPACE_AFTER_COLON;
            return result;
        }
        /* label is valid, cut string at the colon */
        result.label = start;
        *colon = '\0';
        start = colon + 1;
    }
    /* work on line witout the label - remove leading and trailing spaces again */
    start = trim_whitespace(start);
    /* line ends after the label - done */
    if (*start == '\0') 
        return result;
    /* find command and its operands */
    end = start;
    while (*end != '\0' && !isspace((unsigned char)*end)) 
        end++;
    result.command = start;
    /* cut after the command, rest is operands */
    if (*end != '\0') 
    {
        *end = '\0';
        start = end + 1;
    } 
    else 
        start = end;

    start = trim_whitespace(start);
    if (*start != '\0') {
        result.operands = start;
    }
    return result;
}

/* Checks the addressing type of a given operand - immediate, direct, mat, register */
addressing_types get_addressing_mode(char *operand_str) 
{
    char *trimmed = trim_whitespace(operand_str); 
    if (!trimmed || *trimmed == '\0') 
        return INVALID_TYPE;
    /* type is immediate */
    if (trimmed[0] == '#')
        return IMMEDIATE; 
    
    /* if tyoe is register check format and valid range */
    /* 
    Note: Using literal numbers like [0], [1], and [2] here on purpose.
    This is not a magic number, but a direct check of character positions
    in the fixed format of registers. 
    */
    if (trimmed[0] == 'r' && isdigit(trimmed[1]) && trimmed[2] == '\0')     
    {
        int reg_num = trimmed[1] - '0';
        if (reg_num >= FIRST_REGISTER_NUM && reg_num <= LAST_REGISTER_NUM) 
            return REGISTER; /* tt's a valid register */
    }
    
    if (strchr(trimmed, '['))
        return MATRIX; 
    
    return DIRECT; 
}

/* In case of memory allocation error, function frees all allocated memory so far, and exit the programe */
void handle_error(const cleanup_context* ctx, const char* error_message)
{
    printf("ERROR: %s. Exitting program\n", error_message);
    if (ctx != NULL) 
    {
        if (ctx->mt != NULL) free_macro_table(ctx->mt); /* if macro table exists, free its memory */
        if (ctx->st != NULL && *(ctx->st) != NULL) free_symbol_table(*(ctx->st)); /* if symbol table exists, free its memory */
        if (ctx->am_file != NULL) fclose(ctx->am_file);  /* if temporary ".am" file was opened, close it. */
        if (ctx->am_filename != NULL) remove(ctx->am_filename);  /* if temporary ".am" file was created, delete it. */
    }
    exit(EXIT_FAILURE);
}

























