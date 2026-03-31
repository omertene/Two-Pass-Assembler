/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: symbol_table.c                                          */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
   This file contains all the functions for managing the symbol table.
   The symbol table is a linked list that stores all the labels found
   in the code, along with their addresses and types.
   It deals with adding symbol to table, search for one, fix addresses at the end, and free table memory
 
*/
/* ----------------------------------------------------------------------------------------------------------------------- */

#include "symbol_table.h"
#include "constants.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>



/* Checks if symbol name is valid */
int validate_symbol_name(const char *name, symbol_node *head, macro_table* mac_table, int line_number)
{
    int i;
    int len = strlen(name);
    symbol_node *existing_symbol;
    /* check if label name is too long */
    if(len >= MAX_LABEL_LEN) 
    {
        printf("Line %d: ERROR! Label is too long. Max length allowed is %d chars\n", line_number, MAX_LABEL_LEN-1);
        return ERROR;
    }
    /* chack if label name was already defined as a macro name */
    for(i = 0; i < mac_table->used; i++) 
    {
        if(strcmp(mac_table->macros_arr[i].name, name) == 0) 
        {
            printf("Line %d: ERROR! Label '%s' is already defined as a macro.\n", line_number, name);
            return ERROR;
        }
    }
    /* check name first char is a letter */
    if(!isalpha((unsigned char)*name)) 
    {
        printf("Line %d: ERROR! Label must start with a letter.\n", line_number);
        return ERROR;
    }
    /* check name contains valid chars only */
    for(i = 0; i < len; i++) 
    {
        if(!isalnum((unsigned char)name[i])) 
        {
            printf("Line %d: ERROR! Label must contain letters or numbers only.\n", line_number);
            return ERROR;
        }
    }
    /* check if name is an assembly reserved word */
    if(is_reserved_word(name)) 
    {
        printf("Line %d: ERROR! Label '%s' is a reserved word.\n", line_number, name);
        return ERROR;
    }
    /* check if label name is already used */
    existing_symbol = find_symbol(head, name);
    if (existing_symbol != NULL) 
    {
        if (existing_symbol->type.is_external)
            printf("Line %d: ERROR! Label '%s' was already declared as extern and cannot be defined locally.\n", line_number, name);
        else
            printf("Line %d: ERROR! Label '%s' is already defined.\n", line_number, name);
        return ERROR; 
    }

    return SUCCESS; /* valid label name */
}

/* Adds new symbol to the end of symbol table - reallocate memory */
int add_symbol(symbol_node **head, char *name, int address, symbol_type type, int line_number, macro_table* mac_table, cleanup_context* ctx) 
{
    symbol_node *new_node;

    /* first check if name is valid */
    if (validate_symbol_name(name, *head, mac_table, line_number) == ERROR) 
        return ERROR;
    
    /* if new label is extern - check it wasn't defined locally */
    if (type.is_external) 
    {
        symbol_node* existing_symbol = find_symbol(*head, name);
        if (existing_symbol && !existing_symbol->type.is_external) 
        {
            printf("Line %d: ERROR! Label '%s' was already defined locally and cannot be declared as extern.\n", line_number, name);
            return ERROR;
        }
    }

    /* allocate memory for new symbol */
    new_node = (symbol_node*)malloc(sizeof(symbol_node));
    if (!new_node) 
        handle_error(ctx, "Memory allocation for new symbol"); /* free all allocated memory and exit the programe */
    
    /* add new symbol to the end of the symbol list */
    strcpy(new_node->name, name);
    new_node->address = address;
    new_node->type = type;
    new_node->next = *head;
    *head = new_node;
    
    return SUCCESS;
}

/* Checks if symbol exists and return a pointer to its address */
symbol_node* find_symbol(symbol_node *head, const char *name) 
{
    symbol_node *current = head;
    /* loop through all symbols */
    while (current != NULL) 
    {
        if (strcmp(current->name, name) == 0) /* found it */
            return current;
        current = current->next;
    }
    /* doesn't exist */
    return NULL;
}

/* Updates all data symbols to their correct address, after finding the final instructions count */
void update_data_symbol_addresses(symbol_node *head, int icf) 
{
    symbol_node *current = head;
    /* loop through all symbols */
    while (current != NULL) 
    {
        /* shift it to the end of code words */
        if (current->type.is_data)
            current->address += icf;
        current = current->next;
    }
}

/* Frees allocated memory from every symbol and from the symbole table*/
void free_symbol_table(symbol_node *head) 
{
    symbol_node *current = head;
    symbol_node *next_node;
    while (current != NULL) 
    {
        next_node = current->next;
        free(current);
        current = next_node;
    }
}