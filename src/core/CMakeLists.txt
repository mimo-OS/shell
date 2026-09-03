# ---------------------------------------------------------------------------
# wlr-layer-shell
#
# KWin reserves work area for wlr-layer-shell surfaces with an exclusive zone;
# a plasma-shell panel reserves nothing. Layer shell cannot be used for the
# panels themselves, because LayerShellQt selects it through
# QT_WAYLAND_SHELL_INTEGRATION for the whole process and then every menu and
# tooltip becomes a full-screen layer surface too, so the shell talks to the
# protocol directly and uses it only for the invisible spacers in StrutSpacer.
#
# The protocol lives outside wayland-protocols, so a copy is vendored the way
# every other layer-shell client does it. xdg-shell comes along only because
# zwlr_layer_surface_v1.get_popup refers to xdg_popup.
# ---------------------------------------------------------------------------
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

set(WAYLAND_PROTOCOL_SRCS "")
foreach(protocol wlr-layer-shell-unstable-v1 xdg-shell)
    set(_xml ${PROJECT_SOURCE_DIR}/protocols/${protocol}.xml)
    set(_header ${CMAKE_CURRENT_BINARY_DIR}/${protocol}-client-protocol.h)
    set(_code ${CMAKE_CURRENT_BINARY_DIR}/${protocol}-protocol.c)

    add_custom_command(OUTPUT ${_header}
                       COMMAND ${WAYLAND_SCANNER_EXECUTABLE} client-header ${_xml} ${_header}
                       DEPENDS ${_xml} VERBATIM)
    add_custom_command(OUTPUT ${_code}
                       COMMAND ${WAYLAND_SCANNER_EXECUTABLE} private-code ${_xml} ${_code}
                       DEPENDS ${_xml} VERBATIM)

    list(APPEND WAYLAND_PROTOCOL_SRCS ${_header} ${_code})
endforeach()

set_source_files_properties(${WAYLAND_PROTOCOL_SRCS} PROPERTIES SKIP_AUTOGEN ON)

add_library(shell_core OBJECT
    ${WAYLAND_PROTOCOL_SRCS}

    qmltypes.cpp

    # The windows every component is built from.
    window/plasmasurface.cpp
    window/shellsurface.cpp
    window/popupwindow.cpp

    wayland/kwinwaylandinterface.cpp
    wayland/strutspacer.cpp

    # Services shared by more than one component. These used to be duplicated
    # once per binary; in one process there is exactly one of each.
    services/activity.cpp
    services/applicationresolver.cpp
    services/processprovider.cpp
    services/screenhelper.cpp
)

target_link_libraries(shell_core PUBLIC shellcommon)
target_include_directories(shell_core PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
