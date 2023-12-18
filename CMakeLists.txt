# Adapted from https://r4nd0m6uy.ch/cmake-and-cpputest.html

cmake_minimum_required( VERSION 3.4 )
project( merge-vertices )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
SET(CMAKE_CXX_COMPILER
#"/opt/homebrew/bin/g++-11"
"/usr/local/bin/g++-13"
)
endif()

set( CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} \
    -fopenmp \
    -march=native \
    -Wall \
    -g \
    -ansi \
    -Wextra \
    -Wdisabled-optimization \
    -Wunsafe-loop-optimizations \
    -Wpedantic \
    -Wshadow"
)

set( CMAKE_CXX_FLAGS_DEBUG
  "${CMAKE_CXX_FLAGS_DEBUG} \
    -fprofile-arcs -ftest-coverage \
    -Og"
) 
set( CMAKE_CXX_FLAGS_RELEASE
  "${CMAKE_CXX_FLAGS_RELEASE} \
  -O3"
)
set(
  basepath
    "${CMAKE_SOURCE_DIR}"
)

# find_package( Boost 1.62.0 COMPONENTS program_options REQUIRED )

# include_directories( ${Boost_INCLUDE_DIR} )
include_directories( "${basepath}/include" )

# Production code
add_subdirectory(src)
add_subdirectory(rply)
add_subdirectory(filePLY)
