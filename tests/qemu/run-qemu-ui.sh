#!/bin/bash
set -e

cd "$(dirname "$0")/../.."
WORKSPACE_ROOT="$PWD"
QEMU_DIR="$WORKSPACE_ROOT/tests/qemu"
OUT_DIR="$QEMU_DIR/out"
mkdir -p "$OUT_DIR"

TEST_MODE=0
if [ "$1" = "--test" ]; then
    TEST_MODE=1
fi

# Parse version from CMakeLists.txt
VERSION=$(sed -n 's/^project(jfbview VERSION \(.*\))/\1/p' CMakeLists.txt)
DIST="trixie"
ARCH="amd64"
DEB_FILE="jfbview_${VERSION}~${DIST}_${ARCH}.deb"
DEB_PATH="upload/$DEB_FILE"

if [ "$TEST_MODE" = "1" ]; then
    JFBVIEW_CMD="jfbview --status-file=/out/status.log /testdata/bash.pdf"
    QEMU_EXTRA_ARGS="-display none -qmp unix:./qmp.sock,server,nowait"
    # Always regenerate seed.iso to ensure we have the correct command
    rm -f "$OUT_DIR/seed.iso"
else
    JFBVIEW_CMD="jfbview /testdata/bash.pdf"
    QEMU_EXTRA_ARGS=""
fi

if [ ! -f "$DEB_PATH" ]; then
    echo "=========================================================="
    echo "Looking for Debian package at $DEB_PATH"
    echo "Debian package not found. Building it now..."
    echo "=========================================================="
    # Clean previous build artifacts that might conflict (e.g. from host or different containers)
    rm -rf "$WORKSPACE_ROOT/build" "$WORKSPACE_ROOT/build_tests" "$WORKSPACE_ROOT/vendor/mupdf/build"
    ./packaging/build-and-check-package-with-docker.sh deb debian:trixie "$VERSION" "$DIST" "$ARCH"
fi

QCOW_URL="https://cloud.debian.org/images/cloud/trixie/daily/latest/debian-13-generic-amd64-daily.qcow2"
QCOW_IMG="$QEMU_DIR/debian-13-generic-amd64-daily.qcow2"

if [ ! -f "$QCOW_IMG" ]; then
    echo "=========================================================="
    echo "Downloading official Debian 13 (Trixie) QCOW2 image..."
    echo "=========================================================="
    wget -O "$QCOW_IMG" "$QCOW_URL" || {
        echo "Failed to download the daily image. Let's try the generic 'testing' directory..."
        wget -O "$QCOW_IMG" "https://cloud.debian.org/images/cloud/testing/daily/latest/debian-13-generic-amd64-daily.qcow2"
    }
fi

if [ ! -f "$OUT_DIR/seed.iso" ]; then
    echo "=========================================================="
    echo "Generating cloud-init seed.iso..."
    echo "=========================================================="
    cat << EOF > "$OUT_DIR/user-data"
#cloud-config
write_files:
  - path: /etc/systemd/system/getty@tty1.service.d/override.conf
    content: |
      [Service]
      ExecStart=
      ExecStart=-/sbin/agetty -o '-p -f \\\\u' --noclear --autologin root %I \$TERM
  - path: /root/.profile
    content: |
      mkdir -p /upload /testdata /out
      mount -t 9p -o trans=virtio,version=9p2000.L upload_mount /upload
      mount -t 9p -o trans=virtio,version=9p2000.L testdata_mount /testdata
      mount -t 9p -o trans=virtio,version=9p2000.L out_mount /out
      echo "Updating package lists..."
      apt-get update -qq
      echo "Installing $DEB_FILE and dependencies..."
      apt-get install -y "/upload/$DEB_FILE"
      setterm -cursor off 2>/dev/null || true
      dmesg -n 1 || true
      clear
      dd if=/dev/zero of=/dev/fb0 bs=1M count=8 2>/dev/null || true
      sleep 1
      $JFBVIEW_CMD
      exec bash
runcmd:
  - systemctl daemon-reload
  - systemctl restart getty@tty1.service
EOF
    touch "$OUT_DIR/meta-data"
    docker run --rm -v "$OUT_DIR:/qemu" debian:trixie \
        sh -c "apt-get update -qq && apt-get install -y genisoimage && genisoimage -output /qemu/seed.iso -volid cidata -joliet -rock /qemu/user-data /qemu/meta-data && chown $(id -u):$(id -g) /qemu/seed.iso"
fi

echo "=========================================================="
echo "Creating fresh overlay disk..."
echo "=========================================================="
rm -f "$OUT_DIR/overlay.qcow2"
qemu-img create -f qcow2 -b ../debian-13-generic-amd64-daily.qcow2 -F qcow2 "$OUT_DIR/overlay.qcow2"

echo "=========================================================="
echo "Launching QEMU..."
echo "A window should appear. Inside, the VM will boot, install the Debian package, and start jfbview."
echo "Close the QEMU window or press Ctrl+C here to stop the VM."
echo "=========================================================="

cd "$OUT_DIR" # For qemu-img relative paths to work cleanly

KVM_ARGS=""
if [ -e /dev/kvm ]; then
    KVM_ARGS="-enable-kvm"
fi

qemu-system-x86_64 \
    -m 2G \
    $KVM_ARGS \
    $QEMU_EXTRA_ARGS \
    -drive file=overlay.qcow2,format=qcow2,if=virtio \
    -drive file=seed.iso,format=raw,if=virtio \
    -netdev user,id=net0 -device virtio-net-pci,netdev=net0 \
    -virtfs local,path="$WORKSPACE_ROOT/upload",mount_tag=upload_mount,security_model=none,id=upload \
    -virtfs local,path="$WORKSPACE_ROOT/tests/testdata",mount_tag=testdata_mount,security_model=none,id=testdata \
    -virtfs local,path="$OUT_DIR",mount_tag=out_mount,security_model=none,id=out \
    -vga std \
    -device virtio-keyboard-pci \
    -device virtio-tablet-pci
