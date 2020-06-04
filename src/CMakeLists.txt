# Dependencies.
# -------------
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# To support box characters in UTF-8 locales, we need to link against
# libncursesw rather than libncurses:
#
#     https://lists.gnu.org/archive/html/bug-ncurses/2015-02/msg00010.html
#
# Unfortunately, the CURSES_NEED_WIDE flag (which enables linking against
# libncursesw) is only supported in CMake 3.10+. If built using earlier
# versions of CMake, this flag will have no effect and we'll still end up
# linking against libncurses. This will cause box characters (e.g. used in
# search view) to render as x's and q's when running in UTF-8 locales.
set(CURSES_NEED_WIDE ON)
set(CURSES_NEED_NCURSES ON)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

if(ENABLE_LEGACY_IMAGE_IMPL)
  if(ENABLE_IMAGE_SUPPORT)
    find_package(Imlib2 REQUIRED)
    include_directories(${IMLIB2_INCLUDE_DIR})
  else()
    add_definitions(
      -DJFBVIEW_NO_IMLIB2
    )
    set(IMLIB2_LIBRARIES)
  endif()
  set(imlib2_libs ${IMLIB2_LIBRARIES})
else()
  set(imlib2_libs)
endif()

# Build settings.
# ---------------
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(
  -DJFBVIEW_VERSION=\"${PROJECT_VERSION}\"
)
if(ENABLE_LEGACY_IMAGE_IMPL)
  add_definitions(
    -DJFBVIEW_ENABLE_LEGACY_IMAGE_IMPL
  )
endif()
if(ENABLE_LEGACY_PDF_IMPL)
  add_definitions(
    -DJFBVIEW_ENABLE_LEGACY_PDF_IMPL
  )
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

# Generate default config.
# ------------------------
add_executable(
  generate_default_config_cpp
  generate_default_config_cpp.cpp
)
add_custom_command(
  OUTPUT default_config.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/default_config.json
  COMMAND
    generate_default_config_cpp 
      < ${CMAKE_CURRENT_SOURCE_DIR}/default_config.json 
      > default_config.cpp
)

# Base library for handling documents.
# ------------------------------------
add_library(
  jfbview_document
  STATIC
  document.cpp
  fitz_document.cpp
  fitz_utils.cpp
  image_document.cpp
  pdf_document.cpp
  string_utils.cpp
  multithreading.cpp
)
target_link_libraries(
  jfbview_document
  Threads::Threads
  ${vendor_mupdf_libs}
  ${imlib2_libs}
)
add_dependencies(
  jfbview_document
  vendor_mupdf
)
if(ENABLE_LEGACY_PDF_IMPL)
  add_dependencies(
    jfbview_document
    detected_mupdf_version.hpp
  )
endif()

# Common viewer code shared by jfbview and jfbpdf.
# ------------------------------------------------
add_library(
  jfbview_document_viewer
  STATIC
  command.cpp
  framebuffer.cpp
  outline_view.cpp
  pixel_buffer.cpp
  search_view.cpp
  ui_view.cpp
  viewer.cpp
  settings.cpp
  default_config.cpp
)
target_link_libraries(
  jfbview_document_viewer
  jfbview_document
  ${CURSES_LIBRARIES}
)

# jfbview
# -------
set(
  jfbview_sources
  main.cpp
  jpdfgrep.cpp
  jpdfcat.cpp
)
add_executable(jfbview ${jfbview_sources})
target_link_libraries(jfbview jfbview_document_viewer)
install(TARGETS jfbview DESTINATION bin)
add_custom_target(
  jpdfgrep
  ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ./jfbview ${CMAKE_CURRENT_BINARY_DIR}/jpdfgrep
)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jpdfgrep DESTINATION bin)
add_custom_target(
  jpdfcat
  ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ./jfbview ${CMAKE_CURRENT_BINARY_DIR}/jpdfcat
)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jpdfcat DESTINATION bin)

# jfbpdf
# ------
if(ENABLE_LEGACY_IMAGE_IMPL)
  if(NOT ENABLE_IMAGE_SUPPORT)
    add_executable(jfbpdf ${jfbview_sources})
    target_compile_definitions(
      jfbpdf
      PRIVATE
      JFBVIEW_NO_IMLIB2
      JFBVIEW_PROGRAM_NAME=\"JFBPDF\"
      JFBVIEW_BINARY_NAME=\"jfbpdf\"
    )
    target_link_libraries(jfbpdf jfbview_document_viewer)
    install(TARGETS jfbpdf DESTINATION bin)
  endif()
endif()

