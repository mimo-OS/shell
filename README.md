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
    libkf6windowsystem-dev libxcb1-dev
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

## Install

```bash
sudo make install
```

## License

This project has been licensed by GPLv3.
