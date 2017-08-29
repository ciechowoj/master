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
test_cases = list(filter(lambda x: "50" not in x, test_cases))

if not os.path.exists("test_results"):
    os.makedirs("test_results")

num_minutes = 20

def test_command_and_name(test_case, technique, beta, from_light):
    name = os.path.join(
        "test_results",
        os.path.basename(test_case[:-6]) + "." +
        technique + str(beta) +
        ("" if technique == "BPT" else (".from_light" if from_light else ".from_camera")) +
        ".exr")

    command = (["master", test_case,
        "--" + technique,
        "--parallel",
        "--beta=" + str(beta),
        "--output=" + name,
        "--num-minutes=" + str(num_minutes),
        "--batch"] +
        ([] if technique == "BPT" else ["--from-light" if from_light else "--from-camera"]))

    return (command, name)

def make_tests_list():
    commands_and_names = []

    for test_case in test_cases:
        for technique in ["BPT"]:
            for beta in [2]:
                commands_and_names += [test_command_and_name(test_case, technique, beta, True)]
                commands_and_names += [test_command_and_name(test_case, technique, beta, False)]

    results = []

    for item in commands_and_names:
        command, name = item

        if os.path.exists(name):
            results += [(command, name, average(name))]
        else:
            results += [(command, name, None)]

    return results

def average(path):
    result = subprocess.run(["master", "avg", path], stdout=subprocess.PIPE)
    result = list(map(float, result.stdout.split()))

    if len(result) == 3:
        return result
    else:
        None

def error(vec):
    expected = [0.01] * 3

    if vec:
        return sum([(y - x) ** 2 for x,y in zip(vec, expected)])
    else:
        return sum([(y - x) ** 2 for x,y in zip([0, 0, 0], expected)])

def main():
    test_list = sorted(make_tests_list(), key = lambda x: -error(x[2]))

    while True:
        top = test_list[0]

        print(top[1])

        command = (["master", "continue", top[1], "--num-minutes=" + str(num_minutes)]
            if top[2] else top[0])

        print(command)
        subprocess.run(command)

        avg = average(top[1])

        if avg != None:
            test_list[0] = (top[0], top[1], avg)
        else:
            test_list = test_list[1:]

        test_list.sort(key = lambda x: -error(x[2]))

main()

