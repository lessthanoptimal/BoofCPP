#!/usr/bin/env python3

import optparse
import os
import sys
from os.path import join
import json

# Create the plot showing a summary by category
#import matplotlib.pyplot as plt
#import numpy as np

# Handle command line options
p = optparse.OptionParser()
p.add_option('--Input', '-i', default="build/reports/benchmark/result.json",help="Directory containing JMH results")

options, arguments = p.parse_args()

input_path = options.Input
f = open(input_path)
if not f:
    print("Failed to open {}".format(input_path))
    exit(1)

data = json.load(f)

# Create a list with suffix _java and _native
# test name is last period to last _
# relative bar chat with java as the baseline
# use 50% metric

for benchmark in data:
    print(benchmark['benchmark'])
