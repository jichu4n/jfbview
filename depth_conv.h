/*
 * Framebuffer depth conversion utility.
 *
 * Copyright (C) 2012 Chuan Ji
 *
 * This program is released under GNU GPL version 2.
 */

/* Returns whether the current fb depth is supported. */
extern int depth_supported(int bpp);

/* Converts an array of 32bpp pixels to the depth of the current fb device. The
 * new array is stored consecutively starting at the same memory location.
 * Returns the new array at mem. */
extern void *depth_conv(void *mem, int len);

