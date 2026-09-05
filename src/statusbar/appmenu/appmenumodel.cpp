#include "appmenumodel.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QMenu>
#include <QQmlEngine>

#include "../libdbusmenuqt/dbusmenuimporter.h"
#include "core/services/activity.h"
#include "core/wayland/kwinwaylandinterface.h"
#include "menuimporter.h"
#include <QDebug>

class CDBusMenuImporter : public DBusMenuImporter
{
public:
    CDBusMenuImporter(const QString &service, const QString &path, QObject *parent)
        : DBusMenuImporter(service, path, parent)
    {
    }

protected:
    QIcon iconForName(const QString &name) override
    {
        return QIcon::fromTheme(name);
    }
};

AppMenuModel::AppMenuModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_menuAvailable(false)
    , m_menu(nullptr)
    , m_activityMenu(nullptr)
    , m_activityAction(nullptr)
    , m_activity(Activity::self())
    , m_serviceWatcher(new QDBusServiceWatcher(this))
{
    m_activityMenu = new QMenu;
    m_activityAction = new QAction(m_activity->title(), this);
    m_activityAction->setMenu(m_activityMenu);
    m_activityMenu->addAction(QCoreApplication::translate("main", "Quit"),
                              m_activity, &Activity::quit);

    connect(this, &AppMenuModel::modelNeedsUpdate, this, [this] {
        if (!m_updatePending) {
            m_updatePending = true;
            QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);
        }
    });

    // On Wayland KWin exposes the active window and its exported application
    // menu together, so no client-side window identifier is needed.
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    connect(kwin, &KWinWaylandInterface::activeWindowChanged, this, &AppMenuModel::onActiveWindowChanged, Qt::QueuedConnection);
    connect(kwin, &KWinWaylandInterface::clientChanged, this, &AppMenuModel::onActiveWindowChanged, Qt::QueuedConnection);
    connect(kwin, &KWinWaylandInterface::clientRemoved, this, &AppMenuModel::onActiveWindowChanged, Qt::QueuedConnection);

    // Compatibility exporters may register after the window is mapped. Their
    // numeric token is not a Wayland window ID, so only use the exporter's
    // D-Bus process ID to associate the menu with KWin's active window.
    if (MenuImporter *importer = MenuImporter::instance()) {
        connect(importer, &MenuImporter::WindowRegistered, this,
                [this](uint, const QString &, const QDBusObjectPath &) { onActiveWindowChanged(); }, Qt::QueuedConnection);
        connect(importer, &MenuImporter::WindowUnregistered, this,
                [this](uint) { onActiveWindowChanged(); }, Qt::QueuedConnection);
    }

    connect(m_activity, &Activity::titleChanged, this,
            &AppMenuModel::updateActivityVisibility);
    onActiveWindowChanged();

    m_serviceWatcher->setConnection(QDBusConnection::sessionBus());
    // if our current DBus connection gets lost, close the menu
    // we'll select the new menu when the focus changes
    connect(m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &serviceName) {
        if (serviceName == m_serviceName) {
            setMenuAvailable(false);
            emit modelNeedsUpdate();
        }
    });
}

AppMenuModel::~AppMenuModel()
{
    if (m_activityAction)
        m_activityAction->setMenu(nullptr);
    delete m_activityMenu;
}

bool AppMenuModel::menuAvailable() const
{
    return m_menuAvailable;
}

void AppMenuModel::setMenuAvailable(bool set)
{
    if (m_menuAvailable != set) {
        m_menuAvailable = set;
        emit menuAvailableChanged();
    }
}

bool AppMenuModel::visible() const
{
    return m_visible;
}

void AppMenuModel::setVisible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        emit visibleChanged();
    }
}

int AppMenuModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    if (!m_visible) {
        return 0;
    }

    return 1 + (m_menuAvailable && m_menu ? m_menu->actions().count() : 0);
}

void AppMenuModel::update()
{
    beginResetModel();
    endResetModel();
    m_updatePending = false;
}

void AppMenuModel::onActiveWindowChanged()
{
    KWinWaylandInterface *kwin = KWinWaylandInterface::instance();
    const KWinWaylandClient client = kwin->client(kwin->activeWindow());

    QString serviceName = client.appMenuServiceName;
    QString objectPath = client.appMenuObjectPath;

    if ((serviceName.isEmpty() || objectPath.isEmpty()) && client.pid > 0) {
        if (MenuImporter *importer = MenuImporter::instance()) {
            serviceName = importer->serviceForPid(static_cast<uint>(client.pid));
            objectPath = importer->pathForPid(static_cast<uint>(client.pid));
        }
    }

    if (!serviceName.isEmpty() && !objectPath.isEmpty()) {
        setMenuAvailable(true);
        updateApplicationMenu(serviceName, objectPath);
    } else {
        setMenuAvailable(false);
    }

    updateActivityVisibility();
    emit modelNeedsUpdate();
}

