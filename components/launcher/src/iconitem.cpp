#include "iconitem.h"
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QQuickWindow>
#include <QApplication>
#include <QIcon>

IconItem::IconItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setSmooth(true);
}

void IconItem::setSource(const QVariant &source)
{
    if (source == m_source)
        return;

    m_source.clear();
    m_source = source;

    // If the QIcon was created with QIcon::fromTheme(), try to load it as svg.
    if (source.canConvert<QIcon>() && source.value<QIcon>().name().isEmpty()) {
        m_source = source.value<QIcon>().name();
    }

    loadPixmap();
    emit sourceChanged();
}

QVariant IconItem::source() const
{
    return m_source;
}

int IconItem::paintedWidth() const
{
    return boundingRect().size().toSize().width();
}

int IconItem::paintedHeight() const
{
    return boundingRect().size().toSize().height();
}

QSGNode *IconItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData);

    if (m_iconPixmap.isNull() || width() == 0.0 || height() == 0.0) {
        delete oldNode;
        return nullptr;
    }

    QSGSimpleTextureNode *textureNode = dynamic_cast<QSGSimpleTextureNode *>(oldNode);

    if (!textureNode) {
        delete oldNode;
        textureNode = new QSGSimpleTextureNode;
        textureNode->setOwnsTexture(true);
        m_pixmapChanged = true;
    }

    // Re-upload on every change: the node outlives the source it was built for.
    if (m_pixmapChanged) {
        textureNode->setTexture(window()->createTextureFromImage(m_iconPixmap.toImage(),
                                                                 QQuickWindow::TextureCanUseAtlas));
        m_pixmapChanged = false;
    }

    textureNode->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);

    // Size changed
    const QSize newSize = QSize(paintedWidth(), paintedHeight());
    const QRect destRect(QPointF(boundingRect().center() - QPointF(newSize.width(), newSize.height()) / 2).toPoint(), newSize);
    textureNode->setRect(destRect);

    return textureNode;
}

void IconItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    polish();
    update();

    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

void IconItem::componentComplete()
{
    QQuickItem::componentComplete();

    polish();
}

void IconItem::updatePolish()
{
    QQuickItem::updatePolish();

    loadPixmap();
}

void IconItem::refresh()
{
    polish();
}

void IconItem::loadPixmap()
{
    if (!isComponentComplete())
        return;

    int size = qMin(qRound(width()), qRound(height()));

    if (size <= 0) {
        // Clear pixmap
        m_iconPixmap = QPixmap();
        m_pixmapChanged = true;
        update();
        return;
    }

    QString sourceString = m_source.toString();

    // Clear first: the theme lookup below only runs on a null pixmap, so a
    // leftover from the previous source would make it a no-op.
    m_iconPixmap = QPixmap();

    if (m_source.canConvert<QIcon>()) {
        QIcon icon = m_source.value<QIcon>();
        m_iconPixmap = icon.pixmap(QSize(size * qApp->devicePixelRatio(),
                                         size * qApp->devicePixelRatio()));
        m_iconPixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    } else if (m_source.canConvert<QImage>()) {
        QImage image = m_source.value<QImage>();
        m_iconPixmap = QPixmap::fromImage(image).scaled(QSize(size * qApp->devicePixelRatio(),
                                                              size * qApp->devicePixelRatio()));
        m_iconPixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    } else if (!m_source.isNull()) {
        QString localFile;

        if (sourceString.startsWith("file:"))
            localFile = QUrl(sourceString).toLocalFile();
        else if (sourceString.startsWith('/'))
            localFile = sourceString;
        else if (sourceString.startsWith("qrc:/"))
            localFile = sourceString.remove(0, 3);
        else if (sourceString.startsWith(":/"))
            localFile = sourceString;

        if (!localFile.isEmpty()) {
            m_iconPixmap.load(localFile);
            if (!m_iconPixmap.isNull()) {
                m_iconPixmap = m_iconPixmap.scaled(QSize(size * qApp->devicePixelRatio(),
                                                         size * qApp->devicePixelRatio()));
                m_iconPixmap.setDevicePixelRatio(qApp->devicePixelRatio());
            }
        }
    }

    if (m_iconPixmap.isNull()) {
        QIcon icon = QIcon::fromTheme(sourceString, QIcon::fromTheme("application-x-desktop"));
        m_iconPixmap = icon.pixmap(QSize(size * qApp->devicePixelRatio(),
                                         size * qApp->devicePixelRatio()));
        m_iconPixmap.setDevicePixelRatio(qApp->devicePixelRatio());
    }

    m_pixmapChanged = true;
    update();
}
