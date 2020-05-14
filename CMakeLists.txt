cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(jfbview)

add_subdirectory(vendor)
include_directories(BEFORE SYSTEM ${vendor_include_dirs})
link_directories(${vendor_link_dirs})

add_executable(mupdf_version mupdf_version.cpp)
add_dependencies(mupdf_version vendor)
target_link_libraries(mupdf_version ${vendor_mupdf_libs})


