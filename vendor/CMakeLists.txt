# Dependencies.
# -------------
find_package(JPEG REQUIRED)

# MuPDF provides its own version of libjpeg in thirdparty. However, we are
# linking against the system version of Imlib2, which links against the system
# version of libjpeg. Using the bundled version of libjpeg here will cause a
# conflict with the system version of libjpeg and cause Imlib2 to fail. This
# manifests as an "unknown error" when attempting to load a JPEG file.
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
      "USE_SYSTEM_LIBJPEG=yes"
      "prefix=${CMAKE_CURRENT_BINARY_DIR}/mupdf"
      install
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mupdf"
)
add_custom_target(
  vendor_mupdf
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/mupdf"
)
set(
  vendor_mupdf_include_dirs
  "${CMAKE_CURRENT_BINARY_DIR}/mupdf/include"
  "${JPEG_INCLUDE_DIR}"
  PARENT_SCOPE
)
set(vendor_mupdf_link_dirs "${CMAKE_CURRENT_BINARY_DIR}/mupdf/lib" PARENT_SCOPE)
set(
  vendor_mupdf_libs
  "mupdf"
  "mupdf-third"
  ${JPEG_LIBRARIES}
  PARENT_SCOPE
)

