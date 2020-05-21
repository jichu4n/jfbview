add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mupdf"
  COMMAND
    patch
      -Np1
      -i "${CMAKE_CURRENT_SOURCE_DIR}/mupdf-1.16.1-freeglut-fg_gl2-gcc-10.patch"
    || true
  COMMAND
    make
      "USE_SYSTEM_LIBS=no"
      "prefix=${CMAKE_CURRENT_BINARY_DIR}/mupdf"
      install
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mupdf"
)
add_custom_target(
  vendor_mupdf
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mupdf"
)
set(vendor_mupdf_include_dirs "${CMAKE_CURRENT_BINARY_DIR}/mupdf/include" PARENT_SCOPE)
set(vendor_mupdf_link_dirs "${CMAKE_CURRENT_BINARY_DIR}/mupdf/lib" PARENT_SCOPE)
set(vendor_mupdf_libs "mupdf" "mupdf-third" PARENT_SCOPE)

