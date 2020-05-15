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
include_directories(${vendor_include_dirs})
link_directories(${vendor_link_dirs})

# Detect MuPDF version.
# ---------------------
add_executable(
  print_mupdf_version
  print_mupdf_version.cpp
)
add_dependencies(
  print_mupdf_version
  vendor
)
target_link_libraries(
  print_mupdf_version
  ${vendor_mupdf_libs}
)

add_custom_target(
  detected_mupdf_version.hpp
  ALL
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/detect_and_output_mupdf_version.sh"
  DEPENDS print_mupdf_version
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Dependencies.
# -------------

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CURSES_NEED_NCURSES ON)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

find_package(Imlib2 REQUIRED)
include_directories(${IMLIB2_INCLUDE_DIR})

# Base library for handling PDF files.
# ------------------------------------
add_library(
  jfbview_pdf
  STATIC
  document.cpp
  pdf_document.cpp
  string_utils.cpp
  multithreading.cpp
)
target_link_libraries(
  jfbview_pdf
  Threads::Threads
  ${vendor_mupdf_libs}
)
add_dependencies(
  jfbview_pdf
  detected_mupdf_version.hpp
)

# jpdfcat
# -------
add_executable(jpdfcat jpdfcat.cpp)
target_link_libraries(
  jpdfcat
  jfbview_pdf
)
install(TARGETS jpdfcat DESTINATION bin)

# jpdfgrep
# --------
add_executable(jpdfgrep jpdfgrep.cpp)
target_link_libraries(
  jpdfgrep
  jfbview_pdf
  ${CURSES_LIBRARIES}
)
install(TARGETS jpdfgrep DESTINATION bin)

# Common viewer code shared by jfbview and jfbpdf.
# ------------------------------------------------
add_library(
  jfbview_pdf_viewer
  STATIC
  command.cpp
  framebuffer.cpp
  outline_view.cpp
  pixel_buffer.cpp
  search_view.cpp
  ui_view.cpp
  viewer.cpp
)
target_link_libraries(
  jfbview_pdf_viewer
  jfbview_pdf
  ${CURSES_LIBRARIES}
)

# jfbview
# -------
add_executable(
  jfbview
  image_document.cpp
  main.cpp
)
target_link_libraries(
  jfbview
  jfbview_pdf_viewer
  ${IMLIB2_LIBRARIES}
)
install(TARGETS jfbview DESTINATION bin)

# jfbpdf
# ------
add_executable(jfbpdf main.cpp)
target_link_libraries(
  jfbpdf
  jfbview_pdf_viewer
)
target_compile_definitions(
  jfbpdf
  PRIVATE
  JFBVIEW_NO_IMLIB2
  JFBVIEW_PROGRAM_NAME=\"JFBPDF\"
  JFBVIEW_BINARY_NAME=\"jfbpdf\"
)
install(TARGETS jfbpdf DESTINATION bin)

# Docs.
# ---------
add_custom_target(
  jfbview.1.gz
  ALL
  COMMAND
    cat "${CMAKE_CURRENT_SOURCE_DIR}/jfbview.1" |
    gzip > "${CMAKE_CURRENT_BINARY_DIR}/jfbview.1.gz"
  DEPENDS jfbview.1
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/jfbview.1.gz" DESTINATION share/man/man1)
install(FILES README DESTINATION share/doc/jfbview)

