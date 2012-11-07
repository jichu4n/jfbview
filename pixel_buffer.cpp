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

// This file defines the PixelBuffer class, which represents a rectangular
// matrix of pixels.

#include "pixel_buffer.hpp"
#include <cassert>
#include <cstdlib>
#include <cstdio>

PixelBuffer::PixelBuffer(const PixelBuffer::Size &size,
                         const PixelBuffer::Format *format)
    : _size(size), _format(format), _has_ownership(true) {
  _buffer = new unsigned char[_size.Width * _size.Height * _format->Depth];
  Init();
}

PixelBuffer::PixelBuffer(const PixelBuffer::Size &size,
                         const PixelBuffer::Format *format,
                         unsigned char *buffer)
    : _size(size), _format(format), _buffer(buffer), _has_ownership(false) {
  Init();
}

PixelBuffer::~PixelBuffer() {
  if (_has_ownership) {
    delete []_buffer;
  }
}

PixelBuffer::Size PixelBuffer::GetSize() const {
  return _size;
}

void PixelBuffer::WritePixel(int x, int y, int r, int g, int b) {
  _pixel_writer_impl->WritePixel(_format->Pack(r, g, b), GetPixelAddress(x, y));
}

void PixelBuffer::Init() {
  // Detect endian-ness.
  unsigned int x = 1;
  bool little_endian = (reinterpret_cast<unsigned char *>(&x))[0];
  // Set up writer impl.
  switch (_format->Depth) {
   case 1:
    _pixel_writer_impl = &_pixel_writer_impl_1;
    break;
   case 2:
    _pixel_writer_impl = &_pixel_writer_impl_2;
    break;
   case 3:
    if (little_endian) {
      _pixel_writer_impl = &_pixel_writer_impl_3_little_endian;
    } else {
      _pixel_writer_impl = &_pixel_writer_impl_3_big_endian;
    }
    break;
   case 4:
    _pixel_writer_impl = &_pixel_writer_impl_4;
    break;
   default:
    fprintf(stderr, "Unsupported color depth %d", _format->Depth);
    abort();
  }
}

void PixelBuffer::PixelWriterImpl1::WritePixel(unsigned int value, void *dest) {
  *(reinterpret_cast<unsigned char *>(dest)) =
      static_cast<unsigned char>(value);
}

void PixelBuffer::PixelWriterImpl2::WritePixel(unsigned int value, void *dest) {
  assert(sizeof(unsigned short) == 2);
  *(reinterpret_cast<unsigned short *>(dest)) =
      static_cast<unsigned short>(value);
}

void PixelBuffer::PixelWriterImpl3LittleEndian::WritePixel(unsigned int value,
                                                           void *dest) {
  assert(sizeof(unsigned short) == 2);
  *(reinterpret_cast<unsigned short *>(dest)) =
      static_cast<unsigned short>(value);
  *(reinterpret_cast<unsigned char *>(dest) + 2) =
      static_cast<unsigned char>(value >> 16);
}

void PixelBuffer::PixelWriterImpl3BigEndian::WritePixel(unsigned int value,
                                                        void *dest) {
  assert(sizeof(unsigned short) == 2);
  *(reinterpret_cast<unsigned short *>(dest)) =
      static_cast<unsigned short>(value >> 8);
  *(reinterpret_cast<unsigned char *>(dest) + 2) =
      static_cast<unsigned char>(value);
}

void PixelBuffer::PixelWriterImpl4::WritePixel(unsigned int value, void *dest) {
  assert(sizeof(unsigned int) == 4);
  *(reinterpret_cast<unsigned int *>(dest)) = static_cast<unsigned int>(value);
}

unsigned char *PixelBuffer::GetPixelAddress(int x, int y) const {
  assert((x >= 0) && (x < _size.Width));
  assert((y >= 0) && (y < _size.Height));
  return _buffer + (y * _size.Width + x) * _format->Depth;
}

