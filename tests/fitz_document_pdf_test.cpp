#include <gtest/gtest.h>

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

#include "../src/fitz_document.hpp"

namespace {

class DummyPixelWriter : public Document::PixelWriter {
 public:
  DummyPixelWriter() : call_count(0) {}
  void Write(int x, int y, uint8_t r, uint8_t g, uint8_t b) override {
    ++call_count;
  }
  int GetCallCount() const { return call_count; }

 private:
  std::atomic<int> call_count;
};

}  // namespace

TEST(FitzDocumentPDF, ReturnsNullptrIfLoadingEmptyDocument) {
  std::unique_ptr<Document> doc(FitzDocument::Open("", nullptr));
  EXPECT_EQ(doc.get(), nullptr);
}

TEST(FitzDocumentPDF, CanLoadDocument) {
  std::unique_ptr<Document> doc(
      FitzDocument::Open("testdata/bash.pdf", nullptr));
  EXPECT_NE(doc.get(), nullptr);
  EXPECT_EQ(doc->GetNumPages(), 186);
  const Document::PageSize page_size = doc->GetPageSize(0);
  EXPECT_GT(page_size.Height, 0);
  EXPECT_GT(page_size.Width, 0);
  EXPECT_EQ(page_size.Width * 1000 / page_size.Height, int(8.5 * 1000 / 11));
}

TEST(FitzDocumentPDF, CanLoadOutline) {
  std::unique_ptr<Document> doc(
      FitzDocument::Open("testdata/bash.pdf", nullptr));
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

TEST(FitzDocumentPDF, CanSearch) {
  std::unique_ptr<Document> doc(
      FitzDocument::Open("testdata/bash.pdf", nullptr));
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

TEST(FitzDocumentPDF, CanLoadPasswordProtectedDocument) {
  std::unique_ptr<Document> doc(
      FitzDocument::Open("testdata/password-test.pdf", nullptr));
  EXPECT_EQ(doc.get(), nullptr);
  const std::string password = "abracadabra";
  doc.reset(FitzDocument::Open("testdata/password-test.pdf", &password));
  EXPECT_NE(doc.get(), nullptr);
  EXPECT_EQ(doc->GetNumPages(), 1);
  const Document::SearchResult result = doc->Search("SUCCESS", 0, 80, 100);
  EXPECT_EQ(result.SearchHits.size(), 1);
}

TEST(FitzDocumentPDF, MultithreadedAccess) {
  std::unique_ptr<Document> doc(
      FitzDocument::Open("testdata/bash.pdf", nullptr));
  EXPECT_NE(doc.get(), nullptr);
  std::vector<std::thread> threads;
  srand(time(nullptr));
  for (int i = 0; i < 20; ++i) {
    threads.push_back(std::thread([&doc, i]() {
      for (int page = 0; page < doc->GetNumPages(); ++page) {
        const Document::PageSize page_size = doc->GetPageSize(page);
        EXPECT_GT(page_size.Height, 0)
            << " on page " << (page + 1) << " in thread " << i;
        EXPECT_GT(page_size.Width, 0)
            << " on page " << (page + 1) << " in thread " << i;
        if (!(rand() % 10)) {
          DummyPixelWriter dummy_pixel_writer;
          doc->Render(&dummy_pixel_writer, page, 1.0f, 0);
          EXPECT_EQ(
              dummy_pixel_writer.GetCallCount(),
              page_size.Width * page_size.Height)
              << " on page " << (page + 1) << " in thread " << i;
        }
        std::unique_ptr<const Document::OutlineItem> outline(doc->GetOutline());
        EXPECT_NE(outline.get(), nullptr);
      }
    }));
  }
  for (std::thread& thread : threads) {
    thread.join();
  }
}

