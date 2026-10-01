#!/usr/bin/env bash
# Sets up the headless test distro (docs/tech/headless-emulator.md). Run as root inside it:
#   wsl -d emberclutch-test -u root -- bash /mnt/c/.../tools/wsl/setup.sh
# Safe to run again: it installs what's missing and leaves the rest. Azahar is pinned to the
# version on Windows, so a picture that changes is the game's doing, not the emulator's.
set -euo pipefail

AZAHAR_VERSION=2126.1.1
AZAHAR_SHA256=e445dabc18fe7665867e24a18a78e24e133261a9cc320fae6a4a45bd2e7c4783

export DEBIAN_FRONTEND=noninteractive
# Xvfb is the screen nobody sees; Mesa draws OpenGL on the CPU (llvmpipe) or through WSL's
# GPU bridge (d3d12); libopengl0 is the one library the AppImage doesn't carry.
packages=(xvfb xauth mesa-utils libgl1-mesa-dri libglx-mesa0 libegl1 libgl1 libopengl0
          curl ca-certificates file procps)
missing=()
for p in "${packages[@]}"; do dpkg -s "$p" >/dev/null 2>&1 || missing+=("$p"); done
if ((${#missing[@]})); then
    apt-get update -qq
    apt-get install -y -qq "${missing[@]}"
fi

dir=/opt/azahar/$AZAHAR_VERSION
if [[ ! -x $dir/squashfs-root/AppRun ]]; then
    mkdir -p "$dir"
    curl -fsSL -o "$dir/azahar.AppImage" \
        "https://github.com/azahar-emu/azahar/releases/download/$AZAHAR_VERSION/azahar.AppImage"
    echo "$AZAHAR_SHA256  $dir/azahar.AppImage" | sha256sum -c -
    chmod +x "$dir/azahar.AppImage"
    # Extracted rather than mounted: WSL has no FUSE by default.
    (cd "$dir" && ./azahar.AppImage --appimage-extract >/dev/null)
fi
ln -sfn "$dir" /opt/azahar/current

# Missing shared libraries show up here rather than as a silent crash at the first run.
if ldd "$dir/squashfs-root/usr/bin/azahar" | grep -q "not found"; then
    ldd "$dir/squashfs-root/usr/bin/azahar" | grep "not found"
    exit 1
fi
echo "ready: Azahar $AZAHAR_VERSION in $dir"
