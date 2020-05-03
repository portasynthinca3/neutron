[BITS 64]
mov rax, 0xDEADBEEF
mov rdx, 0
mov rcx, 0
;This instrucion will try to divide 0xDEADBEEF (RDX:RAX) by zero :>
;Let's see what will happen :thonkangs:
;Of course it will not throw an exception
div rcx
