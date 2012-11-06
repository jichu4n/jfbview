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

// This file implements the framebuffer abstraction.

#include "framebuffer.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

PixelBuffer::PixelBuffer(Framebuffer *parent,
                         const PixelBuffer::BufferSize &size)
    : _parent(parent), _size(size) {
  _buffer = new unsigned char[_size.Width * _size.Height * _parent->GetDepth()];
}

PixelBuffer::~PixelBuffer() {
  delete []_buffer;
}

PixelBuffer::BufferSize PixelBuffer::GetSize() const {
  return _size;
}

void PixelBuffer::WritePixel(int x, int y, int r, int g, int b) {
  _parent->WritePixel(r, g, b, GetPixelAddress(x, y));
}

unsigned char *PixelBuffer::GetPixelAddress(int x, int y) const {
  assert((x >= 0) && (x < _size.Width));
  assert((y >= 0) && (y < _size.Height));
  return _buffer + (y * _size.Width + x) * _parent->GetDepth();
}

const char * const Framebuffer::DEFAULT_FRAMEBUFFER_DEVICE = "/dev/fb0";

Framebuffer *Framebuffer::Open(const std::string &device) {
  Framebuffer *fb = new Framebuffer();
  if ((fb->_fd = open(device.c_str(), O_RDWR)) != -1) {
    if ((ioctl(fb->_fd, FBIOGET_VSCREENINFO, &(fb->_vinfo)) != -1) &&
        (ioctl(fb->_fd, FBIOGET_FSCREENINFO, &(fb->_finfo)) != -1)) {
      switch (fb->GetDepth()) {
       case 1:
        fb->_write_pixel_impl = &Framebuffer::WritePixel1;
        break;
       case 2:
        fb->_write_pixel_impl = &Framebuffer::WritePixel2;
        break;
       case 3:
        fb->_write_pixel_impl = &Framebuffer::WritePixel3;
        break;
       case 4:
        fb->_write_pixel_impl = &Framebuffer::WritePixel4;
        break;
       default:
        fprintf(stderr, "Unsupported color depth %d", fb->GetDepth());
        abort();
      }
      if ((fb->_buffer = mmap(NULL, fb->GetBufferSize(), PROT_READ | PROT_WRITE,
                              MAP_SHARED, fb->_fd, 0)) != MAP_FAILED) {
        return fb;
      }
    }
  }

  delete fb;
  perror("Error initializing framebuffer");
  return NULL;
}

Framebuffer::Framebuffer() {
  // Detect endian-ness.
  int x = 1;
  _little_endian = (reinterpret_cast<unsigned char *>(&x))[0];
}

Framebuffer::~Framebuffer() {
  if (_buffer != MAP_FAILED) {
    munmap(_buffer, GetBufferSize());
  }
  if (_fd != -1) {
    close(_fd);
  }
}

PixelBuffer *Framebuffer::NewPixelBuffer(const PixelBuffer::BufferSize &size) {
  return new PixelBuffer(this, size);
}

// Argument to BlitWorker.
struct BlitWorkerArg {
  // The source pixel buffer.
  const PixelBuffer *Source;
  // The target Framebuffer object.
  Framebuffer *Dest;
  // The first row in Source to copy.
  int SourceStartY;
  // The last + 1 row in Source to copy.
  int SourceEndY;
  // The in the framebuffer to write to.
  int DestStartY;
  // The pixel row in the source to start rendering from.
  int XOffset;
};

// Launched by Framebuffer::Blit via pthread_create. Copies a rect from a
// PixelBuffer onto the screen.
void *BlitWorker(void *_arg) {
  BlitWorkerArg *arg = reinterpret_cast<BlitWorkerArg *>(_arg);
  const int num_rows = arg->SourceEndY - arg->SourceStartY;
  const int screen_width = arg->Dest->GetSize().Width;
  const int copy_width = std::min(
      screen_width, arg->Source->GetSize().Width - arg->XOffset);
  const int copy_len = arg->Dest->GetDepth() * copy_width;
  const int screen_start_col = (screen_width - copy_width) / 2;

  for (int y = 0; y < num_rows; ++y) {
    void *source_row = arg->Source->GetPixelAddress(
        arg->XOffset, arg->SourceStartY + y);
    void *dest_row = arg->Dest->GetPixelAddress(
        screen_start_col, arg->DestStartY + y);
    memcpy(dest_row, source_row, copy_len);
  }
  return NULL;
}

