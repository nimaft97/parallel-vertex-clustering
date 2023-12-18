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

#include <numeric>
#include <queue>
#include <tuple>
#include <omp.h>
#include <iostream>
#include <iterator>
#include <algorithm>

#include "../include/Eigen/Dense"
#include "../include/geometry/TriangleMeshPWeld.h"

namespace open3d {
namespace geometry {

void TriangleMeshPWeld::reduce(
    std::vector<int>& pid2ccid, 
    std::vector<Eigen::Vector3d>& new_vertices, 
    const std::vector<int>& cp_vec,
    int num_vertices) const
    {
        int n_clusters = 0; // number of clusters
        std::vector<int>num_cluster_members_vec(num_vertices, 1);
        new_vertices.reserve(num_vertices);
        
        for (int i=0; i < num_vertices; i++){
            if (cp_vec[i] == i){ // pid == cid -> centroid
                pid2ccid[i] = n_clusters;
                n_clusters++;
                new_vertices.push_back(vertices_[i]);
            }
            else{ // non-centroid
                const int ccid = pid2ccid[cp_vec[i]]; // cp_vec[i] equals pid of its centroid and pid2ccid[centroid] = compressed cluster id
                const int previous_cluster_size = num_cluster_members_vec[ccid]++;             
                new_vertices[ccid] = (vertices_[i] + previous_cluster_size * (new_vertices[ccid])) / (previous_cluster_size+1.0);
                pid2ccid[i] = ccid;
            }
        }
    }

TriangleMeshPWeld& TriangleMeshPWeld::merge_vertices_forward(KDTreeFlann const& kdtree, double eps)
{
    // forward declarations for compatability with timer scoping
    std::vector<std::vector<int>> pid2nnBigger;
    std::vector<Eigen::Vector3d> new_vertices;
    std::vector<int> cp_vec
                   , pid2ccid
                   , remainingSmallerFinalsVec;

    int numVertices, numIterations;
    numVertices = static_cast<int>( vertices_.size() );

    pid2nnBigger = std::vector<std::vector<int>>( numVertices );
    remainingSmallerFinalsVec = std::vector<int>( numVertices );
    cp_vec = std::vector<int>( numVertices );

    #pragma omp parallel for
    for( int i = 0; i < numVertices; ++i )
    {
        std::vector<double> dists2;
        int numSmallerNeighbors = kdtree.SearchRadiusSmallerAndBigger(vertices_[i], eps, pid2nnBigger[i], dists2, i);
        cp_vec[ i ] = i;
        remainingSmallerFinalsVec[ i ] = numSmallerNeighbors - 1; // all smaller neighbors minus itself
    }

    numIterations = 0;
    bool should_continue = true;

    #pragma omp parallel
    {   

        while (should_continue)
        {
            // this is required to make sure that no thread still on the previous
            // while loop iteration changes should_continue after the single thread
            // reaches line 418 below
            #pragma omp barrier

            #pragma omp single
            {
                ++numIterations;
                should_continue = false;
            }
            // implicit sync barrier at end of single region
            // required to ensure that the thread on line 418 does
            // not overwrite a change that another thread makes below

            #pragma omp for reduction (||:should_continue)
            for( int i = 0; i < numVertices; ++i )
            {
                if( remainingSmallerFinalsVec[ i ] < 0 )
                {
                    continue; // skip old active sources
                }
                else if( remainingSmallerFinalsVec[ i ] == 0 ) // is an active source
                {
                    --remainingSmallerFinalsVec[ i ]; // remove it from active source list
                    bool isCentroid = (cp_vec[i] == i);
                    const auto& inner_vec = pid2nnBigger[i];

                    for( const int biggerNeighborIdx : inner_vec )
                    {
                        if( isCentroid && remainingSmallerFinalsVec[ biggerNeighborIdx ] > 0 )
                        {
                            int expected;
                            int desired = i;
                            do
                            {
                                expected = cp_vec[biggerNeighborIdx];
                                if (desired >= expected) break;
                            }while(!__sync_bool_compare_and_swap(&cp_vec[biggerNeighborIdx], expected, desired));
                        }

                        if ( remainingSmallerFinalsVec[ biggerNeighborIdx ] >= 1 )
                        {
                            should_continue = true;
                        }
                        __sync_fetch_and_add( &remainingSmallerFinalsVec[ biggerNeighborIdx ], -1 );
                    }
                }
            }
        }
    } // end while loop timer 

    // --------- each point knows its own cluster -----------

    pid2ccid = std::vector<int>( numVertices );
    reduce(pid2ccid, new_vertices, cp_vec, numVertices);

    #pragma omp parallel for
    for (auto &triangle : triangles_)
    {
        triangle(0) = pid2ccid[triangle(0)];
        triangle(1) = pid2ccid[triangle(1)];
        triangle(2) = pid2ccid[triangle(2)];
    }

    std::swap(vertices_, new_vertices);
    return *this;
}

TriangleMeshPWeld& TriangleMeshPWeld::merge_vertices_forward_async(KDTreeFlann const& kdtree, double eps)
{
    std::vector<std::vector<int>> pid2nnBigger;
    std::vector<Eigen::Vector3d> new_vertices;
    std::vector<int> cp_vec
                   , pid2ccid
                   , remainingSmallerFinalsVec
                   , num_discovered_centroids;

    int numVertices
      , numIterations
      , num_threads;

    const int ints_per_cache_line = 16;

    numVertices = static_cast<int>( vertices_.size() );

    num_threads = omp_get_max_threads();
    pid2nnBigger = std::vector<std::vector<int>>( numVertices );
    pid2ccid = std::vector<int>( numVertices );
    remainingSmallerFinalsVec = std::vector<int>( numVertices );
    cp_vec = std::vector<int>( numVertices );
    num_discovered_centroids = std::vector<int>( ints_per_cache_line * num_threads + 1, 0);

    #pragma omp parallel for
    for( int i = 0; i < numVertices; ++i )
    {
        std::vector<double> dists2;

        // SearchRadiusNima collects those neighbors whose vertex id is less than or equal to i
        int numSmallerNeighbors = kdtree.SearchRadiusSmallerAndBigger(vertices_[i], eps, pid2nnBigger[i], dists2, i);

        cp_vec[ i ] = i; // initialize to i
        remainingSmallerFinalsVec[ i ] = numSmallerNeighbors - 1; // all smaller neighbors minus itself
    }

    numIterations = 0;
    bool should_continue = true;

    #pragma omp parallel
    {   

        while (should_continue)
        {
            // this is required to make sure that no thread still on the previous
            // while loop iteration changes should_continue after the single thread
            // reaches line 418 below
            #pragma omp barrier

            #pragma omp single
            {
                ++numIterations;
                should_continue = false;
            }
            // implicit sync barrier at end of single region
            // required to ensure that the thread on line 418 does
            // not overwrite a change that another thread makes below

            #pragma omp for reduction (||:should_continue)
            for( int i = 0; i < numVertices; ++i )
            {
                if( remainingSmallerFinalsVec[ i ] < 0 )
                {
                    continue; // skip old active sources
                }
                else if( remainingSmallerFinalsVec[ i ] == 0 ) // is an active source
                {
                    --remainingSmallerFinalsVec[ i ]; // remove it from active source list
                    bool isCentroid = (cp_vec[i] == i);
                    num_discovered_centroids[omp_get_thread_num()*ints_per_cache_line] += isCentroid;
                    const auto& inner_vec = pid2nnBigger[i];

                    for( const int biggerNeighborIdx : inner_vec )
                    {
                        if( isCentroid && remainingSmallerFinalsVec[ biggerNeighborIdx ] > 0 )
                        {
                            int expected;
                            int desired = i;
                            do
                            {
                                expected = cp_vec[biggerNeighborIdx];
                                if (desired >= expected) break;
                            }while(!__sync_bool_compare_and_swap(&cp_vec[biggerNeighborIdx], expected, desired));
                        }

                        if ( remainingSmallerFinalsVec[ biggerNeighborIdx ] >= 1 )
                        {
                            should_continue = true;
                        }
                        __sync_fetch_and_add( &remainingSmallerFinalsVec[ biggerNeighborIdx ], -1 );
                    }
                }
            }
        }
    }


    // --------- each point knows its own cluster -----------

    std::exclusive_scan(std::cbegin(num_discovered_centroids)
                        , std::cend(num_discovered_centroids)
                        , std::begin(num_discovered_centroids)
                        , 0u );
    new_vertices.resize(num_discovered_centroids[num_threads*ints_per_cache_line]);

    #pragma omp parallel
    {
        const int th_id = omp_get_thread_num();
        const int offset = num_discovered_centroids[ th_id * ints_per_cache_line ];
        int current_vertex_id = 0;

        #pragma omp for
        for( int i = 0; i < numVertices; ++i )
        {
            if( cp_vec[ i ] == i )
            { // is a centroid
                const int ccid = offset + current_vertex_id++;
                new_vertices[ ccid ] = vertices_[ i ];
                pid2ccid[ i ] = ccid;
            }
        }
    }
    std::vector<unsigned int> num_cluster_members_vec(num_discovered_centroids[num_threads*ints_per_cache_line], 1u);
    const int num_clusters = num_discovered_centroids[num_threads*ints_per_cache_line];
    
    #pragma omp parallel
    {
        #pragma omp single nowait
        for( int i = 0; i < numVertices; ++i )
        {
            if( cp_vec[ i ] != i ) // not a centroid
            {
                const int ccid = pid2ccid[ cp_vec[ i ] ];
                new_vertices[ccid] = (new_vertices[ccid] * num_cluster_members_vec[ccid]
                                    + vertices_[i])
                                    / static_cast<float>(num_cluster_members_vec[ccid] + 1);
                ++num_cluster_members_vec[ccid];
            }
        }

        #pragma omp for schedule (dynamic, num_clusters / num_threads / 2u)
        for (auto &triangle : triangles_)
        {
            triangle(0) = pid2ccid[cp_vec[triangle(0)]];
            triangle(1) = pid2ccid[cp_vec[triangle(1)]];
            triangle(2) = pid2ccid[cp_vec[triangle(2)]];
        }
    }

    std::swap(vertices_, new_vertices);

    return *this;
}

}  // namespace geometry
}  // namespace open3d
