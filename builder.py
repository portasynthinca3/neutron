import sys, os

config_file = 'neutron.nbuild'
image_file = 'build/neutron.img'
image = bytearray(16 * 1024 * 1024)

config_file_obj = open(config_file, 'r')
config_contents = config_file_obj.read()
config_file_obj.close()

asm_files = list()
asm_stack_files = list()
c_files = list()
fs_files = list()
crawout = '<undefined>'

config_section = ''
config_lines = config_contents.split('\n')
for line_no in range(len(config_lines)):
	line = config_lines[line_no]
	if not line.startswith('#'):
		if line.startswith('.'):
			config_section = line[1:]
			if not config_section in ['asm', 'c', 'fs', 'crawout']:
				print('Error at ' + config_file + ':' + str(line_no + 1) + ': invalid section "' + config_section + '"')
				sys.exit()
		else:
			if config_section == 'asm':
				if line.endswith(' --no-append'):
					asm_files.append(line[:-len(' --no-append')])
				else:
					asm_stack_files.append(line)
			elif config_section == 'c':
				c_files.append(line)
			elif config_section == 'fs':
				fs_files.append(line)
			elif config_section == 'crawout':
				if crawout != '<undefined>':
					print('Error at ' + config_file + ':' + str(line_no + 1) + ': section "' + config_section + '" can only contain one entry')
					sys.exit()
				else:
					crawout = line
			else:
				print('Error at ' + config_file + ':' + str(line_no + 1) + ': data in invalid section "' + config_section + '"')
				sys.exit()

print('Assembling stacking binaries')
stack_pos = 0
for path in asm_stack_files:
	path_bin = 'build/' + os.path.basename(path) + '.bin'
	print('  Assembling: ' + path + ' -> ' + path_bin)
	os.system('yasm -f bin -o ' + path_bin + ' ' + path)
	
	print('  Stacking: ' + path_bin)
	bin_file = open(path_bin, 'rb')
	bin_file_cont = bin_file.read()
	bin_file.close()
	for byte in bin_file_cont:
		image[stack_pos] = byte
		stack_pos = stack_pos + 1

print('Assembling ordinary binaries')
for path in asm_files:
	path_bin = 'build/' + os.path.basename(path) + '.bin'
	print('  Assembling: ' + path + ' -> ' + path_bin)
	os.system('yasm -f bin -o ' + path_bin + ' ' + path)

c_obj = list()
print('Generating C object files')
for path in c_files:
	path_obj = 'build/obj/' + os.path.basename(path) + '.o'
	c_obj.append(path_obj)
	print('  Compiling: ' + path + ' -> ' + path_obj)
	#So, hear me out.
	#I literally spent more than two days trying to find out why my code doesn't work in VirtualBox
	#  but does so in Bochs.
	#It turned out to be a VirtualBox coding error. A newer ENDBR32 instruction is supposed to enhance the branch
	#  prediction mechanism preformance on modern CPUs and is placed by GCC at the start of all functions. Older CPUs should
	#  decode ENDBR32 as REP MULTIBYTE NOP, and that's exactly what Bochs does, so no problems there. But! VirtualBox decodes
	#  it as a sequence of an illegal opcode and STI. I don't have a proper interrupt servicing mechanism yet, so this
	#  creates a huge problem. I spent all this time just to find out why Neutron would crash randomly in absolutely
	#  random places. The -fcf-protection=branch -mmanual-endbr options are here to prevent GCC from placing
	#  ENDBR32. It could improve performance, in theory, but I disabled this feature solely for the sake of compatibility.
	#https://stackoverflow.com/questions/56120231/how-do-old-cpus-execute-the-new-endbr64-and-endbr32-instructions/59219143#59219143
	os.system('cc -o ' + path_obj + ' ' + path + ' -m32 -c -nostdlib -nodefaultlibs -Og -ffunction-sections -fcf-protection=branch -mmanual-endbr')

print('Linking C object files')
print('  Generating liker script')
ld_script = open('build/lds', 'w')
#ld_script.write('SECTIONS {\n.text 0x00000F00 : {\n' + ' (.text)\n'.join(c_obj) + ' (.text)\n}\n.data : {\n' + ' (.data)\n'.join(c_obj) + ' (.data)\n}\n}')
ld_script.write('ENTRY(main)\nSECTIONS\n{\n\t.text 0x00000F00 :\n\t{\n\t\t*(.text.main);\n\t\t*(.text*);\n\t}\n\t.data :\n\t{\n\t\t*(.data*);\n\t}\n}')
ld_script.close()
print('  Invoking linker')
os.system('ld -o build/obj/cbuild.o ' + ' '.join(c_obj) + ' --entry=main -melf_i386 -nostdlib -n -T build/lds')

print('Generating raw binary')
os.system('objcopy -O binary build/obj/cbuild.o ' + crawout)
	
if len(fs_files) > 0:
	print('Building nFS')
	print('  Writing signature')
	image[4608] = 0x00
	image[4609] = 0xF5
	image[4610] = 0xAD
	image[4611] = 0xDE
	
	print('  Writing partition name')
	part_name = 'Neutron Test FS'
	for i in range(len(part_name)):
		c = part_name[i]
		image[4612 + i] = ord(c)
	
	print('  Writing Master FileTable')
	file_sects = list()
	next_f_sect = 11
	for path in fs_files:
		if path != "":
			path_src = path.split('>')[0]
			path_dst = path.split('>')[1]
			f_size = os.path.getsize(path_src)
			f_size_sect = f_size // 512
			if f_size_sect % 512 > 0:
				f_size_sect = f_size_sect + 1
			file_sects.append(next_f_sect)
		
			print('   Writing entry for: ' + path_dst)
			for i in range(len(path_dst)):
				c = path_dst[i]
				image[5120 + (fs_files.index(path) * 32) + i] = ord(c)
			
			image[5120 + (fs_files.index(path) * 32) + 24] = f_size & 0xFF
			image[5120 + (fs_files.index(path) * 32) + 25] = (f_size >> 8) & 0xFF
			image[5120 + (fs_files.index(path) * 32) + 26] = (f_size >> 16) & 0xFF
			image[5120 + (fs_files.index(path) * 32) + 27] = (f_size >> 24) & 0xFF
			image[5120 + (fs_files.index(path) * 32) + 28] = next_f_sect & 0xFF
			image[5120 + (fs_files.index(path) * 32) + 29] = (next_f_sect >> 8) & 0xFF
			image[5120 + (fs_files.index(path) * 32) + 30] = (next_f_sect >> 16) & 0xFF
			image[5120 + (fs_files.index(path) * 32) + 31] = (next_f_sect >> 24) & 0xFF

			next_f_sect = next_f_sect + f_size_sect

	print('  Writing files')
	for path in fs_files:
		if path != "":
			path_src = path.split('>')[0]
			path_dst = path.split('>')[1]
			print('   Writing file: ' + path_dst)
			bin_file = open(path_src, 'rb')
			bin_file_cont = bin_file.read()
			bin_file.close()
			stack_pos = 0
			for byte in bin_file_cont:
				image[(file_sects[fs_files.index(path)] * 512) + stack_pos] = byte
				stack_pos = stack_pos + 1

print('Writing image to: ' + image_file)
image_file_file = open(image_file, 'wb')
image_file_file.write(image)
image_file_file.close()
