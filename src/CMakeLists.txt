include_directories (include)

CHECK_FUNCTION_EXISTS(getopt_long HAVE_GETOPT_LONG)

configure_file(include/cmake_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/cmake_config.h)

set (EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

file(GLOB yuv_and_rgb_src
    "*.cpp"
    "*.c"
    "include/*.h"
    )

if (NOT HAVE_GETOPT_LONG)
    include_directories (getopt_extern)
    set (yuv_and_rgb_src "${yuv_and_rgb_src} getopt_extern/*")
endif(NOT HAVE_GETOPT_LONG)

find_package(Threads REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(OpenGL)
find_package(glfw3)

set (CORELIBS Threads::Threads ${OPENGL_LIBRARY} glfw)

option(FORCE_SSE
    "Force SSE2 YUV to RGB algorithm" OFF)

if (FORCE_SSE)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
endif (FORCE_SSE)

add_executable(yuv_and_rgb ${yuv_and_rgb_src})
target_link_libraries(yuv_and_rgb ${CORELIBS})

enable_testing()

add_executable(vectorcomp tests/vector_test.cpp bmp.cpp convert.cpp fs.cpp include/bmp.h include/convert.h include/fs.h)
add_test(NAME vectorcomp
    COMMAND $<TARGET_FILE:vectorcomp>
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test_data)
add_test(NAME threadcomp
    COMMAND ${CMAKE_COMMAND}
    -DTEST_PROG=$<TARGET_FILE:yuv_and_rgb>
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles/threadcomp.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test_data)
