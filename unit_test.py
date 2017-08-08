#!/usr/bin/python3

import glob
import re
import subprocess
import os
import os.path

def atoi(text):
    return int(text) if text.isdigit() else text

def natural_keys(text):
    '''
    alist.sort(key=natural_keys) sorts in human order
    http://nedbatchelder.com/blog/200712/human_sorting.html
    (See Toothy's implementation in the comments)
    '''
    return [ atoi(c) for c in re.split('(\d+)', text) ]

test_cases = sorted(glob.glob("models/TestCase*.blend"), key=natural_keys)

if not os.path.exists("test_results"):
    os.makedirs("test_results")

num_samples = 10000

def run_test(test_case, technique, beta):
	output = os.path.join("test_results", os.path.basename(test_case[:-6]) + "." + technique + str(beta) + ".exr")
	command = ["master", test_case, "--" + technique, "--parallel", "--beta=" + str(beta), "--output=" + output, "--num-samples=" + str(num_samples), "--batch"]
	print(" ".join(command))
	subprocess.run(command)

for test_case in test_cases:
	for technique in ["PT", "BPT", "UPG"]:
		for beta in [0, 1, 2]:
			run_test(test_case, technique, beta)
