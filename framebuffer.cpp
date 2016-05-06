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

// This file implements the framebuffer abstraction.

#include "framebuffer.hpp"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

const char* const Framebuffer::DEFAULT_FRAMEBUFFER_DEVICE = "/dev/fb0";

Framebuffer* Framebuffer::Open(const std::string& device) {
  std::unique_ptr<Framebuffer> fb(new Framebuffer());

  if ((fb->_fd = open(device.c_str(), O_RDWR)) == -1) {
    goto error;
  }
  if ((ioctl(fb->_fd, FBIOGET_VSCREENINFO, &(fb->_vinfo)) == -1) ||
      (ioctl(fb->_fd, FBIOGET_FSCREENINFO, &(fb->_finfo)) == -1)) {
    goto error;
  }
  fb->_buffer = reinterpret_cast<uint8_t*>(mmap(
        nullptr,
        fb->GetBufferSize(),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fb->_fd,
        0));
  if (fb->_buffer == MAP_FAILED) {
    goto error;
  }

  fb->_format.reset(new Format(fb->_vinfo));
  fb->_pixel_buffer.reset(new PixelBuffer(
      fb->GetSize(), fb->_format.get(), fb->_buffer));
  return fb.release();

error:
  perror("Error initializing framebuffer");
  return nullptr;
}

Framebuffer::Framebuffer()
    : _buffer(nullptr), _format(nullptr), _pixel_buffer(nullptr) {
}

Framebuffer::~Framebuffer() {
  if (_buffer != MAP_FAILED) {
    memset(_buffer, 0, GetBufferSize());
    munmap(_buffer, GetBufferSize());
  }
  if (_fd != -1) {
    close(_fd);
  }
}

PixelBuffer* Framebuffer::NewPixelBuffer(const PixelBuffer::Size& size) {
  return new PixelBuffer(size, _format.get());
}

int Framebuffer::GetBufferSize() const {
  return _finfo.smem_len;
}

PixelBuffer::Size Framebuffer::GetSize() const {
  return PixelBuffer::Size(_vinfo.xres, _vinfo.yres);
}

void Framebuffer::Render(
    const PixelBuffer& src, const PixelBuffer::Rect& rect) {
  src.Copy(rect, _pixel_buffer->GetRect(), _pixel_buffer.get());
}

Framebuffer::Format::Format(const fb_var_screeninfo& vinfo)
    : _vinfo(vinfo) {
}

int Framebuffer::Format::GetDepth() const {
  return (_vinfo.bits_per_pixel + 7) >> 3;
}

uint32_t Framebuffer::Format::Pack(int r, int g, int b) const {
  return ((r >> (8 - _vinfo.red.length)) << _vinfo.red.offset) |
         ((g >> (8 - _vinfo.green.length)) << _vinfo.green.offset) |
         ((b >> (8 - _vinfo.blue.length)) << _vinfo.blue.offset);
}

