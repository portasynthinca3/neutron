.globl   enable_a20
.align   4
.intel_syntax noprefix

enable_a20:													;//enables the A20 line, allowing us to use more than a megabyte of RAM
															;//input: none
															;//output: none
															;//
	push ax													;//save AX
	call a20_wait											;//wait for the keyboard controller to be ready
	mov al, 0xAD											;//write 0xAD
	out 0x64, al											;//to port 0x64 (keyboard controller)
	call a20_wait											;//
	mov al, 0xD0											;//
	out 0x64, al											;//
	call a20_wait_2											;//
	in al, 0x60												;//input from port 0x60
	push ax													;//save AX
	call a20_wait											;//
	mov al, 0xD1											;//
	out 0x64, al											;//
	call a20_wait											;//
	pop ax													;//restore AX
	or al, 2												;//set bit 1 of AL
	out 0x60, al											;//
	call a20_wait											;//
	mov al, 0xAE											;//
	out 0x64, al											;//
	call a20_wait											;//
	pop ax													;//restore AX
	ret														;//return from subroutine
a20_wait:
	in al, 0x64												;//
	test al, 2												;//
	jnz a20_wait											;//
	ret														;//
a20_wait_2:
	in al, 0x64												;//
	test al, 1												;//
	jz a20_wait_2											;//
	ret														;//
