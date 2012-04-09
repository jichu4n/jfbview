/*
 * Framebuffer depth conversion utility.
 *
 * Copyright (C) 2012 Chuan Ji
 *
 * This program is released under GNU GPL version 2.
 */

/* Returns whether the current fb depth is supported. */
extern int depth_supported(int bpp);

/* Converts a framebuffer with a bpp of 32 to the depth of the current fb
 * device. Each row still begins at the same location, but will be compressed
 * accordingly. */
extern void *depth_conv(void *mem, int rows, int cols);

