#!/usr/bin/python3

import sys
import os
import subprocess

if len(sys.argv) < 3:
	print("Provide reference image and output file.")
	exit(1)

reference = sys.argv[1]
output = sys.argv[2]
path = os.getcwd() if len(sys.argv) == 3 else sys.argv[3]

def get_time(path):
	result = subprocess.run(["master", "time", path], stdout = subprocess.PIPE)
	return float(result.stdout.strip())

def get_errors(path, reference):
	result = subprocess.run(["master", "errors", path, reference], stdout = subprocess.PIPE)
	split = result.stdout.strip().split()
	return tuple(map(float, split[0:4])) + tuple(split[4:])

def get_technique(path):
	TECHNIQUES = ["BPT", "VCM", "UPG", "PT"]

	for technique in TECHNIQUES:
		if technique in path:
			return technique

	return "N/A"

PT = []
BPT = []
VCM = []
UPG = []

images = [entry.path for entry in os.scandir(path) if entry.path.endswith(".exr")]

for image in images:
	technique = get_technique(image)
	entry = (get_time(image),) + get_errors(image, reference) + (get_technique(image), os.path.basename(image))

	print(image)

	if technique == "PT":
		PT.append(entry)
	elif technique == "BPT":
		BPT.append(entry)
	elif technique == "VCM":
		VCM.append(entry)
	elif technique == "UPG":
		UPG.append(entry)

PT.sort(key = lambda x: x[0])
BPT.sort(key = lambda x: x[0])
VCM.sort(key = lambda x: x[0])
UPG.sort(key = lambda x: x[0])

def write_data(basename, technique, data):
	filename, file_extension = os.path.splitext(basename)
	file = open(filename + "." + technique + file_extension, "w+")

	for entry in data:
		file.write("{:16} {:16} {:16} {:6} # {}\n".format(*entry))

write_data(output, "pt", PT)
write_data(output, "bpt", BPT)
write_data(output, "vcm", VCM)
write_data(output, "upg", UPG)
