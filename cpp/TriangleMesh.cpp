// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018-2021 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include <numeric>
#include <queue>
#include <tuple>
#include <omp.h>

// nima
#include <iostream>
#include <iterator>
#include <algorithm>

#include "../include/Eigen/Dense"
#include "../include/geometry/TriangleMesh.h"

namespace open3d {
namespace geometry {

TriangleMesh &TriangleMesh::Clear() {
    MeshBase::Clear();
    triangles_.clear();
    triangle_normals_.clear();
    adjacency_list_.clear();

    return *this;
}

TriangleMesh &TriangleMesh::ComputeTriangleNormals(
        bool normalized /* = true*/) {
    triangle_normals_.resize(triangles_.size());
    for (size_t i = 0; i < triangles_.size(); i++) {
        auto &triangle = triangles_[i];
        Eigen::Vector3d v01 = vertices_[triangle(1)] - vertices_[triangle(0)];
        Eigen::Vector3d v02 = vertices_[triangle(2)] - vertices_[triangle(0)];
        triangle_normals_[i] = v01.cross(v02);
    }
    if (normalized) {
        NormalizeNormals();
    }
    return *this;
}

TriangleMesh &TriangleMesh::ComputeVertexNormals(bool normalized /* = true*/) {
    ComputeTriangleNormals(false);
    vertex_normals_.resize(vertices_.size(), Eigen::Vector3d::Zero());
    for (size_t i = 0; i < triangles_.size(); i++) {
        auto &triangle = triangles_[i];
        vertex_normals_[triangle(0)] += triangle_normals_[i];
        vertex_normals_[triangle(1)] += triangle_normals_[i];
        vertex_normals_[triangle(2)] += triangle_normals_[i];
    }
    if (normalized) {
        NormalizeNormals();
    }
    return *this;
}

TriangleMesh &TriangleMesh::ComputeAdjacencyList() {
    adjacency_list_.clear();
    adjacency_list_.resize(vertices_.size());
    for (const auto &triangle : triangles_) {
        adjacency_list_[triangle(0)].insert(triangle(1));
        adjacency_list_[triangle(0)].insert(triangle(2));
        adjacency_list_[triangle(1)].insert(triangle(0));
        adjacency_list_[triangle(1)].insert(triangle(2));
        adjacency_list_[triangle(2)].insert(triangle(0));
        adjacency_list_[triangle(2)].insert(triangle(1));
    }
    return *this;
}

TriangleMesh &TriangleMesh::MergeCloseVertices(KDTreeFlann const& kdtree, double eps) {
    // precompute all neighbours
    std::vector<std::vector<int>> nbs;
    nbs = std::vector<std::vector<int>>(vertices_.size());
    #pragma omp parallel for schedule(static)
    for (int idx = 0; idx < int(vertices_.size()); ++idx) {
        std::vector<double> dists2;
        kdtree.SearchRadius(vertices_[idx], eps, nbs[idx], dists2);
    }

    std::vector<Eigen::Vector3d> new_vertices;
    std::unordered_map<int, int> new_vert_mapping;

    for (int vidx = 0; vidx < int(vertices_.size()); ++vidx) {
        if (new_vert_mapping.count(vidx) > 0) {
            continue;
        }

        int new_vidx = int(new_vertices.size());
        new_vert_mapping[vidx] = new_vidx;

        Eigen::Vector3d vertex = vertices_[vidx];
        int n = 1;
        for (int nb : nbs[vidx]) {
            if (vidx == nb || new_vert_mapping.count(nb) > 0) {
                continue;
            }
            vertex += vertices_[nb];
            new_vert_mapping[nb] = new_vidx;
            n += 1;
        }
        new_vertices.push_back(vertex / n);
    }
    std::swap(vertices_, new_vertices);

    for (auto &triangle : triangles_) {
        triangle(0) = new_vert_mapping[triangle(0)];
        triangle(1) = new_vert_mapping[triangle(1)];
        triangle(2) = new_vert_mapping[triangle(2)];
    }

    return *this;
}

}  // namespace geometry
}  // namespace open3d
