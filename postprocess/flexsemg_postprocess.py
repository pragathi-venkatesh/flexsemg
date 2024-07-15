# Author: Pragathi Venkatesh 04 Jan 2024
#
# Code to convert from various forms of raw data and plot
# 
# Have to run through command line
# Usage: 
# for data log collected through NRF Connect app:
# $ python flexsemg_postprocess.py -src_type nrf_log -num_channels 1 -srate 1000 -fpath <datalog filepath>
# for data collected using rhd2216_util:
# $ python flexsemg_postprocess.py -src_type rhdutil_log -fpath <datalog filepath>
# for testing this module:
# $ python flexsemg_postprocess.py -test
#
# any functions that parse log files should return:
#   time: 1-d np.array of length N. in seconds
#   data: NxM np.array, where M is # of channels and N. in mV
#   properties: dictionary (for any misc properties)
# 

import sys
import re
import math
import argparse
import numpy as np
import os.path
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

def format_nrf_log_file(fpath, number_of_channels, sample_rate_hz):
    file = open(fpath, "r")
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

    properties = {
        "source": fpath,
        "number_of_channels": number_of_channels,
        "sample_rate_hz": sample_rate_hz,
    }
    return time, data, properties

def format_rhdutil_log_file(fpath):
    f = open(fpath, 'r')

    active_chs_mask = 0
    nsamples = 0
    srate = 0
    properties = {}

    try:
        # first line contains active chs msk
        line = f.readline()
        m = re.search(r"active_chs_mask: ([a-fA-F0-9]+)", line)
        active_chs_mask = int(m.group(1), 16)
        print(f"got active_chs_mask: 0x{active_chs_mask:x}")

        # second line contains nsamples 
        line = f.readline()
        m = re.search(r"num_samples: (\d+)", line)
        nsamples = int(m.group(1))
        print(f"got nsamples: {nsamples}")

        # third line contains sample rate
        line = f.readline()
        m = re.search(r"sample rate: (\d+) ", line)
        srate = int(m.group(1))
        # TODO handle things like kHz, MHz, and GHz (even though max srate is ~10000 Hz)
        print(f"got srate (assuming Hz): {srate}")
    except:
        expected_format = "active_chs_mask: xxxx\nnum_samples: ####\nsample rate: #### Hz"
        print("Error getting properties active_chs_mask, nsamples, and srate.")
        print(f"Please check that first 3 lines match format:\n{expected_format}")

    # read the next nsamples lines and add to data
    indeces = bitmask_to_indices(active_chs_mask)
    nrows = len(indeces)
    ncols = nsamples // nrows
    data = np.zeros([len(indeces), ncols])
    for col in range(ncols):
        for row in range(nrows):
            line = f.readline()
            data[row, col] = int(line, 16)
    time = np.arange(ncols) * (1/srate)
    f.close()

    # postprocess data
    data = np.vectorize(unsigned_to_twoscomp)(data)
    # convert data from raw int to voltage
    data = VLSB * data * 1000 # plot in mV

    # handle properties
    properties = {
        "active_chs_mask": active_chs_mask,
        "nsamples": nsamples,
        "srate (Hz)": srate,
    }
    return time, data, properties
      
def plot_nrf_data(time, data, title=""):
    number_of_channels = np.shape(data)[0]
    fig, axs = plt.subplots(number_of_channels, 1, sharex=True, sharey=True)
    for i in range(number_of_channels):
        axs[i].plot(time, data[i, :], linewidth=0.2)
    plt.suptitle(title)
    plt.show()

    return fig, axs

def plot_rhdutil_log_file(fpath):
    time, data, properties = format_rhdutil_log_file(fpath)
    nrows = np.shape(data)[0]
    indeces = bitmask_to_indices(properties["active_chs_mask"])

    if nrows > 1:
        fig, axs = plt.subplots(nrows=nrows, ncols=1, sharex=True, figsize=(8,16))
        for row in range(nrows):
            axs[row].plot(time, data[row])
            axs[row].set_ylabel("ch%02d (mV)" % indeces[row])
        axs[-1].set_xlabel("Time (s)")
        axs[0].set_title(os.path.basename(fpath))
        fig.show()
    else:
        # for only one row, plt.subplots returns fig, ax (only one axes) instead of list of axes
        # if we try to do axs[row] like for the nrows>2 case, we get this error:
        # TypeError: 'Axes' object is not subscriptable
        fig, axs = plt.subplots(nrows=nrows, ncols=1, sharex=True, figsize=(8,16))
        for row in range(nrows):
            axs.plot(time, data[row])
            axs.set_ylabel("ch%02d (mV)" % indeces[row])
        axs.set_xlabel("Time (s)")
        axs.set_title(os.path.basename(fpath))
        fig.show()

    return fig, axs


def do_test():
    print("============testing NRF log plotting===============")
    os.system(f"python  \"{__file__}\" -src_type nrf_log -srate 600 -num_channels 16 -fpath ./example_dlogs/example_nrf_log_16_chs.txt")
    print("============NRF plotting test done.================\n\n")
    print("============testing RHD util log plotting===============")
    os.system(f"python \"{__file__}\" -src_type rhdutil_log -fpath ./example_dlogs/example_rhd2216_util_log_20240525_1839_N1000.txt")
    print("============RHD log plotting test done.================\n\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-test", action="store_true", help="test code on example dlogs.")
    parser.add_argument("-src_type", action="store", type=str, choices=["nrf_log", "rhdutil_log"])
    parser.add_argument("-fpath", action="store", type=str, help="filepath of data")
    parser.add_argument("-num_channels", action="store", type=int, choices=list(range(1,17)))
    parser.add_argument("-srate", action="store", type=int, default=1, help="sampling rate in hz")
    args, unknown = parser.parse_known_args()

    if args.test:
        do_test()
        sys.exit()

    if not args.fpath:
        raise Exception("Please provide datalog filepath. See module docstring for usage.")
    elif not args.src_type:
        raise Exception("Please provide argument -src_type. See module docstring for usage.")
    
    if args.src_type == "nrf_log":
        time, data, properties = format_nrf_log_file(
            fpath=args.fpath, 
            number_of_channels=args.num_channels, 
            sample_rate_hz=args.srate,
            )
        print(f"Returned properties {properties}")
        fig, axs = plot_nrf_data(time,data,title=os.path.basename(args.fpath))
        input("Press enter to exit...")
    elif args.src_type == "rhdutil_log":
        # srate and active chs mask stored in rhd util log file. 
        # no need for user to provide
        time, data, properties = format_rhdutil_log_file(args.fpath)
        print(f"Returned properties {properties}")
        fig, axs = plot_rhdutil_log_file(args.fpath)
        input("Press enter to exit...")

            