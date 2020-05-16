# CMake settings.
# ---------------
cmake_minimum_required(VERSION 3.2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
cmake_policy(SET CMP0048 NEW)

# Project settings.
# -----------------
project(jfbview VERSION 0.5.8)

# Build options.
# --------------
option(
  ENABLE_IMAGE_SUPPORT
  "If OFF, disable support for images via Imlib2 and build a PDF-only variant (jfbpdf)."
  ON
)

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

# Packaging.
# ----------
add_subdirectory(packaging)

