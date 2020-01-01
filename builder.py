import sys, os

def get_object_files(dir_name):
	list_of_files = os.listdir(dir_name)
	all_files = list()
	for entry in list_of_files:
		full_path = os.path.join(dir_name, entry)
		if os.path.isdir(full_path):
			all_files += get_object_files(full_path)
		elif full_path.endswith('.o'):
			all_files.append(full_path)
                
	return all_files

def get_c_files(dir_name):
	list_of_files = os.listdir(dir_name)
	all_files = list()
	for entry in list_of_files:
		full_path = os.path.join(dir_name, entry)
		if os.path.isdir(full_path):
			all_files += get_object_files(full_path)
		elif full_path.endswith('.c'):
			all_files.append(full_path)
                
	return all_files

config_file = 'neutron.nbuild'
image_file = 'build/neutron.img'
image_size_sectors = 2880

config_file_obj = open(config_file, 'r')
config_contents = config_file_obj.read()
config_file_obj.close()

c_files = list()
fs_files = list()

config_section = ''
config_lines = config_contents.split('\n')
for line_no in range(len(config_lines)):
	line = config_lines[line_no]
	if line != "":
		if not line.startswith('#'):
			if line.startswith('.'):
				config_section = line[1:]
				if not config_section in ['c', 'fs']:
					print('Error at ' + config_file + ':' + str(line_no + 1) + ': invalid section "' + config_section + '"')
					sys.exit()
			else:
				if config_section == 'c':
					c_files.append(line)
				elif config_section == 'fs':
					fs_files.append(line)
				else:
					print('Error at ' + config_file + ':' + str(line_no + 1) + ': data in invalid section "' + config_section + '"')
					sys.exit()

c_obj = list()
c_obj.append('gnu-efi/x86_64/gnuefi/libgnuefi.a')
c_obj.append('gnu-efi/x86_64/lib/libefi.a')
print('Compiling C sources')
for path in c_files:
	path_obj = 'build/' + os.path.basename(path) + '.o'
	c_obj.append(path_obj)
	print('  Compiling: ' + path + ' -> ' + path_obj)
	os.system('x86_64-w64-mingw32-gcc -ffreestanding -mcmodel=large -mno-red-zone -m64 -msse2 -Ignu-efi/inc -Ignu-efi/lib -Ignu-efi/inc/x86_64 -Ignu-efi/inc/protocol -nostdlib -c -o ' + path_obj + ' ' + path)

print('Linking')
os.system('x86_64-w64-mingw32-gcc -mcmodel=large -mno-red-zone -m64 -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o build/BOOTX64.EFI ' + ' '.join(c_obj) + ' -lgcc')

print('Bulding system image')
print('  Creating image file')
os.system('dd if=/dev/zero of=build/neutron.img count=' + str(image_size_sectors) + ' > /dev/null 2>&1')
os.system('mformat -i build/neutron.img -f ' + str(image_size_sectors / 2))
print('  Creating filesystem')
os.system('mmd -i build/neutron.img ::/EFI')
os.system('mmd -i build/neutron.img ::/EFI/BOOT')
os.system('mcopy -i build/neutron.img build/BOOTX64.EFI ::/EFI/BOOT')