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
    int iconSize = 60;
    int iconGap = 12;
    int lengthPadding = 14;
    int outerPadding = 16;
    int dotGap = 6;
    int dotSize = 4;
    int edgePadding = 5;

    static constexpr int baseIconSize = 60;
    static constexpr int dividerSlotWidth = 14;

    static DockMetrics forIconSize(int iconSize)
    {
        const qreal scale = qMax(1, iconSize) / qreal(baseIconSize);
        const auto scaled = [scale](int value) {
            return qMax(1, qRound(value * scale));
        };

        DockMetrics metrics;
        metrics.iconSize = qMax(1, iconSize);
        metrics.iconGap = scaled(12);
        metrics.lengthPadding = scaled(14);
        metrics.outerPadding = scaled(16);
        metrics.dotGap = scaled(6);
        metrics.dotSize = scaled(4);
        metrics.edgePadding = scaled(5);
        return metrics;
    }

    // From the screen edge to the far side of the dock.
    int thickness() const
    {
        return edgePadding + dotSize + dotGap + iconSize + outerPadding;
    }

    // Along the dock: the icons plus the padding at both ends and the divider.
    int length(int itemCount) const
    {
        return lengthPadding * 2 + itemCount * iconSize
                + itemCount * iconGap + dividerSlotWidth;
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
