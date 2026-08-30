# Shell

Cutefish desktop shell: the status bar, dock, launcher and desktop in a single
process, sharing one QML engine.

Each component keeps its own window and its own D-Bus name, so the session
behaves exactly as it did when they were separate programs.

The desktop itself is not part of this repository. It is provided by the file
manager as the `org.cutefish.filemanager.desktop` QML module; the shell only
creates a window per screen and hands the desktop the screen it belongs to.

## Dependencies

### Debian/Ubuntu

```bash
sudo apt install cmake extra-cmake-modules pkg-config build-essential \
    qt6-base-dev qt6-declarative-dev qt6-tools-dev \
    libkf6windowsystem-dev kwayland-dev libxcb1-dev
```

[FishUI](https://github.com/cutefishos/fishui) and
[File Manager](https://github.com/cutefishos/filemanager) also need to be
installed before building; the file manager supplies the desktop QML module the
shell imports at runtime.

## Build

```bash
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
make
```

## Run on Wayland

The shell can run as a regular Qt Wayland client. Start it from an existing
Wayland session (for example KWin Wayland) after the file manager and FishUI have
been installed:

```bash
QT_QPA_PLATFORM=wayland ./build/cutefish-shell
```

The shell keeps its X11/EWMH integration for X11 sessions, but does not call
those APIs on Wayland. Under KWin Wayland, the dock and active-window service
use KDE's Plasma Window Management protocol for client discovery, focus, close,
maximize and minimize/restore. X11-only dock struts and appmenu properties are
intentionally skipped.

Only one shell instance can run at a time because it owns the legacy Cutefish
D-Bus service names. If another shell is already running, stop it before
launching this binary.

## Install

```bash
sudo make install
```

## License

This project has been licensed by GPLv3.
