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
  };

  // Constructs a new PixelBuffer object, and allocate memory. Will take
  // ownership of allocated memory.
  PixelBuffer(const Size &size, const Format &format);
  // Constructs a new PixelBuffer object, using a pre-allocated buffer. Will NOT
  // take ownership of the buffer.
  PixelBuffer(const Size &size, const Format &format, unsigned char *buffer);
  // Will free buffer if _has_ownship is true.
  ~PixelBuffer();

  // Returns the size of this buffer in pixels.
  Size GetSize() const;

  // Releases the memory associated with this buffer.
  ~PixelBuffer();

 private:
  // Size of the buffer.
  Size _size;
  // Format of current buffer.
  Format _format;
  // Pointer to buffer memory. Whether we own this memory depends on the
  // _has_ownership flag.
  unsigned char *_buffer;
  // Whether we own _buffer.
  bool _has_ownership;
};

#endif

