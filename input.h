/*
 * Input utility for jfbpdf.
 *
 * Copyright (C) 2012 Chuan Ji <jichuan89@gmail.com>
 *
 * This program is released under GNU GPL version 2.
 */

/* Special character values; using the same as in ncurses. */
#define KEY_DOWN	0402		/* down-arrow key */
#define KEY_UP		0403		/* up-arrow key */
#define KEY_LEFT	0404		/* left-arrow key */
#define KEY_RIGHT	0405		/* right-arrow key */
#define KEY_NPAGE	0522		/* next-page key */
#define KEY_PPAGE	0523		/* previous-page key */
#define KEY_HOME	0406		/* home key */
#define KEY_END		0550		/* end key */

/* Reads a character (does not wait for new line) from stdin. */
extern int GetChar();

