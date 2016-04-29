# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2012-2014 Chuan Ji <ji@chu4n.com>                   #
#                                                                             #
#    Licensed under the Apache License, Version 2.0 (the "License");          #
#    you may not use this file except in compliance with the License.         #
#    You may obtain a copy of the License at                                  #
#                                                                             #
#     http://www.apache.org/licenses/LICENSE-2.0                              #
#                                                                             #
#    Unless required by applicable law or agreed to in writing, software      #
#    distributed under the License is distributed on an "AS IS" BASIS,        #
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. #
#    See the License for the specific language governing permissions and      #
#    limitations under the License.                                           #
#                                                                             #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

CXXFLAGS := -Wall -O2
override CXXFLAGS += -std=c++1y

ifdef MUPDF_VERSION

  override CXXFLAGS += $(MUPDF_VERSION_FLAG)

  LIBS := \
      -lpthread \
      -lform \
      -lncurses \
      -lharfbuzz \
      -lfreetype \
      -lz \
      -ljbig2dec \
      -ljpeg \
      -lopenjp2 \
      -lmupdf
  ifeq ($(shell [ $(MUPDF_VERSION) -ge 10009 ]; echo $$?), 0)
    LIBS += -lmupdfthird \
	-lssl \
	-lcrypto
  else
  ifeq ($(shell [ $(MUPDF_VERSION) -ge 10004 ]; echo $$?), 0)
    LIBS += -lmujs \
	-lssl \
	-lcrypto
  else
    LIBS += -lmupdf-js-none
  endif
  endif

  JFBVIEW_LIBS := $(LIBS) -lImlib2

  COMMON_SRCS := \
      command.cpp \
      document.cpp \
      framebuffer.cpp \
      image_document.cpp \
      multithreading.cpp \
      outline_view.cpp \
      pdf_document.cpp \
      pixel_buffer.cpp \
      search_view.cpp \
      string_utils.cpp \
      ui_view.cpp \
      viewer.cpp

  JFBVIEW_SRCS := $(COMMON_SRCS) \
      main.cpp

  JPDFCAT_SRCS := $(COMMON_SRCS) \
      jpdfcat.cpp

  JPDFGREP_SRCS := $(COMMON_SRCS) \
      jpdfgrep.cpp

endif


.PHONY: all
all: detect_mupdf_version
	$(MAKE) \
	    MUPDF_VERSION=$(MUPDF_VERSION) \
	    MUPDF_VERSION_FLAG=-DMUPDF_VERSION=$(MUPDF_VERSION) \
	    jfbview \
	    jpdfcat \
	    jpdfgrep

.PHONY: detect_mupdf_version
detect_mupdf_version: mupdf_version
	$(eval MUPDF_VERSION = $(shell ./$^))
	@echo '============================='
	@echo "Detected MuPDF version $(MUPDF_VERSION)"
	@echo '============================='

# mupdf_version only depends on -lmupdf.
mupdf_version: mupdf_version.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) -lmupdf

jfbview: $(JFBVIEW_SRCS:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(JFBVIEW_LIBS)

jfbpdf: $(JFBVIEW_SRCS)
	$(CXX) \
	    -DJFBVIEW_NO_IMLIB2 \
	    -DJFBVIEW_PROGRAM_NAME=\"JFBPDF\" \
	    -DJFBVIEW_BINARY_NAME=\"$@\" \
	    $(CXXFLAGS) -o $@ $^ \
	    $(LDLIBS) $(LIBS)

jpdfcat: $(JPDFCAT_SRCS:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(JFBVIEW_LIBS)

jpdfgrep: $(JPDFGREP_SRCS:.cpp=.o)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) $(JFBVIEW_LIBS)


.PHONY: lint
lint:
	cpplint \
	    --extensions=hpp,cpp \
	    --filter=-build/c++11,-readability/streams \
	    *.{h,c}pp

.PHONY: clean
clean:
	-rm -f *.o *.d mupdf_version jfbview jfbpdf jpdfcat jpdfgrep

