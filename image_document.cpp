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

// This file defines ImageDocument, an implementation of the Document
// abstraction using Imlib2. If the macro JFBVIEW_NO_IMLIB2 is defined, this
// file is disabled.

#ifndef JFBVIEW_NO_IMLIB2

#include "image_document.hpp"
#include <stdint.h>
// For the M_PI constant.
#define _USE_MATH_DEFINES
#include <cmath>
#include <cassert>
#include <algorithm>
#include <string>
#include "multithreading.hpp"

// Converts degree angles to radian.
static inline double ToRadians(int degrees) {
  return static_cast<double>(degrees) * M_PI / 180.0;
}

// 2-D point.
struct Point {
  double X, Y;

  explicit Point(double x = 0.0, double y = 0.0)
      : X(x), Y(y) {
  }
};

Point operator + (const Point& a, const Point& b) {
  return Point(a.X + b.X, a.Y + b.Y);
}

Point operator - (const Point& a, const Point& b) {
  return Point(a.X - b.X, a.Y - b.Y);
}

Point operator * (const Point& a, double t) {
  return Point(a.X * t, a.Y * t);
}

// Rotates a point around the origin.
static inline Point Rotate(const Point& p, int degrees) {
  const double radians = ToRadians(degrees);
  const double cos_value = cos(radians), sin_value = sin(radians);
  return Point(p.X * cos_value - p.Y * sin_value,
               p.X * sin_value + p.Y * cos_value);
}

struct Rect {
  Point TopLeft;
  Point TopRight;
  Point BottomLeft;
  Point BottomRight;
};
// Given a rectangle of size (width x height) centered at the origin, perform
// rotation and zoom and returns the coordinates of the four vertices.
Rect ProjectRect(int width, int height, float zoom, int rotation) {
  Rect result;
  const Point origin(width / 2, height / 2);
  result.TopLeft =
      Rotate(Point(0, 0) - origin, rotation) * zoom;
  result.TopRight =
      Rotate(Point(width - 1, 0) - origin, rotation) * zoom;
  result.BottomLeft =
      Rotate(Point(0, height - 1) - origin, rotation) * zoom;
  result.BottomRight =
      Rotate(Point(width - 1, height - 1) - origin, rotation) * zoom;
  return result;
}

Document* ImageDocument::Open(const std::string& path) {
  Imlib_Image image = imlib_load_image_without_cache(path.c_str());
  if (image == nullptr) {
    return nullptr;
  }
  return new ImageDocument(image);
}

ImageDocument::ImageDocument(Imlib_Image image)
    : _src(image) {
  imlib_context_set_image(_src);
  _src_size.Width = imlib_image_get_width();
  _src_size.Height = imlib_image_get_height();
}

ImageDocument::~ImageDocument() {
  imlib_context_set_image(_src);
  imlib_free_image();
}

const Document::PageSize ImageDocument::GetPageSize(
    int page, float zoom, int rotation) {
  assert(page == 0);

  const Rect& projected =
      ProjectRect(_src_size.Width, _src_size.Height, zoom, rotation);
  const double xs[] = {
      projected.TopLeft.X,
      projected.TopRight.X,
      projected.BottomLeft.X,
      projected.BottomRight.X,
  };
  const double ys[] = {
      projected.TopLeft.Y,
      projected.TopRight.Y,
      projected.BottomLeft.Y,
      projected.BottomRight.Y,
  };

  return PageSize(
      static_cast<int>(
          *std::max_element(xs, xs + 4) - *std::min_element(xs, xs + 4)),
      static_cast<int>(
          *std::max_element(ys, ys + 4) - *std::min_element(ys, ys + 4)));
}

void ImageDocument::Render(
    Document::PixelWriter* pw, int page, float zoom, int rotation) {
  assert(page == 0);
  const Rect& projected =
      ProjectRect(_src_size.Width, _src_size.Height, zoom, rotation);
  const PageSize& dest_size = GetPageSize(page, zoom, rotation);
  Imlib_Image dest = imlib_create_image(dest_size.Width, dest_size.Height);
  imlib_context_set_image(dest);
  imlib_context_set_color(0, 0, 0, 255);
  imlib_image_fill_rectangle(0, 0, dest_size.Width, dest_size.Height);

  const Point dest_center(static_cast<double>(dest_size.Width) / 2.0,
                          static_cast<double>(dest_size.Height) / 2.0);
  const Point& dest_top_left = dest_center + projected.TopLeft;
  const Point& h_angle = projected.TopRight - projected.TopLeft;

  imlib_blend_image_onto_image_at_angle(
      _src, 0,
      0, 0, _src_size.Width, _src_size.Height,
      dest_top_left.X, dest_top_left.Y,
      h_angle.X, h_angle.Y);

  uint32_t* buffer = reinterpret_cast<uint32_t*>(
      imlib_image_get_data_for_reading_only());
  ExecuteInParallel([=](int num_threads, int i) {
    const int num_rows_per_thread = dest_size.Height / num_threads;
    const int y_begin = i * num_rows_per_thread;
    const int y_end = (i == num_threads - 1) ?
                          dest_size.Height :
                          (i + 1) * num_rows_per_thread;
    uint32_t* p = buffer + y_begin * dest_size.Width;
    for (int y = y_begin; y < y_end; ++y) {
      for (int x = 0; x < dest_size.Width; ++x) {
        uint8_t r = static_cast<uint8_t>((*p) >> 16),
                g = static_cast<uint8_t>((*p) >> 8),
                b = static_cast<uint8_t>((*p) >> 0);
        pw->Write(x, y, r, g, b);
        ++p;
      }
    }
  });

  imlib_free_image();
}

#endif


