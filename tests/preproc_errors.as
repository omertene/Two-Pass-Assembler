
; macro name is a reserved word
mcro r1

; macro name is too long 
mcro VERYLONGNAMEEEEEEEEEEEEEEEEEEEEEEE

; macro name starts with a number
mcro 123macro

; macro name contains invalid characters
mcro my-macro

; missing macro name
mcro

; extra text after macro name
mcro my_macro extra_text

; redefinition of a macro
mcro DUP_MACRO
stop
mcroend

mcro DUP_MACRO

;extra text after mcroend
mcro ANOTHER_MACRO
stop
mcroend extra_text 
mcroend


mcro A_MACRO
mov r1,r2
mcroend

;extra text after macro call
A_MACRO text 

 ; misplaced comment (starts with a space)

; misplaced comment after a valid line
mov r1,r2 ; invalid comment

;line too long
thislineistoolonggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg