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

/**
 * Small application to find epsilon values (i.e., distance thresholds) for vertex clustering
 * on a 3d triangle mesh that reduces the number of vertices in the mesh by a user-specified
 * percentage.
 */

#include <iostream>
#include <omp.h>
#include <cassert>

#include "../include/io/TriangleMeshIO.h"
#include "../include/geometry/KDTreeFlann.h"
#include "../include/geometry/TriangleMeshPWeld.h"

namespace { // anonymous

  using SpatialIndex = open3d::geometry::KDTreeFlann;
  using Mesh = open3d::geometry::TriangleMeshPWeld;
  using Epsilon = double;
  using ReductionRate = double;

  /** How fast to change epsilon in linear search phase */
  auto const EPSILON_STEP_SIZE = 0.01; // set based on 100x significant digits in prior experiments

  /** Absolute similarity threshold for floating point comparisons on reduction rate */
  auto const REDUCTION_RATE_MIN_ERROR = 0.00001; // set based on prev experiments

  /** Smallest different between min and max epsilon before reporting that they are equal. */
  auto const EPSILON_MIN_RANGE = 0.0000001; // set based on xyz manuscript dataset

  /**
   * An iteration of binary search has a lower and an upper bound.
   * For this algorithm, we iterate epsilon values to find reduction rates;
   * so, the binary search keeps track of both of these at the min and max
   * boundaries of the binary search range.
   */
  struct BinarySearchVals
  {
    Epsilon epsilon_min_boundary;
    Epsilon epsilon_max_boundary;
    ReductionRate reduction_rate_on_min_boundary;
    ReductionRate reduction_rate_on_max_boundary;
  };


  /**
   * Determines the reduction rate of vertex clustering on a mesh at a specific
   * Epsilon value, using a pre-built spatial index.
   */
  ReductionRate get_reduction_rate( SpatialIndex const& kdtree, Mesh mesh, Epsilon epsilon )
  {
    // Records vertex count before clustering, runs the vertex clustering algorithm,
    // then records the vertex count on the reduced mesh.

    std::cout << "Testing epsilon = " << epsilon << std::endl;

    auto const initial_vertex_count = mesh.vertices_.size();
    mesh.merge_vertices_forward( kdtree, epsilon);
    return ( initial_vertex_count - mesh.vertices_.size() ) / static_cast< ReductionRate >( initial_vertex_count );
  }


  /**
   * Finds the Epsilon value within a precision of EPSILON_MIN_RANGE that corresponds to reducing
   * the number of vertices in a Mesh via vertex clustering with a pre-built spatial index by
   * a specified ReductionRate within a precision of REDUCTION_RATE_MIN_ERROR.
   * Uses binary search on the real line in the range specified by the boundaries in a BinarySearchVals struct.
   */
  Epsilon find_epsilon_binary_search(SpatialIndex const& kdtree, Mesh const& mesh, ReductionRate target_reduction_rate, BinarySearchVals const& boundary_data )
  {
    std::cout << boundary_data.epsilon_min_boundary << " "
              << boundary_data.epsilon_max_boundary << " "
              << boundary_data.reduction_rate_on_min_boundary << " "
              << boundary_data.reduction_rate_on_max_boundary << std::endl;

    // First finds the midpoint of the range, since that will be either the return value or the
    // new boundary for a recursive call.
    auto const range = boundary_data.epsilon_max_boundary - boundary_data.epsilon_min_boundary;
    auto const epsilon_midpoint = boundary_data.epsilon_min_boundary + range / 2.0;
    

    // Base cases are given by precision. If the range of epsilon values is narrow enough
    // or the difference between reduction rates at the min and max boundaries is small enough,
    // then we return the midpoint of that range.

    if( boundary_data.epsilon_max_boundary - boundary_data.epsilon_min_boundary <= EPSILON_MIN_RANGE )
    {
      // base case
      return epsilon_midpoint;
    }
    if( boundary_data.reduction_rate_on_max_boundary - boundary_data.reduction_rate_on_min_boundary < REDUCTION_RATE_MIN_ERROR )
    {
      // base case
      return epsilon_midpoint;
    }

    // Otherwise, behaviour depends on the reduction rate at the midpoint.
    // Code is symmetric about whether the reduction rate at the midpoint is larger than than the target or not.
    // Either way, we check first if at the midpoint it is within the precision tolerance as the final base case
    // Otherwise, we recurse on half the range.
    // TODO: Reduce code duplication in this block.
    else
    {
      auto const reduction_rate_on_midpoint = get_reduction_rate( kdtree, mesh, epsilon_midpoint );

      if( reduction_rate_on_midpoint <= target_reduction_rate )
      {
        // base case
        if( target_reduction_rate - reduction_rate_on_midpoint < REDUCTION_RATE_MIN_ERROR )
        {
          return epsilon_midpoint;
        }

        // recursive binary search step
        return find_epsilon_binary_search( kdtree
                                         , mesh
                                         , target_reduction_rate
                                         , { epsilon_midpoint
                                           , boundary_data.epsilon_max_boundary
                                           , reduction_rate_on_midpoint
                                           , boundary_data.reduction_rate_on_max_boundary } );
      }
      else
      {
        // base case
        if( reduction_rate_on_midpoint - target_reduction_rate < REDUCTION_RATE_MIN_ERROR )
        {
          return epsilon_midpoint;
        }

        // recursive binary search step
        return find_epsilon_binary_search( kdtree
                                         , mesh
                                         , target_reduction_rate
                                         , { boundary_data.epsilon_min_boundary
                                           , epsilon_midpoint
                                           , boundary_data.reduction_rate_on_min_boundary
                                           , reduction_rate_on_midpoint } );
      }
    }
  }

