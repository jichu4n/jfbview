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

// This file declares the framebuffer abstraction.

#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include <stdint.h>
#include <linux/fb.h>
#include <memory>
#include <string>
#include "pixel_buffer.hpp"

// An abstraction for a framebuffer device.
class Framebuffer {
 public:
  static const char* const DEFAULT_FRAMEBUFFER_DEVICE;
  // Factory method to initialize a framebuffer device and returns an
  // abstraction object. Returns nullptr if the initialization failed. Caller
  // owns returned object.
  static Framebuffer* Open(
      const std::string& device = DEFAULT_FRAMEBUFFER_DEVICE);
  virtual ~Framebuffer();

  // Creates a new pixel buffer with the given size. The pixel buffer will have
  // the same color settings as the screen. Caller owns returned value.
  PixelBuffer* NewPixelBuffer(const PixelBuffer::Size& size);

  // Retrieve the dimensions of the current display, in pixels.
  PixelBuffer::Size GetSize() const;

  // Renders a region in a pixel buffer onto the framebuffer device. The region
  // must be equal to or smaller than the screen size. If smaller, the source
  // rect is centered on screen.
  void Render(const PixelBuffer& src, const PixelBuffer::Rect& rect);

 private:
  // Color format of the framebuffer.
  class Format : public PixelBuffer::Format {
   public:
    // Grab settings from a fb_var_screeninfo.
    explicit Format(const fb_var_screeninfo& vinfo);
    // This is required to keep C++ happy.
    virtual ~Format() {}
    // See PixelBuffer::Format.
    int GetDepth() const override;
    // See PixelBuffer::Format.
    uint32_t Pack(int r, int g, int b) const override;
   private:
    fb_var_screeninfo _vinfo;
  };

  // File descriptor of the opened framebuffer device.
  int _fd;
  // Framebuffer info structures.
  fb_var_screeninfo _vinfo;
  fb_fix_screeninfo _finfo;
  // mmap'd buffer.
  uint8_t* _buffer;
  std::unique_ptr<Format> _format;
  // Pixel buffer object managing the mmap'ed buffer.
  std::unique_ptr<PixelBuffer> _pixel_buffer;

  // Contructors are disallowed. Use factory method Open() instead.
  Framebuffer();
  // No copying is allowed.
  Framebuffer(const Framebuffer&);
  Framebuffer& operator = (const Framebuffer&);

  // Returns the size of the mmap'd buffer in bytes.
  int GetBufferSize() const;
};

#endif

