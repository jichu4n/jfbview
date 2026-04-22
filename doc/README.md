JFBVIEW
=======

**jfbview** is a PDF and image viewer for the Linux framebuffer. It's fast and has
some advanced features including:

  * Arbitrary zoom (10% - 1000%) and rotation;
  * Table of Contents (TOC) viewer for PDF documents;
  * Interactive text search for PDF documents;
  * Multi-threaded rendering;
  * Asynchronous background pre-caching;
  * Customizable multi-threaded caching.

The home page of jfbview is at https://github.com/jichu4n/jfbview.

![GitHub Workflow Build Status](https://github.com/jichu4n/jfbview/workflows/build/badge.svg)

INSTALLATION
------------

### Arch Linux / Manjaro

Install package `jfbview` from the AUR, e.g.

```
yay -S jfbview
```

Source: https://aur.archlinux.org/packages/jfbview

### Debian / Ubuntu

See [Releases](https://github.com/jichu4n/jfbview/releases) for pre-built
`.deb` packages for the following environments:

  - Debian 13 "trixie": `amd64`, `i386`, `arm64` (ARMv8), `armhf` (ARMv7)
  - Debian 12 "bookworm": `amd64`, `i386`, `arm64`, `armhf`
  - Debian 11 "bullseye": `amd64`, `i386`, `arm64`, `armhf`
  - Ubuntu 24.04 LTS Noble: `amd64`
  - Ubuntu 22.04 LTS Jammy: `amd64`

To build from source, fetch the source code along with transitive dependencies as described in the [Source code](#source-code) section below, then see
[`packaging/build-package-deb.sh`](https://github.com/jichu4n/jfbview/blob/master/packaging/build-package-deb.sh).

### Fedora

See [Releases](https://github.com/jichu4n/jfbview/releases) for pre-built `.rpm` packages for the following environments:

  - Fedora 43: `x86_64`, `aarch64` (ARMv8)
  - Fedora 42: `x86_64`, `aarch64`

To build from source, fetch the source code along with transitive dependencies as described in the [Source code](#source-code) section below, then see
[`packaging/build-package-rpm.sh`](https://github.com/jichu4n/jfbview/blob/master/packaging/build-package-rpm.sh).

### Installing from source

#### Dependencies

  - [NCURSES](https://invisible-island.net/ncurses/ncurses.html)

  - [libjpeg](http://libjpeg.sourceforge.net/) or [libjpeg-turbo](https://libjpeg-turbo.org/)

  - [OpenJPEG / openjp2](https://github.com/uclouvain/openjpeg)

  - [FreeType](https://www.freetype.org/)

  - [HarfBuzz](https://www.freedesktop.org/wiki/Software/HarfBuzz)

  - [zlib](https://www.zlib.net/)

Build-time dependencies:

  - C++ compiler with support for C++14 (GCC 4.9+, Clang 3.5+)

  - [CMake](https://cmake.org/) 3.3+

#### Source code

To fetch the source code along with all transitive dependencies with `git`:

```
git clone https://github.com/jichu4n/jfbview.git
cd jfbview
git submodule update --init --recursive
```

Alternatively, see [Releases](https://github.com/jichu4n/jfbview/releases) for
full source code archives including all transitive dependencies
(`jfbview-<VERSION>-full-source.zip`).

#### Build & install

```
cmake -H. -Bbuild
cd build
make
make install
```

DOCUMENTATION
-------------

See [jfbview man page](https://htmlpreview.github.io/?https://github.com/jichu4n/jfbview/blob/master/doc/jfbview.1.html).

ABOUT
-----

jfbview is written by Chuan Ji, and is distributed under the Apache License v2.

HISTORY
-------

jfbview started as a fork of FBPDF by Ali Gholami Rudi with improvements and bug
fixes, and was named JFBPDF. The JFBPDF code (in C) grew steadily more
convoluted as features were added, and finally was completely rewritten from
scratch in November 2012, with added support for images through Imlib2.

