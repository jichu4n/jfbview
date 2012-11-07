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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

const char * const Framebuffer::DEFAULT_FRAMEBUFFER_DEVICE = "/dev/fb0";

Framebuffer *Framebuffer::Open(const std::string &device) {
  Framebuffer *fb = new Framebuffer();
  if ((fb->_fd = open(device.c_str(), O_RDWR)) != -1) {
    if ((ioctl(fb->_fd, FBIOGET_VSCREENINFO, &(fb->_vinfo)) != -1) &&
        (ioctl(fb->_fd, FBIOGET_FSCREENINFO, &(fb->_finfo)) != -1)) {
      if ((fb->_buffer = reinterpret_cast<unsigned char *> (
              mmap(NULL, fb->GetBufferSize(), PROT_READ | PROT_WRITE,
                   MAP_SHARED, fb->_fd, 0))) != MAP_FAILED) {
        fb->_format = new Format(fb->_vinfo);
        fb->_pixel_buffer = new PixelBuffer(
            fb->GetSize(), fb->_format, fb->_buffer);
        return fb;
      }
    }
  }

  delete fb;
  perror("Error initializing framebuffer");
  return NULL;
}

Framebuffer::Framebuffer()
    : _buffer(NULL), _format(NULL), _pixel_buffer(NULL) {
}

Framebuffer::~Framebuffer() {
  if (_buffer != MAP_FAILED) {
    munmap(_buffer, GetBufferSize());
  }
  if (_fd != -1) {
    close(_fd);
  }
  if (_format != NULL) {
    delete _format;
  }
  if (_pixel_buffer != NULL) {
    delete _pixel_buffer;
  }
}

PixelBuffer *Framebuffer::NewPixelBuffer(const PixelBuffer::Size &size) {
  return new PixelBuffer(size, _format);
}

int Framebuffer::GetBufferSize() const {
  return _finfo.smem_len;
}

PixelBuffer::Size Framebuffer::GetSize() const {
  return PixelBuffer::Size(_vinfo.xres, _vinfo.yres);
}

void Framebuffer::Render(const PixelBuffer &src,
                         const PixelBuffer::Rect &rect) {
  src.Copy(rect, _pixel_buffer->GetRect(), _pixel_buffer);
}

Framebuffer::Format::Format(const fb_var_screeninfo &vinfo)
    : _vinfo(vinfo) {
}

int Framebuffer::Format::GetDepth() const {
  return (_vinfo.bits_per_pixel + 7) >> 3;
}

unsigned int Framebuffer::Format::Pack(int r, int g, int b) const {
  assert(GetDepth() <= static_cast<int>(sizeof(unsigned int)));
  unsigned int v = ((r >> (8 - _vinfo.red.length)) << _vinfo.red.offset) |
                   ((g >> (8 - _vinfo.green.length)) << _vinfo.green.offset) |
                   ((b >> (8 - _vinfo.blue.length)) << _vinfo.blue.offset);
  return v;
}

