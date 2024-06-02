# Author: Pragathi Venkatesh 04 Jan 2024
#
# Code to convert from various forms of raw data and plot
# 
# Have to run through command line
# Usage: 
# * e.g. for data log collected through NRF Connect app:
# >>> python flexsemg_postprocess.py -src_type nrf_log -num_channels 1 -srate 1000 -fpath "C:\Users\praga\Downloads\Log 2024-01-04 17_08_58.txt"
# 

import sys
import re
import math
import argparse
import numpy as np
import os.path
from matplotlib import pyplot as plt

VLSB = 0.195E-6 # V per least-significant bit of ADC channel

parser = argparse.ArgumentParser()
parser.add_argument("-test", action="store_true", help="test code on example dlogs.")
parser.add_argument("-src_type", action="store", type=str, choices=["nrf_log", "rhdutil_log"])
parser.add_argument("-fpath", action="store", type=str, help="filepath of data")
parser.add_argument("-num_channels", action="store", type=int, choices=list(range(1,17)))
parser.add_argument("-srate", action="store", type=int, default=1, help="sampling rate in hz")
args, unknown = parser.parse_known_args()

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

def format_nrf_log_file(file_name, number_of_channels, sample_rate_hz):
    file = open(file_name, "r")
    emg = []
    
    for line in file:
        pattern = re.compile(r"\"\(0x\)\s(.+)\" received")
        m = re.search(pattern, line)
        if m:
            n = 4 # 4 characters per data point (int16)
            data_str = re.sub(r"[^a-fA-F\d]","", m.group(1))
            data = [data_str[i:i+n] for i in range(0,len(data_str),n)] 
            for d in data:
                emg.append(int(d,16))

    file.close()
    
    # reshape
    data = []
    current_channel = 1
    index = 0
    number_points = math.ceil(len(emg) / number_of_channels)
    data = np.zeros((number_of_channels, number_points))

    for i in range(len(emg)):
        data[current_channel - 1, index] = emg[i]
        if current_channel == number_of_channels:
            current_channel = 1
            index += 1
        else:
            current_channel += 1

    data = np.array(data)
    time = np.arange(0, data.shape[1], 1)
    time = time / sample_rate_hz
    return time, data
            
def plot_nrf_data(time, data, title=""):
    number_of_channels = np.shape(data)[0]
    fig, axs = plt.subplots(number_of_channels, 1, sharex=True, sharey=True)
    for i in range(number_of_channels):
        axs[i].plot(time, data[i, :], linewidth=0.2)
    plt.suptitle(title)
    plt.show()

def plot_rhdutil_log_file(fpath):
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
    fig.show()

def do_test():
    print("============testing NRF log plotting===============")
    os.system(f"python -i {__file__} -src_type nrf_log -srate 600 -num_channels 16 -fpath ./example_dlogs/example_nrf_log_16_chs.txt")
    print("============NRF plotting test done.================\n\n")
    print("============testing RHD util log plotting===============")
    os.system(f"python -i {__file__} -src_type rhdutil_log -fpath ./example_dlogs/example_rhd2216_util_log_20240525_1839_N1000.txt")
    print("============RHD log plotting test done.================\n\n")

if __name__ == "__main__":
    if args.test:
        do_test()
        sys.exit()

    usage = "TODO" # lol TODO
    if not args.fpath:
        print("Usage" + usage)
        raise Exception("Please provide datalog filepath.")
    if args.src_type == "nrf_log":
        time, data = format_nrf_log_file(
            file_name=args.fpath, 
            number_of_channels=args.num_channels, 
            sample_rate_hz=args.srate,
            )
        plot_nrf_data(time,data,title=os.path.basename(args.fpath))
    elif args.src_type == "rhdutil_log":
        # srate and active chs mask stored in rhd util log file. 
        # no need for user to provide
        plot_rhdutil_log_file(args.fpath)

            