import fileinput
import utils
import sys
import vpk
import os


if len(sys.argv) <= 1 or sys.argv[1] == '--help':
    print("usage: py checktree.py <path list file>")
    print("output format: CSV")
    print("(see paths_template.txt for an example path list file)")
    exit()


input_paths = []

for path in fileinput.input():

    # strip comments and ws
    path = utils.strip_comments(path).rstrip()

    # cull empty lines
    if path == '':
        continue

    # normalize
    path = os.path.normpath(path)

    # cull invalid dirs
    if not os.path.isdir(path):
        print(f"Error: Invalid directory: {path}")
        continue

    # load path list in reverse order (reverse of GameInfo.txt)
    input_paths.insert(0, path)

grand_total_dupes = 0
tree = {}

def check_vpks(path):
    total_dupes = 0
    files = os.listdir(path)
    for file in files:
        if file.endswith('_dir.vpk'):
            fullpath = os.path.join(path, file)
            pak = vpk.open(fullpath)

            print()
            print(f"VPK: {fullpath} [{len(pak)} files]")

            vpk_id = os.path.join(os.path.basename(path), file)

            hasdupes = 0

            for filepath in pak:
                filepath = os.path.normpath(filepath)
                if filepath in tree:

                    if hasdupes == 0:
                        print("DUPLICATE FILE,SOURCE")

                    hasdupes += 1

                    print(f"{filepath},{tree[filepath]}")
                else:
                    tree[filepath] = vpk_id
            
            print(f"Total Duplicates: {hasdupes}")
            total_dupes += hasdupes
    return total_dupes


def check_dir(path):
    print(f"\nDirectory: {path}") # TODO: print file count like in check_vpks()
    dir_id = os.path.basename(path)

    hasdupes = 0

    for (dirpath, dirnames, filenames) in os.walk(path):

        # this makes `dirpath` relative to `path`
        dirpath = (dirpath + '\\').replace(path + '\\', '')

        for file in filenames:

            if file.endswith('.vpk'):
                continue

            filepath = os.path.join(dirpath, file)
            if filepath in tree:

                if hasdupes == 0:
                    print("DUPLICATE FILE,SOURCE")

                hasdupes += 1

                print(f"{filepath},{tree[filepath]}")
            else:
                tree[filepath] = dir_id
    
    print(f"Total Duplicates: {hasdupes}")
    return hasdupes


for path in input_paths:

    grand_total_dupes += check_vpks(path)
    grand_total_dupes += check_dir(path)

print(f"\nGrand Total: {grand_total_dupes} duplicate files")
