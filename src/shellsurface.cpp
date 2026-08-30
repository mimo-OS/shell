#include "shellsurface.h"

#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QDebug>

ShellSurface::ShellSurface(QQmlEngine *engine, QWindow *parent)
    : QQuickWindow(parent)
    , m_engine(engine)
    , m_context(new QQmlContext(engine->rootContext(), this))
    , m_component(nullptr)
    , m_rootItem(nullptr)
{
}

ShellSurface::~ShellSurface()
{
    delete m_rootItem;
    m_rootItem = nullptr;
}

QQmlEngine *ShellSurface::engine() const
{
    return m_engine;
}

QQmlContext *ShellSurface::context() const
{
    return m_context;
}

QUrl ShellSurface::source() const
{
    return m_source;
}

QQuickItem *ShellSurface::rootItem() const
{
    return m_rootItem;
}

void ShellSurface::setContextProperty(const QString &name, QObject *value)
{
    m_context->setContextProperty(name, value);
}

void ShellSurface::setContextProperty(const QString &name, const QVariant &value)
{
    m_context->setContextProperty(name, value);
}

void ShellSurface::setSource(const QUrl &url)
{
    if (m_source == url && m_rootItem)
        return;

    m_source = url;

    delete m_rootItem;
    m_rootItem = nullptr;
    delete m_component;

    m_component = new QQmlComponent(m_engine, url, this);

    if (m_component->isError()) {
        qWarning() << "ShellSurface: cannot load" << url << m_component->errors();
        return;
    }

    // beginCreate/completeCreate so the root item is parented and sized before
    // its bindings run; otherwise anchors against the window flicker on startup.
    QObject *object = m_component->beginCreate(m_context);
    m_rootItem = qobject_cast<QQuickItem *>(object);

    if (!m_rootItem) {
        qWarning() << "ShellSurface: the root object of" << url << "is not an Item.";
        delete object;
        return;
    }

    m_rootItem->setParentItem(contentItem());
    resizeRootItem();
    m_component->completeCreate();

    if (m_component->isError())
        qWarning() << "ShellSurface:" << url << m_component->errors();
}

void ShellSurface::resizeRootItem()
{
    if (!m_rootItem)
        return;

    m_rootItem->setPosition(QPointF(0, 0));
    m_rootItem->setSize(QSizeF(width(), height()));
}

void ShellSurface::resizeEvent(QResizeEvent *e)
{
    resizeRootItem();
    QQuickWindow::resizeEvent(e);
}
