"""
2024-April-07 Pragathi Venkatesh (PV)
Python wrapper for rhd2216_util.c
+ option for plotting 

Usage:
# TODO
"""

import sys
import argparse
import subprocess

BIN_PATH = "./build/rhd2216_util"

parser = argparse.ArgumentParser()
parser.add_argument("--plot", action="store", type=str, help="filepath of rhd2216_util datalog.")
args, unknown = parser.parse_known_args()

def plot_rhd2216util_dlog():
    pass

if __name__ == "__main__":
    delimiter = " "
    cli_args = delimiter.join(sys.argv)
    cmd = f"{BIN_PATH} {cli_args}"
    print(f"Running subprocess: {cmd}")
    ret = subprocess.run(cmd, shell=True, capture_output=True)

    # plot generated fpath if "--convert or -c tag present in args"
    if ("--convert" in sys.argv) or ("-c" in sys.argv):
        pass

    
    