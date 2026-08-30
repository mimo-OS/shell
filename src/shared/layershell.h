#ifndef CUTEFISH_LAYERSHELL_H
#define CUTEFISH_LAYERSHELL_H

// wayland-scanner names the last argument of get_layer_surface "namespace",
// which a C++ compiler will not accept. The generated header is C, so the
// keyword is renamed for the duration of the include; every other layer-shell
// client written in C++ does the same.
#define namespace layer_shell_namespace
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#undef namespace

#endif // CUTEFISH_LAYERSHELL_H
