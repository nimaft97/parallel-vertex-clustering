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

import re
import pathlib
import sys

if len(sys.argv) > 1:
    WORK_DIR = sys.argv[1]
WORK_DIR = pathlib.Path(WORK_DIR)
CSV_DIR = WORK_DIR / "csv"

def writeToCsv(csv_file, path: pathlib.Path, dataset_name: str, algorithm_name: str, eps: str):
    print(f"input params: dataset: {dataset_name}, alg: {algorithm_name}, eps: {eps}")
    lines = ""
    with path.open("r") as f:
        lines = f.readlines()
    
    if len(lines) == 0:
        return
    header = lines[0]
    body = lines[1:]

    for line in body:
        tokens = line.split(",")
        cores = tokens[0]
        timeAll = tokens[1]
        timeP = tokens[2]
        timeC = tokens[3]
        timeW = tokens[4]
        timeS = tokens[5]
        timeUNV = tokens[6]
        timeUR = tokens[7]
        timeU = tokens[8]
        batch = tokens[9]
        num_iterations = tokens[10]
        num_original_vertices = tokens[11]
        num_simplified_vertices = tokens[12].strip()
        csv_file.write(f"{algorithm_name},{dataset_name},{cores},{eps},{batch},{timeAll},{timeP},{timeC},{timeW},{timeS},{timeUNV},{timeUR},{timeU},{num_iterations},{num_original_vertices},{num_simplified_vertices}\n")

if __name__ == "__main__":
    # create the directory for plots if doesn't exist
    CSV_DIR.mkdir(parents=True, exist_ok=True)
    with open(str(CSV_DIR / "all_data.csv"), "w") as csv_file:
        csv_file.write("Algorithm,Dataset,Core,Eps,Batch,timeAll,timeP,timeC,timeW,timeS,timeUNV,timeUR,timeU,numIter,numOrgVerts,numSimpVerts\n")
    
    with open(str(CSV_DIR / "all_data.csv"), "a") as csv_file:
        for dataset_path in WORK_DIR.iterdir():
            if dataset_path.is_dir():
                dataset_name = dataset_path.name
                for algorithm_path in dataset_path.iterdir():
                    if algorithm_path.is_dir():
                        algorithm_name = algorithm_path.name
                        for file in algorithm_path.iterdir():
                            if file.is_file():
                                eps = re.findall(r"[-+]?(?:\d*\.*\d+)", file.stem)[0]
                                writeToCsv(csv_file, file, dataset_name, algorithm_name, eps)