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
sys.path.append("../postprocess")
from flexsemg_postprocess import plot_rhdutil_log_file

BIN_PATH = "~/flexsemg/rhd_diag/build/rhd2216_util"
PLOT_SCRIPT_PATH = "./postprocess/flexsemg_postprocess.py"
PI_USER = "prag"
PI_IP_ADDR = "10.203.229.1"
VLSB = 0.195E-6 # V per least-significant bit of ADC channel

if __name__ == "__main__":
    delimiter = " "
    cli_args = delimiter.join(sys.argv[1:])
    cmd = f"ssh {PI_USER}@{PI_IP_ADDR} {BIN_PATH} {cli_args}"
    print(f"Running subprocess: {cmd}")
    ret = subprocess.run(cmd, shell=True, capture_output=True)

    # plot generated fpath if "--convert or -c tag present in args"
    if ("--convert" in sys.argv) or ("-c" in sys.argv):
        # plot any files generated from running rhd2216_util
        fpath = None
        m = re.search(r"data stored in (.+)", ret.stdout.decode('utf-8'))
        if m:
            fpath = m.group(1)

        if fpath:
            plot_rhdutil_log_file(fpath)
            # scp file into local dlogs folder
            print("Copying generated datalog to local folder...")
            cmd = f"scp {PI_USER}@{PI_IP_ADDR}:{fpath} ./dlogs"
            subprocess.run(cmd, shell=True)
        else:
            print("INFO: no output datalog found, skipping plotting.")