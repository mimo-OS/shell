#include "verticalmenu.h"

#include <QCoreApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>

VerticalMenu::VerticalMenu(QWidget *parent)
    : QMenu(parent)
{
}

VerticalMenu::~VerticalMenu()
{
}

QMenu *VerticalMenu::leafMenu()
{
    QMenu *leaf = this;
    while (true) {
        QAction *act = leaf->activeAction();
        if (act && act->menu() && act->menu()->isVisible()) {
            leaf = act->menu();
            continue;
        }
        return leaf == this ? nullptr : leaf;
    }
    return nullptr; // make gcc happy
}

void VerticalMenu::paintEvent(QPaintEvent *pe)
{
    QMenu::paintEvent(pe);
    if (QWidget::mouseGrabber() == this)
        return;
    if (QWidget::mouseGrabber())
        QWidget::mouseGrabber()->releaseMouse();
    grabMouse();
    grabKeyboard();
}

#define FORWARD(_EVENT_, _TYPE_)                                                                                                                               \
    void VerticalMenu::_EVENT_##Event(Q##_TYPE_##Event *e)                                                                                                     \
    {                                                                                                                                                          \
        if (QMenu *leaf = leafMenu())                                                                                                                          \
            QCoreApplication::sendEvent(leaf, e);                                                                                                              \
        else                                                                                                                                                   \
            QMenu::_EVENT_##Event(e);                                                                                                                          \
    }

FORWARD(keyPress, Key)
FORWARD(keyRelease, Key)
