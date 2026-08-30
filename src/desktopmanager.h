#ifndef DESKTOPMANAGER_H
#define DESKTOPMANAGER_H

#include <QHash>
#include <QObject>

class DesktopSurface;
class QQmlEngine;
class QScreen;

/**
 * Keeps exactly one DesktopSurface alive per connected screen, and keeps the
 * "primary" flag pointing at the right one as monitors come and go.
 */
class DesktopManager : public QObject
{
    Q_OBJECT

public:
    explicit DesktopManager(QQmlEngine *engine, QObject *parent = nullptr);
    ~DesktopManager() override;

private slots:
    void screenAdded(QScreen *screen);
    void screenRemoved(QScreen *screen);
    void updatePrimaryScreen();

private:
    QQmlEngine *m_engine;
    QHash<QScreen *, DesktopSurface *> m_surfaces;
};

#endif // DESKTOPMANAGER_H
