# vpk.exe path relative to script's folder
# use / or \\ only
vpk_path = "..\\bin\\vpk.exe"

# Script begins
import sys
import os
import subprocess

if len(sys.argv) <= 1 or sys.argv[1] == '--help':
    print("usage: py make_vpk.py <folder>")
    exit() 

src_folder = sys.argv[1]
src_folder_name = os.path.basename(src_folder)
src_folder_len = len(src_folder) + 1


# generate filename and path for our response file
response_file_name = f"{src_folder_name}_list.txt"
response_file_path = os.path.join(os.getcwd(), response_file_name)

# translate vpk_path to be relative to location of this .py file
vpk_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), vpk_path)

out = open(response_file_path, 'w')

for (dirpath, dirnames, filenames) in os.walk(src_folder):
    for file in filenames:
        # make path relative to parent folder of src folder and fix slashes
        out.write(src_folder_name + "\\" + os.path.join(dirpath[src_folder_len:].replace("/", "\\"), file) + "\n")

out.close()

print(f"Generated {response_file_name}. Building vpk...")

# generate vpk file name from src dir name, remove _dir suffix
vpk_file_name = f"{src_folder_name.replace('_dir', '')}"

# vpk.exe -M a <vpk file> @<response file>
subprocess.call([vpk_path, "-M", "a", vpk_file_name, "@" + response_file_path])