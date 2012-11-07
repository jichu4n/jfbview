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

// This file declares the PixelBuffer class, which represents a rectangular
// matrix of pixels.

#ifndef PIXEL_BUFFER_HPP
#define PIXEL_BUFFER_HPP

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
  struct Format {
    // Length of a pixel, in bytes. Must be between 0 and 4.
    int Depth;
    // Method to pack an RGB tuple into a pixel value.
    virtual unsigned int Pack(int r, int g, int b) const = 0;
  };

  // Constructs a new PixelBuffer object, and allocate memory. Will take
  // ownership of allocated memory. Does NOT take ownership of format.
  PixelBuffer(const Size &size, const Format *format);
  // Constructs a new PixelBuffer object, using a pre-allocated buffer. Will NOT
  // take ownership of the buffer. Does NOT take ownership of format.
  PixelBuffer(const Size &size, const Format *format, unsigned char *buffer);
  // Will free buffer if _has_ownship is true.
  ~PixelBuffer();

  // Returns the size of this buffer in pixels.
  Size GetSize() const;

  // Writes a pixel value to a location in the buffer.
  void WritePixel(int x, int y, int r, int g, int b);

 private:
  // Prototype for a method that writes a pixel value to a location.
  class PixelWriterImpl {
   public:
    virtual void WritePixel(unsigned int value, void *dest) = 0;
  };
  // A pixel writer impl for depth = 1.
  class PixelWriterImpl1: public PixelWriterImpl {
   public:
    virtual void WritePixel(unsigned int value, void *dest);
  } _pixel_writer_impl_1;
  // A pixel writer impl for depth = 2.
  class PixelWriterImpl2: public PixelWriterImpl {
   public:
    virtual void WritePixel(unsigned int value, void *dest);
  } _pixel_writer_impl_2;
  // A pixel writer impl for depth = 3.
  class PixelWriterImpl3LittleEndian: public PixelWriterImpl {
   public:
    virtual void WritePixel(unsigned int value, void *dest);
  } _pixel_writer_impl_3_little_endian;
  // A pixel writer impl for depth = 3.
  class PixelWriterImpl3BigEndian: public PixelWriterImpl {
   public:
    virtual void WritePixel(unsigned int value, void *dest);
  } _pixel_writer_impl_3_big_endian;
  // A pixel writer impl for depth = 4.
  class PixelWriterImpl4: public PixelWriterImpl {
   public:
    virtual void WritePixel(unsigned int value, void *dest);
  } _pixel_writer_impl_4;

  // Size of the buffer.
  Size _size;
  // Format of current buffer.
  const Format *_format;
  // Pointer to buffer memory. Whether we own this memory depends on the
  // _has_ownership flag.
  unsigned char *_buffer;
  // Whether we own _buffer.
  bool _has_ownership;
  // Pixel writer implementation for current buffer. One of
  // _pixel_writer_impl_*.
  PixelWriterImpl *_pixel_writer_impl;

  // Common initialization called by both constructors.
  void Init();
  // Returns the address in memory corresponding to the pixel (x, y).
  unsigned char *GetPixelAddress(int x, int y) const;
};

#endif

