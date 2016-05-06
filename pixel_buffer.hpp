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

// This file declares the PixelBuffer class, which represents a rectangular
// matrix of pixels.

#ifndef PIXEL_BUFFER_HPP
#define PIXEL_BUFFER_HPP

#include <stdint.h>

// A class that represents a rectangular matrix of pixels.
class PixelBuffer {
 public:
  // Size in pixels.
  struct Size {
    int Width;
    int Height;

    Size(int width, int height)
        : Width(width), Height(height) {
    }
  };
  // Color format of a pixel buffer.
  class Format {
   public:
    // Length of a pixel, in bytes. Must be between 0 and 4.
    virtual int GetDepth() const = 0;
    // Method to pack an RGB tuple into a pixel value.
    virtual uint32_t Pack(int r, int g, int b) const = 0;
    // This is required to keep C++ happy.
    virtual ~Format() {}
  };
  // A rectangular area on this pixel buffer.
  struct Rect {
    // Coordinates of the top-left corner of the rect.
    int X, Y;
    // Size of the rect.
    int Width, Height;

    explicit Rect(int x = 0, int y = 0, int width = 0, int height = 0)
        : X(x), Y(y), Width(width), Height(height) {
    }
  };

  // Constructs a new PixelBuffer object, and allocate memory. Will take
  // ownership of allocated memory. Does NOT take ownership of format.
  PixelBuffer(const Size& size, const Format* format);
  // Constructs a new PixelBuffer object, using a pre-allocated buffer. Will NOT
  // take ownership of the buffer. Does NOT take ownership of format.
  PixelBuffer(const Size& size, const Format* format, uint8_t* buffer);
  // Will free buffer if _has_ownship is true.
  ~PixelBuffer();

  // Returns the size of this buffer in pixels.
  Size GetSize() const;
  // Returns a rect covering the buffer exactly.
  Rect GetRect() const;

  // Writes a pixel value to a location in the buffer.
  void WritePixel(int x, int y, int r, int g, int b);

  // Copies a region in the current pixel buffer to another pixel buffer. The
  // destination region must be at least as large in both dimensions than the
  // source region. The source region is centered if the destination region is
  // larger, and the unaffected areas are set to black. This is multi-threaded.
  void Copy(
      const Rect& src_rect, const Rect& dest_rect, PixelBuffer* dest) const;

 private:
  // Prototype for a method that writes a pixel value to a location.
  class PixelWriterImpl {
   public:
    virtual void WritePixel(uint32_t value, void* dest) = 0;
  };
  // A pixel writer impl for depth = 1.
  class PixelWriterImpl1 : public PixelWriterImpl {
   public:
    void WritePixel(uint32_t value, void* dest) override;
  } _pixel_writer_impl_1;
  // A pixel writer impl for depth = 2.
  class PixelWriterImpl2 : public PixelWriterImpl {
   public:
    void WritePixel(uint32_t value, void* dest) override;
  } _pixel_writer_impl_2;
  // A pixel writer impl for depth = 3.
  class PixelWriterImpl3LittleEndian : public PixelWriterImpl {
   public:
    void WritePixel(uint32_t value, void* dest) override;
  } _pixel_writer_impl_3_little_endian;
  // A pixel writer impl for depth = 3.
  class PixelWriterImpl3BigEndian : public PixelWriterImpl {
   public:
    void WritePixel(uint32_t value, void* dest) override;
  } _pixel_writer_impl_3_big_endian;
  // A pixel writer impl for depth = 4.
  class PixelWriterImpl4 : public PixelWriterImpl {
   public:
    void WritePixel(uint32_t value, void* dest) override;
  } _pixel_writer_impl_4;

  // Size of the buffer.
  Size _size;
  // Format of current buffer.
  const Format* _format;
  // Pointer to buffer memory. Whether we own this memory depends on the
  // _has_ownership flag.
  uint8_t* _buffer;
  // Whether we own _buffer.
  bool _has_ownership;
  // Pixel writer implementation for current buffer. One of
  // _pixel_writer_impl_*.
  PixelWriterImpl* _pixel_writer_impl;

  // Common initialization called by both constructors.
  void Init();
  // Returns the size of the buffer in bytes.
  int GetBufferSize() const;
  // Returns the address in memory corresponding to the pixel (x, y).
  uint8_t* GetPixelAddress(int x, int y) const;

  // Disable copy and assign.
  PixelBuffer(const PixelBuffer&);
  PixelBuffer& operator = (const PixelBuffer&);
};

#endif

