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

# convert-to-ply.py directory
# converts all files in a directory from any readable mesh into .PLY
# mostly designed to convert the .STL Thingi10k files to .PLY

import os, argparse, pathlib, meshio

def convert_to_ply():
    my_parser = argparse.ArgumentParser(description="Takes all mesh files and creates a new version in .PLY format.")
    my_parser.add_argument('input_dir', metavar='input_directory', type=pathlib.Path, help='the directory containing the mesh files')
    my_parser.add_argument('dest_dir', metavar='output_directory', type=pathlib.Path, help='the directory in which to save the .PLY meshes')
    input_directory = my_parser.parse_args().input_dir
    output_directory = my_parser.parse_args().dest_dir

    all_files = [file.name for file in os.scandir(input_directory) if file.is_file()]

    try:
        os.mkdir(output_directory)
    except FileExistsError:
        pass

    for sequence_num, filename in enumerate(all_files):
        print("Converting mesh #" + str(sequence_num) + " of " + str(len(all_files)))
        try:
            mesh = meshio.read(os.path.join(input_directory, filename))
            output_path = pathlib.Path(filename).stem + ".ply"
            mesh.write(os.path.join(output_directory, output_path))
        except:
            pass

if __name__ == "__main__":
    convert_to_ply()
