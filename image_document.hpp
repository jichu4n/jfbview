/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012-2014 Chuan Ji <ji@chu4n.com>                          *
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

// This file declares ImageDocument, an implementation of the Document
// abstraction using Imlib2. If the macro JFBVIEW_NO_IMLIB2 is defined, this
// class is disabled.

#ifndef JFBVIEW_NO_IMLIB2

#ifndef IMAGE_DOCUMENT_HPP
#define IMAGE_DOCUMENT_HPP

#include <string>
#include <vector>
#include "Imlib2.h"
#include "document.hpp"

// Document implementation using Imlib2.
class ImageDocument : public Document {
 public:
  virtual ~ImageDocument();
  // Factory method to construct an instance of ImageDocument. path gives the
  // path to an image file. Returns nullptr if the file cannot be opened.
  static Document* Open(const std::string& path);
  // See Document.
  int GetNumPages() override {
    return 1;
  }
  // See Document.
  const PageSize GetPageSize(int page, float zoom, int rotation) override;
  // See Document. Thread-safe.
  void Render(PixelWriter* pw, int page, float zoom, int rotation) override;
  // See Document.
  const OutlineItem* GetOutline()  override {
    return nullptr;
  }
  // See Document.
  int Lookup(const OutlineItem* item)  override {
    return -1;
  }

 protected:
  // See Document.
  std::vector<SearchHit> SearchOnPage(
      const std::string& search_string, int page, int context_length) override {
    return std::vector<SearchHit>();
  }

 private:
  // Handle to the source Imlib2 image, without zoom or rotation.
  Imlib_Image _src;
  // Original (unscaled, unrotated) size of _src.
  PageSize _src_size;

  // We disallow the constructor; use the factory method Open() instead.
  explicit ImageDocument(Imlib_Image image);
  // We disallow copying because we store lots of heap allocated state.
  explicit ImageDocument(const ImageDocument& other);
  ImageDocument& operator = (const ImageDocument& other);
};

#endif

#endif