  /**
   * Finds the linear interval of width EPSILON_STEP_SIZE that contains the epsilon
   * value that corresponds to the target_reduction_rate on a given mesh using a pre-built spatial index.
   * @note Does not search past eps = 10.0 which is a value an order of magnitude larger than any
   * epsilon encountered in previous experiments
   */
  BinarySearchVals find_epsilon_range_by_linear_search( SpatialIndex const& kdtree, Mesh const& mesh, ReductionRate target_reduction_rate )
  {
    // Implementation is a simple for loop starting at 0.0 and walking up by EPSILON_STEP_SIZE to find the first
    // epsilon value that creates a reduction rate larger than the target, i.e., finds an upper bound on discrete intervals.
    // Then returns the result from the previous loop iteration as the lower bound and this loop interation as the upper bound.

    auto const max_epsilon_searched = 10.0; // don't go past this. For loop stop condition for wild input parameters.
    auto prev_reduction_rate = 0.0; // memoize previous result to construct BinarySearchVals

    for( auto epsilon = EPSILON_STEP_SIZE; epsilon < max_epsilon_searched; epsilon += EPSILON_STEP_SIZE )
    {
      auto const reduction_rate = get_reduction_rate( kdtree, mesh, epsilon );
      if( reduction_rate >= target_reduction_rate )
      {
        return BinarySearchVals{ epsilon - EPSILON_STEP_SIZE
                               , epsilon
                               , prev_reduction_rate
                               , reduction_rate };
      }
      else
      {
        prev_reduction_rate = reduction_rate;
      }
    }

    assert( "Really?! epsilon > 10? Are you sure the target reduction rate is reasonable?? This line should be unreachable." && false );
    return BinarySearchVals{ 0.0, 10.0, 0.0, 100.0 }; // using std::optional instead turns this into an lvalue at call site
  }


  /**
   * Using a pre-built spatial index, determines the Epsilon value that,
   * using vertex clustering, reduces the number of vertices in a Mesh
   * by a given ReductionRate, at least within an error tolerance of
   * REDUCTION_RATE_MIN_ERROR.
   */
  Epsilon find_epsilon( SpatialIndex const& kdtree, Mesh const& mesh, ReductionRate reduction_rate )
  {
    // Note that searching large epsilon values is expensive.
    // This implementation first uses a linear grid search to find
    // the approximate range of the correct epsilon value,
    // then uses binary search within that range to improve the precision.
    return find_epsilon_binary_search( kdtree
                                     , mesh
                                     , reduction_rate
                                     , find_epsilon_range_by_linear_search( kdtree, mesh, reduction_rate ) );
  }
} // namespace anonymous


int main( int argc, char** argv )
{
  if( argc < 4 )
  {
    std::cout << "Enter the following:" << std::endl;
    std::cout << "\t-path to data (must be .ply)" << std::endl;
    std::cout << "\t-percentage of vertices to merge (e.g., 0.1)" << std::endl;
    std::cout << "\t-number of threads to use (e.g., 4)" << std::endl;
    std::cout << "\t-e.g., " << argv[ 0 ] << " ../src/data/xyzrgb_manuscript.ply 0.1 4" << std::endl;
    return 0;
  }

  std::string const  dataPath = argv[ 1 ];
  ReductionRate const reduction_rate = std::stod( argv[ 2 ] );
  std::size_t const num_threads = std::stoi( argv[ 3 ] );


  std::cout << "Configuration:" << std::endl;
  std::cout << "\t-path to dataset: " << dataPath << std::endl;
  std::cout << "\t-reduction rate: " << reduction_rate << "%" << std::endl;
  std::cout << "\t-number of threads: " << num_threads << std::endl;
  std::cout << "Initialising mesh and spatial index" << std::endl;

  omp_set_num_threads( num_threads );
  auto mesh = open3d::geometry::TriangleMesh();
  open3d::io::ReadTriangleMesh( dataPath, mesh );
  open3d::geometry::KDTreeFlann kdtree( mesh );

  std::cout << "--**--**--**--**--**--**--**--**--" << std::endl;

  std::cout << "Epsilon: " << find_epsilon( kdtree, mesh, reduction_rate / 100.0 ) << std::endl;

  return 0;
}

