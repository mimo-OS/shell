#ifndef STRUTSPACER_H
#define STRUTSPACER_H

#include <QObject>
#include <QPointer>
#include <QScreen>

struct wl_surface;
struct zwlr_layer_surface_v1;

namespace KWayland
{
namespace Client
{
class Buffer;
class Surface;
}
}

/**
 * An invisible wlr-layer-shell surface whose only job is to reserve work area.
 *
 * KWin reserves space for layer surfaces with an exclusive zone, and for
 * nothing else: a panel that uses org_kde_plasma_shell
 * stacks above other windows but does not keep maximized ones out of its way.
 * Layer shell is no use for the panels themselves either, because LayerShellQt
 * selects it for the whole process through QT_WAYLAND_SHELL_INTEGRATION, which
 * turns every menu, tooltip and dialog into a full-screen layer surface as well.
 *
 * So the panel stays an ordinary window and a spacer claims the space for it:
 * a fully transparent surface with an empty input region, anchored to the same
 * screen edge, carrying the exclusive zone. It is invisible, it never receives
 * a click, and it needs none of the QPA integration.
 */
class StrutSpacer : public QObject
{
    Q_OBJECT

public:
    explicit StrutSpacer(QObject *parent = nullptr);
    ~StrutSpacer() override;

    //! The screen edge the panel is attached to. Only the four edges are
    //! meaningful; anything else leaves the spacer unmapped.
    void setEdge(Qt::Edge edge);

    //! How much space to keep clear, measured from the edge. Zero (or less)
    //! tears the surface down, which is what the dock wants while it is in
    //! auto-hide mode or while the launcher is up.
    void setThickness(int thickness);

    void setScreen(QScreen *screen);

    // Called from the wlr-layer-shell listener.
    static void configureCallback(void *data, zwlr_layer_surface_v1 *surface,
                                  quint32 serial, quint32 width, quint32 height);
    static void closedCallback(void *data, zwlr_layer_surface_v1 *surface);

private:
    void update();
    void destroySurface();
    void handleConfigure(quint32 serial, quint32 width, quint32 height);

    KWayland::Client::Surface *m_surface = nullptr;
    zwlr_layer_surface_v1 *m_layerSurface = nullptr;
    QPointer<QScreen> m_screen;
    QSize m_bufferSize;
    Qt::Edge m_edge = Qt::BottomEdge;
    int m_thickness = 0;
};

#endif // STRUTSPACER_H
