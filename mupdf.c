#include <stdlib.h>
#include <string.h>
#include "fitz.h"
#include "mupdf.h"
#include "draw.h"
#include "doc.h"

struct doc {
  fz_context *context;
  pdf_document *document;
  int rows, cols;
  struct outline *outline;
};

/* Convert fz_outline to outline_iterator. */
static struct outline *convert_outline_impl(
    fz_outline *src, struct outline *parent, struct outline *prev, int depth) {
  if (src == NULL) {
    return NULL;
  }
  struct outline *dest = malloc(sizeof(struct outline));
  dest->title = src->title;
  dest->depth = depth;
  dest->parent = parent;
  dest->prev = prev;
  dest->first_child = convert_outline_impl(src->down, dest, NULL, depth + 1);
  dest->last_child = dest->first_child;
  while (dest->last_child && dest->last_child->next) {
    dest->last_child = dest->last_child->next;
  }
  dest->next = convert_outline_impl(src->next, parent, dest, depth);
  dest->expand = !depth;
  dest->data = src;
  return dest;
}

/* Convert fz_outline to outline_iterator. */
static struct outline *convert_outline(fz_outline *src) {
  return convert_outline_impl(src, NULL, NULL, 0);
}

/* Free doc outline. */
static void free_outline(struct outline *it) {
  if (it != NULL) {
    free_outline(it->first_child);
    free_outline(it->next);
    free(it);
  }
}

void *doc_draw(struct doc *doc, int p, int zoom, int rotate) {
  fz_matrix ctm;
  fz_bbox bbox;
  fz_pixmap *pix;
  fz_device *dev;
  fz_display_list *list;
  pdf_page *page;
  int x, y;
  int bpp = FBM_BPP(fb_mode());
  void *buffer = NULL;
  int buffer_size = 0;

  if ((page = pdf_load_page(doc->document, p - 1)))
    return NULL;
  list = fz_new_display_list(doc->context);
  dev = fz_new_list_device(doc->context, list);
  pdf_run_page(doc->document, page, dev, fz_identity, NULL);
  fz_free_device(dev);

  ctm = fz_translate(0, -pdf_bound_page(doc->document, page).y1);
  ctm = fz_concat(ctm, fz_scale((float) zoom / 10, (float) -zoom / 10));
  if (rotate)
    ctm = fz_concat(ctm, fz_rotate(rotate));
  bbox = fz_round_rect(fz_transform_rect(ctm,
                                         pdf_bound_page(doc->document, page)));

  pix = fz_new_pixmap_with_bbox(doc->context, fz_device_rgb, bbox);
  fz_clear_pixmap_with_value(doc->context, pix, 0xff);

  dev = fz_new_draw_device(doc->context, pix);
  fz_run_display_list(list, dev, ctm, bbox, NULL);
  fz_free_device(dev);

  doc->rows = fz_pixmap_height(doc->context, pix);
  doc->cols = fz_pixmap_width(doc->context, pix);
  buffer_size = doc->rows * doc->cols * bpp;
  if ((buffer = malloc(buffer_size)) == NULL) {
    fz_free_context(doc->context);
    return NULL;
  }
  memset(buffer, 0, buffer_size);
  for (y = 0; y < doc->rows; y++) {
    for (x = 0; x < doc->cols; x++) {
      unsigned char *s = fz_pixmap_samples(doc->context, pix) +
        ((y * doc->cols + x) << 2);
      fbval_t *d = (fbval_t *)(buffer + (y * doc->cols + x) * bpp);
      *d = FB_VAL(s[0], s[1], s[2]);
    }
  }
  fz_drop_pixmap(doc->context, pix);
  fz_free_display_list(doc->context, list);
  pdf_free_page(doc->document, page);
  return buffer;
}

int doc_pages(struct doc *doc) {
  return pdf_count_pages(doc->document);
}

struct doc *doc_open(char *path) {
  struct doc *doc = malloc(sizeof(*doc));
  doc->context = fz_new_context(NULL, NULL, FZ_STORE_DEFAULT);
  if ((doc->document = pdf_open_document(doc->context, path)) != NULL) {
    doc->outline = convert_outline(pdf_load_outline(doc->document));
    return doc;
  } else {
    fz_free_context(doc->context);
    free(doc);
    return NULL;
  }
}

void doc_close(struct doc *doc)
{
  free_outline(doc->outline);
  pdf_close_document(doc->document);
  free(doc);
  fz_free_context(doc->context);
}

void doc_geometry(struct doc *doc, int *rows, int *cols) {
  *rows = doc->rows;
  *cols = doc->cols;
}

struct outline *doc_outline(struct doc *doc) {
  return doc->outline;
}

int doc_lookup_page(struct doc *doc, struct outline *it) {
  return ((fz_outline *)(it->data))->dest.ld.gotor.page + 1;
}

