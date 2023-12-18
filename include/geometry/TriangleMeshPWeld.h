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

#pragma once

#include "TriangleMesh.h"

namespace open3d {
namespace geometry {

struct CP{
public:
    int cid;
    int pid;
};

/// \class TriangleMesh
///
/// \brief Triangle mesh contains vertices and triangles represented by the
/// indices to the vertices.
///
/// Optionally, the mesh may also contain triangle normals, vertex normals and
/// vertex colors.
class TriangleMeshPWeld : public TriangleMesh {
public:
    /// \brief Default Constructor.
    TriangleMeshPWeld() : TriangleMesh() {}
    /// \brief Parameterized Constructor.
    ///
    /// \param vertices list of vertices.
    /// \param triangles list of triangles.
    TriangleMeshPWeld(const std::vector<Eigen::Vector3d> &vertices,
                 const std::vector<Eigen::Vector3i> &triangles) : 
                 TriangleMesh(vertices, triangles){};
    /// \brief Parameterized Constructor.
    ///
    /// \param triangle_mesh instance of open3d::geometry::TriangleMesh.
    TriangleMeshPWeld(const open3d::geometry::TriangleMesh& triangle_mesh) : 
                 TriangleMeshPWeld(triangle_mesh.vertices_, triangle_mesh.triangles_){};
    ~TriangleMeshPWeld() override {}

    virtual void reduce(std::vector<int>& pid2ccid, std::vector<Eigen::Vector3d>& new_vertices, const std::vector<int>& cp_vec, int num_vertices) const;

    virtual TriangleMeshPWeld& merge_vertices_forward(KDTreeFlann const& index, double eps); 
    virtual TriangleMeshPWeld& merge_vertices_forward_async(KDTreeFlann const& index, double eps);    

};

}  // namespace geometry
}  // namespace open3d
