;Neutron project
;Standard library

db "STDLIB"

;=============================================== MEMORY OPERATIONS ===============================================

enable_a20:													;enables the A20 line, allowing us to use more than a megabyte of RAM
															;input: none
															;output: none
															;
	push ax													;save AX
	call a20_wait											;wait for the keyboard controller to be ready
	mov al, 0xAD											;write 0xAD
	out 0x64, al											;to port 0x64 (keyboard controller)
	call a20_wait											;
	mov al, 0xD0											;
	out 0x64, al											;
	call a20_wait_2											;
	in al, 0x60												;input from port 0x60
	push ax													;save AX
	call a20_wait											;
	mov al, 0xD1											;
	out 0x64, al											;
	call a20_wait											;
	pop ax													;restore AX
	or al, 2												;set bit 1 of AL
	out 0x60, al											;
	call a20_wait											;
	mov al, 0xAE											;
	out 0x64, al											;
	call a20_wait											;
	pop ax													;restore AX
	ret														;return from subroutine
a20_wait:
	in al, 0x64												;
	test al, 2												;
	jnz a20_wait											;
	ret														;
a20_wait_2:
	in al, 0x64												;
	test al, 1												;
	jz a20_wait_2											;
	ret														;

map_memory:													;processes the memory map created by STG2
															;input: none
															;output: none
															;
	push esi												;save the registers
	mov esi, 0x8FC00										;load 0x8FC00 into ESI
	;TODO
	pop esi													;restore the registers
	ret														;return from subroutine

malloc:														;allocates a chunk of memory
															;input: EDX = size in bytes
															;output: GS = GDT entry number * 8
															;        GS = 0 if no RAM is available
															;
	push eax												;save the registers
	push ebx												;
	push ecx												;
	push edi												;
	push esi												;
	push edx												;
	mov esi, gdt_offset										;load GDT offset into ESI
	add esi, 24												;skip the first 3 descriptors
	mov ecx, 0xFFFFFFFF										;set ECX to the maximal 32-bit value
	xor edi, edi											;clear EDI
	malloc_find_gdt_suiting:								;
		cmp dword [ds:gdt_offset+esi], 0					;check if the end has been reached
		je malloc_gtd_end									;return if so
		mov byte al, [ds:gdt_offset+esi+7]					;read segment base into EAX
		shl eax, 8											;
		mov byte al, [ds:gdt_offset+esi+4]					;
		shl eax, 16											;
		mov word ax, [ds:gdt_offset+esi+2]					;
		xor ebx, ebx										;clear EBX as the upper 8 bits will not be overwritten
		mov byte bl, [ds:gdt_offset+esi+6]					;read segment limit into EBX
		and bl, 0x0F										;
		shl ebx, 8											;
		mov word bx, [ds:gdt_offset+esi+0]					;
		push cx												;save CX
		mov byte cl, [ds:gdt_offset+esi+5]					;read access byte
		test cl, 7											;check presence flag
		pop cx												;restore CX
		jnz malloc_find_gdt_skip							;skip the entry if the segment is present
		push cx												;save CX
		mov byte cl, [ds:gdt_offset+esi+6]					;read flags
		test cl, 7											;check granularity flag
		pop cx												;restore CX
		jnz malloc_find_gdt_skip							;skip the entry if granularity is in pages
		cmp ebx, edx										;check if the size is suiting
		jb malloc_find_gdt_skip								;skip the entry if not
		cmp ebx, ecx										;check if the size is less than the least optimal one
		ja malloc_find_gdt_skip								;skip the entry if not
		mov edi, esi										;copy ESI into EDI
		mov ecx, ebx										;copy EBX into ECX
		malloc_find_gdt_skip:								;
		add esi, 8											;move on to the next descriptor
		jmp malloc_find_gdt_suiting							;loop
	malloc_gtd_end:											;
		cmp edi, 0											;check if no existing entries were found
		jne malloc_skip_ediesi								;skip EDI<-ESI assignment if they were
		mov edi, esi										;load ESI into EDI
		mov ecx, edx										;load the requested size into ECX
		sub edi, 8											;point to the previous block
		malloc_skip_ediesi:									;
		cmp edi, 0x10										;check if the entry we're on is a standard code/data entry
		jne malloc_skip_stwrt								;skip standard value write
		mov eax, malloc_start								;start from the starting location
		add word [ds:0], 8									;add 8 to the length of the GDT in the GDT descriptor
		jmp malloc_skip_basedet								;skip determining the base as we already did
		malloc_skip_stwrt:									;
		add word [ds:0], 8									;add 8 to the length of the GDT in the GDT descriptor
		mov byte al, [ds:gdt_offset+edi+7]					;read segment base into EAX
		shl eax, 8											;
		mov byte al, [ds:gdt_offset+edi+4]					;
		shl eax, 16											;
		mov word ax, [ds:gdt_offset+edi+2]					;
		xor ebx, ebx										;clear EBX as the upper 8 bits will not be overwritten
		mov byte bl, [ds:gdt_offset+edi+6]					;read segment limit into EBX
		and bl, 0x0F										;
		shl ebx, 16											;
		mov word bx, [ds:gdt_offset+edi+0]					;
		add eax, ebx										;add EBX to EAX to get the base for the next block
		malloc_skip_basedet:								;
		xor edx, edx										;clear EDX
		xor ebx, ebx										;clear EBX
		mov bx, ax											;set base 0:15
		shl ebx, 16											;shift it to the higher word
		shr eax, 16											;expose base 16:31 in AX
		mov bx, cx											;set limit 0:15
		shr ecx, 16											;expose limit 16:19 in CX (CL)
		mov dx, ax											;load base 16:31 into DX
		and cl, 0x0F										;clear 4 upper bits of CL
		mov dl, cl											;move CL into DL
		or dl, 01000000b									;set flags: Gr=0, Sz=1
		shl edx, 16											;shift base 24:31, flags, limit 16:19 into upper 16 bits
		mov dh, 10010010b									;set flags: present, ring 0, non-system, data, writable
		mov dl, al											;load base 16:23 into DL
		add edi, 8											;point to the current block as it was pointing to the revious one before
		mov dword [ds:edi+0], ebx							;save base 0:15, limit 0:15
		mov dword [ds:edi+4], edx							;save the rest
		lgdt [0]
		mov gs, di											;load EDI into GS
	malloc_exit:											;
	pop edx													;restore the registers
	pop esi													;
	pop edi													;
	pop ecx													;
	pop ebx													;
	pop eax													;
	ret														;return from subroutine
	malloc_exit_err:
	xor ax, ax												;set GS to NULL
	mov gs, ax												;
	pop edx													;restore the registers
	pop esi													;
	pop edi													;
	pop ecx													;
	pop ebx													;
	pop eax													;
	ret														;return from subroutine

