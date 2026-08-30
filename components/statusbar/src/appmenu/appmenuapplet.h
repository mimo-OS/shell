#ifndef APPMENUAPPLET_H
#define APPMENUAPPLET_H

#include <QObject>
#include <QQuickItem>
#include <QPointer>
#include <QMenu>

#include "appmenumodel.h"

class AppMenuApplet : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AppMenuModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QQuickItem *buttonGrid READ buttonGrid WRITE setButtonGrid NOTIFY buttonGridChanged)

public:
    explicit AppMenuApplet(QObject *parent = nullptr);

    int currentIndex() const;

    QQuickItem *buttonGrid() const;
    void setButtonGrid(QQuickItem *buttonGrid);

    AppMenuModel *model() const;
    void setModel(AppMenuModel *model);

Q_SIGNALS:
    void modelChanged();
    void viewChanged();
    void currentIndexChanged();
    void buttonGridChanged();
    void requestActivateIndex(int index);
    void mousePosChanged(int x, int y);

public slots:
    Q_INVOKABLE void trigger(QQuickItem *ctx, int idx);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QMenu *createMenu(int idx) const;
    void setCurrentIndex(int currentIndex);
    void onMenuAboutToHide();

    int m_currentIndex = -1;
    QPointer<QMenu> m_currentMenu;
    QPointer<QQuickItem> m_buttonGrid;
    QPointer<AppMenuModel> m_model;
    static int s_refs;
};

#endif // APPMENUAPPLET_H
