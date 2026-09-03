#include "core/window/popupwindow.h"

#include <QKeyEvent>
#include <QMouseEvent>

PopupWindow::PopupWindow(QQuickWindow *parent)
    : QQuickWindow(parent)
{
    setFlags(Qt::Popup);
    setColor(Qt::transparent);
    installEventFilter(this);
}

void PopupWindow::open()
{
    setVisible(true);
    setMouseGrabEnabled(true);
    setKeyboardGrabEnabled(true);
}

bool PopupWindow::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        if (QWindow *window = qobject_cast<QWindow *>(object)) {
            const QPoint position = static_cast<QMouseEvent *>(event)->globalPosition().toPoint();
            if (!window->geometry().contains(position))
                setVisible(false);
        }
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape)
            setVisible(false);
        break;
    case QEvent::Show:
        emit opened();
        break;
    case QEvent::Hide:
        setMouseGrabEnabled(false);
        setKeyboardGrabEnabled(false);
        break;
    default:
        break;
    }

    return QQuickWindow::eventFilter(object, event);
}
