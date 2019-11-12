%define break xchg bx, bx

;standard VGA text mode colors
%define color_black			0x0
%define color_blue			0x1
%define color_green			0x2
%define color_cyan			0x3
%define color_red			0x4
%define color_pink			0x5
%define color_brown			0x6
%define color_light_gray	0x7
%define color_dark_gray		0x8
%define color_light_blue	0x9
%define color_light_green	0xA
%define color_light_cyan	0xB
%define color_light_red		0xC
%define color_light_pink	0xD
%define color_yellow		0xE
%define color_white			0xF

;video parameters
%define scrn_w				640
%define scrn_h				480

;memory allocation stuff
%define gdt_offset			0
%define malloc_start		0x500000						;start memory allocation from the 5th megabyte counting from zero
%define stack_top			malloc_start - 1				;stack grows downwards