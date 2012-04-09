/*
 * Framebuffer depth conversion utility.
 *
 * Copyright (C) 2012 Chuan Ji
 *
 * This program is released under GNU GPL version 2.
 */

#include "draw.h"
#include <stdint.h>
#include <stdlib.h>

int depth_supported(int bpp) {
  return (bpp >= 2) && (bpp <= 4);
}

void *depth_conv_32(void *mem, int len) {
  return mem;
}

/* Convert 32bpp to 24bpp. */
void *depth_conv_24(void *mem, int len) {
  unsigned int i;
  uint32_t *mem_dw = (uint32_t *)mem, *p_dw;
  for (i = 0; i < len; ++i) {
    p_dw = (uint32_t *)(mem + i * 3);
    *p_dw = mem_dw[i];
  }
  return mem;
}

/* Convert 32bpp to 16bpp. */
void *depth_conv_16(void *mem, int len) {
  unsigned int i;
  uint32_t *mem_dw = (uint32_t *)mem;
  uint16_t *mem_w = (uint16_t *)mem;
  for (i = 0; i < len; ++i) {
    mem_w[i] = mem_dw[i];
  }
  return mem;
}

typedef void *(*depth_conv_function)(void *, int);

static depth_conv_function depth_conv_functions[] = {
  NULL,
  NULL,
  &depth_conv_16,
  &depth_conv_24,
  &depth_conv_32
};

void *depth_conv(void *mem, int rows, int cols) {
  depth_conv_function f =
      depth_conv_functions[FBM_BPP(fb_mode())];
  unsigned int i;
  for (i = 0; i < rows; ++i) {
    (*f)(mem + ((i * cols) << 2), cols);
  }
  return mem;
}

