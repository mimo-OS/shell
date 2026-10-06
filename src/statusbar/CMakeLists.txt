add_library(shell_statusbar OBJECT
    qmltypes.cpp
    statusbar.cpp

    statusbarforegroundhelper.cpp
    battery.cpp
    brightness.cpp
    notifications.cpp
    poweractions.cpp
    continuitymanager.cpp
    clipboardhelper.cpp

    systemtray/statusnotifieritemsource.cpp
    systemtray/statusnotifieritemhost.cpp
    systemtray/statusnotifierwatcher.cpp
    systemtray/systemtraymodel.cpp
    systemtray/systemtraytypes.cpp

    libdbusmenuqt/dbusmenuimporter.cpp
    libdbusmenuqt/dbusmenushortcut_p.cpp
    libdbusmenuqt/dbusmenutypes_p.cpp
    libdbusmenuqt/utils.cpp

    appmenu/appmenu.cpp
    appmenu/appmenudbus.cpp
    appmenu/appmenumodel.cpp
    appmenu/dbusmenu_interface.cpp
    appmenu/menuimporter.cpp
    appmenu/verticalmenu.cpp

    qmenu/qmenumodel.cpp

    statusbar.qrc
)

# ---------------------------------------------------------------------------
# D-Bus glue
# ---------------------------------------------------------------------------
qt6_add_dbus_interface(GENERATED systemtray/org.kde.StatusNotifierWatcher.xml
                       statusnotifierwatcher_interface)
qt6_add_dbus_interface(GENERATED systemtray/org.freedesktop.DBus.Properties.xml dbusproperties)

set_source_files_properties(systemtray/org.kde.StatusNotifierItem.xml PROPERTIES
    NO_NAMESPACE false
    INCLUDE "statusbar/systemtray/systemtraytypes.h"
    CLASSNAME OrgKdeStatusNotifierItem
)
qt6_add_dbus_interface(GENERATED systemtray/org.kde.StatusNotifierItem.xml
                       statusnotifieritem_interface)

qt6_add_dbus_adaptor(GENERATED systemtray/org.kde.StatusNotifierWatcher.xml
                     statusbar/systemtray/statusnotifierwatcher.h StatusNotifierWatcher)

set_source_files_properties(libdbusmenuqt/com.canonical.dbusmenu.xml PROPERTIES
    NO_NAMESPACE true
    INCLUDE "statusbar/libdbusmenuqt/dbusmenutypes_p.h"
    CLASSNAME DBusMenuInterface
)

qt6_add_dbus_adaptor(GENERATED appmenu/com.canonical.AppMenu.Registrar.xml
                     statusbar/appmenu/menuimporter.h MenuImporter
                     menuimporteradaptor MenuImporterAdaptor)
qt6_add_dbus_adaptor(GENERATED appmenu/com.cutefish.cappmenu.xml
                     statusbar/appmenu/appmenudbus.h AppmenuDBus
                     appmenuadaptor AppmenuAdaptor)
qt6_add_dbus_adaptor(GENERATED com.cutefish.Statusbar.xml
                     statusbar/statusbar.h StatusBar)

# The adaptors qt6_add_dbus_adaptor generates already contain their moc output,
# so AUTOMOC must leave them alone.
set_source_files_properties(${GENERATED} PROPERTIES GENERATED TRUE SKIP_AUTOGEN ON)

target_sources(shell_statusbar PRIVATE ${GENERATED})
target_include_directories(shell_statusbar PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(shell_statusbar PUBLIC shellcommon shell_core)
