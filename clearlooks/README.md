# Clearlooks for KDE Plasma 6

A native KWin decoration inspired by the GNOME 2 Clearlooks Gummy theme. The
blue active title bar, warm grey inactive frame, beveled buttons, and button
symbols are drawn directly with Qt in logical coordinates. Qt renders them at
the display's scale. The window menu uses a drawn symbol. The plugin requires
no image assets.

## Install on KDE neon

Build on the machine running KWin so the plugin links against its KDecoration3
version. In a terminal, change into this source directory and run:

```sh
sudo apt update
sudo apt install build-essential cmake ninja-build extra-cmake-modules qt6-base-dev libkf6coreaddons-dev libkdecorations3-dev
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr &&
cmake --build build &&
sudo cmake --install build
```

Open the decoration settings:

```sh
kcmshell6 kcm_kwindecoration
```

Select **Clearlooks** and click **Apply**. If an already-open settings window
does not show it, close and reopen the window. The theme follows Plasma's
title font, button placement, and border-size settings.

## Remove

Switch to another decoration first. CMake records the installed module's
location in `build/install_manifest.txt`. Display the file with
`cat build/install_manifest.txt`, then remove the listed
`org.kde.clearlooks.so` with `sudo rm`.

## References

- [Clearlooks Gummy Metacity theme source](https://github.com/jpfleury/clearlooks-phenix/blob/master/metacity-1/metacity-theme-1.xml)
- [Clearlooks-Phénix desktop screenshot](https://github.com/jpfleury/clearlooks-phenix/blob/master/doc/exemple1.png)
- [Original Clearlooks screenshot gallery](https://clearlooks.sourceforge.net/screenshots/)
- [KDE KDecoration API and examples](https://github.com/KDE/kdecoration)
