# Two-Pass Assembler
A C-based assembler that processes assembly language files into machine code for a hypothetical CPU.

## Project Structure
- **Pre-Processor:** Handles macro expansion and cleans the source code. 
- **First Pass:** Builds the symbol table and calculates memory addresses (IC & DC). 
- **Second Pass:** Generates binary machine code (in special Base-4 format) and handles entry/extern directives.
- **Output Generator:** Produces .ob, .ent, and .ext files. 

## How to Build
Run the following command in the terminal:
`make`
