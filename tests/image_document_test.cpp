#include <gtest/gtest.h>

#include <memory>

// Must include AFTER gtest as Xlib macros conflict with gtest.
#include "../src/image_document.hpp"

TEST(ImageDocument, ReturnsNullptrIfLoadingEmptyImage) {
  std::unique_ptr<Document> doc(ImageDocument::Open(""));
  EXPECT_EQ(doc.get(), nullptr);
}

TEST(ImageDocument, CanLoadPNGImage) {
  std::unique_ptr<Document> doc(ImageDocument::Open("testdata/panda.png"));
  EXPECT_NE(doc.get(), nullptr);
  EXPECT_EQ(doc->GetNumPages(), 1);
  EXPECT_EQ(doc->GetPageSize(0).Width, 160);
  EXPECT_EQ(doc->GetPageSize(0).Height, 200);
}

TEST(ImageDocument, CanLoadJPEGImage) {
  std::unique_ptr<Document> doc(ImageDocument::Open("testdata/panda.jpg"));
  EXPECT_NE(doc.get(), nullptr);
  EXPECT_EQ(doc->GetNumPages(), 1);
  EXPECT_EQ(doc->GetPageSize(0).Width, 320);
  EXPECT_EQ(doc->GetPageSize(0).Height, 400);
}

