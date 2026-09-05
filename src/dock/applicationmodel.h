#ifndef APPLICATIONMODEL_H
#define APPLICATIONMODEL_H

#include <QAbstractListModel>
#include "applicationitem.h"
#include "core/wayland/kwinwaylandinterface.h"

class ApplicationModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        AppIdRole = Qt::UserRole + 1,
        IconNameRole,
        VisibleNameRole,
        ActiveRole,
        WindowCountRole,
        WindowIdsRole,
        IsPinnedRole,
        DesktopFileRole,
        FixedItemRole,
        AnimationTokenRole
    };

    explicit ApplicationModel(QObject *parent = nullptr);

    bool initialWindowsLoaded() const { return m_initialWindowsLoaded; }

    void setLauncher(class Launcher *launcher);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addItem(const QString &desktopFile);
    void removeItem(const QString &desktopFile);
    bool desktopContains(const QString &desktopFile);
    bool isDesktopPinned(const QString &desktopFile);

    Q_INVOKABLE void save() { savePinAndUnPinList(); }

    Q_INVOKABLE void clicked(const QString &id);
    Q_INVOKABLE void raiseWindow(const QString &id);

    Q_INVOKABLE bool openNewInstance(const QString &appId);
    Q_INVOKABLE void closeAllByAppId(const QString &appId);
    Q_INVOKABLE void quit(const QString &appId);
    Q_INVOKABLE void pin(const QString &appId);
    Q_INVOKABLE void unPin(const QString &appId);

    Q_INVOKABLE void move(int from, int to);

signals:
    void countChanged();

    void itemAdded();
    void itemRemoved();

private:
    ApplicationItem *findItemByWId(quint64 wid);
    ApplicationItem *findItemById(const QString &id);
    ApplicationItem *findItemByDesktop(const QString &desktop);
    void updateFromDesktopEntry(ApplicationItem *item, class DesktopEntry *entry);

    bool contains(const QString &id);
    int indexOf(const QString &id);
    void initPinnedApplications();
    int animationTokenForNewItem();
    void savePinAndUnPinList();
    void onApplicationsChanged();

    void handleDataChangedFromItem(ApplicationItem *item);

    void loadExistingWindows();
    QString desktopPathFor(quint64 wid) const;

    void onWindowAdded(quint64 wid);
    void onWindowRemoved(quint64 wid);
    void onWindowChanged(quint64 wid);
    void onActiveChanged(quint64 wid);

private:
    KWinWaylandInterface *m_kwin;
    QList<ApplicationItem *> m_appItems;
    Launcher *m_launcher = nullptr;
    bool m_pinsInitialized = false;
    bool m_initialWindowsLoaded = false;
    int m_nextAnimationToken = 0;
};

#endif // APPLICATIONMODEL_H
