

		.entry LOOP
	.extern W

mcro    	HELLO
	mov 	#5,        r1
mcroend

MAIN:	add #1, r2
	HELLO
			mov STR, r3
LOOP:	jmp W
     sub r1, r2
	bne LOOP
			stop

STR:			.string "hello"
VARS:	.data 10,    -20, 30