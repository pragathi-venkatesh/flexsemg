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

def reshape_to_num_channels(emg, number_of_channels):
    """
    emg: 
    """
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

    return data

def format_nrf_log_file(file_name, number_of_channels, sample_rate_hz):
    file = open(file_name, "r")
    emg = []
    
    for line in file:
        pattern = re.compile(r"\"\(0x\)\s(.+)\" received")
        m = re.search(pattern, line)
        if m:
            n = 4 # 4 characters per data point (int16)
            data_str = re.sub("[^a-fA-F\d]","", m.group(1))
            data = [data_str[i:i+n] for i in range(0,len(data_str),n)] 
            for d in data:
                emg.append(int(d,16))

    file.close()
          
    data = reshape_to_num_channels(emg,number_of_channels) 

    data = np.array(data)
    time = np.arange(0, data.shape[1], 1)
    time = time / sample_rate
    return time, data

def format_rhdutil_log_file(file_name, active_chs_mask, sample_rate_hz):
    time = []
    data = []
    return time, data
            
def plot_emg_data(time, data, title=""):
    number_of_channels = np.shape(data)[0]
    for i in range(number_of_channels):
        plt.subplot(number_of_channels, 1, (i + 1))
        plt.plot(time, data[i, :], linewidth=0.2)
        plt.title(title)
        plt.show()

parser = argparse.ArgumentParser()
parser.add_argument("-src_type", type=str, choices=["nrf_log", "rhdutil_log"])
parser.add_argument("-fpath", type=str, help="filepath of data")
parser.add_argument("-num_channels", type=int, choices=list(range(1,17)))
parser.add_argument("-active_chs_mask", type=lambda x: int(x,16))
parser.add_argument("-srate", type=int, default=1, help="sampling rate in hz")
args, unknown = parser.parse_known_args()

if __name__ == "__main__":
    if args.src_type == "nrf_log":
        time, data = format_nrf_log_file(
            file_name=args.fpath, 
            number_of_channels=args.num_channels, 
            sample_rate_hz=args.srate,
            )
    elif args.src_type == "rhdutil_log":
        time, data = format_nrf_log_file(
            file_name=args.fpath, 
            active_chs_mask=args.active_chs_mask, 
            sample_rate_hz=args.srate,
            )
    plot_emg_data(time,data,title=os.path.basename(args.fpath))