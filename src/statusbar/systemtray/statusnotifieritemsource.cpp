#include "statusnotifieritemsource.h"
#include "systemtraytypes.h"

#include "../libdbusmenuqt/dbusmenuimporter.h"
#include "applicationregistry.h"
#include "core/services/applicationresolver.h"
#include "desktopentry.h"

#include <QDBusConnectionInterface>
#include <QDBusReply>
#include <QDebug>
#include <QDir>
#include <netinet/in.h>

class TrayMenuImporter : public DBusMenuImporter
{
public:
    using DBusMenuImporter::DBusMenuImporter;

protected:
    QIcon iconForName(const QString & name) override {
        return QIcon::fromTheme(name);
    }
};

StatusNotifierItemSource::StatusNotifierItemSource(const QString &notifierItemId, QObject *parent)
    : QObject(parent)
    , m_menuImporter(nullptr)
    , m_refreshing(false)
    , m_needsReRefreshing(false)
    , m_titleUpdate(true)
    , m_iconUpdate(true)
    , m_tooltipUpdate(true)
    , m_statusUpdate(true)
    , m_id(notifierItemId)
{
    setObjectName(notifierItemId);

    qDBusRegisterMetaType<KDbusImageStruct>();
    qDBusRegisterMetaType<KDbusImageVector>();
    qDBusRegisterMetaType<KDbusToolTipStruct>();

    m_name = notifierItemId;

    int slash = notifierItemId.indexOf('/');
    if (slash == -1) {
        qWarning() << "Invalid notifierItemId:" << notifierItemId;
        m_valid = false;
        m_statusNotifierItemInterface = nullptr;
        return;
    }

    QString service = notifierItemId.left(slash);
    QString path = notifierItemId.mid(slash);

    m_statusNotifierItemInterface = new org::kde::StatusNotifierItem(service, path, QDBusConnection::sessionBus(), this);

    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(10);
    connect(&m_refreshTimer, &QTimer::timeout, this, &StatusNotifierItemSource::performRefresh);

    connect(ApplicationRegistry::instance(), &ApplicationRegistry::applicationsChanged,
            this, [this]() {
        if (m_iconName.isEmpty() && m_icon.isNull())
            refresh();
    });

    m_valid = !service.isEmpty() && m_statusNotifierItemInterface->isValid();

    if (m_valid) {
        connect(m_statusNotifierItemInterface, &OrgKdeStatusNotifierItem::NewTitle, this, &StatusNotifierItemSource::refreshTitle);
        connect(m_statusNotifierItemInterface, &OrgKdeStatusNotifierItem::NewIcon, this, &StatusNotifierItemSource::refreshIcons);
        connect(m_statusNotifierItemInterface, &OrgKdeStatusNotifierItem::NewAttentionIcon, this, &StatusNotifierItemSource::refreshIcons);
        connect(m_statusNotifierItemInterface, &OrgKdeStatusNotifierItem::NewOverlayIcon, this, &StatusNotifierItemSource::refreshIcons);
        connect(m_statusNotifierItemInterface, &OrgKdeStatusNotifierItem::NewToolTip, this, &StatusNotifierItemSource::refreshToolTip);
        connect(m_statusNotifierItemInterface, &OrgKdeStatusNotifierItem::NewStatus, this, &StatusNotifierItemSource::syncStatus);
        refresh();
    }
}

StatusNotifierItemSource::~StatusNotifierItemSource()
{
    if (m_statusNotifierItemInterface)
        delete m_statusNotifierItemInterface;
}

QString StatusNotifierItemSource::id() const
{
    return m_id;
}

QString StatusNotifierItemSource::appId() const
{
    return m_appId;
}

QString StatusNotifierItemSource::title() const
{
    return m_title;
}

QString StatusNotifierItemSource::tooltip() const
{
    return m_tooltip;
}

QString StatusNotifierItemSource::subtitle() const
{
    return m_subTitle;
}

QString StatusNotifierItemSource::iconName() const
{
    return m_iconName;
}

QIcon StatusNotifierItemSource::icon() const
{
    return m_icon;
}

