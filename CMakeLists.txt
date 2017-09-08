cmake_minimum_required (VERSION 3.5)
project (YUV_TO_RGB LANGUAGES C CXX)
include(CTest)
include(CheckFunctionExists)

set (YUV_TO_RGB_VERSION_MAJOR 1)
set (YUV_TO_RGB_VERSION_MINOR 0)

set (CMAKE_CXX_FLAGS "${CXXFLAGS}")
set (CMAKE_CXX_STANDARD 11)

add_subdirectory(src)
