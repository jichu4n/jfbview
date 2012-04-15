/*
 * fbpdf - a small framebuffer pdf viewer using mupdf
 *
 * Copyright (C) 2009-2011 Ali Gholami Rudi
 * Copyright (C) 2012 Chuan Ji <jichuan89@gmail.com>
 *
 * This program is released under GNU GPL version 2.
 */
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pty.h>
#include <limits.h>
#include "draw.h"
#include "doc.h"
#include "outline.h"
#include "input.h"

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

#define PAGESTEPS		8
#define HPAGESTEPS		16
#define CTRLKEY(x)		((x) - 96)
#define MAXWIDTH		2
#define MAXHEIGHT		3
#define PDFCOLS			(1 << 11)
#define PDFROWS			(1 << 12)
#define MAXZOOM                 30
#define MINZOOM                 1

static fbval_t pbuf[PDFROWS * PDFCOLS];
static struct doc *doc;

static int num = 1;
static struct termios termios;
static char filename[256];
static int mark[128];		/* page marks */
static int mark_head[128];	/* head in page marks */
static int zoom = 10;
static int rotate;
static int head;
static int left;
static int count;
static int bpp;                 /* set by main(). */
static int zoom_step = 1;       /* multiples of 10% to zoom in or out */
static int page_rows = PDFROWS; /* actual width of current page in pixels */
static int page_cols = PDFCOLS; /* actual height of current page in pixels */

static void draw(void)
{
        int i;
        int start_col = MAX((fb_cols() - page_cols) >> 1, 0),
            start_row = MAX((fb_rows() - page_rows) >> 1, 0);

        left = MAX(0, MIN(page_cols - fb_cols(), left));
        head = MAX(0, MIN(page_rows - fb_rows(), head));

        for (i = 0; i <= start_row; ++i) {
                fb_set(i, 0, ((void *)pbuf) + (sizeof(pbuf) - PDFCOLS * bpp),
                       fb_cols());
        }

	for (i = head; i < head + fb_rows() - start_row; i++)
		fb_set(i - head + start_row, start_col,
                       ((void *)pbuf) + (i * PDFCOLS + left) * bpp,
                       fb_cols());
}

static int showpage(int p, int h)
{
	if (p < 1 || p > doc_pages(doc))
		return 0;
	memset(pbuf, 0x00, sizeof(pbuf));
        zoom = MAX(MIN(zoom, MAXZOOM), MINZOOM);
	doc_draw(doc, pbuf, p, PDFROWS, PDFCOLS, zoom, rotate);
        doc_geometry(doc, &page_rows, &page_cols);
	num = p;
	head = h;
	return 0;
}

static int getcount(int def)
{
	int result = count ? count : def;
	count = 0;
	return result;
}

static void printinfo(void)
{
	printf("\x1b[H");
	printf("JFBPDF:     file:%s  page:%d(%d)  zoom:%d%% \x1b[K",
		filename, num, doc_pages(doc), zoom * 10);
	fflush(stdout);
}

static void term_setup(void)
{
	struct termios newtermios;
	tcgetattr(STDIN_FILENO, &termios);
	newtermios = termios;
	newtermios.c_lflag &= ~ICANON;
	newtermios.c_lflag &= ~ECHO;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &newtermios);
}

static void term_cleanup(void)
{
	tcsetattr(STDIN_FILENO, 0, &termios);
}

static void sigcont(int sig)
{
	term_setup();
}

static void reload(void)
{
	doc_close(doc);
	doc = doc_open(filename);
	showpage(num, head);
}

/* Sets zoom and redraw. */
static void set_zoom(int new_zoom) {
  if (new_zoom != zoom) {
    int center_x = left + (MIN(page_cols, fb_cols()) >> 1),
        center_y = head + (MIN(page_rows, fb_rows()) >> 1);
    double center_ratio_x = (double) center_x / page_cols,
           center_ratio_y = (double) center_y / page_rows;

    zoom = new_zoom;
    showpage(num, 0);

    left = center_ratio_x * page_cols - (fb_cols() >> 1);
    head = center_ratio_y * page_rows - (fb_rows() >> 1);
  }
}

/* Automatically adjust zoom to fit current page to screen width. */
static void fit_to_width() {
  doc_draw(doc, pbuf, num, PDFROWS, PDFCOLS, 10, rotate);
  doc_geometry(doc, &page_rows, &page_cols);
  set_zoom(fb_cols() * 10 / page_cols);
}

