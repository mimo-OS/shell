cmake_minimum_required(VERSION 3.14)

project(cutefish-shell LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

find_package(Qt6 CONFIG REQUIRED Widgets DBus Concurrent Quick QuickControls2 LinguistTools)
find_package(KF6WindowSystem REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(XCB REQUIRED IMPORTED_TARGET xcb)

# ---------------------------------------------------------------------------
# Shell core: the single QQmlEngine, the window base class, and the desktop.
# ---------------------------------------------------------------------------
set(SHELL_SRCS
    src/main.cpp
    src/shell.cpp
    src/shellsurface.cpp
    src/desktopsurface.cpp
    src/desktopmanager.cpp

    # Services shared by more than one component. These used to be duplicated
    # once per binary; in one process there is exactly one of each.
    src/shared/activity.cpp
    src/shared/capplications.cpp
    src/shared/processprovider.cpp

    shell.qrc
)

# ---------------------------------------------------------------------------
# Status bar
# ---------------------------------------------------------------------------
set(STATUSBAR_SRCS
    components/statusbar/src/statusbar.cpp
    components/statusbar/src/controlcenterdialog.cpp
    components/statusbar/src/brightness.cpp
    components/statusbar/src/battery.cpp
    components/statusbar/src/appearance.cpp
    components/statusbar/src/notifications.cpp
    components/statusbar/src/backgroundhelper.cpp
    components/statusbar/src/poweractions.cpp

    components/statusbar/src/systemtray/statusnotifieritemsource.cpp
    components/statusbar/src/systemtray/systemtraytypes.cpp
    components/statusbar/src/systemtray/systemtraytypedefs.h
    components/statusbar/src/systemtray/systemtraymodel.cpp
    components/statusbar/src/systemtray/statusnotifierwatcher.cpp
    components/statusbar/src/systemtray/statusnotifieritemhost.cpp

    components/statusbar/src/libdbusmenuqt/dbusmenuimporter.cpp
    components/statusbar/src/libdbusmenuqt/dbusmenushortcut_p.cpp
    components/statusbar/src/libdbusmenuqt/dbusmenutypes_p.cpp
    components/statusbar/src/libdbusmenuqt/utils.cpp

    components/statusbar/src/appmenu/kdbusimporter.h
    components/statusbar/src/appmenu/appmenu.cpp
    components/statusbar/src/appmenu/appmenudbus.cpp
    components/statusbar/src/appmenu/verticalmenu.cpp
    components/statusbar/src/appmenu/menuimporter.cpp
    components/statusbar/src/appmenu/appmenumodel.cpp
    components/statusbar/src/appmenu/appmenuapplet.cpp
    components/statusbar/src/appmenu/dbusmenu_interface.cpp

    components/statusbar/statusbar.qrc
)

set(statusnotifierwatcher_xml components/statusbar/src/systemtray/org.kde.StatusNotifierWatcher.xml)
qt6_add_dbus_interface(STATUSBAR_GENERATED ${statusnotifierwatcher_xml} statusnotifierwatcher_interface)
qt6_add_dbus_interface(STATUSBAR_GENERATED components/statusbar/src/systemtray/org.freedesktop.DBus.Properties.xml dbusproperties)

set(statusnotifieritem_xml components/statusbar/src/systemtray/org.kde.StatusNotifierItem.xml)
set_source_files_properties(${statusnotifieritem_xml} PROPERTIES
   NO_NAMESPACE false
   INCLUDE "systemtray/systemtraytypes.h"
   CLASSNAME OrgKdeStatusNotifierItem
)
qt6_add_dbus_interface(STATUSBAR_GENERATED ${statusnotifieritem_xml} statusnotifieritem_interface)

qt6_add_dbus_adaptor(STATUSBAR_GENERATED components/statusbar/src/systemtray/org.kde.StatusNotifierWatcher.xml
                     components/statusbar/src/systemtray/statusnotifierwatcher.h StatusNotifierWatcher)

set_source_files_properties(components/statusbar/src/libdbusmenuqt/com.canonical.dbusmenu.xml PROPERTIES
   NO_NAMESPACE true
   INCLUDE "libdbusmenuqt/dbusmenutypes_p.h"
   CLASSNAME DBusMenuInterface
)

qt6_add_dbus_adaptor(STATUSBAR_GENERATED components/statusbar/src/appmenu/com.canonical.AppMenu.Registrar.xml
    components/statusbar/src/appmenu/menuimporter.h MenuImporter menuimporteradaptor MenuImporterAdaptor)
qt6_add_dbus_adaptor(STATUSBAR_GENERATED components/statusbar/src/appmenu/com.cutefish.cappmenu.xml
    components/statusbar/src/appmenu/appmenudbus.h AppmenuDBus appmenuadaptor AppmenuAdaptor)
qt6_add_dbus_adaptor(STATUSBAR_GENERATED components/statusbar/src/com.cutefish.Statusbar.xml
                     components/statusbar/src/statusbar.h StatusBar)

# ---------------------------------------------------------------------------
# Dock
# ---------------------------------------------------------------------------
set(DOCK_SRCS
    components/dock/src/applicationitem.h
    components/dock/src/applicationmodel.cpp
    components/dock/src/docksettings.cpp
    components/dock/src/mainwindow.cpp
    components/dock/src/systemappmonitor.cpp
    components/dock/src/systemappitem.cpp
    components/dock/src/trashmanager.cpp
    components/dock/src/utils.cpp
    components/dock/src/xwindowinterface.cpp
    components/dock/src/fakewindow.cpp

    components/dock/dock.qrc
)

qt6_add_dbus_adaptor(DOCK_GENERATED components/dock/src/com.cutefish.Dock.xml
                     components/dock/src/mainwindow.h MainWindow)

# ---------------------------------------------------------------------------
# Launcher
# ---------------------------------------------------------------------------
set(LAUNCHER_SRCS
    components/launcher/src/desktopproperties.cpp
    components/launcher/src/launcher.cpp
    components/launcher/src/launchermodel.cpp
    components/launcher/src/appitem.cpp
    components/launcher/src/pagemodel.cpp
    components/launcher/src/ucunits.cpp
    components/launcher/src/listmodelmanager.cpp
    components/launcher/src/iconitem.cpp
    components/launcher/src/appmanager.cpp

    components/launcher/launcher.qrc
)

qt6_add_dbus_adaptor(LAUNCHER_GENERATED
    components/launcher/src/com.cutefish.Launcher.xml
    components/launcher/src/launcher.h
    Launcher
    launcheradaptor
    LauncherAdaptor
)

# The adaptors qt6_add_dbus_adaptor generates already contain their moc output,
# so AUTOMOC must leave them alone.
set_source_files_properties(${DOCK_GENERATED} ${LAUNCHER_GENERATED} PROPERTIES SKIP_AUTOGEN ON)

add_executable(cutefish-shell
    ${SHELL_SRCS}
    ${STATUSBAR_SRCS}
    ${STATUSBAR_GENERATED}
    ${DOCK_SRCS}
    ${DOCK_GENERATED}
    ${LAUNCHER_SRCS}
    ${LAUNCHER_GENERATED}
)

target_include_directories(cutefish-shell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/components/statusbar/src
    ${CMAKE_CURRENT_SOURCE_DIR}/components/dock/src
    ${CMAKE_CURRENT_SOURCE_DIR}/components/launcher/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(cutefish-shell PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Concurrent
    Qt6::DBus
    KF6::WindowSystem
    PkgConfig::XCB
)

# ---------------------------------------------------------------------------
# Translations
#
# The three merged components each brought their own catalogue with the same
# per-locale file names, so the sources are prefixed (dock_zh_CN.ts, ...) and
# the shell loads one catalogue per component at startup.
# ---------------------------------------------------------------------------
file(GLOB TS_FILES translations/*.ts)
qt6_add_translation(QM_FILES ${TS_FILES})
add_custom_target(translations DEPENDS ${QM_FILES} SOURCES ${TS_FILES})
add_dependencies(cutefish-shell translations)
install(FILES ${QM_FILES} DESTINATION /usr/share/cutefish-shell/translations)

install(TARGETS cutefish-shell RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES components/dock/cutefish-dock-list.conf DESTINATION /etc)
