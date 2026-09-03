# What cutefish-notificationd was: the org.freedesktop.Notifications server, the
# banners, and the notification history. The centre itself is a popup window
# declared in the status bar's QML, the way the control centre is.
add_library(shell_notification OBJECT
    qmltypes.cpp
    notificationmanager.cpp

    datehelper.cpp
    historymodel.cpp
    notification.cpp
    notificationpopup.cpp
    notificationserver.cpp
    notificationsettings.cpp
    notificationsmodel.cpp
    notificationutils.cpp
    dbus/notificationsadaptor.cpp

    notification.qrc
)

qt6_add_dbus_adaptor(GENERATED com.cutefish.Notification.xml
                     notification/notificationmanager.h NotificationManager
                     notificationadaptor NotificationAdaptor)
set_source_files_properties(${GENERATED} PROPERTIES GENERATED TRUE SKIP_AUTOGEN ON)

target_sources(shell_notification PRIVATE ${GENERATED})
target_include_directories(shell_notification PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(shell_notification PUBLIC shellcommon shell_core)