void Framebuffer::Blit(const PixelBuffer &pixel_buffer,
                       int x_offset, int y_offset) {
  const ScreenSize &screen_size = GetSize();

  int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
  int num_rows_per_thread = screen_size.Height / num_threads;
  pthread_t *threads = new pthread_t[num_threads];
  BlitWorkerArg *args = new BlitWorkerArg[num_threads];
  for (int i = 0; i < num_threads; ++i) {
    args[i].Source = &pixel_buffer;
    args[i].Dest = this;
    args[i].SourceStartY = y_offset + i * num_rows_per_thread;
    args[i].SourceEndY =
        (i == num_threads - 1) ? y_offset + screen_size.Height
                               : y_offset + (i + 1) * num_rows_per_thread;
    args[i].DestStartY = i * num_rows_per_thread;
    args[i].XOffset = x_offset;

    pthread_create(&(threads[i]), NULL, &BlitWorker, &(args[i]));
  }
  for (int i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }
  delete []args;
  delete []threads;
}

int Framebuffer::GetBufferSize() const {
  return _finfo.smem_len;
}

int Framebuffer::GetDepth() const {
  return (_vinfo.bits_per_pixel + 7) >> 3;
}

void Framebuffer::WritePixel(int x, int y, int r, int g, int b) {
  WritePixel(r, g, b, GetPixelAddress(x, y));
}

Framebuffer::ScreenSize Framebuffer::GetSize() const {
  return ScreenSize(_vinfo.xres, _vinfo.yres);
}

void *Framebuffer::GetPixelAddress(int x, int y) const {
  return reinterpret_cast<unsigned char *>(_buffer) +
      (y + _vinfo.yoffset) * _finfo.line_length +
      (x + _vinfo.xoffset) * GetDepth();
}

unsigned int Framebuffer::GetPixelValue(int r, int g, int b) const {
  assert(GetDepth() <= static_cast<int>(sizeof(unsigned int)));
  unsigned int v = ((r >> (8 - _vinfo.red.length)) << _vinfo.red.offset) |
                   ((g >> (8 - _vinfo.green.length)) << _vinfo.green.offset) |
                   ((b >> (8 - _vinfo.blue.length)) << _vinfo.blue.offset);
  return v;
}

void Framebuffer::WritePixel(int r, int g, int b, void *dest) {
  (this->*_write_pixel_impl)(GetPixelValue(r, g, b), dest);
}

void Framebuffer::WritePixel1(unsigned int v, void *dest) {
  *(reinterpret_cast<unsigned char *>(dest)) = static_cast<unsigned char>(v);
}

void Framebuffer::WritePixel2(unsigned int v, void *dest) {
  assert(sizeof(unsigned short) == 2);
  *(reinterpret_cast<unsigned short *>(dest)) = static_cast<unsigned short>(v);
}

void Framebuffer::WritePixel3(unsigned int v, void *dest) {
  assert(sizeof(unsigned short) == 2);
  if (_little_endian) {
    *(reinterpret_cast<unsigned short *>(dest)) =
        static_cast<unsigned short>(v);
    *(reinterpret_cast<unsigned char *>(dest) + 2) =
        static_cast<unsigned char>(v >> 16);
  } else {
    *(reinterpret_cast<unsigned short *>(dest)) =
        static_cast<unsigned short>(v >> 8);
    *(reinterpret_cast<unsigned char *>(dest) + 2) =
        static_cast<unsigned char>(v);
  }
}

void Framebuffer::WritePixel4(unsigned int v, void *dest) {
  assert(sizeof(unsigned int) == 4);
  *(reinterpret_cast<unsigned int *>(dest)) = static_cast<unsigned int>(v);
}

