#ifndef CUTEFISH_POPUPWINDOW_H
#define CUTEFISH_POPUPWINDOW_H

#include <QQuickWindow>

/**
 * A panel popup: the control centre, the notification centre.
 *
 * A Qt::Popup QQuickWindow declared from the status bar's QML, so it is
 * anchored to the item that opens it and drawn by the shell's shared engine.
 * It grabs the pointer and the keyboard while it is up and closes itself on a
 * click outside or on Escape.
 */
class PopupWindow : public QQuickWindow
{
    Q_OBJECT

public:
    explicit PopupWindow(QQuickWindow *parent = nullptr);

    Q_INVOKABLE void open();

signals:
    //! The window has become visible. Contents that go stale while hidden
    //! (relative timestamps, for one) refresh from here.
    void opened();

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
};

#endif // CUTEFISH_POPUPWINDOW_H
