import sys, os, time

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
				if not config_section in ['c', 'fs']:
					print('Error: ' + config_file + ':' + str(line_no + 1) + ': invalid section "' + config_section + '"')
					sys.exit()
			else:
				if config_section == 'c':
					c_files.append(line)
				elif config_section == 'fs':
					fs_files.append(line)
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
print_status('Bulding system image')
print('  Creating image file')
execute('dd if=/dev/zero of=build/neutron.img count=' + str(image_size_sectors) + ' > /dev/null 2>&1')
print('  Creating filesystem')
execute('mformat -i build/neutron.img -f ' + str(image_size_sectors / 2))
print('    Creating EFI executable')
execute('mmd -i build/neutron.img ::/EFI')
execute('mmd -i build/neutron.img ::/EFI/BOOT')
execute('mcopy -i build/neutron.img build/BOOTX64.EFI ::/EFI/BOOT')
print('  Making ISO')
execute(f'dd if={image_file} of={iso_file} > /dev/null 2>&1')

build_end = time.time()
build_took = build_end - build_start
print_status(f'Build done, took {int(build_took * 1000)} ms')