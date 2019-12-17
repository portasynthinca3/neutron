;Neutron project
;Standard library for STG2

print_char:					;prints a char
							;input: CL = character to print
							;       CH = color of the characther
							;       BX = position
							;            bx = (x * 80 + y) * 2
							;output: character on the screen
							;       BX = BX + 2
							;
	push ds					;save the DS
	push 0xB800				;
	pop ds					;point to the VGA buffer
	mov [ds:bx], cx			;move the data to the buffer
	add bx, 2				;increment the pointer by 2
	pop ds					;restore the DS
	cmp bx, 4000			;check if we're out of the screen bounds
	je print_char_scroll	;scroll if so
	ret						;return from subroutine
print_char_scroll:
	call print_ln			;print a new line
	ret						;return from subroutine
	
print_str:					;prints a string
							;input: DS:DX = first character of the zero-terminated string
							;       CH = color
							;       BX = position
							;            bx = (x * 80 + y) * 2
							;output: string on the screen
							;       BX = BX + 2
							;
	xchg bx, dx				;swap BX and DX
	mov cl, [ds:bx]			;load the char
	xchg bx, dx				;swap BX and DX
	cmp cl, 0				;compare the char with 0
	je print_ret			;return if equal
	call print_char			;print the char
	inc dx					;increment the pointer
	jmp print_str			;it's a loop after all
print_ret:
	ret						;return from subroutine

print_32b_hex:
	xor dx, dx
	print_32b_hex_loop:
	movzx si, al
	and si, 0xf
	mov cl, [ds:si+hex_num_const]
	call print_char
	shr eax, 4
	inc dx
	cmp dx, 8
	jb print_32b_hex_loop
	ret
	
print_str_line:				;prints a string and then goes to a new line
							;
	call print_str			;normal string printout
	call print_ln			;go to the new line
	ret
	
print_ln:					;moves the text position one line down and scrolls if needed
							;
	push dx					;save DX
	xchg ax, bx				;swap AX and BX
	xor dx, dx				;clear DX
	mov cx, 160				;set the dvider
	div cx					;divide DX AX (AX) by CX (160)
	inc	ax					;go to the next line
	cmp ax, 25				;if we're out of the screen bounds
	jl continue_print_ln	;continue execution
	call shift_scrn_up		;shift the screen up
	mov ax, 24				;go to the last line
	continue_print_ln:
	mul cx					;multiply AX by CX (160)
	xchg ax, bx				;swap AX and BX
	pop dx					;restore DX
	ret						;return from subroutine
	
shift_scrn_up:
	pusha					;save all registers
	push cx					;save video attributes
	mov ah, 6				;select BIOS function: scroll up
	mov cx, 0				;top left corner: ln,col 0,0
	mov dh, 24				;bottom right corner: ln 24
	mov dl, 79				;                     col 79
	mov al, 1				;add 1 line
	pop bx					;restore video attributes
	xor bl, bl				;clear BL
	int 0x10				;call BIOS routine
	popa					;restore all registers
	ret						;return from subroutine
	
str_cmp:					;compares two strings
							;input: string1 = GS:AX
							;       string2 = FS:DX
							;output:
							;       AL = 0 if they aren't equal
							;       AL = 1 if they are
	push ax					;save AX
	push bx					;save BX
	push cx					;save CX
	push dx					;save DX
	mov bx, ax				;move AX to BX
str_cmp_loop:
	mov bx, ax				;load AX into the indexing register
	mov ch, [gs:bx]			;load str2 byte at the desired offset
	mov bx, dx				;load DX into the indexing register
	mov cl, [fs:bx]			;load str1 byte at the desired offset
	cmp cl, ch				;compare str1 and str2 bytes at the desired offset
	jne str_neq				;jump to the inequality return subroutine
	mov bx, ax				;load AX into the indexing register
	cmp byte [gs:bx], 0		;compare str1 at the desired offset to zero
	je str_zero_check		;check the second
	inc ax					;increment the str1 offset
	inc dx					;increment the str2 offset
	jmp str_cmp_loop		;looping
str_neq:
	pop dx					;restore DX
	pop cx					;restore CX
	pop bx					;restore BX
	pop ax					;restore AX
	xor al, al				;clear AL
	ret						;return from subroutine
str_zero_check:
	mov bx, dx				;load DX into the indexing register
	cmp byte [fs:bx], 0		;compare str2 at the desired offset to zero
	je str_eq				;jump to the equality return subroutine
	jmp str_neq				;jump to the inequality return subroutine
str_eq:
	pop dx					;restore DX
	pop cx					;restore CX
	pop bx					;restore BX
	pop ax					;restore AX
	mov al, 1				;set the AL to 1
	ret						;return from subroutine

map_memory:					;makes the memory map
							;input: ES:DI = location to save the map to
							;output: the map at ES:DI
							;
	pushad					;save the registers
	xor ebx, ebx			;clear EBX
	mov edx, 0x534D4150		;magic
map_memory_cycle:			;
	mov dword eax, 0xE820	;magic
	mov ecx, 24				;magic
	int 0x15				;gently ask the BIOS
	jc map_memory_done		;exit when done
	cmp ebx, 0				;
	je map_memory_done		;the same thing
	xor ch, ch				;clear CH
	add di, cx				;increment the pointer
	cmp cl, 24				;Make all the entries
	je map_memory_skip_add	;24-byte aligned
	add di, 4				;
	map_memory_skip_add:	;
	jmp map_memory_cycle	;do it again
map_memory_done:			;
	add di, 20				;zero the next entry out
	mov dword [ds:di], 0	;to indicate the end
	popad					;restore the registers
	ret						;return from subroutine

hex_num_const: db '0123456789ABCDEF'