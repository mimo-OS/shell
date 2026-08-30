#ifndef SYSTEMTRAYTYPEDEFS_H
#define SYSTEMTRAYTYPEDEFS_H

#include <QByteArray>
#include <QMetaType>
#include <QString>
#include <QVector>

struct KDbusImageStruct {
    int width;
    int height;
    QByteArray data;
};

typedef QVector<KDbusImageStruct> KDbusImageVector;

struct KDbusToolTipStruct {
    QString icon;
    KDbusImageVector image;
    QString title;
    QString subTitle;
};

Q_DECLARE_METATYPE(KDbusImageStruct)
Q_DECLARE_METATYPE(KDbusImageVector)
Q_DECLARE_METATYPE(KDbusToolTipStruct)

#endif