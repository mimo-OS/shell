#include "backgroundhelper.h"
#include "statusbar.h"

#include <QApplication>
#include <QDebug>
#include <QPixmap>
#include <QPixmapCache>
#include <QScreen>
#include <QRgb>

BackgroundHelper::BackgroundHelper(QObject *parent)
    : QObject(parent)
    // Logical pixels, like every other size here: the image is scaled to the
    // screen's logical size before the strip is cut out of it.
    , m_statusBarHeight(StatusBar::Height)
    , m_type(0)
{
    onPrimaryScreenChanged();
    connect(qApp, &QApplication::primaryScreenChanged, this, &BackgroundHelper::onPrimaryScreenChanged);
}

void BackgroundHelper::setColor(QColor c)
{
    m_color = c;
    m_type = 1;

    bool isDark = (c.red() * 0.299 +
                   c.green() * 0.587 +
                   c.blue() * 0.114) < 186;

    emit newColor(c, isDark);
}

void BackgroundHelper::setBackgound(const QString &fileName)
{
    m_wallpaper = fileName;
    m_type = 0;

    // Nothing to sample yet: the wallpaper is set once the theme daemon has
    // told the shell about it.
    if (fileName.isEmpty())
        return;

    QImage img(fileName);
    if (img.isNull())
        return;

    const QSize screenSize = qApp->primaryScreen()->geometry().size();
    if (screenSize.isEmpty())
        return;

    // Only the strip the bar covers is sampled, so cut it out of the source
    // first: scaling the whole wallpaper to the screen and then throwing all
    // but a few rows away costs a full-size resample on every change.
    const int sourceHeight = qBound(1,
                                    qRound(img.height() * qreal(m_statusBarHeight) / screenSize.height()),
                                    img.height());
    img = img.copy(QRect(0, 0, img.width(), sourceHeight))
             .scaled(screenSize.width() * 0.8, m_statusBarHeight);

    // scanLine() is read as QRgb below, which an indexed or grayscale image
    // would not be.
    img.convertTo(QImage::Format_RGB32);

    const QSize size = img.size();
    if (size.isEmpty())
        return;

    long long sumR = 0, sumG = 0, sumB = 0;
    const int measureArea = size.width() * size.height();

    for (int y = 0; y < size.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(img.constScanLine(y));

        for (int x = 0; x < size.width(); ++x) {
            sumR += qRed(line[x]);
            sumG += qGreen(line[x]);
            sumB += qBlue(line[x]);
        }
    }

    sumR /= measureArea;
    sumG /= measureArea;
    sumB /= measureArea;

    QColor c = QColor(sumR, sumG, sumB);
    QColor textColor = (sumR * 0.299 +
                        sumG * 0.587 +
                        sumB * 0.114) > 186 ? "#000000" : "#FFFFFF";

    emit newColor(c, textColor == "#FFFFFF");

    // clear cache.
    QPixmapCache::clear();
}

void BackgroundHelper::onPrimaryScreenChanged()
{
    // QObject::disconnect(receiver) drops this object's *own* signals to that
    // receiver, which is not what has to go here: the connections to drop are
    // the ones from the screen that is no longer the primary one.
    if (m_screen)
        m_screen->disconnect(this);

    m_screen = qApp->primaryScreen();

    if (!m_screen)
        return;

    connect(m_screen, &QScreen::geometryChanged, this, &BackgroundHelper::onChanged);
    connect(m_screen, &QScreen::virtualGeometryChanged, this, &BackgroundHelper::onChanged);
    connect(m_screen, &QScreen::physicalDotsPerInchChanged, this, &BackgroundHelper::onChanged);

    onChanged();
}

void BackgroundHelper::onChanged()
{
    switch (m_type) {
    case 0:
        setBackgound(m_wallpaper);
        break;
    case 1:
        setColor(m_color);
        break;
    default:
        break;
    }
}
