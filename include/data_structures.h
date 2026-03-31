
/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: data_structures.h                                         */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
    This file is like a dictionary for the whole project.
    It doesn't have any functions, but it defines all the main data
    structures and enums that we use in the other files.
*/
/* ----------------------------------------------------------------------------------------------------------------------- */


#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "constants.h"

#include <stdio.h>


/* An enum for all the addressing mode types. */
typedef enum
{
    INVALID_TYPE = -1,
    IMMEDIATE,
    DIRECT,
    MATRIX,
    REGISTER
}addressing_types;

/* AN enum to mark all symbol types */
typedef enum 
{ 
    UNKNOWN_DIRECTIVE = -1,
    DATA_DIRECTIVE, 
    STRING_DIRECTIVE, 
    MAT_DIRECTIVE, 
    ENTRY_DIRECTIVE, 
    EXTERN_DIRECTIVE
    
}directive_type;

/* An enum for the error codes that split_line can return */
typedef enum 
{
    NO_SPLIT_ERROR,
    LABEL_SPACE_BEFORE_COLON, /* LABEL : */
    LABEL_NO_SPACE_AFTER_COLON /* "LABEL:text" */
} split_error_code;


/* A small struct with flags to tell us what kind of symbol we have. */
typedef struct 
{ 
    int is_code, 
    is_data, 
    is_external, 
    is_entry; 
}symbol_type;

/* Linked list for symbols. it holds the symbol's name, current address, type, and pointer the next one in list */
typedef struct symbol_node
{ 
    char name[MAX_LABEL_LEN];
    int address; 
    symbol_type type; 
    struct symbol_node *next;
 }symbol_node;

/* struct for assmebly line - points the label if exists, to the command start, and to start of the operands. Is error - in case
line is not valid */
typedef struct 
{ 
    char *label, *command, *operands;
    split_error_code is_error; 
}splitted_line;

/* Struct represents a machine word - consists of a 10-bit field (bits) for storing data or instructions,
 and an integer field (are) to hold information about the word's addressing type.
*/
typedef struct 
{ 
    unsigned int bits: 10; 
    int are; 
} machine_word;

/* Linked list for all the externs symbols. Holds the symbol's name, its current address, and pointer to the next one */
typedef struct extern_node 
{ 
    char name[MAX_LABEL_LEN]; 
    int address; 
    struct extern_node *next; 
}extern_node;

/* Struct for each macro - it holds a pointer to its content, macro's name, size of its content, and number of lines in it. */
typedef struct 
{
    char name[MAX_LABEL_LEN];
    char* content;
    size_t content_size;
    int line_count;
} macro;

/* Struct for table of macros. It hold pointer to all existing macros, used - how much macros are there,
max - the allowed capacity for the macros array, can grow by reallocating memory*/
typedef struct 
{
    macro* macros_arr;
    size_t used;
    size_t max;
} macro_table;

/* A struct to hold pointers to all major allocated resources for easy memory cleanup */
typedef struct cleanup_context 
{
    macro_table* mt;
    symbol_node** st;
    FILE* am_file;
    char* am_filename;
} cleanup_context;



#endif