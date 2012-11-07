CC := gcc
CFLAGS := -Wall -O2

all: jfbpdf
%.o: %.c
	$(CC) -c $(CFLAGS) $<

# pdf support using mupdf
jfbpdf: fbpdf.o mupdf.o draw.o outline.o input.o
	$(CC) -o $@ $^ $(LDFLAGS) -lfitz -lfreetype \
			-ljbig2dec -ljpeg -lz -lopenjpeg -lm

# jfbpdf v2 targets. DO NOT BUILD.
CXXFLAGS := -Wall -O2 -g
LDFLAGS := -lfitz -lfreetype -ljbig2dec -ljpeg -lz -lopenjpeg -lpthread \
	   -lncurses++

SRCS := $(wildcard *.cpp)

.PHONY: clean
clean:
	-rm -f *.o *.d jfbpdf jfbpdf2

%.d: %.cpp
	@$(SHELL) -ec '$(CXX) -MM $(CXXFLAGS) $< \
	  | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
	  [ -s $@ ] || rm -f $@'
-include $(SRCS:.cpp=.d)

jfbpdf2: $(SRCS:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

