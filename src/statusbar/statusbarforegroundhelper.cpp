#include "statusbarforegroundhelper.h"
#include "statusbar.h"

#include <QApplication>
#include <QFutureWatcher>
#include <QImage>
#include <QImageReader>
#include <QScreen>
#include <QTimer>
#include <QRgb>
#include <QtMath>

#include <QtConcurrent/QtConcurrentRun>

namespace {

// {sampled, mean relative luminance}
using SampleResult = QPair<bool, double>;

constexpr int SampleMaxDimension = 256;
// Mirrors the ColorOverlay opacity in desktop/qml/Wallpaper.qml.
constexpr qreal WallpaperDimOpacity = 0.4;

double srgbToLinear(double value)
{
    return value <= 0.04045 ? value / 12.92
                            : qPow((value + 0.055) / 1.055, 2.4);
}

double relativeLuminance(int red, int green, int blue)
{
    return 0.2126 * srgbToLinear(red / 255.0)
         + 0.7152 * srgbToLinear(green / 255.0)
         + 0.0722 * srgbToLinear(blue / 255.0);
}

bool preferLightForeground(double luminance)
{
    const double blackContrast = (luminance + 0.05) / 0.05;
    const double whiteContrast = 1.05 / (luminance + 0.05);
    return whiteContrast >= blackContrast;
}

SampleResult sampleWallpaper(const QString &fileName,
                             const QSize &screenSize,
                             int statusBarHeight,
                             bool dimmed)
{
    if (fileName.isEmpty() || screenSize.isEmpty())
        return {false, 0.0};

    QImageReader reader(fileName);
    reader.setAutoTransform(true);

    QSize scaledSize = reader.size();
    if (scaledSize.isEmpty())
        return {false, 0.0};

    // Only ever scale down: QSize::scaled would blow a small wallpaper up to
    // the bound and then charge us for reading every invented pixel.
    if (scaledSize.width() > SampleMaxDimension || scaledSize.height() > SampleMaxDimension)
        scaledSize.scale(SampleMaxDimension, SampleMaxDimension, Qt::KeepAspectRatio);

    reader.setScaledSize(scaledSize);

    QImage image = reader.read();
    if (image.isNull())
        return {false, 0.0};

    // Keep this transform in sync with the desktop's centered PreserveAspectCrop.
    const qreal scale = qMax(qreal(screenSize.width()) / image.width(),
                             qreal(screenSize.height()) / image.height());
    const qreal visibleWidth = screenSize.width() / scale;
    const qreal visibleHeight = screenSize.height() / scale;
    const qreal visibleBarHeight = qMin(statusBarHeight, screenSize.height()) / scale;

    const QRectF sourceRect((image.width() - visibleWidth) / 2.0,
                            (image.height() - visibleHeight) / 2.0,
                            visibleWidth,
                            visibleBarHeight);
    const QRect sampleRect = sourceRect.toAlignedRect().intersected(image.rect());
    if (sampleRect.isEmpty())
        return {false, 0.0};

    // scanLine() is read as QRgb below, which an indexed or grayscale image
    // would not be.
    image = image.copy(sampleRect).convertToFormat(QImage::Format_RGB32);

    qint64 redSum = 0, greenSum = 0, blueSum = 0;
    qint64 pixelCount = 0;

    for (int y = 0; y < image.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));

        for (int x = 0; x < image.width(); ++x) {
            redSum += qRed(line[x]);
            greenSum += qGreen(line[x]);
            blueSum += qBlue(line[x]);
            ++pixelCount;
        }
    }

    if (pixelCount == 0)
        return {false, 0.0};

    // Average the colour and judge that one colour. Averaging per-pixel
    // luminance instead lets a few bright pixels outvote the whole strip:
    // a mid purple bar with a bright corner reads as a light background.
    const qreal brightnessFactor = (dimmed ? 1.0 - WallpaperDimOpacity : 1.0) / pixelCount;
    return {true, relativeLuminance(qRound(redSum * brightnessFactor),
                                    qRound(greenSum * brightnessFactor),
                                    qRound(blueSum * brightnessFactor))};
}

} // namespace

