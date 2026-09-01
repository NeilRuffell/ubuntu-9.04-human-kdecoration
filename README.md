# Ubuntu 9.04 Human KDecoration3

A native **KDecoration3** recreation of the Ubuntu **9.04 (Jaunty Jackalope) Human** Metacity window decoration for modern KDE Plasma / KWin.

The implementation is based on the original Ubuntu 9.04 Human theme definition from `human-theme_0.28.8_all.deb`, including:

- the two-stage active titlebar gradient
- the inactive titlebar gradient
- 5 px frame geometry
- Human close/minimize/maximize/restore artwork
- Metacity-style HLS `shade()` colour calculations
- active/inactive/hover/pressed button gradients
- the four-pass active title shadow
- application icon as the menu button
- maximized-window geometry

## Status

The decoration builds and links as a KDecoration3 plugin.

The current implementation is a faithful native translation of the Jaunty Human drawing operations, but two areas are still being refined for pixel-level matching:

- exact hand-drawn Metacity corner pixels
- Qt text metrics versus the original Pango/Metacity metrics

## Original Ubuntu 9.04 source

The reference package is:

```
human-theme_0.28.8_all.deb
```

from Ubuntu's old-releases archive.

This repository does **not** bundle the original Human PNG assets. Run:

```bash
./fetch-jaunty-assets.sh
```

The script downloads the original Ubuntu package and extracts only the eight 10x10 Metacity button images required by this decoration.

## Fedora build dependencies

On Fedora KDE:

```bash
sudo dnf install -y \
  gcc-c++ cmake ninja-build extra-cmake-modules \
  qt6-qtbase-devel kf6-kcoreaddons-devel kdecoration-devel
```

## Build

```bash
./fetch-jaunty-assets.sh

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr

cmake --build build
```

## Install

```bash
sudo cmake --install build
kbuildsycoca6
```

Then open:

```bash
kcmshell6 kcm_kwindecoration
```

and select **Ubuntu 9.04 Human**.

## Uninstall

The module installs under the KDE KDecoration3 plugin directory as:

```
org.kde.karmichuman.so
```

To locate it:

```bash
find /usr/lib64 -path '*/org.kde.kdecoration3/*' \
  -name 'org.kde.karmichuman.so' -print
```

## Accuracy reference

The original Jaunty Metacity geometry specifies:

- left frame: 5 px
- right frame: 5 px
- bottom frame: 5 px
- left/right titlebar edge: 4 px
- title vertical padding: 4 px
- button aspect ratio: 0.9
- maximized left/right frame: 0 px
- maximized bottom frame: 1 px

The active titlebar is explicitly composed from two vertical gradients:

```
shade(gtk:bg[SELECTED], 1.30) -> shade(..., 1.00)
shade(gtk:bg[SELECTED], 0.97) -> shade(..., 1.10)
```

That produces the characteristic Human dark/light/dark raised appearance rather than a single linear gradient.

## Repository name

The plugin internally retains the historical development identifier `karmichuman` for compatibility with the work-in-progress source tree. The displayed decoration name is **Ubuntu 9.04 Human**.
