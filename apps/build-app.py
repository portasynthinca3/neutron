#!/usr/bin/python3

# Neutron application builder

import sys
import os
import os.path as path
import json

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

if len(sys.argv) != 2:
    print('Usage: python3 build-app.py app-dir')
    exit()

app_dir = sys.argv[1]

if not path.exists(app_dir):
    print('Error: application directory not found')
    exit()

build_dir = app_dir + '/build'
src_dir = app_dir + '/src'
res_dir = app_dir + '/res'
desc_path = app_dir + '/app.json'

desc = {}

# load app description
try:
    with open(desc_path, 'r') as f:
        desc = json.load(f)
except Exception:
    print('Error: error while loading application description')
    exit()

# create build directory
if not path.exists(build_dir):
    os.mkdir(build_dir)

print_status('Compiling')

# build source files
src_files = [f for f in os.listdir(src_dir) if path.isfile(path.join(src_dir, f))]
for file in src_files:
    print('  Building: ' + src_dir + '/' + file)
    code = os.system('gcc -nostdlib -ffreestanding -m64 -mno-sse2 -O0 -c -o ' + build_dir + '/' + file + '.o ' + src_dir + '/' + file)
    # terminate on compilation error
    if code != 0:
        exit()

# link
print_status('Linking')
code = os.system('ld -melf_x86_64 -nostdlib -e main -o ' + build_dir + '/app.elf '
               + ' '.join([build_dir + '/' + f + '.o' for f in src_files]))