import sys, os, time
from os import listdir
from os.path import isfile, join

class bcolors:
	HEADER = '\033[95m'
	OKBLUE = '\033[94m'
	OKGREEN = '\033[92m'
	WARNING = '\033[93m'
	FAIL = '\033[91m'
	ENDC = '\033[0m'
	BOLD = '\033[1m'
	UNDERLINE = '\033[4m'

def print_status(text):
	print(bcolors.BOLD + bcolors.UNDERLINE + text + bcolors.ENDC + bcolors.ENDC)

def execute(cmd):
	if '-v' in sys.argv:
		print(bcolors.OKBLUE + cmd + bcolors.ENDC)
	os.system(cmd)

build_start = time.time()

if '-h' in sys.argv:
	print('Additional arguments:\n  -h    display this message\n  -v    print every command being executed')
	exit()

config_file = 'nbuild'
image_file = 'build/neutron.img'
iso_file = 'build/neutron.iso'
image_size_sectors = 2880

config_file_obj = open(config_file, 'r')
config_contents = config_file_obj.read()
config_file_obj.close()

c_files = list()
fs_files = list()

if not os.path.exists('build'):
	os.mkdir('build')

config_section = ''
config_lines = config_contents.split('\n')
for line_no in range(len(config_lines)):
	line = config_lines[line_no]
	if line != "":
		if not line.startswith('#'):
			if line.startswith('.'):
				config_section = line[1:]
				if not config_section in ['c']:
					print('Error: ' + config_file + ':' + str(line_no + 1) + ': invalid section "' + config_section + '"')
					sys.exit()
			else:
				if config_section == 'c':
					c_files.append(line)
				else:
					print('Error: ' + config_file + ':' + str(line_no + 1) + ': data in invalid section "' + config_section + '"')
					sys.exit()

c_obj = list()
print_status('Compiling C sources')
for path in c_files:
	path_obj = 'build/' + os.path.basename(path) + '.o'
	c_obj.append(path_obj)
	print('  Compiling: ' + path)
	execute('x86_64-w64-mingw32-gcc -ffreestanding -mcmodel=large -mno-red-zone -m64 -mno-sse -Os -fstack-protector -Ignu-efi/inc -Ignu-efi/lib -Ignu-efi/inc/x86_64 -Ignu-efi/inc/protocol -nostdlib -c -o ' + path_obj + ' ' + path)

print_status('Linking')
execute('x86_64-w64-mingw32-gcc -mcmodel=large -mno-red-zone -m64 -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main -o build/BOOTX64.EFI ' + ' '.join(c_obj))

print_status('Building INITRD')
initrd_img = bytearray()
initrd_files = [f for f in listdir('initrd') if isfile(join('initrd', f))]
initrd_size = 64
for f in initrd_files:
	initrd_size = initrd_size + 64 + os.path.getsize(join('initrd', f)) #64 bytes per file entry
print("INITRD size: " + str(initrd_size) + " bytes (" + str(initrd_size // 1024) + " kB, " + str(len(initrd_files)) + " files)")
initrd_img = bytearray(initrd_size)
initrd_file_pos = (len(initrd_files) + 1) * 64
initrd_file_list_pos = 0
for f in initrd_files:
	with open(join('initrd', f), 'rb') as file:
		f_data = file.read()
		size_bytes = os.path.getsize(join('initrd', f)).to_bytes(4, byteorder="little")
		pos_bytes = initrd_file_pos.to_bytes(4, byteorder="little")
		initrd_img[initrd_file_list_pos + 0] = pos_bytes[0]
		initrd_img[initrd_file_list_pos + 1] = pos_bytes[1]
		initrd_img[initrd_file_list_pos + 2] = pos_bytes[2]
		initrd_img[initrd_file_list_pos + 3] = pos_bytes[3]
		initrd_file_list_pos = initrd_file_list_pos + 4
		initrd_img[initrd_file_list_pos + 0] = size_bytes[0]
		initrd_img[initrd_file_list_pos + 1] = size_bytes[1]
		initrd_img[initrd_file_list_pos + 2] = size_bytes[2]
		initrd_img[initrd_file_list_pos + 3] = size_bytes[3]
		initrd_file_list_pos = initrd_file_list_pos + 4
		for c in f:
			initrd_img[initrd_file_list_pos] = ord(c)
			initrd_file_list_pos = initrd_file_list_pos + 1
		initrd_img[initrd_file_list_pos] = 0
		initrd_file_list_pos = initrd_file_list_pos + 56 - len(f)
		for b in f_data:
			initrd_img[initrd_file_pos] = b
			initrd_file_pos = initrd_file_pos + 1
with open('build/initrd', 'wb') as initrd_file:
	initrd_file.write(initrd_img)

print_status('Bulding system image')
print('  Creating image file')
execute('dd if=/dev/zero of=build/neutron.img count=' + str(image_size_sectors) + ' > /dev/null 2>&1')
print('  Creating filesystem')
execute('mformat -i build/neutron.img -f ' + str(image_size_sectors / 2))
execute('mmd -i build/neutron.img ::/EFI')
execute('mmd -i build/neutron.img ::/EFI/BOOT')
execute('mmd -i build/neutron.img ::/EFI/nOS')
print('    Moving EFI executable')
execute('mcopy -i build/neutron.img build/BOOTX64.EFI ::/EFI/BOOT')
print('    Moving INITRD')
execute('mcopy -i build/neutron.img build/initrd ::/EFI/nOS/initrd')
print('  Making ISO')
execute(f'dd if={image_file} of={iso_file} > /dev/null 2>&1')

build_end = time.time()
build_took = build_end - build_start
print_status(f'Build done, took {int(build_took * 1000)} ms')