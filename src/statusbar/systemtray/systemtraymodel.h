#ifndef SYSTEMTRAYMODEL_H
#define SYSTEMTRAYMODEL_H

#include <QAbstractListModel>

#include <QQuickItem>
#include <QQuickWindow>

#include "statusnotifierwatcher.h"
#include "statusnotifieritemhost.h"
#include "statusnotifieritemsource.h"

class SystemTrayModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        IconNameRole,
        IconRole,
        TitleRole,
        ToolTipRole,
        CanColorOverlay
    };

    explicit SystemTrayModel(QObject *parent = nullptr);
    ~SystemTrayModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    int indexOf(const QString &id);
    StatusNotifierItemSource *findItemById(const QString &id);

    Q_INVOKABLE void leftButtonClick(const QString &id, int x, int y);
    // The menu of the tray item, for QMenuModel to show. Returns nullptr for
    // an item that exports no menu; such an item is asked over DBus to show
    // its own instead.
    Q_INVOKABLE QObject *contextMenu(const QString &id, int x, int y);
    Q_INVOKABLE void middleButtonClick(const QString &id, int x, int y);

    Q_INVOKABLE void move(int from, int to);

    /**
     * Find out global coordinates for a popup given local MouseArea
     * coordinates
     * from plasma-workspace/applets/systemtray/systemtray.cpp
     */
    Q_INVOKABLE QPointF popupPosition(QQuickItem *visualParent, int x, int y);

private slots:
    void onItemAdded(const QString &service);
    void onItemRemoved(const QString &service);
    void updated(StatusNotifierItemSource *item);

private:
    StatusNotifierWatcher *m_watcher;
    StatusNotifierItemHost *m_sniHost;
    QList<StatusNotifierItemSource *> m_items;
    QString m_hostName;
};

#endif // SYSTEMTRAYMODEL_H