StatusBarForegroundHelper::StatusBarForegroundHelper(QObject *parent)
    : QObject(parent)
    , m_sampleTimer(new QTimer(this))
    // Logical pixels, like every other size here.
    , m_statusBarHeight(StatusBar::Height)
    , m_type(Type::Unavailable)
    , m_dimmed(false)
    , m_requestSerial(0)
    , m_valid(false)
    , m_useLightForeground(false)
{
    // A burst of property changes from the appearance service reaches us as
    // several reload() calls; only the last one is worth sampling.
    m_sampleTimer->setSingleShot(true);
    m_sampleTimer->setInterval(0);
    connect(m_sampleTimer, &QTimer::timeout, this, &StatusBarForegroundHelper::startWallpaperSampling);

    onPrimaryScreenChanged();
    connect(qApp, &QApplication::primaryScreenChanged, this, &StatusBarForegroundHelper::onPrimaryScreenChanged);
}

bool StatusBarForegroundHelper::isValid() const
{
    return m_valid;
}

bool StatusBarForegroundHelper::useLightForeground() const
{
    return m_useLightForeground;
}

void StatusBarForegroundHelper::setColor(const QColor &color)
{
    ++m_requestSerial;
    m_sampleTimer->stop();
    m_color = color;
    m_type = Type::Color;

    if (!color.isValid()) {
        setForeground(false, false);
        return;
    }

    setForeground(true, preferLightForeground(relativeLuminance(color.red(),
                                                                color.green(),
                                                                color.blue())));
}

void StatusBarForegroundHelper::setBackground(const QString &fileName, bool dimmed)
{
    ++m_requestSerial;
    m_sampleTimer->stop();
    m_wallpaper = fileName;
    m_dimmed = dimmed;
    m_type = Type::Wallpaper;

    if (fileName.isEmpty() || !m_screen) {
        setForeground(false, false);
        return;
    }

    m_sampleTimer->start();
}

void StatusBarForegroundHelper::setUnavailable()
{
    ++m_requestSerial;
    m_sampleTimer->stop();
    m_type = Type::Unavailable;
    m_wallpaper.clear();
    m_color = QColor();
    m_dimmed = false;
    setForeground(false, false);
}

void StatusBarForegroundHelper::setForeground(bool valid, bool useLight)
{
    if (m_valid == valid && m_useLightForeground == useLight)
        return;

    m_valid = valid;
    m_useLightForeground = useLight;
    emit foregroundChanged();
}

void StatusBarForegroundHelper::startWallpaperSampling()
{
    if (m_type != Type::Wallpaper || m_wallpaper.isEmpty() || !m_screen)
        return;

    const QString fileName = m_wallpaper;
    const QSize screenSize = m_screen->geometry().size();
    const bool dimmed = m_dimmed;
    const quint64 requestSerial = m_requestSerial;

    auto *watcher = new QFutureWatcher<SampleResult>(this);
    connect(watcher, &QFutureWatcher<SampleResult>::finished, this,
            [this, watcher, requestSerial] {
                const SampleResult result = watcher->result();
                watcher->deleteLater();

                // A newer request has replaced this one while it ran.
                if (requestSerial != m_requestSerial)
                    return;

                setForeground(result.first,
                              result.first && preferLightForeground(result.second));
            });

    watcher->setFuture(QtConcurrent::run(sampleWallpaper,
                                         fileName,
                                         screenSize,
                                         m_statusBarHeight,
                                         dimmed));
}

void StatusBarForegroundHelper::onPrimaryScreenChanged()
{
    // QObject::disconnect(receiver) drops this object's *own* signals to that
    // receiver, which is not what has to go here: the connections to drop are
    // the ones from the screen that is no longer the primary one.
    if (m_screen)
        m_screen->disconnect(this);

    m_screen = qApp->primaryScreen();

    if (!m_screen) {
        ++m_requestSerial;
        m_sampleTimer->stop();
        if (m_type == Type::Wallpaper)
            setForeground(false, false);
        return;
    }

    connect(m_screen, &QScreen::geometryChanged, this, &StatusBarForegroundHelper::onScreenChanged);
    connect(m_screen, &QScreen::virtualGeometryChanged, this, &StatusBarForegroundHelper::onScreenChanged);
    connect(m_screen, &QScreen::physicalDotsPerInchChanged, this, &StatusBarForegroundHelper::onScreenChanged);

    onScreenChanged();
}

void StatusBarForegroundHelper::onScreenChanged()
{
    // Only the wallpaper crop depends on the screen; a plain colour does not.
    if (m_type == Type::Wallpaper)
        setBackground(m_wallpaper, m_dimmed);
}
