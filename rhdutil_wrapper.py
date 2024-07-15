"""
2024-April-07 Pragathi Venkatesh (PV)
Python wrapper for rhd2216_util.c then plots.
Run on a windows machine that has ssh access
to your pi.

Test commands:
python prag_rhdutil_wrapper
"""
import os
import re
import sys
import argparse
import subprocess
import numpy as np
from matplotlib import pyplot as plt
sys.path.append("./postprocess")
from flexsemg_postprocess import plot_rhdutil_log_file

BIN_PATH = "./build/rhd2216_util"
PLOT_SCRIPT_PATH = "./postprocess/flexsemg_postprocess.py"
PI_USER = "prag"
PI_IP_ADDR = "10.0.0.228" #"10.203.229.1"
VLSB = 0.195E-6 # V per least-significant bit of ADC channel

if __name__ == "__main__":
    delimiter = " "
    cli_args = delimiter.join(sys.argv[1:])
    cmd = f"ssh {PI_USER}@{PI_IP_ADDR} \"cd /home/prag/flexsemg/rhd_diag;{BIN_PATH} {cli_args}\""
    print(f"Running subprocess: {cmd}")
    ret = subprocess.run(cmd, shell=True, capture_output=True)
    print("Printing subprocess output:")
    print(ret.stdout.decode('utf-8'))

    # plot generated fpath if "--convert or -c tag present in args"
    if ("--convert" in sys.argv) or ("-c" in sys.argv):
        # plot any files generated from running rhd2216_util
        fpath = None
        m = re.search(r"data stored in (.+)", ret.stdout.decode('utf-8'))
        if m:
            fpath = m.group(1)

        if fpath:
            abs_fpath = "/home/prag/flexsemg/rhd_diag/" + fpath
            dest_fpath = "./rhd_diag/" + fpath
            # scp file into local dlogs folder
            print("Copying generated datalog to local folder...")
            cmd = f"scp {PI_USER}@{PI_IP_ADDR}:{abs_fpath} {dest_fpath}"
            ret = subprocess.run(cmd, shell=True)
            # finally plot
            plt.ion() # enable interactive plots
            fig, axs = plot_rhdutil_log_file(dest_fpath)
            input("Press enter to close plot and exit script...")
        else:
            print("INFO: no output datalog found, skipping plotting.")