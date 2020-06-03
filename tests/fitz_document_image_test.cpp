#include <gtest/gtest.h>

#include <memory>

#include "../src/fitz_document.hpp"

// Fitz scales images to a DPI of 72. Our test images have DPI 96.
inline int ScaleToFitzDpi(int x) { return x * 72 / 96; }

TEST(FitzDocumentImage, ReturnsNullptrIfLoadingEmptyImage) {
  std::unique_ptr<Document> doc(FitzDocument::Open("", nullptr));
  EXPECT_EQ(doc.get(), nullptr);
}

TEST(FitzDocumentImage, CanLoadPNGImage) {
  std::unique_ptr<Document> doc(
      FitzDocument::Open("testdata/panda.png", nullptr));
  EXPECT_NE(doc.get(), nullptr);
  EXPECT_EQ(doc->GetNumPages(), 1);
  EXPECT_EQ(doc->GetPageSize(0).Width, ScaleToFitzDpi(160));
  EXPECT_EQ(doc->GetPageSize(0).Height, ScaleToFitzDpi(200));
}

TEST(FitzDocumentImage, CanLoadJPEGImage) {
  std::unique_ptr<Document> doc(
      FitzDocument::Open("testdata/panda.jpg", nullptr));
  EXPECT_NE(doc.get(), nullptr);
  EXPECT_EQ(doc->GetNumPages(), 1);
  EXPECT_EQ(doc->GetPageSize(0).Width, ScaleToFitzDpi(320));
  EXPECT_EQ(doc->GetPageSize(0).Height, ScaleToFitzDpi(400));
}