gdt_laddr:													;fetches the linear address of a GDT entry
															;input: ES = selector
															;output: EAX = linear offset of the GDT entry
															;
	push bx													;save the registers
															;
	mov bx, es												;move ES into BX
															;
	mov byte al, [ds:gdt_offset+bx+7]						;read segment base into EAX
	shl eax, 8												;
	mov byte al, [ds:gdt_offset+bx+4]						;
	shl eax, 16												;
	mov word ax, [ds:gdt_offset+bx+2]						;
															;
	pop bx													;restore the registers
															;
	ret														;return from subroutine

;=============================================== TEXT MODE SCREEN OPERATIONS ===============================================

screen_clear:												;clears the screen
															;input: none
															;output: none
															;
	push ecx												;save ECX
	push esi												;save ESI
	xor esi, esi											;clear ESI
	mov ecx, 80 * 25 * 2									;repeat 80*25*2 times (the entire VGA buffer)
	screen_clear_loop:										;
		mov byte [ds:esi+0xB8000], 0						;write a zero to the VGA buffer
		inc esi												;increment pointer
		loop screen_clear_loop								;loop
	pop esi													;restore ESI
	pop ecx													;restore ECX
	ret														;return from subroutine

print_char:													;prints a char
															;input: CL = character to print
															;       CH = color of the characther
															;       BX = position
															;            bx = (x * 80 + y) * 2
															;output: character on the screen
															;       BX = BX + 2
															;
	push esi												;save ESI
	xor esi, esi											;clear ESI
	mov si, bx												;move BX into SI
	mov [ds:esi+0xB8000], cx								;move the data to the VGA buffer
	add bx, 2												;increment the pointer by 2
	pop esi													;restore ESI
	ret														;return from subroutine

print_str:													;prints a string
															;input: ESI = first character of the zero-terminated string
															;       CH = color
															;       BX = position
															;            bx = (x * 80 + y) * 2
															;output: string on the screen
															;       BX = BX + 2
															;
	mov cl, [ds:esi]										;load the char
	cmp cl, 0												;compare the char with 0
	je print_ret											;return if equal
	call print_char											;print the char
	inc esi													;increment the pointer
	jmp print_str											;it's a loop after all
print_ret:
	ret														;return from subroutine