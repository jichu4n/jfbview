#ifndef DOC_H
#define DOC_H

#include <stdbool.h>

/* framebuffer depth */
typedef unsigned int fbval_t;

/* optimized version of fb_val() */
#define FB_VAL(r, g, b)	fb_val((r), (g), (b))

struct doc *doc_open(char *path);
int doc_pages(struct doc *doc);
int doc_draw(struct doc *doc, fbval_t *bitmap, int page, int rows, int cols, int zoom, int rotate);
void doc_close(struct doc *doc);
/* Retrieves the width and height of the previously drawn page of the document
 * in pixels. */
void doc_geometry(struct doc *doc, int *rows, int *cols);

/* Universal data structure for representing an outline item. */
struct outline {
  char *title;  /* Displayed title of the item. */
  int depth;    /* Nested depth; 0 = top level. */
  struct outline *next, *prev,               /* Siblings of item. */
                 *first_child, *last_child,  /* Children of item. */
                 *parent;                    /* Parent of item. */
  bool expand;  /* Whether the children of the item are displayed. */
  void *data;   /* Implementation-specific data. */
};

/* Returns an outline iterator for a document. Returns NULL if the document
 * does not contain outline information. */
extern struct outline *doc_outline(struct doc *doc);
/* Returns the page number linked by outline item. A negative return value
 * indicates an error. Page numbers are 1-based. */
extern int doc_lookup_page(struct doc *doc, struct outline *it);

#endif

