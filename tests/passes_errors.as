; LABELS
DUPLABEL: .data 1
; Duplicate label definition
DUPLABEL: .data 2

;  Label name is a reserved word
r1: .data 10

; Space before colon in label
LABELSPACE : .data 1

; No space after colon
LABELNOSPACE:mov r1,r2

;  Label without a command or directive
JUSTALABEL:

; DIRECTIVES
;Trailing comma in .data
.data 1,2,
; Double comma in .data
.data 1,,2
; Missing comma in .data
.data 1 2
;  Missing closing quote in .string
.string "abc
;  Missing opening quote in .string
.string abc"
;  Zero dimension in .mat
.mat [2][0]
;  Non-constant dimension in .mat
.mat [1][r1]
;  Missing space before initializer list in .mat
.mat [1][1]1,2
; Too many initializers in .mat
.mat [1][1] 1,2,3

; Local definition of an extern symbol
.extern L1
L1: .data 10

; Symbol cannot be both extern and entry
.entry L1


; INSTRUCTIONS
; Unknown command name
move r1,r2
; Too many operands for stop
stop r1
; Too few operands for mov
mov r1
;  Invalid source addressing mode for lea
lea #10, r1
; Invalid destination addressing mode for add
add r1, #5 
; Matrix index must be a register
mov M[1][r2], r3
;  Extra text after operands
mov M[r1][r2] extra


; SECOND_PASS
; Undefined symbol 
jmp UNDEFINEDLABEL
; Entry for a non-existent symbol
.entry NONEXISTENT