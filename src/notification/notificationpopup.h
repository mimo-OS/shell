#ifndef NOTIFICATIONPOPUP_H
#define NOTIFICATIONPOPUP_H

#include <QQuickWindow>

class PlasmaSurface;

/**
 * One on-screen notification banner.
 *
 * A QQuickWindow rather than a QQuickView: the contents are declared in
 * QmlNotificationPopup.qml, so the window never loads a source of its own and
 * a QQuickView would only add a second QQmlEngine to the shell process.
 */
class NotificationPopup : public QQuickWindow
{
    Q_OBJECT
    //! Where the banner should sit, in global coordinates. See
    //! PlasmaSurface::setPosition().
    Q_PROPERTY(QPoint surfacePosition READ surfacePosition WRITE setSurfacePosition NOTIFY surfacePositionChanged)

public:
    explicit NotificationPopup(QQuickWindow *parent = nullptr);

    QPoint surfacePosition() const;
    void setSurfacePosition(const QPoint &position);

signals:
    void surfacePositionChanged();

private:
    PlasmaSurface *m_plasmaSurface;
};

#endif // NOTIFICATIONPOPUP_H
