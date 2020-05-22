/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Copyright (C) 2012-2018 Chuan Ji                                         *
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
#include <sstream>
#include <string>

const char* const Framebuffer::DEFAULT_FRAMEBUFFER_DEVICE = "/dev/fb0";

Framebuffer* Framebuffer::Open(const std::string& device) {
  std::unique_ptr<Framebuffer> fb(new Framebuffer(device));

  if ((fb->_fd = open(device.c_str(), O_RDWR)) == -1) {
    goto error;
  }
  if ((ioctl(fb->_fd, FBIOGET_VSCREENINFO, &(fb->_vinfo)) == -1) ||
      (ioctl(fb->_fd, FBIOGET_FSCREENINFO, &(fb->_finfo)) == -1)) {
    goto error;
  }
  fb->_buffer = reinterpret_cast<uint8_t*>(mmap(
      nullptr, fb->GetBufferByteSize(), PROT_READ | PROT_WRITE, MAP_SHARED,
      fb->_fd, 0));
  if (fb->_buffer == MAP_FAILED) {
    goto error;
  }

  fb->_format.reset(new Format(fb->_vinfo));
  fb->_pixel_buffer.reset(new PixelBuffer(
      fb->GetSize(), fb->_format.get(), fb->_buffer, fb->GetAllocatedSize(),
      fb->GetOffset()));
  return fb.release();

error:
  perror(("Error initializing framebuffer device \"" + device + "\"").c_str());
  return nullptr;
}

Framebuffer::Framebuffer(const std::string& device)
    : _device(device),
      _buffer(nullptr),
      _format(nullptr),
      _pixel_buffer(nullptr) {}

Framebuffer::~Framebuffer() {
  if (_buffer != nullptr && _buffer != MAP_FAILED) {
    memset(_buffer, 0, GetBufferByteSize());
    munmap(_buffer, GetBufferByteSize());
  }
  if (_fd != -1) {
    close(_fd);
  }
}

std::string Framebuffer::GetDebugInfoString() {
  std::ostringstream out;

  out << "Device:\t\t\t" << _device << std::endl;
  out << "Visible resolution:\t" << _vinfo.xres << "x" << _vinfo.yres
      << std::endl;
  out << "Virtual resolution:\t" << _vinfo.xres_virtual << "x"
      << _vinfo.yres_virtual << std::endl;
  out << "Offset:\t\t\t" << _vinfo.xoffset << ", " << _vinfo.yoffset
      << std::endl;
  out << "Bits per pixel:\t\t" << _vinfo.bits_per_pixel << std::endl;
  out << "Bit depth:\t\t" << _format->GetDepth() << std::endl;
  out << "Red:\t\t\t"
      << "length " << _vinfo.red.length << ", offset " << _vinfo.red.offset
      << std::endl;
  out << "Green:\t\t\t"
      << "length " << _vinfo.green.length << ", offset " << _vinfo.green.offset
      << std::endl;
  out << "Blue:\t\t\t"
      << "length " << _vinfo.blue.length << ", offset " << _vinfo.blue.offset
      << std::endl;
  out << "Non-std pixel format:\t" << _vinfo.nonstd << std::endl;

  return out.str();
}

PixelBuffer* Framebuffer::NewPixelBuffer(const PixelBuffer::Size& size) {
  return new PixelBuffer(size, _format.get());
}

int Framebuffer::GetBufferByteSize() const { return _finfo.smem_len; }

PixelBuffer::Size Framebuffer::GetSize() const {
  return PixelBuffer::Size(_vinfo.xres, _vinfo.yres);
}

PixelBuffer::Size Framebuffer::GetAllocatedSize() const {
  return PixelBuffer::Size(_vinfo.xres_virtual, _vinfo.yres_virtual);
}

PixelBuffer::Size Framebuffer::GetOffset() const {
  return PixelBuffer::Size(_vinfo.xoffset, _vinfo.yoffset);
}

void Framebuffer::Render(
    const PixelBuffer& src, const PixelBuffer::Rect& rect) {
  src.Copy(rect, _pixel_buffer->GetRect(), _pixel_buffer.get());
}

Framebuffer::Format::Format(const fb_var_screeninfo& vinfo) : _vinfo(vinfo) {}

int Framebuffer::Format::GetDepth() const {
  return (_vinfo.bits_per_pixel + 7) >> 3;
}

uint32_t Framebuffer::Format::Pack(int r, int g, int b) const {
  return ((r >> (8 - _vinfo.red.length)) << _vinfo.red.offset) |
         ((g >> (8 - _vinfo.green.length)) << _vinfo.green.offset) |
         ((b >> (8 - _vinfo.blue.length)) << _vinfo.blue.offset);
}

