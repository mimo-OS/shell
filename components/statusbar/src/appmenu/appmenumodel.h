#ifndef APPMENUMODEL_H
#define APPMENUMODEL_H

#include <QAbstractListModel>
#include <QPointer>
#include <QRect>
#include <QStringList>

class QMenu;
class QModelIndex;
class QDBusServiceWatcher;
class CDBusMenuImporter;

class AppMenuModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool menuAvailable READ menuAvailable WRITE setMenuAvailable NOTIFY menuAvailableChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)

public:
    explicit AppMenuModel(QObject *parent = nullptr);
    ~AppMenuModel() override;

    enum AppMenuRole {
        MenuRole = Qt::UserRole + 1, // TODO this should be Qt::DisplayRole
        ActionRole,
    };

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    void updateApplicationMenu(const QString &serviceName, const QString &menuObjectPath);

    bool menuAvailable() const;
    void setMenuAvailable(bool set);

    bool visible() const;

Q_SIGNALS:
    void requestActivateIndex(int index);

private Q_SLOTS:
    void onActiveWindowChanged();
    void setVisible(bool visible);
    void update();

Q_SIGNALS:
    void menuAvailableChanged();
    void modelNeedsUpdate();
    void visibleChanged();

private:
    bool m_menuAvailable;
    bool m_updatePending = false;
    bool m_visible = true;

    QPointer<QMenu> m_menu;

    QDBusServiceWatcher *m_serviceWatcher;
    QString m_serviceName;
    QString m_menuObjectPath;

    QPointer<CDBusMenuImporter> m_importer;
};

#endif
