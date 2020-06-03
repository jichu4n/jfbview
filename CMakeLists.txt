# CMake settings.
# ---------------
cmake_minimum_required(VERSION 3.2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
cmake_policy(SET CMP0048 NEW)

# Project settings.
# -----------------
project(jfbview VERSION 0.6.0)

# Build options.
# --------------
option(
  ENABLE_IMAGE_SUPPORT
  "If OFF, disable support for images via Imlib2 and build a PDF-only variant (jfbpdf)."
  ON
)
set(
  PACKAGE_FORMAT
  "DEB"
  CACHE STRING
  "CPack package generator to enable.")
set(
  LIBJPEG_PACKAGE_NAME
  "libjpeg62-turbo"
  CACHE STRING
  "The name of the libjpeg package to include as a dependency in the generated package.")
set(
  PACKAGE_FILE_PREFIX
  "jfbview"
  CACHE STRING
  "CPack package file name prefix.")
option(BUILD_TESTING "Build unit tests." OFF)
option(
  ENABLE_LEGACY_PDF_IMPL
  "If ON, enables legacy PDF document implementation based on low-level MuPDF APIs."
  OFF
)
option(
  ENABLE_LEGACY_IMAGE_IMPL
  "If ON, enables legacy image document implementation based on Imlib2."
  OFF
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

# Tests.
# ----------
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
  include(CTest)
  add_subdirectory(tests)
endif()

