#include "fitz_document.hpp"

#include <cassert>

Document* FitzDocument::Open(
    const std::string& path, const std::string* password) {
  fz_context* fz_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
  // Disable warning messages in the console.
  fz_set_warning_callback(
      fz_ctx, [](void* user, const char* message) {}, nullptr);

  fz_document* fz_doc = nullptr;
  fz_try(fz_ctx) {
    fz_doc = fz_open_document(fz_ctx, path.c_str());
    if ((fz_doc == nullptr) || (!fz_count_pages(fz_ctx, fz_doc))) {
      fz_throw(
          fz_ctx, FZ_ERROR_GENERIC,
          const_cast<char*>("Cannot open document \"%s\""), path.c_str());
    }
    if (fz_needs_password(fz_ctx, fz_doc)) {
      if (password == nullptr) {
        fz_throw(
            fz_ctx, FZ_ERROR_GENERIC,
            const_cast<char*>(
                "Document \"%s\" is password protected.\n"
                "Please provide the password with \"-P <password>\"."),
            path.c_str());
      }
      if (!fz_authenticate_password(fz_ctx, fz_doc, password->c_str())) {
        fz_throw(
            fz_ctx, FZ_ERROR_GENERIC,
            const_cast<char*>("Incorrect password for document \"%s\"."),
            path.c_str());
      }
    }
  }
  fz_catch(fz_ctx) {
    if (fz_doc != nullptr) {
      fz_drop_document(fz_ctx, fz_doc);
    }
    fz_drop_context(fz_ctx);
    return nullptr;
  }

  return new FitzDocument(fz_ctx, fz_doc);
}

FitzDocument::FitzDocument(fz_context* fz_ctx, fz_document* fz_doc)
    : _fz_ctx(fz_ctx), _fz_doc(fz_doc) {
  assert(_fz_ctx != nullptr);
  assert(_fz_doc != nullptr);
}

FitzDocument::~FitzDocument() {
  std::lock_guard<std::mutex> lock(_fz_mutex);
  fz_drop_document(_fz_ctx, _fz_doc);
  fz_drop_context(_fz_ctx);
}

int FitzDocument::GetNumPages() {
  std::lock_guard<std::mutex> lock(_fz_mutex);
  return fz_count_pages(_fz_ctx, _fz_doc);
}

const Document::PageSize FitzDocument::GetPageSize(
    int page, float zoom, int rotation) {
  return PageSize(0, 0);
}

void Document::Render(
    Document::PixelWriter* pw, int page, float zoom, int rotation) {
  // no-op
}