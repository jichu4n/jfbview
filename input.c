/*
 * Input utility for jfbpdf.
 *
 * Copyright (C) 2012 Chuan Ji <jichuan89@gmail.com>
 *
 * This program is released under GNU GPL version 2.
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include "input.h"

/* An escape sequence to translate. */
typedef struct {
  const char *Sequence;
  int Code;
} EscapeSequence;
/* VT-100 key codes to translate, excluding the leading escape. */
static EscapeSequence EscapeSequences[] = {
  { "[A",  KEY_UP },
  { "[B",  KEY_DOWN },
  { "[D",  KEY_LEFT },
  { "[C",  KEY_RIGHT },
  { "[5~", KEY_PPAGE },
  { "[6~", KEY_NPAGE },
  { "OH",  KEY_HOME },
  { "[H",  KEY_HOME },
  { "[1~", KEY_HOME },
  { "OF",  KEY_END },
  { "[F",  KEY_HOME },
  { "[4~", KEY_END },
};

int GetChar() {
  static unsigned char buffer[3];
  static int buffer_size = 0;
  int c = 0;
  int read_chars;

  if (buffer_size) {
    return buffer[sizeof(buffer) - (buffer_size--)];
  }

  /* Assuming little-endian. */
  if ((read_chars = read(STDIN_FILENO, &c, 1)) <= 0) {
    return -1;
  }

  if (c == 0x1b) {  /* ESC or arrow keys. */
    /* If another 2 keys are available, assume an ANSI escape sequence.
     * Otherwise, assume plain ESC. */
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL), i;
    char seq[sizeof(buffer)];
    bool translated = false;

    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);
    read_chars = read(STDIN_FILENO, &seq, sizeof(buffer));
    if (read_chars > 0) {
      for (i = 0; i < sizeof(EscapeSequences) / sizeof(EscapeSequence); ++i) {
        if (!strncmp(EscapeSequences[i].Sequence, seq, read_chars)) {
          c = EscapeSequences[i].Code;
          translated = true;
          break;
        }
      }
    }
    if ((read_chars > 0) && (!translated)) {
      memcpy(buffer, seq, read_chars);
      buffer_size = read_chars;
    }
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags);
  }

  return c;
}

