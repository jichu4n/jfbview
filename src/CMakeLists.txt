# Dependencies.
# -------------
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CURSES_NEED_NCURSES ON)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

if(ENABLE_IMAGE_SUPPORT)
  find_package(Imlib2 REQUIRED)
  include_directories(${IMLIB2_INCLUDE_DIR})
endif()

# Detect MuPDF version.
# ---------------------
add_executable(
  print_mupdf_version
  print_mupdf_version.cpp
)
add_dependencies(
  print_mupdf_version
  vendor_mupdf
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
set(jfbview_sources image_document.cpp main.cpp)
add_executable(jfbview ${jfbview_sources})
if(ENABLE_IMAGE_SUPPORT)
  target_link_libraries(jfbview ${IMLIB2_LIBRARIES})
else()
  target_compile_definitions(jfbview PRIVATE JFBVIEW_NO_IMLIB2)
endif()
target_link_libraries(jfbview jfbview_pdf_viewer)
install(TARGETS jfbview DESTINATION bin)

# jfbpdf
# ------
if(NOT ENABLE_IMAGE_SUPPORT)
  add_executable(jfbpdf ${jfbview_sources})
  target_compile_definitions(
    jfbpdf
    PRIVATE
    JFBVIEW_NO_IMLIB2
    JFBVIEW_PROGRAM_NAME=\"JFBPDF\"
    JFBVIEW_BINARY_NAME=\"jfbpdf\"
  )
  target_link_libraries(jfbpdf jfbview_pdf_viewer)
  install(TARGETS jfbpdf DESTINATION bin)
endif()

