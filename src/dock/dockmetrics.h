#ifndef DOCKMETRICS_H
#define DOCKMETRICS_H

#include <QVariantMap>
#include <QtMath>

// The dock geometry in one place, so that the window size computed in C++ and
// the layout drawn by QML can never disagree.
//
// Figma baseline, for the default icon size of 54 and a bottom dock:
//
//     +--------------------------------------------------+  <- far side
//     |                    outerPadding 16               |
//     |  16 |  icon 54  | 16 |  icon 54  | 16 |          |  <- lengthPadding
//     |                      dotGap 8                    |
//     |                      dot 4                       |
//     |                      edgePadding 4               |
//     +--------------------------------------------------+  <- screen edge
//
// A left or right dock is the same block, rotated: the running dot always
// stays on the side that faces the screen edge.
struct DockMetrics
{
    int iconSize = 54;
    int iconGap = 16;
    int lengthPadding = 16;
    int outerPadding = 16;
    int dotGap = 8;
    int dotSize = 4;
    int edgePadding = 4;

    static constexpr int baseIconSize = 54;

    static DockMetrics forIconSize(int iconSize)
    {
        const qreal scale = qMax(1, iconSize) / qreal(baseIconSize);
        const auto scaled = [scale](int value) {
            return qMax(1, qRound(value * scale));
        };

        DockMetrics metrics;
        metrics.iconSize = qMax(1, iconSize);
        metrics.iconGap = scaled(16);
        metrics.lengthPadding = scaled(16);
        metrics.outerPadding = scaled(16);
        metrics.dotGap = scaled(8);
        metrics.dotSize = scaled(4);
        metrics.edgePadding = scaled(4);
        return metrics;
    }

    // From the screen edge to the far side of the dock.
    int thickness() const
    {
        return edgePadding + dotSize + dotGap + iconSize + outerPadding;
    }

    // Along the dock: the icons plus the padding at both ends.
    int length(int itemCount) const
    {
        return lengthPadding * 2 + itemCount * iconSize
                + qMax(0, itemCount - 1) * iconGap;
    }

    QVariantMap toVariantMap() const
    {
        return QVariantMap {
            { QStringLiteral("iconSize"), iconSize },
            { QStringLiteral("iconGap"), iconGap },
            { QStringLiteral("lengthPadding"), lengthPadding },
            { QStringLiteral("outerPadding"), outerPadding },
            { QStringLiteral("dotGap"), dotGap },
            { QStringLiteral("dotSize"), dotSize },
            { QStringLiteral("edgePadding"), edgePadding },
            { QStringLiteral("thickness"), thickness() }
        };
    }

    bool operator==(const DockMetrics &other) const
    {
        return iconSize == other.iconSize
                && iconGap == other.iconGap
                && lengthPadding == other.lengthPadding
                && outerPadding == other.outerPadding
                && dotGap == other.dotGap
                && dotSize == other.dotSize
                && edgePadding == other.edgePadding;
    }

    bool operator!=(const DockMetrics &other) const { return !(*this == other); }
};

#endif // DOCKMETRICS_H
