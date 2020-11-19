cmake_minimum_required(VERSION 3.13)

# CRISPR requires LLVM 8.
# If your system uses a different version, you'll need to build it
# yourself and include this on the CMake command line:
# -DLLVM_DIR="<path/to/llvm-project>/build/lib/cmake/llvm"

find_package(LLVM 8.0 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -fno-rtti -fno-exceptions -O0")

add_definitions(${LLVM_DEFINITIONS})

include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${CMAKE_INSTALL_PREFIX}/lib)

llvm_map_components_to_libnames(llvm_libs all)

add_executable(
        crispr
        crispr.cpp
        CrisprMemoryManager.cpp
        CrisprCompiler.cpp
)

# Link against LLVM libraries
target_link_libraries(
        crispr
        ${llvm_libs}
)
