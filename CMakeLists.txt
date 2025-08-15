cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(gp25_exercises)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(ARCH x64)

add_subdirectory(lib/SDL)

# it is generally recommended to add include dirs to specific targets only, BUT
# VS doesn't understand it. This way, we can actually navigate the header files
#include_directories(lib/SDL/include)

add_subdirectory(examples)
