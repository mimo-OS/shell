#ifndef QMENUMODEL_H
#define QMENUMODEL_H

#include <QAbstractListModel>
#include <QPointer>
#include <QHash>
#include <QSet>

class QAction;
class QMenu;

// Exposes a QMenu tree - the application menus and the system tray menus both
// arrive as one, built by DBusMenuImporter - to QML, so that they can be shown
// with FishUI's desktop menu instead of a QWidget popup.
//
// Only visible actions become rows. Submenus are separate models, created on
// demand and cached per action, so a menu is only ever as deep as it has been
// opened.
class QMenuModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QObject *menu READ menuObject WRITE setMenuObject NOTIFY menuChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit QMenuModel(QObject *parent = nullptr);

    enum Roles {
        TextRole = Qt::UserRole + 1,
        IconNameRole,
        SeparatorRole,
        EnabledRole,
        CheckableRole,
        CheckedRole,
        HasSubmenuRole,
    };

    QObject *menuObject() const;
    void setMenuObject(QObject *menu);

    int count() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // The model of the submenu at @p row, or nullptr when the row has none.
    Q_INVOKABLE QMenuModel *submenu(int row);

    Q_INVOKABLE void trigger(int row);

    // A DBus menu only fills in its contents once the exporter has been told
    // that the menu is about to be shown; the importer listens for the menu's
    // own aboutToShow()/aboutToHide().
    Q_INVOKABLE void aboutToShow();
    Q_INVOKABLE void aboutToHide();

Q_SIGNALS:
    void menuChanged();
    void countChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QAction *actionAt(int row) const;
    void rebuild();
    void scheduleRefresh();
    void refresh();
    void onActionChanged();
    void onActionDestroyed(QObject *object);

    QPointer<QMenu> m_menu;
    QList<QAction *> m_actions;
    QHash<QAction *, QMenuModel *> m_submenus;
    // The actions that were invisible when the rows were last built, so that
    // a change of visibility can be told from any other change.
    QSet<QAction *> m_hidden;
    bool m_refreshPending = false;
};

#endif // QMENUMODEL_H
