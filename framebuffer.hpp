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

// This file declares the framebuffer abstraction.

#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include <linux/fb.h>
#include <string>

// An abstraction for a framebuffer device.
class Framebuffer {
 public:
  static const char * const DEFAULT_FRAMEBUFFER_DEVICE;
  // Size in pixels.
  struct ScreenSize {
    int Width;
    int Height;

    ScreenSize(int width, int height)
        : Width(width), Height(height) {
    }
  };
  // Factory method to initialize a framebuffer device and returns an
  // abstraction object. Returns NULL if the initialization failed. Caller owns
  // returned object.
  static Framebuffer *Open(
      const std::string &device = DEFAULT_FRAMEBUFFER_DEVICE);
  virtual ~Framebuffer();
  // Returns the size of the mmap'd buffer.
  int GetBufferSize() const;
  // Returns the color depth, i.e., number of bytes per pixel.
  int GetDepth() const;
  // Set a pixel on the framebuffer.
  void WritePixel(int x, int y, int r, int g, int b);
  // Retrieve the dimensions of the current display, in pixels.
  ScreenSize GetSize() const;
 private:
  // File descriptor of the opened framebuffer device.
  int _fd;
  // Framebuffer info structures.
  fb_var_screeninfo _vinfo;
  fb_fix_screeninfo _finfo;
  // Whether we're on a little-endian system.
  bool _little_endian;
  // mmap'd buffer.
  void *_buffer;
  // Handle to WritePixel[1-4], depending on present bpp.
  void (Framebuffer::*_write_pixel_impl)(unsigned int v, void *dest);
  // Contructors are disallowed. Use factory method Open() instead.
  Framebuffer();
  // No copying is allowed.
  Framebuffer(const Framebuffer &);
  Framebuffer &operator = (const Framebuffer &);
  // Returns the position within the buffer of the pixel at (x, y).
  void *GetPixelAddress(int x, int y) const;
  // Writes a pixel value to a location, assuming 8bpp.
  void WritePixel1(unsigned int v, void *dest);
  // Writes a pixel value to a location, assuming 16bpp.
  void WritePixel2(unsigned int v, void *dest);
  // Writes a pixel value to a location, assuming 24bpp.
  void WritePixel3(unsigned int v, void *dest);
  // Writes a pixel value to a location, assuming 32bpp.
  void WritePixel4(unsigned int v, void *dest);
};

#endif

