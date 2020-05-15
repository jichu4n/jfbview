cmake_minimum_required(VERSION 3.2)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(jfbview)

add_subdirectory(vendor)
include_directories(${vendor_include_dirs})
link_directories(${vendor_link_dirs})

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
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/detect_and_output_mupdf_version.sh"
  DEPENDS print_mupdf_version
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CURSES_NEED_NCURSES ON)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

find_package(Imlib2)
include_directories(${IMLIB2_INCLUDE_DIR})

add_library(
  pdf_document
  STATIC
  document.cpp
  pdf_document.cpp
  string_utils.cpp
  multithreading.cpp
)
target_link_libraries(
  pdf_document
  Threads::Threads
  ${vendor_mupdf_libs}
)
add_dependencies(
  pdf_document
  detected_mupdf_version.hpp
)

add_executable(jpdfcat jpdfcat.cpp)
target_link_libraries(
  jpdfcat
  pdf_document
)

add_executable(jpdfgrep jpdfgrep.cpp)
target_link_libraries(
  jpdfgrep
  pdf_document
  ${CURSES_LIBRARIES}
)

add_executable(
  jfbview
  command.cpp
  framebuffer.cpp
  image_document.cpp
  outline_view.cpp
  pixel_buffer.cpp
  search_view.cpp
  ui_view.cpp
  viewer.cpp
  main.cpp
)
target_link_libraries(
  jfbview
  pdf_document
  ${CURSES_LIBRARIES}
  ${IMLIB2_LIBRARIES}
)

