;Neutron project
;Quark - the kernel

;[map all quark.map]										;tell YASM to write all the label addresses into quark.map file [currently disabled]
%include "stdlib_def.asm"									;include the standard library definitions
%define gqv_siz 18											;define the total GQV size

org 0xD00
use32

start:
	cli														;disable interrupts
	mov esp, stack_top										;set the stack pointer
	xor eax, eax											;clear the registers except for ECX, it contains the video ptr
	xor ebx, ebx											;
	xor edx, edx											;
	mov word fs, ax											;
	mov word gs, ax											;
	mov word ax, 0x10										;move 0x10 into DS
	mov word ds, ax											;
	call enable_a20											;enable the A20 line
															;
	push ecx												;save ECX
	push esi												;save ESI
	mov esi, 0x18											;set ESI to the first empty GDT entry
	mov ecx, 390*8											;clean 390 entries 8 bytes each
	gdt_clear_loop:											;
		mov byte [ds:esi], 0								;write a zero
		inc esi												;increment the pointer
		loop gdt_clear_loop									;loop
	pop esi													;restore ESI
	pop ecx													;restore ECX
	mov edx, gqv_siz										;allocate RAM for GQV - Global Quark Variables
	call malloc												;
	mov [gqv], gs											;save the GQV selector
															;
	push esi												;save ESI
	push ecx												;save ECX
	mov ecx, gqv_siz										;loop gqv_siz times
	xor esi, esi											;clear ESI
	clr_loop:												;
		mov byte [gs:esi], 0								;clear the byte
		loop clr_loop										;loop
	pop ecx													;restore ECX
	pop esi													;restore ESI
															;
	call gfx_init											;initialize	the graphics
	call panic_init											;initialize the panic subsystem
	call gui_mouse_init										;initialize the mouse
	cycle:													;
		call gui_full_redraw								;redraw the GUI completely
		mov ebx, (scrn_h - 8) * scrn_w
		mov al, 0x0F
		mov esi, corner_text
		call gfx_draw_str
		call gfx_flip										;transfer dbuf->vram
		call gui_update_kbdms								;update the keyboard buffer/mouse position
		jmp cycle											;repeat
	jmp $													;hang
	
%include "stdlib.asm"										;include the standard library
%include "gfx.asm"											;include the graphics library
%include "gui.asm"											;include the user interface library
%include "panic.asm"										;include the panic library

gqv: dw 0													;GQV selector

corner_text: db "I <3 assembly language", 0