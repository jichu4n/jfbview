/*
 * Outline view for jfbpdf.
 *
 * Copyright (C) 2012 Chuan Ji <jichuan89@gmail.com>
 *
 * This program is released under GNU GPL version 2.
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "doc.h"
#include "draw.h"

#define KEY_UP ((unsigned char)-2)
#define KEY_DOWN ((unsigned char)-3)
#define KEY_LEFT ((unsigned char)-4)
#define KEY_RIGHT ((unsigned char)-5)

static struct doc *doc = NULL;  /* Current document. */
static struct outline *head;  /* First outline item visible. */
static struct outline *tail;  /* Last outline item visible. */
static struct outline *selected;  /* Outline item with focus. */
static int rows;  /* Height of screen in lines. */

/* Returns the previous displayed outline item. Returns 'it' if first in all
 * displayed outline items. Children of non-expanded items are skipped. */
static struct outline *outline_prev(struct outline *it) {
  struct outline *r = it;
  if (it->prev) {
    r = it->prev;
    while (r->expand && r->last_child) {
      r = r->last_child;
    }
  } else if (it->parent) {
    r = it->parent;
  }
  return r;
}

/* Returns the next displayed outline item. Returns 'it' if last in all
 * displayed outline items. Children of non-expanded items are skipped. */
static struct outline *outline_next(struct outline *it) {
  struct outline *r = it;
  if (it->expand && it->first_child) {
    r = it->first_child;
  } else if (it->next) {
    r = it->next;
  } else {
    while (it->parent) {
      if (it->parent->next) {
        r = it->parent->next;
        break;
      }
      it = it->parent;
    }
  }
  return r;
}

static void draw() {
  struct outline *it = head, *next;
  int i, j;

  printf("\x1b[2J\x1b[H");
  for (i = 0; i < rows; ++i) {
    for (j = 0; j < it->depth; ++j) {
      printf("|   ");
    }
    printf(it->first_child ? (it->expand ? "+ " : "- ")
                           : "  ");
    if (it == selected) {
      printf("\x1b[7m");
    }
    printf("%s\x1b[0m\x1b[K", it->title);
    if (i < rows - 1) {
      printf("\n");
    }
    tail = it;
    if ((next = outline_next(it)) == it) {
      break;
    } else {
      it = next;
    }
  }
  fflush(stdout);
}

static int main_loop() {
  for (;;) {
    unsigned char c;

    draw();

    read(STDIN_FILENO, &c, 1);
    if (c == 0x1b) {  /* ESC or arrow keys. */
      /* If another 2 keys are available, assume an ANSI escape sequence.
       * Otherwise, assume plain ESC. */
      int stdin_flags = fcntl(STDIN_FILENO, F_GETFL);
      unsigned char seq[2];
      fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);
      if (read(STDIN_FILENO, &seq, 2) == 2) {
        switch (seq[1]) {
         case 'A':
          c = KEY_UP;
          break;
         case 'B':
          c = KEY_DOWN;
          break;
         case 'C':
          c = KEY_RIGHT;
          break;
         case 'D':
          c = KEY_LEFT;
          break;
         default:
          break;
        }
      }
      fcntl(STDIN_FILENO, F_SETFL, stdin_flags);
    }
    switch (c) {
     case 'j':
     case KEY_DOWN:
      if ((selected == tail) && (tail != outline_next(tail))) {
        head = outline_next(head);
      }
      selected = outline_next(selected);
      break;
     case 'k':
     case KEY_UP:
      if (selected == head) {
        head = outline_prev(head);
      }
      selected = outline_prev(selected);
      break;
     case ' ':
      selected->expand = !(selected->expand);
      break;
     case '=':
     case '+':
     case KEY_RIGHT:
      selected->expand = true;
      break;
     case '-':
     case KEY_LEFT:
      selected->expand = false;
      break;
     case '\n':
     case 'g':
      return doc_lookup_page(doc, selected);
      break;
     case '\t':
     case 'q':
     case 0x1b:
      return -1;
    }
  }
}

int outline_view(struct doc *_doc) {
  struct winsize screen_size;

  if (_doc != doc) {
    if ((head = doc_outline(_doc)) == NULL) {
      printf("\x1b[HNo outline found.\x1b[K");
      fflush(stdout);
      return -1;
    }
    doc = _doc;
    selected = head;
  }

  ioctl(0, TIOCGWINSZ, &screen_size);
  rows = screen_size.ws_row;

  return main_loop();
}
