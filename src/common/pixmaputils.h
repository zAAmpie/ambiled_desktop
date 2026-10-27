#pragma once
#include <QPixmap>

class PixmapUtils
{
public:
    enum ArrowDirection { LeftArrow, RightArrow, UpArrow, DownArrow };

    //Draws an arrow on a QPixmap in a specified direction
    static QPixmap &drawArrow(QPixmap& input, ArrowDirection direction, Qt::GlobalColor colour = Qt::yellow, int offset = 0, qreal height = -1.0, qreal width = -1.0, QString text = "");

    //Draws lines at a specific offset on a QPixmap
    static QPixmap& drawOffsetLines(QPixmap& input, Qt::GlobalColor colour, float offsetPercentage = 0.1);
};

