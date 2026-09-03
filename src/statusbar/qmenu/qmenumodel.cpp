#include "qmenumodel.h"

#include <QAction>
#include <QActionEvent>
#include <QMenu>
#include <QQmlEngine>

// "&File" and "Save && Exit" are how a QAction spells its mnemonic; a QML
// label wants neither.
static QString plainText(const QString &text)
{
    QString result;
    result.reserve(text.size());

    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('&')) {
            if (i + 1 < text.size() && text.at(i + 1) == QLatin1Char('&')) {
                result.append(QLatin1Char('&'));
                ++i;
            }
            continue;
        }

        result.append(text.at(i));
    }

    return result;
}

QMenuModel::QMenuModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QObject *QMenuModel::menuObject() const
{
    return m_menu;
}

void QMenuModel::setMenuObject(QObject *menu)
{
    QMenu *newMenu = qobject_cast<QMenu *>(menu);
    if (m_menu == newMenu)
        return;

    if (m_menu)
        m_menu->removeEventFilter(this);

    // The cached submenu models belong to the menu that is being replaced.
    qDeleteAll(m_submenus);
    m_submenus.clear();

    m_menu = newMenu;

    if (m_menu) {
        // QML would otherwise take ownership of a menu that has no parent -
        // the root menu of a tray item is one - and collect it under the
        // importer that is still using it.
        QQmlEngine::setObjectOwnership(m_menu, QQmlEngine::CppOwnership);

        m_menu->installEventFilter(this);
        connect(m_menu, &QObject::destroyed, this, &QMenuModel::scheduleRefresh, Qt::UniqueConnection);
    }

    beginResetModel();
    rebuild();
    endResetModel();

    emit menuChanged();
    emit countChanged();
}

int QMenuModel::count() const
{
    return m_actions.count();
}

int QMenuModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_actions.count();
}

QHash<int, QByteArray> QMenuModel::roleNames() const
{
    return {
        {TextRole, QByteArrayLiteral("text")},
        {IconNameRole, QByteArrayLiteral("iconName")},
        {SeparatorRole, QByteArrayLiteral("isSeparator")},
        {EnabledRole, QByteArrayLiteral("isEnabled")},
        {CheckableRole, QByteArrayLiteral("isCheckable")},
        {CheckedRole, QByteArrayLiteral("isChecked")},
        {HasSubmenuRole, QByteArrayLiteral("hasSubmenu")},
    };
}

QAction *QMenuModel::actionAt(int row) const
{
    if (row < 0 || row >= m_actions.count())
        return nullptr;

    return m_actions.at(row);
}

QVariant QMenuModel::data(const QModelIndex &index, int role) const
{
    QAction *action = actionAt(index.row());
    if (!action)
        return QVariant();

    switch (role) {
    case TextRole:
        return plainText(action->text());
    case IconNameRole:
        return action->icon().name();
    case SeparatorRole:
        return action->isSeparator();
    case EnabledRole:
        return action->isEnabled();
    case CheckableRole:
        return action->isCheckable();
    case CheckedRole:
        return action->isChecked();
    case HasSubmenuRole:
        return action->menu() != nullptr;
    }

    return QVariant();
}

QMenuModel *QMenuModel::submenu(int row)
{
    QAction *action = actionAt(row);
    if (!action || !action->menu())
        return nullptr;

    QMenuModel *model = m_submenus.value(action);
    if (!model) {
        model = new QMenuModel(this);
        model->setMenuObject(action->menu());
        m_submenus.insert(action, model);

        // The cache owns the model; QML must not collect it while the menu
        // it belongs to is still around.
        QQmlEngine::setObjectOwnership(model, QQmlEngine::CppOwnership);
    }

    return model;
}

void QMenuModel::trigger(int row)
{
    QAction *action = actionAt(row);
    if (!action || action->menu())
        return;

    action->trigger();
}

void QMenuModel::aboutToShow()
{
    if (!m_menu)
        return;

    // aboutToShow() is a signal of QMenu, and emitting it is what tells the
    // DBus importer to fetch this menu's contents.
    QMetaObject::invokeMethod(m_menu, "aboutToShow");
}

void QMenuModel::aboutToHide()
{
    if (!m_menu)
        return;

    QMetaObject::invokeMethod(m_menu, "aboutToHide");
}

void QMenuModel::rebuild()
{
    m_actions.clear();
    m_hidden.clear();

    if (!m_menu)
        return;

    const auto actions = m_menu->actions();
    for (QAction *action : actions) {
        connect(action, &QAction::changed, this, &QMenuModel::onActionChanged, Qt::UniqueConnection);
        connect(action, &QObject::destroyed, this, &QMenuModel::onActionDestroyed, Qt::UniqueConnection);

        if (!action->isVisible()) {
            m_hidden.insert(action);
            continue;
        }

        // An application groups its menu with separators and then hides the
        // entries that do not apply, which leaves runs of separators behind -
        // and a menu exported over DBus often ends with one as well. A
        // separator is only worth a line when it sits between two entries.
        if (action->isSeparator() && (m_actions.isEmpty() || m_actions.constLast()->isSeparator()))
            continue;

        m_actions.append(action);
    }

    while (!m_actions.isEmpty() && m_actions.constLast()->isSeparator())
        m_actions.removeLast();
}

void QMenuModel::scheduleRefresh()
{
    if (m_refreshPending)
        return;

    // A menu update arrives as a burst of action events; rebuild once.
    m_refreshPending = true;
    QMetaObject::invokeMethod(this, &QMenuModel::refresh, Qt::QueuedConnection);
}

void QMenuModel::refresh()
{
    m_refreshPending = false;

    beginResetModel();
    rebuild();
    endResetModel();

    emit countChanged();
}

void QMenuModel::onActionChanged()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;

    // An action that appeared or disappeared changes the rows themselves;
    // anything else is a property of a row that is already there. Resetting
    // the model unnecessarily would tear down an open submenu. Absence from
    // the rows is not the test: a separator that was folded away is visible
    // and still has no row of its own.
    if (action->isVisible() == m_hidden.contains(action)) {
        scheduleRefresh();
        return;
    }

    const int row = m_actions.indexOf(action);
    if (row >= 0)
        emit dataChanged(index(row, 0), index(row, 0));
}

// A menu that an application reloads deletes the actions it dropped. The row
// has to go at that moment: a queued refresh would leave the model handing out
// a pointer to an action that is already gone.
void QMenuModel::onActionDestroyed(QObject *object)
{
    QAction *action = static_cast<QAction *>(object);

    delete m_submenus.take(action);
    m_hidden.remove(action);

    const int row = m_actions.indexOf(action);
    if (row < 0)
        return;

    beginRemoveRows(QModelIndex(), row, row);
    m_actions.removeAt(row);
    endRemoveRows();

    emit countChanged();
}

bool QMenuModel::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_menu) {
        switch (event->type()) {
        case QEvent::ActionAdded:
        case QEvent::ActionRemoved:
            scheduleRefresh();
            break;
        default:
            break;
        }
    }

    return QAbstractListModel::eventFilter(watched, event);
}
