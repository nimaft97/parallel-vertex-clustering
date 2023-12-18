#!/usr/bin/env python3

# ----------------------------------------------------------------------------
# MIT License
#
# Copyright (c) 2023 Nima Fathollahi, Sean Chester
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# ----------------------------------------------------------------------------


# calculate-epsilon-values.py
# reads all meshes in a directory and outputs to a file the epsilon values
# at which vertex clustering on those meshes removes 0.1%, 1%, 10%, and 50%
# of vertices, respectively.

import os, argparse, pathlib, csv, subprocess

def calculate_epsilons():
    my_parser = argparse.ArgumentParser(description="Takes all .PLY mesh files and computes epsilon values for 0.1%, 1%, 10%, and 50% reductions.")
    my_parser.add_argument('input_dir', metavar='input_directory', type=pathlib.Path, help='the directory containing the mesh files')
    my_parser.add_argument('dest_file', metavar='output_file', type=pathlib.Path, help='the .csv file in which to save the discovered epsilon values')
    my_parser.add_argument('epsilon_finder_app', metavar='eps_finder', type=pathlib.Path, help='The path to executable that discovers epsilon values')
    input_directory = my_parser.parse_args().input_dir
    output_file = my_parser.parse_args().dest_file
    epsilon_finder_executable = my_parser.parse_args().epsilon_finder_app
    num_threads = '2'

    all_files = [file.name for file in os.scandir(input_directory) if file.is_file() and pathlib.Path(file).suffix == ".ply"]
    reduction_rates = ['0.1', '1.0', '10.0', '50.0']
    all_epsilons = list()

    for sequence_num, filename in enumerate(all_files):
        print("Discovering epsilon values for mesh #" + str(sequence_num) + " of " + str(len(all_files)) + ": " + filename)
        try:
            file_path = os.path.join(input_directory, filename)
            epsilon_values = [subprocess.run([epsilon_finder_executable, file_path, eps, num_threads], stdout=subprocess.PIPE).stdout.decode('utf-8').split()[-1] for eps in reduction_rates]
            all_epsilons.append([filename] + epsilon_values)
        except:
            pass

    with open(output_file, "wt") as fp:
        writer = csv.writer(fp, delimiter=",")
        writer.writerows(all_epsilons)

if __name__ == "__main__":
    calculate_epsilons()
