// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018-2021 www.open3d.org
// Copyright (c) 2023 Nima Fathollahi, Sean Chester
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
#include <vector>

#include "../Eigen/Core"
#include "Geometry.h"
#include "KDTreeSearchParam.h"
#include "../pipelines/registration/Feature.h"

/// @cond
namespace nanoflann {
struct metric_L2;
template <class MatrixType, int DIM, class Distance, bool row_major>
struct KDTreeEigenMatrixAdaptor;
}  // namespace nanoflann
/// @endcond

namespace open3d {
namespace geometry {

/// \class KDTreeFlann
///
/// \brief KDTree with FLANN for nearest neighbor search.
class KDTreeFlann {
public:
    /// \brief Default Constructor.
    KDTreeFlann();
    /// \brief Parameterized Constructor.
    ///
    /// \param data Provides set of data points for KDTree construction.
    KDTreeFlann(const Eigen::MatrixXd &data);
    /// \brief Parameterized Constructor.
    ///
    /// \param geometry Provides geometry from which KDTree is constructed.
    KDTreeFlann(const Geometry &geometry);
    /// \brief Parameterized Constructor.
    ///
    /// \param feature Provides a set of features from which the KDTree is
    /// constructed.
    KDTreeFlann(const pipelines::registration::Feature &feature);
    ~KDTreeFlann();
    KDTreeFlann(const KDTreeFlann &) = delete;
    KDTreeFlann &operator=(const KDTreeFlann &) = delete;

public:
    /// Sets the data for the KDTree from a matrix.
    ///
    /// \param data Data points for KDTree Construction.
    bool SetMatrixData(const Eigen::MatrixXd &data);
    /// Sets the data for the KDTree from geometry.
    ///
    /// \param geometry Geometry for KDTree Construction.
    bool SetGeometry(const Geometry &geometry);
    /// Sets the data for the KDTree from the feature data.
    ///
    /// \param feature Set of features for KDTree construction.
    bool SetFeature(const pipelines::registration::Feature &feature);

    template <typename T>
    int Search(const T &query,
               const KDTreeSearchParam &param,
               std::vector<int> &indices,
               std::vector<double> &distance2) const;

    template <typename T>
    int SearchKNN(const T &query,
                  int knn,
                  std::vector<int> &indices,
                  std::vector<double> &distance2) const;

    template <typename T>
    int SearchRadius(const T &query,
                     double radius,
                     std::vector<int> &indices,
                     std::vector<double> &distance2) const;

    /*
     * Populates indices vector with the id of points within a given radius
     * whose ids are not larger than the given query point.
     * Returns the size of that vector or -1 if there is an error.
    */
    template <typename T>
    int SearchRadiusNotGreater (
        const T &query,
        double radius,
        std::vector<int> &indices,
        [[maybe_unused]]std::vector<double> &distance2,
        int currentIdx
    ) const;

    /*
    returns the number of neighbours with smaller ids and 
    populates indices_bigger vector with the id of neighbours with bigger ids
    */
    template <typename T>
    int SearchRadiusSmallerAndBigger(const T &query,
                         double radius,
                         std::vector<int> &indices_bigger,
                         [[maybe_unused]]std::vector<double> &distance2,
                         const int currentIdx) const;

    template <typename T>
    int SearchHybrid(const T &query,
                     double radius,
                     int max_nn,
                     std::vector<int> &indices,
                     std::vector<double> &distance2) const;

private:
    /// \brief Sets the KDTree data from the data provided by the other methods.
    ///
    /// Internal method that sets all the members of KDTree by data provided by
    /// features, geometry, etc.
    bool SetRawData(const Eigen::Map<const Eigen::MatrixXd> &data);

protected:
    using KDTree_t = nanoflann::KDTreeEigenMatrixAdaptor<
            Eigen::Map<const Eigen::MatrixXd>,
            -1,
            nanoflann::metric_L2,
            false>;

    std::vector<double> data_;
    std::unique_ptr<Eigen::Map<const Eigen::MatrixXd>> data_interface_;
    std::unique_ptr<KDTree_t> nanoflann_index_;
    size_t dimension_ = 0;
    size_t dataset_size_ = 0;
};

}  // namespace geometry
}  // namespace open3d
