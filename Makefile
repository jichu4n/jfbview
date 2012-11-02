CC := gcc
CFLAGS := -Wall -O2

all: jfbpdf
%.o: %.c
	$(CC) -c $(CFLAGS) $<
clean:
	-rm -f *.o jfbpdf

# pdf support using mupdf
jfbpdf: fbpdf.o mupdf.o draw.o outline.o input.o
	$(CC) -o $@ $^ $(LDFLAGS) -lfitz -lfreetype \
			-ljbig2dec -ljpeg -lz -lopenjpeg -lm
