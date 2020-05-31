/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012-2020 Chuan Ji                                         *
 *                                                                           *
 *  Licensed under the Apache License, Version 2.0 (the "License");          *
 *  you may not use this file except in compliance with the License.         *
 *  You may obtain a copy of the License at                                  *
 *                                                                           *
 *   http://www.apache.org/licenses/LICENSE-2.0                              *
 *                                                                           *
 *  Unless required by applicable law or agreed to in writing, software      *
 *  distributed under the License is distributed on an "AS IS" BASIS,        *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 *  See the License for the specific language governing permissions and      *
 *  limitations under the License.                                           *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// This file declares FitzDocument, an implementation of the Document
// abstraction using Fitz.

#ifndef FITZ_DOCUMENT_HPP
#define FITZ_DOCUMENT_HPP

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "document.hpp"
extern "C" {
#include "mupdf/fitz.h"
}

// Document implementation using Fitz.
class FitzDocument : public Document {
 public:
  virtual ~FitzDocument();
  // Factory method to construct an instance of FitzDocument. path gives the
  // path to a file. password is the password to use to unlock the document;
  // specify nullptr if no password was provided. Does not take ownership of
  // password. Returns nullptr if the file cannot be opened.
  static Document* Open(const std::string& path, const std::string* password);
  // See Document.
  int GetNumPages() override;
  // See Document.
  const PageSize GetPageSize(int page, float zoom, int rotation) override;
  // See Document. Thread-safe.
  void Render(PixelWriter* pw, int page, float zoom, int rotation) override;
  // See Document.
  const OutlineItem* GetOutline() override { return nullptr; }
  // See Document.
  int Lookup(const OutlineItem* item) override { return -1; }

 protected:
  // See Document.
  std::vector<SearchHit> SearchOnPage(
      const std::string& search_string, int page, int context_length) override {
    return std::vector<SearchHit>();
  }

 private:
  // MuPDF structures.
  fz_context* _fz_ctx;
  fz_document* _fz_doc;
  // Mutex guarding MuPDF structures.
  std::mutex _fz_mutex;

  // We disallow the constructor; use the factory method Open() instead.
  FitzDocument(fz_context* _fz_context, fz_document* fz_document);
  // We disallow copying because we store lots of heap allocated state.
  explicit FitzDocument(const FitzDocument& other);
  FitzDocument& operator=(const FitzDocument& other);
};

#endif
