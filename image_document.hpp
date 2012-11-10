/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012 Chuan Ji <jichuan89@gmail.com>                        *
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
// abstraction using Imlib2.

#ifndef IMAGE_DOCUMENT_HPP
#define IMAGE_DOCUMENT_HPP

#include "document.hpp"
#include <Imlib2.h>

// Document implementation using Imlib2.
class ImageDocument: public Document {
 public:

  virtual ~ImageDocument();
  // Factory method to construct an instance of ImageDocument. path gives the
  // path to an image file. Returns NULL if the file cannot be opened.
  static Document *Open(const std::string &path);
  // See Document.
  virtual int GetPageCount() {
    return 1;
  }
  // See Document.
  virtual const PageSize GetPageSize(int page, float zoom, int rotation);
  // See Document. Thread-safe.
  virtual void Render(PixelWriter *pw, int page, float zoom, int rotation);
  // See Document.
  virtual const OutlineItem *GetOutline() {
    return NULL;
  }
  // See Document.
  virtual int Lookup(const OutlineItem *item) {
    return -1;
  }

 private:
  // Handle to the source Imlib2 image, without zoom or rotation.
  Imlib_Image _src;
  // Original (unscaled, unrotated) size of _src.
  PageSize _src_size;

  // We disallow the constructor; use the factory method Open() instead.
  ImageDocument(Imlib_Image image);
  // We disallow copying because we store lots of heap allocated state.
  ImageDocument(const ImageDocument &other);
  ImageDocument &operator = (const ImageDocument &other);
};

#endif

