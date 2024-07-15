# Author: Pragathi Venkatesh 2024-Jan-04
#
# Code to convert from various forms of raw datalogs, and filter+plot signals.
# 
# Have to run through command line
# Usage: 
# run from flexsemg/postprocess directory.
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
from scipy.signal import butter, sosfilt

VLSB = 0.195E-6 # V per least-significant bit of ADC channel
NRF_LOG = "nrf_log"
RHDUTIL_LOG = "rhdutil_log"

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

    from chatgpt.
    """
    n = int(n)
    return n - 0x10000 if n & 0x8000 else n

def apply_bandpass_filter(data, srate, lowcut, highcut, order=4):
    """
    Apply a bandpass filter to each row of the data array.
    
    :param data (numpy.ndarray): An NxM array where each row is a signal.
    :param srate (float): sampling rate in Hz.
    :param lowcut (float): The low frequency cutoff for the bandpass filter.
    :param highcut (float): The high frequency cutoff for the bandpass filter.
    :param order (int): The order of the filter. Default is 4.
    
    :return filtered_data (numpy.ndarray): The filtered data array.

    from chatgpt.
    """
    # design the bandpass filter
    sos = butter(order, [lowcut, highcut], btype='band', fs=srate, output='sos')
    # apply the filter to each row of the data array
    filtered_data = np.array([sosfilt(sos, row) for row in data])
    return filtered_data

def get_fft(data, srate):
    """
    :param data: NxM array
    :param srate: sample rate in Hz
    :return: freqs (Nx1 array) and fft (NxM array)

    NOTE: uses rfft since the signals are always real
    """
    # compute the fft for each row
    fft = np.fft.rfft(data, axis=1)
    # get corresponding frequences
    N = data.shape[0]
    M = data.shape[1]
    fft_M = np.shape(fft)[1]
    freqs = np.fft.fftfreq(M, d=1/srate)[0:fft_M]
    return freqs, fft

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
        "src": fpath,
        "src_type": NRF_LOG,
        "channels": list(range(number_of_channels)),
        "srate_hz": sample_rate_hz,
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
    indices = bitmask_to_indices(active_chs_mask)
    nrows = len(indices)
    ncols = nsamples // nrows
    data = np.zeros([len(indices), ncols])
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
        "channels": indices,
        "src": fpath,
        "src_type": RHDUTIL_LOG,
        "active_chs_mask": active_chs_mask,
        "nsamples": nsamples,
        "srate_hz": srate,
    }
    return time, data, properties
      
def plot_data(time, data, properties, plot_fft=True):
    nrows = np.shape(data)[0]
    ncols = 1
    srate_hz = 1
    freqs = None
    fft = None
    title = ""
    channels = ["?"] * nrows

    # get properties
    if 'src' in properties:
        title = properties['src']
    
    if 'srate_hz' in properties:
        srate_hz = properties['srate_hz']
    else:
        srate_hz = np.diff(time)[0]

    if 'channels' in properties:
        channels = properties['channels']
    
    # add another column to plot fft (if plot_fft specified)
    if plot_fft:
        freqs, fft = get_fft(data, srate_hz)
        ncols = 2

    fig, axs = plt.subplots(nrows=nrows, ncols=ncols, figsize=(8,16))
    if nrows==1:
        axs = [axs]
    if ncols==1:
        axs = [[ax] for ax in axs]
    
    # plot signal
    for row in range(nrows):
        axs[row, 0].plot(time, data[row], c='royalblue')
        axs[row, 0].set_ylabel("ch %s (mV)" % str(channels[row]))
        axs[row, 0].sharey(axs[0,0])
        axs[row, 0].sharex(axs[0,0])
        # plot fft in right column
        if plot_fft:
            axs[row, 1].plot(freqs, np.abs(fft[row]), c='orangered')
            axs[row, 1].sharey(axs[0,1])
            axs[row, 1].sharex(axs[0,1])

    # add labels 
    axs[0,0].set_title("signal")
    axs[-1, 0].set_xlabel("Time (s)")
    if plot_fft:
        axs[-1, 1].set_xlabel("Frequency (Hz)")
        axs[0,1].set_title("FFT")


    plt.tight_layout()

    return fig, axs

def do_test():
    print("============testing NRF log plotting===============")
    os.system(f"python  \"{__file__}\" -src_type nrf_log -srate 600 -num_channels 16 -fpath ./example_dlogs/example_nrf_log_16_chs.txt")
    print("============NRF plotting test done.================\n\n")
    print("============testing RHD util log plotting===============")
    os.system(f"python \"{__file__}\" -src_type rhdutil_log -fpath ./example_dlogs/example_rhd2216_util_convert_20240713_2357_N100000.txt")
    print("============RHD log plotting test done.================\n\n")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-test", action="store_true", help="test code on example dlogs.")
    parser.add_argument("-src_type", action="store", type=str, choices=["nrf_log", "rhdutil_log"])
    parser.add_argument("-fpath", action="store", type=str, help="filepath of data")
    parser.add_argument("-num_channels", action="store", type=int, choices=list(range(1,17)))
    parser.add_argument("-srate", action="store", type=int, default=1, help="sampling rate in hz")
    parser.add_argument("--fft", action='store_true', help="flag to plot fft along with time-domain data")
    args, unknown = parser.parse_known_args()

    if args.test:
        do_test()
        print("Test done, exiting.")
        sys.exit()

    if not args.fpath:
        raise Exception("Please provide datalog filepath. See module docstring for usage.")
    elif not args.src_type:
        raise Exception("Please provide argument -src_type. See module docstring for usage.")
    
    plt.ion() # turn on interactive plots
    if args.src_type == "nrf_log":
        time, data, properties = format_nrf_log_file(
            fpath=args.fpath, 
            number_of_channels=args.num_channels, 
            sample_rate_hz=args.srate,
            )
        print(f"Returned properties {properties}")
    elif args.src_type == "rhdutil_log":
        # srate and active chs mask stored in rhd util log file. 
        # no need for user to provide
        time, data, properties = format_rhdutil_log_file(args.fpath)
        print(f"Returned properties {properties}")

    fig, axs = plot_data(time, data, properties)
    plt.show()
    input("Press enter to exit...") 

    
"""
lowcut, highcut = 5, 1000
print(f"Filtering data with lowcut-highcut {lowcut}-{highcut} Hz.")
filtered_data = apply_bandpass_filter(data, properties['srate_hz'], lowcut, highcut, order=4) # TODO default when no srate in properties
fig, axs = plot_data(time[200:], filtered_data[:,200:], properties)
plt.show()
"""