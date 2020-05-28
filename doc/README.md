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
[![Travis CI Build Status](https://travis-ci.org/jichu4n/jfbview.svg?branch=master)](https://travis-ci.org/jichu4n/jfbview)

INSTALLATION
------------

### Arch Linux / Manjaro

Install package `jfbview` from the AUR, e.g.

```
yay -S jfbview
```

Source: https://aur.archlinux.org/packages/jfbview

### Debian / Ubuntu

See [Releases page](https://github.com/jichu4n/jfbview/releases) for pre-built `.deb` packages:

  - Debian 10 "buster": `amd64`, `i386`, `arm64` (ARMv8), `armhf` (ARMv7), `rpi` (Raspbian on ARMv6)
  - Debian 9 "stretch": `amd64`, `i386`, `arm64`, `armhf`, `rpi`
  - Ubuntu 20.04 LTS Focal: `amd64`
  - Ubuntu 18.04 LTS Bionic: `amd64`, `i386`
  - Ubuntu 16.04 LTS Xenial: `amd64`, `i386`

To build from source, see [`packaging/build-package-deb.sh`](https://github.com/jichu4n/jfbview/blob/master/packaging/build-package-deb.sh).

### CentOS / Fedora

See [Releases page](https://github.com/jichu4n/jfbview/releases) for pre-built `.rpm` packages:

  - CentOS 8 and Fedora 28+: `x86_64`, `aarch64` (ARMv8)

To build from source, see [`packaging/build-package-rpm.sh`](https://github.com/jichu4n/jfbview/blob/master/packaging/build-package-rpm.sh).

### Installing from source

Dependencies:

  - [NCURSES](https://invisible-island.net/ncurses/ncurses.html)

  - [Imlib2](https://docs.enlightenment.org/api/imlib2/html/index.html)

  - [libjpeg](http://libjpeg.sourceforge.net/) or [libjpeg-turbo](https://libjpeg-turbo.org/)

Build-time dependencies:

  - C++ compiler with support for C++14 (GCC 4.9+, Clang 3.5+)

  - [CMake](https://cmake.org/) 3.2+

Build:

```
git clone https://github.com/jichu4n/jfbview.git
cd jfbview
git submodule update --init --recursive

cmake -H. -Bbuild
make
make install
```

It is also possible to build a variant of jfbview without support for images
and without dependency on Imlib2. To build this variant, specify the CMake flag
`-DENABLE_IMAGE_SUPPORT=OFF`. For backwards compatibility, this will also
enable an alias for `jfbview` called `jfbpdf`.


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

