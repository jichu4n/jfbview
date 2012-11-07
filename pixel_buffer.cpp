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
#include <cstring>
#include <pthread.h>
#include <unistd.h>

// Argument to CopyWorker.
struct CopyWorkerArg {
  // Source and destination buffers.
  const PixelBuffer *Src;
  PixelBuffer *Dest;
  // Source and destination rects.
  PixelBuffer::Rect SrcRect, DestRect;
};

// Copies a PixelBuffer rect sequentially. Passed to pthread_create. The
// argument is of type CopyWorkerArg.
void *CopyWorker(void *_arg) {
  CopyWorkerArg *arg = reinterpret_cast<CopyWorkerArg *>(_arg);
  assert(arg->SrcRect.Width == arg->DestRect.Width);
  assert(arg->SrcRect.Height == arg->DestRect.Height);
  assert(arg->Src->_format->GetDepth() == arg->Dest->_format->GetDepth());

  for (int y = 0; y < arg->SrcRect.Height; ++y) {
    void *src_row = arg->Src->GetPixelAddress(
        arg->SrcRect.X, arg->SrcRect.Y + y);
    void *dest_row = arg->Dest->GetPixelAddress(
        arg->DestRect.X, arg->DestRect.Y + y);
    memcpy(dest_row, src_row,
           arg->SrcRect.Width * arg->Src->_format->GetDepth());
  }

  return NULL;
}

PixelBuffer::PixelBuffer(const PixelBuffer::Size &size,
                         const PixelBuffer::Format *format)
    : _size(size), _format(format), _has_ownership(true) {
  assert(_format != NULL);
  _buffer = new unsigned char[_size.Width * _size.Height * _format->GetDepth()];
  Init();
}

PixelBuffer::PixelBuffer(const PixelBuffer::Size &size,
                         const PixelBuffer::Format *format,
                         unsigned char *buffer)
    : _size(size), _format(format), _buffer(buffer), _has_ownership(false) {
  assert(_format != NULL);
  assert(_buffer != NULL);
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

void PixelBuffer::Copy(const PixelBuffer::Rect &src_rect,
                       const PixelBuffer::Rect &dest_rect,
                       PixelBuffer *dest) const {
  assert(dest_rect.Width >= src_rect.Width);
  assert(dest_rect.Height >= src_rect.Height);
  assert(_format->GetDepth() == dest->_format->GetDepth());
  assert(_size.Width >= src_rect.X + src_rect.Width);
  assert(_size.Height >= src_rect.Y + src_rect.Height);
  assert(dest->_size.Width >= dest_rect.X + dest_rect.Width);
  assert(dest->_size.Height >= dest_rect.Y + dest_rect.Height);

  // Centers src_rect in dest_rect.
  Rect centered_dest_rect;
  centered_dest_rect.Width = src_rect.Width;
  centered_dest_rect.X = dest_rect.X + (dest_rect.Width - src_rect.Width) / 2;
  centered_dest_rect.Height = src_rect.Height;
  centered_dest_rect.Y = dest_rect.Y + (dest_rect.Height - src_rect.Height) / 2;

  // Launch workers.
  int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
  int num_rows_per_thread = src_rect.Height / num_threads;
  pthread_t *threads = new pthread_t[num_threads];
  CopyWorkerArg *args = new CopyWorkerArg[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    args[i].Src = this;
    args[i].Dest = dest;
    args[i].SrcRect.X = src_rect.X;
    args[i].SrcRect.Width = src_rect.Width;
    args[i].SrcRect.Y = src_rect.Y + i * num_rows_per_thread;
    args[i].SrcRect.Height =
        (i == num_threads - 1) ?
            (src_rect.Height - i * num_rows_per_thread) :
            num_rows_per_thread;
    args[i].DestRect.X = centered_dest_rect.X;
    args[i].DestRect.Width = centered_dest_rect.Width;
    args[i].DestRect.Y = centered_dest_rect.Y + i * num_rows_per_thread;
    args[i].DestRect.Height = args[i].SrcRect.Height;

    pthread_create(&(threads[i]), NULL, &CopyWorker, &(args[i]));
  }
  for (int i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }
  delete []args;
  delete []threads;
}

void PixelBuffer::Init() {
  // Detect endian-ness.
  unsigned int x = 1;
  bool little_endian = (reinterpret_cast<unsigned char *>(&x))[0];
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
  return _buffer + (y * _size.Width + x) * _format->GetDepth();
}

