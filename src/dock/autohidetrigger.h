#ifndef AUTOHIDETRIGGER_H
#define AUTOHIDETRIGGER_H

#include <QQuickWindow>
#include <QTimer>

class DockSettings;
class PlasmaSurface;

/**
 * The strip along the screen edge that brings a hidden dock back.
 *
 * A bare QQuickWindow: it draws nothing and loads no QML, it exists only to be
 * a surface the compositor can deliver enter/leave and drag events on. It must
 * stack above ordinary windows to catch the pointer, and must never reserve
 * work area of its own.
 */
class AutoHideTrigger : public QQuickWindow
{
    Q_OBJECT

public:
    explicit AutoHideTrigger(DockSettings *settings, QWindow *parent = nullptr);

    bool containsMouse() const;
    void updateGeometry();

signals:
    void containsMouseChanged(bool contains);
    void dragEntered();

protected:
    bool event(QEvent *e) override;

private:
    void setContainsMouse(bool contains);

    DockSettings *m_settings;
    PlasmaSurface *m_plasmaSurface;
    QTimer m_delayedMouseTimer;
    bool m_delayedContainsMouse;
    bool m_containsMouse;
};

#endif // AUTOHIDETRIGGER_H
