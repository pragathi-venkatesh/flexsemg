"""
2024-April-07 Pragathi Venkatesh (PV)
plotting datalogs created by rhd2216_util.c
requires a machine that can run matplotlib
DO NOT RUN ON PI, debian does not support matplotlib.

Usage:
# TODO
"""
import os
import re
import sys
import argparse
import numpy as np
from matplotlib import pyplot as plt

VLSB = 0.195E-6 # V per least-significant bit of ADC channel

def install_and_import(package):
    import importlib
    try:
        importlib.import_module(package)
    except ImportError:
        import pip
        pip.main(['install', package])
    finally:
        globals()[package] = importlib.import_module(package)

def bitmask_to_indices(bitmask):
    """
    Convert a bitmask to an array of indices where the bit is set to 1.
    
    :param bitmask: An integer representing the bitmask.
    :return: A list of indices where the bit is set to 1.

    from chatgpt.
    """
    return [index for index in range(bitmask.bit_length()) if bitmask & (1 << index)]

def unsigned_to_twoscomp(n):
    """
    Convert a 16-bit unsigned integer to a two's complement signed integer.
    
    :param n: A 16-bit unsigned integer.
    :return: The corresponding two's complement signed integer.

    from chatgpt
    """
    n = int(n)
    return n - 0x10000 if n & 0x8000 else n

def plot_rhd2216util_dlog(fpath):
    f = open(fpath, 'r')

    active_chs_mask = 0
    nsamples = 0
    srate = 0

    # first line contains active chs msk
    line = f.readline()
    m = re.search(r"active_chs_mask: ([a-fA-F0-9]+)", line)
    active_chs_mask = int(m.group(1), 16)
    print(f"got active chs mask: 0x{active_chs_mask:x}")

    # second line contains nsamples 
    line = f.readline()
    m = re.search(r"num_samples: (\d+)", line)
    nsamples = int(m.group(1))
    print(f"got nsamples: {nsamples}")

    # third line contains sample rate
    line = f.readline()
    m = re.search(r"sample rate: (\d+)", line)
    srate = int(m.group(1))
    print(f"got srate: {srate}")

    # read the next nsamples lines and add to data
    indeces = bitmask_to_indices(active_chs_mask)
    nrows = len(indeces)
    ncols = nsamples // nrows
    data = np.zeros([len(indeces), ncols])
    for col in range(ncols):
        for row in range(nrows):
            line = f.readline()
            data[row, col] = int(line, 16)
    times = np.arange(ncols) * (1/srate)
    f.close()

    # postprocess data
    data = np.vectorize(unsigned_to_twoscomp)(data)
    data = VLSB * data * 1000 # plot in mV

    # convert data from raw int to voltage
    # plot? ugh. but a bullet through my skull.
    fig, axs = plt.subplots(nrows=nrows, ncols=1, sharex=True, figsize=(8,16))
    for row in range(nrows):
        axs[row].plot(times, data[row])
        axs[row].set_ylabel("ch%02d (mV)" % indeces[row])
    axs[-1].set_xlabel("Time (s)")
    axs[0].set_title(os.path.basename(fpath))


if __name__ == "__main__":
    usage = "python rhd2216_util.py <DATALOG FILEPATH>"
    if len(sys.argv) < 2:
        print("Usage: " + usage)
        raise Exception("Please provide datalog filepath.")

    fpath = sys.argv[1]
    plot_rhd2216util_dlog(fpath)