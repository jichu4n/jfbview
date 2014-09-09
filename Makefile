CXXFLAGS := -Wall -O3 -std=c++11
LIBS := -lpthread -lform -lncurses -lfreetype -ljbig2dec -ljpeg -lz -lopenjp2 -lmupdf -lmujs -lssl -lcrypto
JFBVIEW_LIBS := $(LIBS) -lImlib2

SRCS := $(wildcard *.cpp)


all: jfbview

.PHONY: lint
lint:
	cpplint \
	    --extensions=hpp,cpp \
	    --filter=-build/c++11 \
	    *.{h,c}pp

.PHONY: clean
clean:
	-rm -f *.o *.d jfbview jfbpdf

%.d: %.cpp
	@$(SHELL) -ec '$(CXX) -MM $(CXXFLAGS) $< \
	  | sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
	  [ -s $@ ] || rm -f $@'
-include $(SRCS:.cpp=.d)

jfbview: $(SRCS:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(JFBVIEW_LIBS)

jfbpdf: $(SRCS)
	$(CXX) \
	    -DJFBVIEW_NO_IMLIB2 \
	    -DJFBVIEW_PROGRAM_NAME=\"JFBPDF\" \
	    -DJFBVIEW_BINARY_NAME=\"$@\" \
	    $(CXXFLAGS) -o $@ $^ \
	    $(LDLIBS) $(LIBS)
