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

Framebuffer::~Framebuffer() {
  if (_buffer != MAP_FAILED) {
    munmap(_buffer, GetBufferSize());
  }
  if (_fd != -1) {
    close(_fd);
  }
}

Framebuffer::Framebuffer() {
  // Detect endian-ness.
  int x = 1;
  _little_endian = (reinterpret_cast<unsigned char *>(&x))[0];
}

int Framebuffer::GetBufferSize() const {
  return _finfo.smem_len;
}

int Framebuffer::GetDepth() const {
  return (_vinfo.bits_per_pixel + 7) >> 3;
}

void Framebuffer::WritePixel(int x, int y, int r, int g, int b) {
  assert(GetDepth() <= sizeof(unsigned int));
  unsigned int v = ((r >> (8 - _vinfo.red.length)) << _vinfo.red.offset) |
                   ((g >> (8 - _vinfo.green.length)) << _vinfo.green.offset) |
                   ((b >> (8 - _vinfo.blue.length)) << _vinfo.blue.offset);
  (this->*_write_pixel_impl)(v, GetPixelAddress(x, y));
}

Framebuffer::ScreenSize Framebuffer::GetSize() const {
  return ScreenSize(_vinfo.xres, _vinfo.yres);
}

void *Framebuffer::GetPixelAddress(int x, int y) const {
  return reinterpret_cast<unsigned char *>(_buffer) +
      (y + _vinfo.yoffset) * _finfo.line_length +
      (x + _vinfo.xoffset) * GetDepth();
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

#include <iostream>
#include <pthread.h>
using namespace std;

Framebuffer *fb;
void *update(void *_line) {
  int line = static_cast<int>(reinterpret_cast<long>(_line));
  for (int i = 0; i < 1000; ++i) {
    for (int y = 0; y < 75; ++y) {
      for (int x = 0; x < 800; ++x) {
        fb->WritePixel(x, y + line, i % 0x100, i % 0x100, i % 0x100);
      }
    }
  }
  return NULL;
}

int main() {
  fb = Framebuffer::Open();
  pthread_t a, b, c, d, e, f, g, h;
  pthread_create(&a, NULL, &update, reinterpret_cast<void *>(0));
  pthread_create(&b, NULL, &update, reinterpret_cast<void *>(75));
  pthread_create(&c, NULL, &update, reinterpret_cast<void *>(150));
  pthread_create(&d, NULL, &update, reinterpret_cast<void *>(225));
  pthread_create(&e, NULL, &update, reinterpret_cast<void *>(300));
  pthread_create(&f, NULL, &update, reinterpret_cast<void *>(375));
  pthread_create(&g, NULL, &update, reinterpret_cast<void *>(450));
  pthread_create(&h, NULL, &update, reinterpret_cast<void *>(525));
  pthread_join(a, NULL);
  pthread_join(b, NULL);
  pthread_join(c, NULL);
  pthread_join(d, NULL);
  pthread_join(e, NULL);
  pthread_join(f, NULL);
  pthread_join(g, NULL);
  pthread_join(h, NULL);
  delete fb;
}
