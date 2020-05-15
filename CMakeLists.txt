# Project settings.
# -----------------
cmake_minimum_required(VERSION 3.2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(jfbview)

# Third-party code in vendor.
# ---------------------------
add_subdirectory(vendor)
include_directories(${vendor_mupdf_include_dirs})
link_directories(${vendor_mupdf_link_dirs})

# Project source code.
# --------------------
add_subdirectory(src)

# Docs.
# -----
add_subdirectory(doc)

