#include "pixmaputils.h"
#include <QPainter>

//Draws an arrow on a QPixmap in a specified direction
QPixmap& PixmapUtils::drawArrow(QPixmap& input, ArrowDirection direction, Qt::GlobalColor colour, int offset, qreal height, qreal width, QString text)
{
    qreal inHeight = input.height();
    qreal inWidth = input.width();

    //Clamp input
    qreal requestedHeight = qMax(0.0f, qMin(static_cast<qreal>(height), inHeight));
    qreal requestedWidth = qMax(0.0f, qMin(static_cast<qreal>(width), inWidth));

    //Early out if we're not going to be drawing anything
    if (requestedHeight == 0 || requestedWidth == 0)
        return input;

    //Paint arrow body
    QPainter painter(&input);
    QPen pen;
    pen.setColor(Qt::black);
    QBrush brush(Qt::SolidPattern);
    brush.setColor(colour);

    qreal halfHeight = inHeight / 2.0;
    qreal halfWidth = inWidth / 2.0;
    qreal headHeight;
    qreal headWidth;
    qreal arrowWidth;

    QPolygonF arrow;

    //TODO: Make this configurable
    pen.setWidth(1);
    painter.setPen(pen);
    painter.setBrush(brush);

    QTextOption textOption;
    textOption.setAlignment(Qt::AlignCenter);

    QFont font;
    //TODO: Make this configurable
    font.setFamily("Verdana");
    font.setPointSizeF(6);
    painter.setFont(font);

    switch (direction)
    {
    case LeftArrow:
        /*
        (0,0)/|
           /  -------------
        <                 |
           \  -------------
             \|

        Height is 50% of total image height
        Positive offset moves arrow to right
        */

        headHeight = requestedHeight / 2.0;
        headWidth = qMin(requestedHeight / 2.0, inWidth / 2.0);
        arrowWidth = requestedHeight / 4.0;

        //Polygon for above
        arrow.append(QPointF(offset, halfHeight));
        arrow.append(QPointF(offset + headWidth, halfHeight - headHeight / 2.0));
        arrow.append(QPointF(offset + headWidth, halfHeight - arrowWidth / 2.0));
        arrow.append(QPointF(offset + requestedWidth, halfHeight - arrowWidth / 2.0));
        arrow.append(QPointF(offset + requestedWidth, halfHeight + arrowWidth / 2.0));
        arrow.append(QPointF(offset + headWidth, halfHeight + arrowWidth / 2.0));
        arrow.append(QPointF(offset + headWidth, halfHeight + headHeight / 2.0));

        painter.drawConvexPolygon(arrow);

        if (text != "")
        {
            //Draw text on top of the arrow
            painter.drawText(QRectF(offset, 0, requestedWidth, requestedHeight), text, textOption);
        }

        break;
    case RightArrow:
        /*
        (0,0)        |\
         -------------  \
        |                 >
         -------------  /
                     |/

        Height is 50% of total image height
        Positive offset moves arrow to left
        */
        headHeight = requestedHeight / 2.0;
        headWidth = qMin(requestedHeight / 2.0, inWidth / 2.0);
        arrowWidth = requestedHeight / 4.0;

        //Polygon for above
        arrow.append(QPointF(inWidth - offset, halfHeight));
        arrow.append(QPointF(inWidth - headWidth - offset, halfHeight - headHeight / 2.0));
        arrow.append(QPointF(inWidth - headWidth - offset, halfHeight - arrowWidth / 2.0));
        arrow.append(QPointF(inWidth - requestedWidth - offset, halfHeight - arrowWidth / 2.0));
        arrow.append(QPointF(inWidth - requestedWidth - offset, halfHeight + arrowWidth / 2.0));
        arrow.append(QPointF(inWidth - headWidth - offset, halfHeight + arrowWidth / 2.0));
        arrow.append(QPointF(inWidth - headWidth - offset, halfHeight + headHeight / 2.0));

        painter.drawConvexPolygon(arrow);

        if (text != "")
        {
            //Draw text on top of the arrow
            painter.drawText(QRectF(inWidth - requestedWidth - offset, 0, requestedWidth, requestedHeight), text, textOption);
        }

        break;
    case UpArrow:
        /*
        (0,0) ^
            /   \
          /_     _\
            |   |
            |   |
            |   |
            -----

        Width is 50% of total image height
        Positive offset moves arrow down
        */
        headHeight = qMin(requestedWidth / 2.0, inHeight / 2.0);
        headWidth = requestedWidth / 2.0;
        arrowWidth = requestedWidth / 4.0;

        //Polygon for above (clockwise starting at tip)
        arrow.append(QPointF(halfWidth, offset));
        arrow.append(QPointF(halfWidth + headWidth / 2.0, headHeight + offset));
        arrow.append(QPointF(halfWidth + arrowWidth / 2.0, headHeight + offset));
        arrow.append(QPointF(halfWidth + arrowWidth / 2.0, requestedHeight + offset));
        arrow.append(QPointF(halfWidth - arrowWidth / 2.0, requestedHeight + offset));
        arrow.append(QPointF(halfWidth - arrowWidth / 2.0, headHeight + offset));
        arrow.append(QPointF(halfWidth - headWidth / 2.0, headHeight + offset));

        painter.drawConvexPolygon(arrow);

        if (text != "")
        {
            painter.save();
            //Draw text on top of the arrow
            painter.translate(0, inHeight);
            painter.rotate(-90);

            painter.drawText(QRectF(inHeight - requestedHeight - offset, 0, requestedHeight, requestedWidth), text, textOption);
            painter.restore();
        }

        break;
    case DownArrow:
        /*
        (0,0)----
            |   |
            |   |
            |   |
           \-   -/
            \   /
              *

        Width is 50% of total image height
        Positive offset moves arrow up
        */
        headHeight = qMin(requestedWidth / 2.0, inHeight / 2.0);
        headWidth = requestedWidth / 2.0;
        arrowWidth = requestedWidth / 4.0;

        //Polygon for above (clockwise starting at tip)
        arrow.append(QPointF(halfWidth, inHeight - offset));
        arrow.append(QPointF(halfWidth + headWidth / 2.0, inHeight - headHeight - offset));
        arrow.append(QPointF(halfWidth + arrowWidth / 2.0, inHeight - headHeight - offset));
        arrow.append(QPointF(halfWidth + arrowWidth / 2.0, inHeight - requestedHeight - offset));
        arrow.append(QPointF(halfWidth - arrowWidth / 2.0, inHeight - requestedHeight - offset));
        arrow.append(QPointF(halfWidth - arrowWidth / 2.0, inHeight - headHeight - offset));
        arrow.append(QPointF(halfWidth - headWidth / 2.0, inHeight - headHeight - offset));

        painter.drawConvexPolygon(arrow);

        if (text != "")
        {
            painter.save();
            //Draw text on top of the arrow
            painter.translate(0, inHeight);
            painter.rotate(-90);

            painter.drawText(QRectF(offset, 0, requestedHeight, requestedWidth), text, textOption);
            painter.restore();
        }

        break;
    }

    return input;
}

//Draws lines at a specific offset on a QPixmap
QPixmap& PixmapUtils::drawOffsetLines(QPixmap& input, Qt::GlobalColor colour, float offsetPercentage)
{
    //Early out if we're not drawing anything
    if (offsetPercentage <= 0.0)
        return input;

    int inHeight = input.height();
    int inWidth = input.width();

    int stripWidthHorizontal = qCeil(inHeight * offsetPercentage);
    int stripWidthVertical = qCeil(inWidth * offsetPercentage);

    QPainter painter(&input);

    QPen pen;
    pen.setColor(colour);
    //TODO: Make this configurable
    pen.setWidth(2);
    painter.setPen(pen);

    QBrush brush(Qt::NoBrush);
    brush.setColor(colour);
    painter.setBrush(brush);

    //Draw rectangles - x,y,width,height
    painter.drawRect(0, 0, inWidth, stripWidthHorizontal); //top
    painter.drawRect(0, inHeight - stripWidthHorizontal, inWidth, stripWidthHorizontal); //bottom
    painter.drawRect(0, 0, stripWidthVertical, inHeight); //left
    painter.drawRect(inWidth - stripWidthVertical, 0, stripWidthVertical, inHeight); //right

    return input;
}
