

/* ---------------------------------------------------------------------------------------------------------------------- */
/*                                           Description of File: constants.h                                         */                                   
/* ---------------------------------------------------------------------------------------------------------------------- */
/* 
    This file holds all the global constants for the project.
    It is used in order to avoid "magic numbers" directly in the
    code which makes the code easier to read and change if an update is needed.
*/
/* ----------------------------------------------------------------------------------------------------------------------- */


#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MAX_LABEL_LEN 31 
#define MAX_LINE_LEN 81 /* 80 characters + new line char */
#define MCRO_STR "mcro"
#define MCRO_LEN 4
#define MCROEND_STR "mcroend"
#define MCROEND_LEN 7
#define INITIAL_MACRO_CAPACITY 10
#define MAX_MACRO_NAME_LEN 31
#define MAX_FILE_NAME_LEN 500
#define DECIMAL_BASE 10
#define QUOTES_NUM 2
#define MATRIX_OPERAND_PARTS 3 /* Number of parts in a matrix operand: name, row, col */
#define BASE_4 4
#define ADDRESS_WORD_LENGTH 4 /* The length of a base-4 address string in the .ob file */
#define MACHINE_WORD_LENGTH 5 /* The length of a base-4 machine word string in the .ob file */
#define TEMP_STR_BUFFER 32 /* A safe buffer size for integer to string conversions */
#define MIN_ARGC 2
#define ADDRESING_TYPES_NUM 4


/* The range of a 10-bit and 8 bit signed number (2's complement) */
#define MAX_10_BIT_VAL 511
#define MIN_10_BIT_VAL -512
#define MAX_8_BIT_VAL 127
#define MIN_8_BIT_VAL -128

#define NUM_OF_MAT_DIMENTIONS 2
#define MATRIX_OPERAND_WORDS 2
#define ERROR -1 
#define SUCCESS 1
#define GROWTH_FACTOR 2 /* will be used to grow the capacity of an array with 'realloc' */
#define ASCII_MIN_PRINT 32
#define ASCII_MAX_PRINT 126
#define CODE_START_IDX 100
#define MAX_MEMORY 256
#define MAX_PROGRAM_SIZE 156 /* MAX_MEMORY(256) - CODE_START_IDX(100) */

/* Register numbers */
#define LAST_REGISTER_NUM 7
#define FIRST_REGISTER_NUM 0

/* Operand counts */
#define NO_OPERANDS 0
#define ONE_OPERAND 1
#define TWO_OPERANDS 2

/* Bit shift amounts for operand addressing modes in the first machine word */
#define DEST_OPERAND_SHIFT 2
#define SOURCE_OPERAND_SHIFT 4

/* Bit shift amounts for register encoding in extra data words */
#define SOURCE_REGISTER_SHIFT 6
#define MATRIX_ROW_REG_SHIFT 6 
#define MATRIX_COL_REG_SHIFT 2 
#define SINGLE_SOURCE_REG_SHIFT 6 
#define SINGLE_DEST_REG_SHIFT 2  
/* Bit shift amount for the opcode field in the first machine word */
#define OPCODE_SHIFT 6

#define ARE_SHIFT 2 /* Number of bits to shift to make space for A,R,E */

/* Bit masks */
#define IMMEDIATE_MASK 0xFF    /* Bitmask for an 8-bit immediate value */
#define TEN_BIT_MASK 0x3FF     /* Bitmask for 10 bits (1023 in decimal) */



#endif