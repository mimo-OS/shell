add_library(shell_launcher OBJECT
    qmltypes.cpp
    launcher.cpp

    appitem.cpp
    appmanager.cpp
    iconitem.cpp
    launchermodel.cpp
    pagemodel.cpp

    launcher.qrc
)

qt6_add_dbus_adaptor(GENERATED com.cutefish.Launcher.xml launcher/launcher.h Launcher
                     launcheradaptor LauncherAdaptor)
set_source_files_properties(${GENERATED} PROPERTIES GENERATED TRUE SKIP_AUTOGEN ON)

target_sources(shell_launcher PRIVATE ${GENERATED})
target_include_directories(shell_launcher PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(shell_launcher PUBLIC shellcommon shell_core)
