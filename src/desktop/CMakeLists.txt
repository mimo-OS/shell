# The desktop window per screen. The shell draws the background itself; the
# icons and every file operation come from the org.cutefish.filemanager.desktop
# QML plugin.
add_library(shell_desktop OBJECT
    desktopmanager.cpp
    desktopsurface.cpp

    desktop.qrc
)

target_link_libraries(shell_desktop PUBLIC shellcommon shell_core)
