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

// This file defines the PixelBuffer class, which represents a rectangular
// matrix of pixels.

#include "pixel_buffer.hpp"
#include <unistd.h>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include "multithreading.hpp"

PixelBuffer::PixelBuffer(
    const PixelBuffer::Size& size,
    const PixelBuffer::Format* format)
    : _size(size), _format(format), _has_ownership(true) {
  assert(_format != nullptr);
  _buffer = new uint8_t[GetBufferSize()];
  Init();
}

PixelBuffer::PixelBuffer(
    const PixelBuffer::Size& size,
    const PixelBuffer::Format* format,
    uint8_t* buffer)
    : _size(size), _format(format), _buffer(buffer), _has_ownership(false) {
  assert(_format != nullptr);
  assert(_buffer != nullptr);
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

PixelBuffer::Rect PixelBuffer::GetRect() const {
  return Rect(0, 0, _size.Width, _size.Height);
}

void PixelBuffer::WritePixel(int x, int y, int r, int g, int b) {
  _pixel_writer_impl->WritePixel(_format->Pack(r, g, b), GetPixelAddress(x, y));
}

void PixelBuffer::Copy(
    const PixelBuffer::Rect& src_rect,
    const PixelBuffer::Rect& dest_rect,
    PixelBuffer* dest) const {
  assert(dest_rect.Width >= src_rect.Width);
  assert(dest_rect.Height >= src_rect.Height);
  assert(_format->GetDepth() == dest->_format->GetDepth());
  assert(_size.Width >= src_rect.X + src_rect.Width);
  assert(_size.Height >= src_rect.Y + src_rect.Height);
  assert(dest->_size.Width >= dest_rect.X + dest_rect.Width);
  assert(dest->_size.Height >= dest_rect.Y + dest_rect.Height);

  const int margin_top = (dest_rect.Height - src_rect.Height) / 2;
  const int margin_bottom = dest_rect.Height - margin_top - src_rect.Height;
  const int margin_left = (dest_rect.Width - src_rect.Width) / 2;
  const int margin_right = dest_rect.Width - margin_left - src_rect.Width;

  // Clear un-overwritten top and bottom margins.
  for (int y = 0; y < margin_top; ++y) {
    memset(dest->GetPixelAddress(dest_rect.X, dest_rect.Y + y),
           0, dest_rect.Width * dest->_format->GetDepth());
  }
  for (int y = 0; y < margin_bottom; ++y) {
    memset(dest->GetPixelAddress(
               dest_rect.X,
               dest_rect.Y + margin_top + src_rect.Height + y),
           0, dest_rect.Width * dest->_format->GetDepth());
  }

  // Launch workers to copy source rows.
  const int src_row_size = src_rect.Width * _format->GetDepth();
  ExecuteInParallel([=](int num_threads, int i) {
    const int num_rows_per_thread = src_rect.Height / num_threads;
    const int height = (i == num_threads - 1) ?
                           (src_rect.Height - i * num_rows_per_thread) :
                           num_rows_per_thread;
    const int src_y_begin = src_rect.Y + i * num_rows_per_thread;
    const int dest_y_begin = dest_rect.Y + margin_top + i * num_rows_per_thread;

    for (int y = 0; y < height; ++y) {
      const int src_y = src_y_begin + y;
      const int dest_y = dest_y_begin + y;
      // 1. Clear un-overwritten left and right margins.
      if (margin_left) {
        memset(dest->GetPixelAddress(dest_rect.X, dest_y),
               0, margin_left * dest->_format->GetDepth());
      }
      if (margin_right) {
        memset(dest->GetPixelAddress(dest_rect.X + margin_left + src_rect.Width,
                                     dest_y),
               0, margin_right * dest->_format->GetDepth());
      }
      // 2. Copy row content.
      void* src_row = GetPixelAddress(src_rect.X, src_y);
      void* dest_row = dest->GetPixelAddress(dest_rect.X + margin_left, dest_y);
      memcpy(dest_row, src_row, src_row_size);
    }
  });
}

void PixelBuffer::Init() {
  // Detect endian-ness.
  uint16_t x = 1;
  bool little_endian = (reinterpret_cast<uint8_t*>(&x))[0];
  // Set up writer impl.
  switch (_format->GetDepth()) {
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
      fprintf(stderr, "Unsupported color depth %d", _format->GetDepth());
      abort();
  }
}

void PixelBuffer::PixelWriterImpl1::WritePixel(uint32_t value, void* dest) {
  *(reinterpret_cast<uint8_t*>(dest)) =
      static_cast<uint8_t>(value);
}

void PixelBuffer::PixelWriterImpl2::WritePixel(uint32_t value, void* dest) {
  *(reinterpret_cast<uint16_t*>(dest)) =
      static_cast<uint16_t>(value);
}

void PixelBuffer::PixelWriterImpl3LittleEndian::WritePixel(
    uint32_t value, void* dest) {
  *(reinterpret_cast<uint16_t*>(dest)) =
      static_cast<uint16_t>(value);
  *(reinterpret_cast<uint8_t*>(dest) + 2) =
      static_cast<uint8_t>(value >> 16);
}

void PixelBuffer::PixelWriterImpl3BigEndian::WritePixel(
    uint32_t value, void* dest) {
  *(reinterpret_cast<uint16_t*>(dest)) =
      static_cast<uint16_t>(value >> 8);
  *(reinterpret_cast<uint8_t*>(dest) + 2) =
      static_cast<uint8_t>(value);
}

void PixelBuffer::PixelWriterImpl4::WritePixel(uint32_t value, void* dest) {
  *(reinterpret_cast<uint32_t*>(dest)) = static_cast<uint32_t>(value);
}

int PixelBuffer::GetBufferSize() const {
  return _size.Width * _size.Height * _format->GetDepth();
}

uint8_t* PixelBuffer::GetPixelAddress(int x, int y) const {
  assert((x >= 0) && (x < _size.Width));
  assert((y >= 0) && (y < _size.Height));
  return _buffer + (y * _size.Width + x) * _format->GetDepth();
}

