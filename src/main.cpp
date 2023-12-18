// ----------------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2023 Nima Fathollahi, Sean Chester
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// ----------------------------------------------------------------------------

#include <iostream>
#include <vector>
#include <cassert>
#include <fstream>
#include <string>
#include <stdexcept>
#include <functional>
#include <omp.h>
#include "../include/Eigen/Dense"

#include "../include/io/TriangleMeshIO.h"
#include "../include/geometry/TriangleMeshPWeld.h"
#include "../include/geometry/KDTreeFlann.h"

enum class Version{OPEN3D=0, FORWARD, FORWARD_ASYNC};
std::string programName[] = {"Open3D", "forward", "forward_async"};

int main(int argc, char** argv){

  if (argc == 1){
    std::cout << "Enter the following:\n";
    std::cout << "\t-eps (e.g., 0.001)\n";
    std::cout << "\t-Version:\n\t\t0: Open3D, 1: forward, 2: forward_async\n";
    std::cout << "\t-path to data (must be .ply)\n";
    std::cout << "\t-number of cores for all parallel versions (default: 8)\n";
    std::cout << "\t-output path to write the reduced mesh (must end in .ply)";
    std::cout << "\t-e.g., ./main 0.001 1 ../src/data/xyzrgb_manuscript.ply [4] [../src/data/output.ply]\n";
    return 0;
  }

  const double eps = std::stod(argv[1]);
  const int version = std::stoi(argv[2]);
  const std::string dataPath = argv[3];
  const int numCores = (argc >= 5 ? std::stoi(argv[4]) : 1);
  const std::string outputDir = (argc >= 6 ? argv[5] : "");
  const bool verbose = true;

  std::cout << "Configuration:\n";
  std::cout << "\t-eps: " << eps << "\n";
  std::cout << "\t-program: " << programName[version] << "\n";
  std::cout << "\t-path to dataset: " + dataPath + "\n";
  std::cout << "--**--**--**--**--**--**--**--**--\n";

  auto mesh = open3d::geometry::TriangleMesh();
  open3d::io::ReadTriangleMesh(dataPath, mesh);
  open3d::geometry::KDTreeFlann kdtree(mesh);

  size_t num_vertices_after_reduction;

  auto mesh_pweld = open3d::geometry::TriangleMeshPWeld(mesh);
    
  if (verbose){
    std::cout << "number of original vertices: " << mesh_pweld.vertices_.size() << "\n";
    std::cout << "number of original triangles: " << mesh_pweld.triangles_.size() << "\n";
  }

  omp_set_num_threads(numCores); // set the number of cores for the entire program
  switch ((Version)version)
  {
    case Version::OPEN3D:
      mesh_pweld.MergeCloseVertices(kdtree, eps); // Open3D
      break;
    case Version::FORWARD:
      mesh_pweld.merge_vertices_forward(kdtree, eps); // forward
      break;
    case Version::FORWARD_ASYNC:
      mesh_pweld.merge_vertices_forward_async(kdtree, eps); // forward-async
      break;
    
  }

  num_vertices_after_reduction = mesh_pweld.vertices_.size();
  if (!outputDir.empty())
  {
    std::cout << "Writing the simplified mesh to: " << outputDir << "\n";
    open3d::io::WriteTriangleMesh(outputDir, mesh_pweld, false, true);
  }
  if (verbose){
    std::cout << "number of vertices after clustering: " << num_vertices_after_reduction << "\n";
  }
}
