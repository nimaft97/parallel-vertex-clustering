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

# copy output logs to the outputs folder
# remove err files
# put this python file in the scripts folder
# run this python file to extract data

import re
import pathlib
import sys

if len(sys.argv) > 1:
    WORK_DIR = sys.argv[1]
WORK_DIR = pathlib.Path(WORK_DIR)

def extractNumber(s: str) -> str:
    return "".join(re.findall(r"(\d+(?:\.\d+)?)(e[+-]{0,1}\d+)?", s)[0])

def extractRow(s: str, pattern: str) -> str:
    return re.findall(pattern, s)[0]

def write2File(dataset_name: str, eps: str, algorithm_name: str, thread_num: str, time_all: str,
               time_p:str, time_c: str, time_w: str, time_s: str, 
               time_unv: str, time_ur: str, time_u: str, batchSize: str, 
               num_iterations: str, num_original_vertices: str, num_simplified_vertices: str):
    header = "threadNum,timeAll,timeP,timeC,timeW,timeS,timeUNV,timeUR,timeU,batch,numIter,numOrgVerts,numSimpVerts\n"
    # create the directory if it doesn't exist
    pathlib.Path(WORK_DIR / dataset_name).mkdir(parents=True, exist_ok=True)
    # create the directory if it doesn't exist
    pathlib.Path(WORK_DIR / dataset_name / algorithm_name).mkdir(parents=True, exist_ok=True)
    file_path = pathlib.Path(WORK_DIR / dataset_name / algorithm_name / f"eps{eps}.dat")
    # if file eps<eps>.dat doesn't already exist, make it
    if not file_path.is_file():
        file_path.touch()
        with file_path.open("w") as f:
            f.write(header)
        
    # write the passed content to the file
    with file_path.open("a") as f:
        f.write(f"{thread_num},{time_all},{time_p},{time_c},{time_w},{time_s},{time_unv},{time_ur},{time_u},{batchSize},{num_iterations},{num_original_vertices},{num_simplified_vertices}\n")

def sortFileContent(path: pathlib.Path):
    lines = ""
    with path.open("r") as f:
        lines = f.readlines()
    if len(lines) == 0:
        return
    with path.open("w") as f:
        header = lines[0]
        f.write(f"{header}")
        body = lines[1:]
        body.sort(key=lambda x: int(x.split(',')[0]))
        for line in body:
            f.write(f"{line}")



if __name__ == "__main__":
    if not WORK_DIR.is_dir():
        print("work dir doesn't exist")
    
    for file in WORK_DIR.iterdir():
        if (file.is_file()):
            eps = dataset_name = algorithm_name = thread_num = time_p = time_c = ""
            with file.open() as f:
                name = file.stem
                eps = extractNumber(extractRow(name, r"eps.+(?=alg)"))
                dataset_name = extractRow(name, r"(?<=data).*").strip()
                algorithm_name = extractRow(name, r"(?<=alg).+(?=n\d)")
                batchSize = "-1"
                if (algorithm_name.find("batch") >= 0):
                    batchSize = extractNumber(extractRow(name, r"batchSize.+(?=eps)"))
                thread_num = extractRow(name, r"(?<=t)\d+(?=data)")
                #print(f"name: {name}, eps: {eps}, dataset_name: {dataset_name}, algorithm_name: {algorithm_name}, thread_num: {thread_num}")
                time_all = "" # avg_time
                time_p = "" # population
                time_c = "" # clustering
                time_s = "-1" # single region
                time_w = "-1" # while loop
                time_u = "-1" # update mesh
                time_ur = "-1" # update representatives
                time_unv = "-1" # update new vertices
                num_iterations = "-1"
                num_original_vertices = "-1"
                num_simplified_vertices = "-1"
                lines = f.readlines()
                for line in lines:
                    if (line.find("average time") > -1):
                        time_all = extractNumber(line)
                    if (line.find("adj list took") > -1): 
                        time_p = extractNumber(line)
                    elif (line.find("Clustering took") > -1): 
                        time_c = extractNumber(line)
                    elif (line.find("Update mesh took") > -1):
                        time_u = extractNumber(line)
                    elif (line.find("Single region") > -1):
                        time_s = extractNumber(line)
                    elif (line.find("While loop took") > -1):
                        time_w = extractNumber(line)
                    elif (line.find("Update new vertices took") > -1):
                        time_unv = extractNumber(line)
                    elif (line.find("Update representatives took") > -1):
                        time_ur = extractNumber(line)
                    elif (line.find("numIterations") > -1):
                        num_iterations = extractNumber(line)
                    elif (line.find("original vertices") > -1):
                        num_original_vertices = extractNumber(line)
                    elif (line.find("vertices after") > -1):
                        num_simplified_vertices = extractNumber(line)
                    else: pass
            # everything is extracted, write them to file
            if (time_p):  write2File(dataset_name, eps, algorithm_name, thread_num, time_all,
                                     time_p, time_c, time_w, time_s, time_unv, time_ur, time_u, batchSize,
                                     num_iterations, num_original_vertices, num_simplified_vertices)

    for dataset_path in WORK_DIR.iterdir():
        if dataset_path.is_dir():
            for algorithm_path in dataset_path.iterdir():
                if algorithm_path.is_dir():
                    for file in algorithm_path.iterdir():
                        if file.is_file():
                            sortFileContent(file)




    

