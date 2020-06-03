# Dependencies.
# -------------
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Build settings.
# ---------------
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tests.
# ------

if(ENABLE_LEGACY_IMAGE_IMPL AND ENABLE_IMAGE_SUPPORT)
  add_executable(image_document_test image_document_test.cpp)
  target_link_libraries(
    image_document_test
    jfbview_document
    ${GTEST_BOTH_LIBRARIES}
  )
  add_test(
    NAME image_document_test
    COMMAND image_document_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

if(ENABLE_LEGACY_PDF_IMPL)
  add_executable(pdf_document_test pdf_document_test.cpp)
  target_link_libraries(
    pdf_document_test
    jfbview_document
    ${GTEST_BOTH_LIBRARIES}
  )
  add_test(
    NAME pdf_document_test
    COMMAND pdf_document_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

add_executable(fitz_document_pdf_test fitz_document_pdf_test.cpp)
target_link_libraries(
  fitz_document_pdf_test
  jfbview_document
  ${GTEST_BOTH_LIBRARIES}
)
add_test(
  NAME fitz_document_pdf_test
  COMMAND fitz_document_pdf_test
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(fitz_document_image_test fitz_document_image_test.cpp)
target_link_libraries(
  fitz_document_image_test
  jfbview_document
  ${GTEST_BOTH_LIBRARIES}
)
add_test(
  NAME fitz_document_image_test
  COMMAND fitz_document_image_test
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME smoke_test
  COMMAND
    sh -c
      "env PATH=$PATH:${CMAKE_BINARY_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/smoke-test.sh"
)

