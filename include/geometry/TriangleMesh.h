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

#pragma once

#include <memory>
#include <numeric>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <atomic>
#include <memory>

#include "../Eigen/Core"
#include "MeshBase.h"
#include "../include/geometry/KDTreeFlann.h"

namespace open3d {
namespace geometry {

/// \class TriangleMesh
///
/// \brief Triangle mesh contains vertices and triangles represented by the
/// indices to the vertices.
///
/// Optionally, the mesh may also contain triangle normals, vertex normals and
/// vertex colors.
class TriangleMesh : public MeshBase {
public:
    /// \brief Default Constructor.
    TriangleMesh() : MeshBase(Geometry::GeometryType::TriangleMesh) {}
    /// \brief Parameterized Constructor.
    ///
    /// \param vertices list of vertices.
    /// \param triangles list of triangles.
    TriangleMesh(const std::vector<Eigen::Vector3d> &vertices,
                 const std::vector<Eigen::Vector3i> &triangles)
        : MeshBase(Geometry::GeometryType::TriangleMesh, vertices),
          triangles_(triangles) {}
    ~TriangleMesh() override {}

public:
    virtual TriangleMesh &Clear() override;

public:

    /// Returns `true` if the mesh contains triangles.
    bool HasTriangles() const {
        return vertices_.size() > 0 && triangles_.size() > 0;
    }

    /// Returns `true` if the mesh contains triangle normals.
    bool HasTriangleNormals() const {
        return HasTriangles() && triangles_.size() == triangle_normals_.size();
    }

    /// Returns `true` if the mesh contains adjacency normals.
    bool HasAdjacencyList() const {
        return vertices_.size() > 0 &&
               adjacency_list_.size() == vertices_.size();
    }

    /// Normalize both triangle normals and vertex normals to length 1.
    TriangleMesh &NormalizeNormals() {
        MeshBase::NormalizeNormals();
        for (size_t i = 0; i < triangle_normals_.size(); i++) {
            triangle_normals_[i].normalize();
            if (std::isnan(triangle_normals_[i](0))) {
                triangle_normals_[i] = Eigen::Vector3d(0.0, 0.0, 1.0);
            }
        }
        return *this;
    }

/// \brief Function that will merge close by vertices to a single one.
/// The vertex position, normal and color will be the average of the
/// vertices.
///
/// \param eps defines the maximum distance of close by vertices.
/// This function might help to close triangle soups.
TriangleMesh &MergeCloseVertices(KDTreeFlann const& index, double eps);      

    /// \brief Function to compute triangle normals, usually called before
    /// rendering.
    TriangleMesh &ComputeTriangleNormals(bool normalized = true);

    /// \brief Function to compute vertex normals, usually called before
    /// rendering.
    TriangleMesh &ComputeVertexNormals(bool normalized = true);

    /// \brief Function to compute adjacency list, call before adjacency list is
    /// needed.
    TriangleMesh &ComputeAdjacencyList();

public:
    /// List of triangles denoted by the index of points forming the triangle.
    std::vector<Eigen::Vector3i> triangles_;
    /// Triangle normals.
    std::vector<Eigen::Vector3d> triangle_normals_;
    /// The set adjacency_list[i] contains the indices of adjacent vertices of
    /// vertex i.
    std::vector<std::unordered_set<int>> adjacency_list_;

};

}  // namespace geometry
}  // namespace open3d