QIcon StatusNotifierItemSource::applicationIcon() const
{
    const int slash = m_id.indexOf(QLatin1Char('/'));
    if (slash == -1)
        return {};

    const QString service = m_id.left(slash);
    const QDBusReply<uint> pidReply = QDBusConnection::sessionBus().interface()->servicePid(service);
    if (!pidReply.isValid())
        return {};

    DesktopEntry *entry = ApplicationResolver::instance()->resolve(m_appId,
                                                                    pidReply.value(),
                                                                    m_appId);
    if (!entry || entry->icon().isEmpty())
        return {};

    const QString iconName = entry->icon();
    if (QDir::isAbsolutePath(iconName))
        return QIcon(iconName);

    QIcon icon = QIcon::fromTheme(iconName);
    if (icon.isNull() && !iconName.endsWith(QStringLiteral("-symbolic")))
        icon = QIcon::fromTheme(iconName + QStringLiteral("-symbolic"));

    return icon;
}

void StatusNotifierItemSource::activate(int x, int y)
{
    if (m_statusNotifierItemInterface && m_statusNotifierItemInterface->isValid()) {
        QDBusMessage message = QDBusMessage::createMethodCall(m_statusNotifierItemInterface->service(),
                                                              m_statusNotifierItemInterface->path(),
                                                              m_statusNotifierItemInterface->interface(),
                                                              QStringLiteral("Activate"));

        message << x << y;
        QDBusPendingCall call = m_statusNotifierItemInterface->connection().asyncCall(message);
        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, &StatusNotifierItemSource::activateCallback);
    }
}

void StatusNotifierItemSource::secondaryActivate(int x, int y)
{
    if (m_statusNotifierItemInterface && m_statusNotifierItemInterface->isValid()) {
        m_statusNotifierItemInterface->call(QDBus::NoBlock, QStringLiteral("SecondaryActivate"), x, y);
    }
}

void StatusNotifierItemSource::scroll(int delta, const QString &direction)
{
    if (m_statusNotifierItemInterface && m_statusNotifierItemInterface->isValid()) {
        m_statusNotifierItemInterface->call(QDBus::NoBlock, QStringLiteral("Scroll"), delta, direction);
    }
}

QMenu *StatusNotifierItemSource::contextMenuObject()
{
    if (!m_menuImporter)
        return nullptr;

    QMenu *menu = m_menuImporter->menu();
    if (!menu)
        return nullptr;

    m_menuImporter->updateMenu();

    return menu;
}

void StatusNotifierItemSource::requestContextMenu(int x, int y)
{
    qWarning() << "Could not find DBusMenu interface, falling back to calling ContextMenu()";

    if (m_statusNotifierItemInterface && m_statusNotifierItemInterface->isValid()) {
        m_statusNotifierItemInterface->call(QDBus::NoBlock, QStringLiteral("ContextMenu"), x, y);
    }
}

void StatusNotifierItemSource::contextMenuReady()
{
    if (m_menuImporter && m_menuImporter->menu()) {
        emit contextMenuReady(m_menuImporter->menu());
    }
}

void StatusNotifierItemSource::refreshTitle()
{
    m_titleUpdate = true;
    refresh();
}

void StatusNotifierItemSource::refreshIcons()
{
    m_iconUpdate = true;
    refresh();
}

void StatusNotifierItemSource::refreshToolTip()
{
    m_tooltipUpdate = true;
    refresh();
}

void StatusNotifierItemSource::refresh()
{
    if (!m_refreshTimer.isActive()) {
        m_refreshTimer.start();
    }
}

void StatusNotifierItemSource::performRefresh()
{
    if (m_refreshing) {
        m_needsReRefreshing = true;
        return;
    }

    m_refreshing = true;
    QDBusMessage message = QDBusMessage::createMethodCall(m_statusNotifierItemInterface->service(),
                                                          m_statusNotifierItemInterface->path(),
                                                          QStringLiteral("org.freedesktop.DBus.Properties"),
                                                          QStringLiteral("GetAll"));

    message << m_statusNotifierItemInterface->interface();
    QDBusPendingCall call = m_statusNotifierItemInterface->connection().asyncCall(message);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &StatusNotifierItemSource::refreshCallback);
}

void StatusNotifierItemSource::syncStatus(QString)
{

}

