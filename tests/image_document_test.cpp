#include <gtest/gtest.h>

#include <memory>

// Must include AFTER gtest as Xlib macros conflict with gtest.
#include "../src/image_document.hpp"
#include "../src/pdf_document.hpp"

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

// This is not a real test but ensures that MuPDF is actually linked with this
// test binary. The reason is that both MuPDF and Imlib2 ship with libjpeg /
// libjpeg-turbo, and this has historically caused errors when loading a JPEG
// image through Imlib2.
TEST(ImageDocument, EnsureMuPDFLinkage) {
  std::unique_ptr<Document> doc(PDFDocument::Open(""));
}

