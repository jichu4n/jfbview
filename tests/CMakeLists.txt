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

if(ENABLE_IMAGE_SUPPORT)
  add_executable(image_document_test image_document_test.cpp)
  target_link_libraries(
    image_document_test
    jfbview_document
    ${GTEST_BOTH_LIBRARIES}
  )
  add_test(
    NAME image_document_test
    COMMAND image_document_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

