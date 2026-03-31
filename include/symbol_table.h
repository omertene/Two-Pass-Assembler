#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "data_structures.h"



/*
   A helper for add_symbol that does all the validation for a new symbol name.
   It checks the length, characters, if it's a reserved word, or if it's already defined.

   - name: the label name to check.
   - head: a pointer to the start of the symbol table, to check for duplicates.
   - mac_table: the macro table, to check for name conflicts.
   - line_number: the current line number, for error messages.

   Return SUCCESS if the name is valid, otherwise ERROR.
*/
int validate_symbol_name(const char *name, symbol_node *head, macro_table* mac_table, int line_number);

/*
   The main function to add a new symbol to the symbol table. It first calls
   a helper to validate the name, and if it's ok, it allocates memory
   and adds the new symbol to the linked list.

   - head: a pointer to the head of the symbol table linked list.
   - name: the name of the symbol to add.
   - address: the address of the symbol.
   - type: a struct that holds the flags for the symbol's type.
   - line_number: the current line number.
   - mac_table: the macro table for validation.
   - ctx: the cleanup struct for memory errors.

   Return SUCCESS if the symbol was added, otherwise ERROR.
*/
int add_symbol(symbol_node **head, char *name, int address, symbol_type type, int line_number, macro_table* mac_table, cleanup_context* ctx);

/*
   A helper function that searches the symbol table for a symbol with a given name.

   - head: a pointer to the start of the symbol table.
   - name: the name of the symbol to find.

   Return a pointer to the symbol's node if found, otherwise NULL.
*/
symbol_node* find_symbol(symbol_node *head, const char *name);

/*
   A function that is called at the end of the first pass. It goes through
   the whole symbol table and adds the final IC value to all the 'data'
   symbols, to place them correctly in memory after the code.

   - head: a pointer to the start of the symbol table.
   - icf: the final value of the Instruction Counter.
*/
void update_data_symbol_addresses(symbol_node *head, int icf);

/*
   Frees all the memory used by the symbol table linked list.

   - head: a pointer to the start of the list to free.
*/
void free_symbol_table(symbol_node *head);

#endif