static void mainloop(void)
{
	int step = fb_rows() / PAGESTEPS;
	int hstep = fb_cols() / HPAGESTEPS;
	int c, c2;
	term_setup();
	signal(SIGCONT, sigcont);
        fit_to_width();
        draw();
	while ((c = GetChar()) != -1) {
                int maxhead;
                int scrolled_unit = page_rows;
		switch (c) {
		case 'i':
			printinfo();
			break;
		case 'q':
			term_cleanup();
			return;
		case 27:
			count = 0;
			break;
		case 'm':
			c2 = GetChar();
			if (isalpha(c2)) {
				mark[c2] = num;
				mark_head[c2] = head;
			}
			break;
		case 'e':
			reload();
			break;
                case 's':
                        fit_to_width();
                        break;
                case '\t': {
                          int dest_page = outline_view(doc);
                          if (dest_page > 0) {
                            showpage(dest_page, 0);
                          }
                          draw();
                          break;
                        }
		case '`':
		case '\'':
			c2 = GetChar();
			if (isalpha(c2) && mark[c2])
				showpage(mark[c2], c == '`' ? mark_head[c2] : 0);
			break;
		default:
			if (isdigit(c))
				count = count * 10 + c - '0';
		}
		switch (c) {
		case CTRLKEY('f'):
		case 'J':
			showpage(num + getcount(1), 0);
			break;
		case CTRLKEY('b'):
		case 'K':
			showpage(num - getcount(1), 0);
			break;
                case KEY_HOME:
                        showpage(1, 0);
                        break;
                case KEY_END:
			showpage(doc_pages(doc), 0);
                        break;
		case 'G':
			showpage(getcount(doc_pages(doc)), 0);
			break;
		case 'z':
			set_zoom(getcount(15));
			break;
		case '=':
		case '+':
			set_zoom(zoom + zoom_step * getcount(1));
			break;
		case '-':
			set_zoom(zoom - zoom_step * getcount(1));
			break;
		case 'r':
			rotate = getcount(0);
			showpage(num, 0);
			break;
                case KEY_DOWN:
		case 'j':
                        head += step * getcount(1);
                        scrolled_unit = step;
                        break;
                       
                case KEY_UP:
		case 'k':
			head -= step * getcount(1);
                        scrolled_unit = step;
			break;
                case KEY_RIGHT:
		case 'l':
			left += hstep * getcount(1);
			break;
                case KEY_LEFT:
		case 'h':
			left -= hstep * getcount(1);
			break;
		case 'H':
			head = 0;
			break;
		case 'L':
			head = page_rows;
			break;
		case 'M':
			head = page_rows >> 1;
			break;
		case ' ':
		case CTRL('d'):
                case KEY_NPAGE:
			head += fb_rows() * getcount(1) - step;
                        scrolled_unit = fb_rows() - step;
			break;
		case 127:
		case CTRL('u'):
                case KEY_PPAGE:
			head -= fb_rows() * getcount(1) - step;
                        scrolled_unit = fb_rows() - step;
			break;
		case CTRLKEY('l'):
			break;
		default:
			/* no need to redraw */
			continue;
		}

                /* Scroll to next/prev page if we've moved down/up enough. */
		maxhead = page_rows - fb_rows();
                if (head > maxhead) {
                  if ((head < maxhead + scrolled_unit) ||
                      (num >= doc_pages(doc))) {
                    head = maxhead;
                  } else {
                    showpage(num + 1, 0);
                  }
                } else if (head < 0) {
                  if ((head > -scrolled_unit) || (num <= 1)) {
                    head = 0;
                  } else {
                    doc_draw(doc, pbuf, num - 1, PDFROWS, PDFCOLS, zoom,
                        rotate);
                    doc_geometry(doc, &page_rows, &page_cols);
                    showpage(num - 1, page_rows - fb_rows());
                  }
                }
		draw();
	}
}

static char *usage =
	"usage: jfbpdf [-r rotation] [-z zoom x10] [-p page] filename\n";

int main(int argc, char *argv[])
{
	char *hide = "\x1b[?25l";
	char *show = "\x1b[?25h";
	char *clear = "\x1b[2J";
	char *repos = "\x1b[H";
	int i = 1;
	if (argc < 2) {
		printf(usage);
		return 1;
	}
	strcpy(filename, argv[argc - 1]);
	doc = doc_open(filename);
	if (!doc) {
		fprintf(stderr, "cannot open <%s>\n", filename);
		return 1;
	}
	while (i + 2 < argc && argv[i][0] == '-') {
		if (argv[i][1] == 'r')
			rotate = atoi(argv[i + 1]);
		if (argv[i][1] == 'z')
			zoom = atoi(argv[i + 1]);
		if (argv[i][1] == 'p')
			num = atoi(argv[i + 1]);
		i += 2;
	}

	write(STDIN_FILENO, hide, strlen(hide));
	write(STDOUT_FILENO, clear, strlen(clear));
	printinfo();
	if (fb_init())
		return 1;
        bpp = FBM_BPP(fb_mode());
        mainloop();
	fb_free();
	write(STDOUT_FILENO, clear, strlen(clear));
	write(STDOUT_FILENO, repos, strlen(clear));
	write(STDIN_FILENO, show, strlen(show));
	doc_close(doc);
	return 0;
}
