[BITS 64]
[SECTION .text]

loop:
	inc rax
	jmp loop

[SECTION .comment]
db "HewwOwO here", 0
