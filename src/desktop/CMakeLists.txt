# The desktop window per screen. Everything drawn inside it -- wallpaper, icons,
# selection, drag and drop, menus, file operations -- comes from the
# org.cutefish.filemanager.desktop QML plugin.
add_library(shell_desktop OBJECT
    desktopmanager.cpp
    desktopsurface.cpp

    desktop.qrc
)

target_link_libraries(shell_desktop PUBLIC shellcommon shell_core)
