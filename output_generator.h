#ifndef OUTPUT_GENERATOR_H
#define OUTPUT_GENERATOR_H

#include "data_structures.h"

/*
   Function that takes a number and converts it
   to the special base 4 (a,b,c,d). It also adds padding with 'a'
   at the start to make sure the string has a fixed length.

   - n: the number to convert.
   - base4_str: a pointer to a buffer where the result string will be stored.
   - digits: the fixed length the final string should have.
*/
void convert_to_base4(unsigned int n, char *base4_str, int digits);

/*
   The main function for creating the output files. It is called at the
   end of the assembly process, only if both passes were successful.
   It creates the .ob, .ent, and .ext files.

   - filename: the base name for the output files.
   - icf: the final value of the instruction counter.
   - dcf: the final value of the data counter.
   - code_image: the array with all the encoded instruction words.
   - data_image: the array with all the encoded data words.
   - symbol_table: the completed symbol table, to find entry points.
   - extern_list: the list of all external symbols.
*/
void write_output_files(const char *filename, int icf, int dcf, machine_word code_image[],
     machine_word data_image[], symbol_node *symbol_table, extern_node *extern_list); 
/*
   A different convert_to_base4 that does not add
   padding. It's used for the header of the .ob file, which needs
   to show the exact size of the code and data sections.

   - n: the number to convert.
   - base4_str: a pointer to a buffer where the result string will be stored.
*/
void convert_to_base4_unpadded(unsigned int n, char *base4_str);

#endif