void StatusNotifierItemSource::refreshCallback(QDBusPendingCallWatcher *call)
{
    m_refreshing = false;
    if (m_needsReRefreshing) {
        m_needsReRefreshing = false;
        performRefresh();
        call->deleteLater();
        return;
    }

    QDBusPendingReply<QVariantMap> reply = *call;
    if (reply.isError()) {
        m_valid = false;
    } else {
        QVariantMap properties = reply.argumentAt<0>();
        const QString iconThemePath = properties[QStringLiteral("IconThemePath")].toString();

        if (!iconThemePath.isEmpty()) {
            const QString path = QDir::cleanPath(iconThemePath);
            QStringList fallbackSearchPaths = QIcon::fallbackSearchPaths();
            if (!fallbackSearchPaths.contains(path)) {
                fallbackSearchPaths.append(path);
                QIcon::setFallbackSearchPaths(fallbackSearchPaths);
            }
        }

        m_title = properties[QStringLiteral("Title")].toString();
        m_appId = properties[QStringLiteral("Id")].toString();

        const QString iconName = properties[QStringLiteral("IconName")].toString();
        m_iconName.clear();
        m_icon = QIcon();

        auto loadIconByName = [](const QString &name) {
            if (name.isEmpty())
                return QString();

            QIcon icon = QIcon::fromTheme(name);
            if (icon.isNull() && !name.endsWith(QStringLiteral("-symbolic"))) {
                const QString symbolicName = name + QStringLiteral("-symbolic");
                icon = QIcon::fromTheme(symbolicName);
                if (!icon.isNull())
                    return symbolicName;
            }

            if (!icon.isNull())
                return name;

            return QString();
        };

        m_iconName = loadIconByName(iconName);
        if (m_iconName.isEmpty())
            m_iconName = loadIconByName(m_appId);

        // ToolTip
        KDbusToolTipStruct toolTip;
        properties[QStringLiteral("ToolTip")].value<QDBusArgument>() >> toolTip;
        m_tooltip = toolTip.title;
        m_subTitle = toolTip.subTitle;

        // Icon
        KDbusImageVector image;
        properties[QStringLiteral("IconPixmap")].value<QDBusArgument>() >> image;
        if (m_icon.isNull() && !image.isEmpty()) {
            m_icon = imageVectorToPixmap(image);
        }

        // Some tray clients do not expose a usable SNI icon. Try the icon
        // from the matching desktop entry before presenting a generic one.
        if (m_icon.isNull())
            m_icon = applicationIcon();

        // Menu
        if (!m_menuImporter) {
            QString menuObjectPath = properties[QStringLiteral("Menu")].value<QDBusObjectPath>().path();
            if (!menuObjectPath.isEmpty()) {
                if (menuObjectPath.startsWith(QLatin1String("/NO_DBUSMENU"))) {
                    // This is a hack to make it possible to disable DBusMenu in an
                    // application. The string "/NO_DBUSMENU" must be the same as in
                    // KStatusNotifierItem::setContextMenu().
                    qWarning() << "DBusMenu disabled for this application";
                } else {
                    m_menuImporter = new TrayMenuImporter(m_statusNotifierItemInterface->service(),
                                                      menuObjectPath, this);
                    connect(m_menuImporter, &TrayMenuImporter::menuUpdated, this, [this](QMenu *menu) {
                        if (menu == m_menuImporter->menu()) {
                            contextMenuReady();
                        }
                    });
                }
            }
        }

        // qDebug() << newTitle << newIconName << newToolTip << image.isEmpty();

        emit updated(this);
    }

    call->deleteLater();
}

void StatusNotifierItemSource::activateCallback(QDBusPendingCallWatcher *call)
{
    QDBusPendingReply<void> reply = *call;
    emit activateResult(!reply.isError());
    call->deleteLater();
}

QPixmap StatusNotifierItemSource::KDbusImageStructToPixmap(const KDbusImageStruct &image) const
{
    // swap from network byte order if we are little endian
    if (QSysInfo::ByteOrder == QSysInfo::LittleEndian) {
        uint *uintBuf = (uint *)image.data.data();
        for (uint i = 0; i < image.data.size() / sizeof(uint); ++i) {
            *uintBuf = ntohl(*uintBuf);
            ++uintBuf;
        }
    }
    if (image.width == 0 || image.height == 0) {
        return QPixmap();
    }

    // avoid a deep copy of the image data
    // we need to keep a reference to the image.data alive for the lifespan of the image, even if the image is copied
    // we create a new QByteArray with a shallow copy of the original data on the heap, then delete this in the QImage cleanup
    auto dataRef = new QByteArray(image.data);

    QImage iconImage(
        reinterpret_cast<const uchar *>(dataRef->data()),
        image.width,
        image.height,
        QImage::Format_ARGB32,
        [](void *ptr) {
            delete static_cast<QByteArray *>(ptr);
        },
        dataRef);
    return QPixmap::fromImage(iconImage);
}

QIcon StatusNotifierItemSource::imageVectorToPixmap(const KDbusImageVector &vector) const
{
    QIcon icon;

    for (int i = 0; i < vector.size(); ++i) {
        icon.addPixmap(KDbusImageStructToPixmap(vector[i]));
    }

    return icon;
}
