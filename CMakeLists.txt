cmake_minimum_required (VERSION 3.1)

project (fastcryptosolver)
add_executable (fastcryptosolver fastcryptosolver.cpp)
set_property(TARGET fastcryptosolver PROPERTY CXX_STANDARD 11)

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-march=native" HAS_FLAG_MARCH_NATIVE)
if (HAS_FLAG)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
endif()

CHECK_CXX_COMPILER_FLAG("-mtune=native" HAS_FLAG_MTUNE_NATIVE)
if (HAS_FLAG)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mtune=native")
endif()
