#include "../src/pdf_document.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <vector>

TEST(PDFDocument, ReturnsNullptrIfLoadingEmptyDocument) {
  std::unique_ptr<Document> doc(PDFDocument::Open("", nullptr));
  EXPECT_EQ(doc.get(), nullptr);
}

TEST(PDFDocument, CanLoadDocument) {
  std::unique_ptr<Document> doc(
      PDFDocument::Open("testdata/bash.pdf", nullptr));
  EXPECT_NE(doc.get(), nullptr);
  EXPECT_EQ(doc->GetNumPages(), 186);
  const Document::PageSize page_size = doc->GetPageSize(0);
  EXPECT_GT(page_size.Height, 0);
  EXPECT_GT(page_size.Width, 0);
  EXPECT_EQ(page_size.Width * 1000 / page_size.Height, int(8.5 * 1000 / 11));
}

TEST(PDFDocument, CanLoadOutline) {
  std::unique_ptr<Document> doc(
      PDFDocument::Open("testdata/bash.pdf", nullptr));
  EXPECT_NE(doc.get(), nullptr);
  std::unique_ptr<const Document::OutlineItem> outline(doc->GetOutline());
  EXPECT_NE(outline.get(), nullptr);

  EXPECT_EQ(outline->GetNumChildren(), 14);

  const Document::OutlineItem* item = outline->GetChild(1);
  EXPECT_EQ(item->GetTitle(), "Definitions");
  EXPECT_EQ(item->GetNumChildren(), 0);
  EXPECT_EQ(doc->Lookup(item), 8);

  item = outline->GetChild(2);
  EXPECT_EQ(item->GetTitle(), "Basic Shell Features");
  EXPECT_EQ(item->GetNumChildren(), 8);
  EXPECT_EQ(doc->Lookup(item), 10);

  item = item->GetChild(0);
  EXPECT_EQ(item->GetTitle(), "Shell Syntax");
  EXPECT_EQ(item->GetNumChildren(), 3);
  EXPECT_EQ(doc->Lookup(item), 10);

  item = item->GetChild(1);
  EXPECT_EQ(item->GetTitle(), "Quoting");
  EXPECT_EQ(item->GetNumChildren(), 5);
  EXPECT_EQ(doc->Lookup(item), 11);
}

TEST(PDFDocument, CanSearch) {
  std::unique_ptr<Document> doc(
      PDFDocument::Open("testdata/bash.pdf", nullptr));
  EXPECT_NE(doc.get(), nullptr);
  const Document::SearchResult result = doc->Search("HISTIGNORE", 0, 80, 100);
  EXPECT_EQ(result.SearchString, "HISTIGNORE");
  EXPECT_EQ(result.LastSearchedPage, doc->GetNumPages());
  const std::vector<int> expected_result_pages(
      {84, 85, 85, 85, 130, 148, 148, 180});
  EXPECT_EQ(result.SearchHits.size(), expected_result_pages.size());
  for (int i = 0; i < result.SearchHits.size(); ++i) {
    const auto& hit = result.SearchHits[i];
    EXPECT_EQ(hit.Page, expected_result_pages[i]);
    EXPECT_LE(hit.ContextText.length(), 80);
    EXPECT_NE(hit.ContextText.find("HISTIGNORE"), std::string::npos);
  }
}

TEST(PDFDocument, CanLoadPasswordProtectedDocument) {
  std::unique_ptr<Document> doc(
      PDFDocument::Open("testdata/password-test.pdf", nullptr));
  EXPECT_EQ(doc.get(), nullptr);
  const std::string password = "abracadabra";
  doc.reset(PDFDocument::Open("testdata/password-test.pdf", &password));
  EXPECT_NE(doc.get(), nullptr);
  EXPECT_EQ(doc->GetNumPages(), 1);
  const Document::SearchResult result = doc->Search("SUCCESS", 0, 80, 100);
  EXPECT_EQ(result.SearchHits.size(), 1);
}

