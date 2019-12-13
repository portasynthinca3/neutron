;Neutron project
;Muon - second stage loader

%define nfs_buffer   0x800
%define kbd_buf      0x105
%define app_segment  0x175

org 0x0000
use16

dw 0xEBA1					;signature for the 1st stage loader

entry:
	cli						;disable interrupts
	mov al, 0x80			;disable non-maskable interrupts
	out 0x70, al			;
	mov ax, cs				;
	mov ds, ax				;set DS to CS
	mov byte al, [ds:0]		;fetch the drive number
	mov byte [ds:nfs_drive_no], al ;tell it to the nFS driver
	mov bx, 320				;set the screen position: 2nd line counting from 0
	mov ch, 0x0F			;set the colors: white on black
	call nfs_init			;read boot drive geometry
	call nfs_load_master_sector ;load the nFS data
	mov ch, 0x0F			;set the colors: white on black
	jmp krnl				;try to load the kernel
krnl:
	call krnl_load			;load the kernel
	cmp ax, 0				;check if the load was successful
	jne krnl_ret			;return if not
	push 0x93C0				;segment 0x93C0 - this is where the map is stored
	pop es					;
	xor di, di				;clear DI
	call map_memory			;map the memory while we didn't overwrite the IVT
	jmp krnl_run			;run the kernel
krnl_ret:
	mov ch, 0x0C
	mov dx, krnl_error
	call print_str_line
	jmp $
krnl_load:
	;load the kernel into RAM
	mov dx, krnl_debug_ldstart ;print the debug message
	mov ch, 0x0F			;
	call print_str			;
	push gs					;save the registers
	push es					;
	push si					;
	push di					;
	push ax					;
	push dx					;
	push ecx				;
	push bx					;
	mov ax, ds				;move DS
	mov gs, ax				;into GS
	mov ax, krnl_file		;find the kernel file
	call nfs_find			;
	cmp byte [ds:nfs_status], 0 ;check if the operation was successful
	jne krnl_load_ret_err	;return immediately if not
	mov bx, dx				;load DX into the indexing register
	mov ax, [ds:bx+28]		;load the file location into AX
	mov dx, [ds:bx+24]		;load the file size into DX
	push 0xF0				;load 0xF0 into
	pop es					;ES
	xor di, di				;clear DI
krnl_load_cycle:
	cmp dx, 0				;check if the entire file was read
	jle krnl_load_ret		;return if so
	pop bx
	push dx
	push cx
	mov dx, krnl_debug_ld
	mov ch, 0x0F
	call print_str
	pop cx
	pop dx
	push bx
	call nfs_read_drive_sector ;read one sector
	cmp byte [ds:nfs_status], 0 ;check if the operation was successful
	jne krnl_load_ret_err	;return immediately if not
	mov ecx, 512			;set the number of bytes to copy: 512
	mov si, nfs_buffer		;set SI to the nFS buffer
	rep movsb				;copy the bytes
	inc ax					;increment the sector number
	sub dx, 512				;subtract the sector size from the file size
	jmp krnl_load_cycle		;looping
krnl_load_ret_err:
	pop bx					;restore the registers
	pop ecx					;
	pop dx					;
	pop ax					;
	pop di					;
	pop si					;
	pop es					;
	pop gs					;
	mov ax, 1				;tell that the error occured
	ret						;return from subroutine
krnl_load_ret:
	pop bx					;restore the registers
	pop ecx					;
	pop dx					;
	pop ax					;
	pop di					;
	pop si					;
	pop es					;
	pop gs					;
	mov ax, 0				;tell that the load was successfull
	ret						;return from subroutine
krnl_run:
	;shit is getting unreal here
	;i mean, switch the CPU into the protected mode
	;(opposite of what we've had before up until now, the real mode)
	mov ch, 0x0F			;print the debug message
	mov dx, krnl_debug_run	;
	call print_str_line		;
	cli						;disable interrupts
	mov al, 0x80			;disable non-maskable interrupts
	out 0x70, al			;
	push 0x050				;load 0x050
	pop ds					;into the data segment
	mov ecx, gdt_end - gdt	;copy the entire GDT
	mov si, gdt				;source: predefined GDT
	push 0					;destination: start of the memory
	pop es					;
	xor di, di				;
	rep movsb				;copy
	mov ecx, 6				;copy the entire GDT descriptor
	mov si, gdt_desc		;source: predefined GDT descriptor
	xor di, di				;destination: start of the memory
	rep movsb				;copy
	xor dx, dx				;set DS to 0
	mov ds, dx				;
	lgdt [0]				;Load Global Descriptor Table
	push cs					;load CS into DS
	pop ds					;
	mov ch, 0x0F			;print another debug message
	mov dx, krnl_debug_video;
	call print_str			;
	call gfx_go_best		;exit the text mode, as the kernel can't do it on its own
	push 0x8FC0				;load 0x8C0
	pop es					;into ES
	mov [es:0], ecx			;save ECX (gfx buf linear address) in RAM
	mov [es:4], edx			;save EDX (gfx resolution) in RAM
	mov [es:8], ah			;save AH (gfx bits per pixel) in RAM
	mov dl, [ds:nfs_drive_no] ;get the drive number
	mov byte [es:8], dl		;save boot drive in RAM
	mov eax, cr0			;load the control register 0 into EAX
	or eax, 1				;set its last bit
	mov cr0, eax			;load the EAX the control register 0
	;YAAY! we're in Protected Mode now!
	cli						;disable interrupts
	;jmp $
	mov ax, 0x10			;load 0x10
	mov ds, ax				;into DS
	mov ss, ax				;and SS
	mov es, ax				;and ES
	mov fs, ax				;and FS
	mov gs, ax				;and GS
	mov esp, 0x4FFFFF		;set the top-of-stack pointer
	pushf					;set IOPL to 3
	pop ax					;
	or ax, 3 << 12			;
	push ax					;
	popf					;
	jmp 8:0xF00				;far jump into the beforehand-loaded kernel
	
%include "nfs_driver.asm"	;include the nFS driver
%include "gfx_driver.asm"	;include the graphics driver
%include "stdlib.asm"		;include the "standard library"

krnl_error: db "An error occured while loading Quark", 0
krnl_file: db "quark", 0
krnl_debug_ldstart: db "Loading Quark", 0
krnl_debug_ld: db ".", 0
krnl_debug_run: db "done", 0
krnl_debug_video: db "Choosing video mode", 0

gdt:                    ;Global Descriptor Table
gdt_null:               ;null segment
	dd 0
    dd 0
gdt_code:               ;GDT code segment
    dw 0xFFFF
    dw 0
    db 0
    db 10011010b
    db 11001111b
    db 0
gdt_data:               ;GDT data segment
    dw 0xFFFF
    dw 0
    db 0
    db 10010010b
    db 11001111b
    db 0
gdt_end:
gdt_desc:                   ;GDT descriptor
    dw gdt_end - gdt - 1
    dd 0					;we store GDT at the start of the memory

reboot_cmd:
	db 0xEA
	dw 0x0000, 0xFFFF
	ret
