# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                                             #
#    Copyright (C) 2012-2016 Chuan Ji <ji@chu4n.com>                          #
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

PREFIX ?= /usr/local
BINDIR := $(PREFIX)/bin
DATADIR := $(PREFIX)/share
DOCDIR := $(DATADIR)/doc/jfbview
MANDIR := $(DATADIR)/man

CONFIG_MK := config.mk

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                 Main Targets                                #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
ifeq ($(shell [ -f $(CONFIG_MK) ]; echo $$?), 0)

include $(CONFIG_MK)

override CXXFLAGS += -DMUPDF_VERSION=$(MUPDF_VERSION)

LIBS := \
    -lpthread \
    -lform \
    -lncurses \
    -lmupdf \
    -lfreetype \
    -lharfbuzz \
    -lz \
    -ljbig2dec \
    -ljpeg \
    -l$(OPENJP2)
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


.PHONY: all
all: jfbview jpdfcat jpdfgrep


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


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                         Build Environment Detection                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
else

.PHONY: all
all: $(CONFIG_MK)
	@$(MAKE)

$(CONFIG_MK): detect_mupdf_version detect_libopenjp2
	@echo
	@echo '============================='
	@echo "Detected configuration:"
	@echo
	@cat $(CONFIG_MK)
	@echo '============================='
	@echo

.PHONY: detect_mupdf_version
detect_mupdf_version: mupdf_version
	$(eval MUPDF_VERSION = $(shell ./$^))
	@echo "MUPDF_VERSION = $(MUPDF_VERSION)" >> $(CONFIG_MK)

.PHONY: detect_libopenjp2
detect_libopenjp2:
	$(eval OPENJP2 = $(shell ldconfig -p | grep -q libopenjp2 && echo 'openjp2' || echo 'openjpeg'))
	@echo "OPENJP2 = $(OPENJP2)" >> $(CONFIG_MK)

# mupdf_version only depends on -lmupdf.
mupdf_version: mupdf_version.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS) -lmupdf

endif


# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                     Misc                                    #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

.PHONY: install
install: all jfbview.1.gz
	for bin in jfbview jpdfcat jpdfgrep; do \
	  install -Dm755 ./$${bin} "$(BINDIR)/$${bin}"; \
	done
	install -Dm644 ./README "$(DOCDIR)/README"
	install -Dm644 ./jfbview.1.gz "$(MANDIR)/man1/jfbview.1.gz"

jfbview.1.gz: jfbview.1
	cat $^ | gzip > $@

.PHONY: lint
lint:
	cpplint \
	    --extensions=hpp,cpp \
	    --filter=-build/c++11,-readability/streams \
	    *.{h,c}pp

.PHONY: clean
clean:
	-rm -f *.o *.d $(CONFIG_MK) mupdf_version jfbview jfbpdf jpdfcat jpdfgrep

