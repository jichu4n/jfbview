#include <stdlib.h>
#include <string.h>
#include "fitz.h"
#include "mupdf.h"
#include "draw.h"
#include "doc.h"

struct doc {
	fz_glyph_cache *glyphcache;
	pdf_xref *xref;
        int rows, cols;
        struct outline *outline;
};

/* Convert pdf_outline to outline_iterator. */
static struct outline *convert_outline_impl(
    pdf_outline *src, struct outline *parent, struct outline *prev, int depth) {
  if (src == NULL) {
    return NULL;
  }
  struct outline *dest = malloc(sizeof(struct outline));
  dest->title = src->title;
  dest->depth = depth;
  dest->parent = parent;
  dest->prev = prev;
  dest->first_child = convert_outline_impl(src->child, dest, NULL, depth + 1);
  dest->last_child = dest->first_child;
  while (dest->last_child && dest->last_child->next) {
    dest->last_child = dest->last_child->next;
  }
  dest->next = convert_outline_impl(src->next, parent, dest, depth);
  dest->expand = !depth;
  dest->data = src;
  return dest;
}

/* Convert pdf_outline to outline_iterator. */
static struct outline *convert_outline(pdf_outline *src) {
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

	if (pdf_load_page(&page, doc->xref, p - 1))
		return NULL;
	list = fz_new_display_list();
	dev = fz_new_list_device(list);
	if (pdf_run_page(doc->xref, page, dev, fz_identity))
		return NULL;
	fz_free_device(dev);

	ctm = fz_translate(0, -page->mediabox.y1);
	ctm = fz_concat(ctm, fz_scale((float) zoom / 10, (float) -zoom / 10));
	if (rotate)
		ctm = fz_concat(ctm, fz_rotate(rotate));
	bbox = fz_round_rect(fz_transform_rect(ctm, page->mediabox));

	pix = fz_new_pixmap_with_rect(fz_device_rgb, bbox);
	fz_clear_pixmap_with_color(pix, 0xff);

	dev = fz_new_draw_device(doc->glyphcache, pix);
	fz_execute_display_list(list, dev, ctm, bbox);
	fz_free_device(dev);

        doc->rows = pix->h;
        doc->cols = pix->w;
        buffer_size = doc->rows * doc->cols * bpp;
        if ((buffer = malloc(buffer_size)) == NULL) {
          return NULL;
        }
        memset(buffer, 0, buffer_size);
        for (y = 0; y < doc->rows; y++) {
          for (x = 0; x < doc->cols; x++) {
            unsigned char *s = pix->samples + ((y * doc->cols + x) << 2);
            fbval_t *d = (fbval_t *)(buffer + (y * doc->cols + x) * bpp);
            *d = FB_VAL(s[0], s[1], s[2]);

          }
        }
	fz_drop_pixmap(pix);
	fz_free_display_list(list);
	pdf_free_page(page);
	pdf_age_store(doc->xref->store, 3);
	return buffer;
}

int doc_pages(struct doc *doc)
{
	return pdf_count_pages(doc->xref);
}

struct doc *doc_open(char *path)
{
	struct doc *doc = malloc(sizeof(*doc));
	fz_accelerate();
	doc->glyphcache = fz_new_glyph_cache();
	if (pdf_open_xref(&doc->xref, path, NULL)) {
		free(doc);
		return NULL;
	}
	if (pdf_load_page_tree(doc->xref)) {
		free(doc);
		return NULL;
	}
        doc->outline = convert_outline(pdf_load_outline(doc->xref));
	return doc;
}

void doc_close(struct doc *doc)
{
        free_outline(doc->outline);
	pdf_free_xref(doc->xref);
	fz_free_glyph_cache(doc->glyphcache);
	free(doc);
}

void doc_geometry(struct doc *doc, int *rows, int *cols) {
        *rows = doc->rows;
        *cols = doc->cols;
}

struct outline *doc_outline(struct doc *doc) {
  return doc->outline;
}

int doc_lookup_page(struct doc *doc, struct outline *it) {
  pdf_link *link = ((pdf_outline *)(it->data))->link;
  return pdf_find_page_number(doc->xref, fz_array_get(link->dest, 0)) + 1;
}

