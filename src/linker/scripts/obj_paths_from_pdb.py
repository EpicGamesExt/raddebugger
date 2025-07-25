import subprocess
import sys
import os

def get_sorted_objs(pdb_path):
    result = subprocess.run(["llvm-pdbutil", "dump", "--modules", pdb_path], stdout=subprocess.PIPE, text=True)
    lines = result.stdout.strip().split('\n')
    filtered_lines = [line for line in lines if line.startswith("Mod ")]
    # sort by the obj_path portion (line format: "Mod <imod> <obj_path>")
    def extract_path(line):  return line.split(maxsplit=2)[2].lower()
    sorted_lines = sorted(filtered_lines, key=extract_path)
    return sorted_lines

if __name__ == "__main__":
    sorted_objs = get_sorted_objs(sys.argv[1])
    for l in sorted_objs: print(l)
