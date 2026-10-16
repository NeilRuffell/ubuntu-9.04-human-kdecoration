# Ubuntu / KDE neon build instructions

This is a **native Plasma KWin window decoration**. It is **not** a GTK theme, a Kvantum theme, or a Plasma desktop theme.

## Compatibility

The project targets **Plasma 6 with KDecoration3**. Your installed distribution must provide `KDecoration3Config.cmake` and the matching development headers. Plasma 6 releases that still use KDecoration2 cannot load this KDecoration3 plugin. Do not substitute `libkdecorations2-dev`.

The Ubuntu package names below are confirmed for **Ubuntu 26.04**. KDE neon package availability depends on its current Ubuntu base and KDE repositories; check the installed versions before building.

## Dependencies (Ubuntu 26.04 / compatible KDE neon)

```bash
sudo apt update
sudo apt install --no-install-recommends \
  build-essential cmake ninja-build extra-cmake-modules \
  qt6-base-dev libkf6coreaddons-dev libkf6guiaddons-dev libkdecorations3-dev \
  curl binutils
```

Check that KDecoration3 is available:

```bash
apt-cache policy libkdecorations3-dev
```

If APT reports the package as unavailable, stop and check the enabled Ubuntu/KDE neon repositories and Plasma version. Do **not** mix development packages from another Ubuntu release or Fedora.

## Build (no root access needed)

From the repository root:

```bash
bash build-ubuntu.sh
```

This downloads the eight original Ubuntu 9.04 button images if missing, configures CMake for `/usr`, and compiles the plugin. It does **not** install anything system-wide.

## Install

Only after a successful build:

```bash
sudo cmake --install build
kbuildsycoca6
```

Open **System Settings → Colors & Themes → Window Decorations** and select **Ubuntu 9.04 Human**. If the decoration is not visible, log out and log back in, then check the plugin location.

## Confirm the installed plugin location

```bash
find /usr/lib -type f -path '*/org.kde.kdecoration3/org.kde.karmichuman.so' -print
```

On Ubuntu's multiarch systems this will normally be below `/usr/lib/<architecture-triplet>/`; it is not necessarily under Fedora's `/usr/lib64/`.

## Uninstall

First identify the installed file with the `find` command above. Remove **only** the installed `org.kde.karmichuman.so` file once its path is verified; then run `kbuildsycoca6`.

## Notes

- Compile against the KDecoration3 version shipped by the target computer. Do not copy a Fedora-built binary into KDE neon.
- The original Metacity artwork remains downloaded separately through `fetch-jaunty-assets.sh`.
- This repository currently has no distro CI build or KDE neon runtime test. A successful compile and appearance in System Settings must be verified on the actual target installation.
