# Dependencies.
# -------------
find_package(JPEG REQUIRED)
find_package(HarfBuzz REQUIRED)
find_package(Freetype REQUIRED)
find_package(ZLIB REQUIRED)
find_package(OpenJPEG REQUIRED)

# MuPDF provides its own version of libjpeg in thirdparty. However, we are
# linking against the system version of Imlib2, which links against the system
# version of libjpeg. Using the bundled version of libjpeg here will cause a
# conflict with the system version of libjpeg and cause Imlib2 to fail. This
# manifests as an "unknown error" when attempting to load a JPEG file.
add_custom_command(
  OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/mupdf/build/release"
  COMMAND
    make
      "USE_SYSTEM_LIBS=no"
      "USE_SYSTEM_LIBJPEG=yes"
      "USE_SYSTEM_HARFBUZZ=yes"
      "USE_SYSTEM_FREETYPE=yes"
      "USE_SYSTEM_ZLIB=yes"
      "USE_SYSTEM_OPENJPEG=yes"
      "prefix=${CMAKE_CURRENT_BINARY_DIR}/mupdf"
      libs
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mupdf"
)
add_custom_target(
  vendor_mupdf
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mupdf/build/release"
)
set(
  vendor_mupdf_include_dirs
  "${CMAKE_CURRENT_SOURCE_DIR}/mupdf/include"
  "${JPEG_INCLUDE_DIR}"
  "${HarfBuzz_INCLUDE_DIRS}"
  "${FREETYPE_INCLUDE_DIRS}"
  "${ZLIB_INCLUDE_DIRS}"
  "${OpenJPEG_INCLUDE_DIRS}"
  PARENT_SCOPE
)
set(vendor_mupdf_link_dirs "${CMAKE_CURRENT_SOURCE_DIR}/mupdf/build/release" PARENT_SCOPE)
set(
  vendor_mupdf_libs
  "mupdf"
  "mupdf-third"
  ${JPEG_LIBRARIES}
  ${HarfBuzz_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${OpenJPEG_LIBRARIES}
  PARENT_SCOPE
)

set(
  vendor_rapidjson_include_dirs
  "${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include"
  PARENT_SCOPE
)

