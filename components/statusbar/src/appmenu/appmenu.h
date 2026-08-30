#ifndef APPMENU_H
#define APPMENU_H

#include <QObject>
#include <xcb/xcb.h>

#include "menuimporter.h"
#include <QPointer>

class QDBusServiceWatcher;
class KDBusMenuImporter;
class AppmenuDBus;
class VerticalMenu;

class AppMenu : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    explicit AppMenu(QObject *parent = nullptr);
    ~AppMenu() override;

    bool eventFilter(QObject *object, QEvent *event) override;

Q_SIGNALS:
    /**
     * We do not know where is menu decoration button, so tell kwin to show menu
     */
    void showRequest(const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId);
    /**
     * This signal is emitted whenever popup menu/menubar is shown
     * Useful for decorations to know if menu button should look pressed
     */
    void menuShown(const QString &service, const QDBusObjectPath &objectPath);
    /**
     * This signal is emitted whenever popup menu/menubar is hidden
     * Useful for decorations to know if menu button should be release
     */
    void menuHidden(const QString &service, const QDBusObjectPath &objectPath);

private Q_SLOTS:
    /**
     * A new window was registered to AppMenu
     *
     * For compatibility this will set the DBus service name and menu object path as properties
     * on the window so we keep working with clients that use the DBusMenu "properly".
     */
    void slotWindowRegistered(WId id, const QString &serviceName, const QDBusObjectPath &menuObjectPath);
    /**
     * Show menu at QPoint(x,y) for DBus serviceName and menuObjectPath
     * if x or y == -1, show in application window
     */
    void slotShowMenu(int x, int y, const QString &serviceName, const QDBusObjectPath &menuObjectPath, int actionId);
    /**
     * Reconfigure module
     */
    void reconfigure();

    void itemActivationRequested(int actionId, uint timeStamp);


private:
    void hideMenu();

    void fakeUnityAboutToShow(const QString &service, const QDBusObjectPath &menuObjectPath);

    KDBusMenuImporter *getImporter(const QString &service, const QString &path);

    MenuImporter *m_menuImporter = nullptr;
    AppmenuDBus *m_appmenuDBus;
    QDBusServiceWatcher *m_menuViewWatcher;
    QPointer<VerticalMenu> m_menu;
    xcb_connection_t *m_xcbConn = nullptr;
};

#endif // APPMENU_H
