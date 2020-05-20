add_custom_command(
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/mupdf"
  COMMAND
    make
    -C "${CMAKE_CURRENT_SOURCE_DIR}/mupdf"
    "USE_SYSTEM_LIBS=no"
    "prefix=${CMAKE_CURRENT_BINARY_DIR}/mupdf"
    install
)
add_custom_target(
  vendor_mupdf
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mupdf"
)
set(vendor_mupdf_include_dirs "${CMAKE_CURRENT_BINARY_DIR}/mupdf/include" PARENT_SCOPE)
set(vendor_mupdf_link_dirs "${CMAKE_CURRENT_BINARY_DIR}/mupdf/lib" PARENT_SCOPE)
set(vendor_mupdf_libs "mupdf" "mupdf-third" PARENT_SCOPE)