void AppMenuModel::updateActivityVisibility()
{
    const QString title = m_activity->title();
    const bool visible = !title.isEmpty();
    const bool titleChanged = m_activityAction->text() != title;
    if (titleChanged)
        m_activityAction->setText(title);

    if (m_visible != visible) {
        setVisible(visible);
        emit modelNeedsUpdate();
    } else if (titleChanged && visible) {
        const QModelIndex modelIndex = index(0, 0);
        emit dataChanged(modelIndex, modelIndex);
    }
}

QHash<int, QByteArray> AppMenuModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames[MenuRole] = QByteArrayLiteral("activeMenu");
    roleNames[ActionRole] = QByteArrayLiteral("activeActions");
    return roleNames;
}

QVariant AppMenuModel::data(const QModelIndex &index, int role) const
{
    QAction *action = actionAt(index.row());
    if (!action) {
        return QVariant();
    }

    if (role == MenuRole) { // TODO this should be Qt::DisplayRole
        return action->text();
    } else if (role == ActionRole) {
        return QVariant::fromValue((void *)action);
    }

    return QVariant();
}

QObject *AppMenuModel::menuAt(int row) const
{
    QAction *action = actionAt(row);
    if (!action)
        return nullptr;

    QMenu *menu = action->menu();

    // Both synthetic and imported menus are owned by C++.
    if (menu)
        QQmlEngine::setObjectOwnership(menu, QQmlEngine::CppOwnership);

    return menu;
}

void AppMenuModel::trigger(int row)
{
    QAction *action = actionAt(row);
    if (!action || action->menu())
        return;

    action->trigger();
}

QAction *AppMenuModel::actionAt(int row) const
{
    if (!m_visible || row < 0)
        return nullptr;

    if (row == 0)
        return m_activityAction;

    if (!m_menuAvailable || !m_menu)
        return nullptr;

    const auto actions = m_menu->actions();
    const int actionIndex = row - 1;
    if (actionIndex >= actions.count())
        return nullptr;

    return actions.at(actionIndex);
}

int AppMenuModel::rowForAction(QAction *action) const
{
    if (!action || !m_visible)
        return -1;

    if (action == m_activityAction)
        return 0;

    if (!m_menuAvailable || !m_menu)
        return -1;

    const int actionIndex = m_menu->actions().indexOf(action);
    return actionIndex < 0 ? -1 : actionIndex + 1;
}

void AppMenuModel::updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath)
{
    if (m_serviceName == serviceName && m_menuObjectPath == menuObjectPath) {
        if (m_importer) {
            QMetaObject::invokeMethod(m_importer, "updateMenu", Qt::QueuedConnection);
        }
        return;
    }

    m_serviceName = serviceName;
    m_serviceWatcher->setWatchedServices(QStringList({m_serviceName}));

    m_menuObjectPath = menuObjectPath;

    m_menu = nullptr;

    if (m_importer) {
        m_importer->deleteLater();
    }

    m_importer = new CDBusMenuImporter(serviceName, menuObjectPath, this);
    QMetaObject::invokeMethod(m_importer, "updateMenu", Qt::QueuedConnection);

    connect(m_importer.data(), &DBusMenuImporter::menuUpdated, this, [=](QMenu *menu) {
        m_menu = m_importer->menu();
        if (m_menu.isNull() || menu != m_menu) {
            return;
        }

        // cache first layer of sub menus, which we'll be popping up
        const auto actions = m_menu->actions();
        for (QAction *a : actions) {
            // signal dataChanged when the action changes
            connect(a, &QAction::changed, this, [this, a] {
                const int actionRow = rowForAction(a);
                if (actionRow >= 0) {
                    const QModelIndex modelIdx = index(actionRow, 0);
                    emit dataChanged(modelIdx, modelIdx);
                }
            });

            connect(a, &QAction::destroyed, this, &AppMenuModel::modelNeedsUpdate);

            if (a->menu()) {
                m_importer->updateMenu(a->menu());
            }
        }

        setMenuAvailable(true);
        emit modelNeedsUpdate();
    });

    connect(m_importer.data(), &DBusMenuImporter::actionActivationRequested, this, [this](QAction *action) {
        // TODO submenus
        if (!m_menuAvailable || !m_menu) {
            return;
        }

        const auto actions = m_menu->actions();
        auto it = std::find(actions.begin(), actions.end(), action);
        if (it != actions.end()) {
            Q_EMIT requestActivateIndex((it - actions.begin()) + 1);
        }
    });
}
