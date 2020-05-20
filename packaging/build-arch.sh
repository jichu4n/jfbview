#!/bin/bash

src_dir="$(realpath "$(dirname "$0")/..")"

function budo() {
  sudo -u builduser "$@"
}

set -ex

pacman -Sy --needed --noconfirm \
  base-devel git sudo

useradd -m builduser
passwd -d builduser
echo -e '\nbuilduser ALL=(ALL) NOPASSWD:ALL\n' | tee -a /etc/sudoers

budo git clone https://aur.archlinux.org/jfbview-git.git /home/builduser/jfbview-git
cd /home/builduser/jfbview-git
budo makepkg --syncdeps --noconfirm

mkdir -p "${src_dir}"/upload
mv *.pkg.tar.xz "${src_dir}"/upload/

