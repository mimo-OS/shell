add_library(shell_dock OBJECT
    qmltypes.cpp
    dockwindow.cpp

    applicationmodel.cpp
    autohidetrigger.cpp
    docksettings.cpp
    trashmanager.cpp

    dock.qrc
)

qt6_add_dbus_adaptor(GENERATED com.cutefish.Dock.xml dock/dockwindow.h DockWindow)
set_source_files_properties(${GENERATED} PROPERTIES GENERATED TRUE SKIP_AUTOGEN ON)

target_sources(shell_dock PRIVATE ${GENERATED})
target_include_directories(shell_dock PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(shell_dock PUBLIC shellcommon shell_core)
