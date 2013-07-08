/*
 * Copyright 2013 Nicolai Hähnle <nhaehnle@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
/*
Parts of this file are from the following files, subject
to the following copyright notices.

   Copyright (C) 2005-2007 Gael de Chalendar <kleag@free.fr>

   KGraphViewer is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA

   This file was part of KCachegrind.
   Copyright (C) 2003 Josef Weidendorfer <Josef.Weidendorfer@gmx.de>

   KCachegrind is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation, version 2.
*/

#include "dotgraphdelegate.h"

#include <QGraphicsItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include <KDebug>

#include "graphviz/gvc.h"

#include "dotgraphmodel.h"
#include "dotgrammar.h"
#include "dotrenderop.h"
#include "FontsCache.h"
#include "dotgraph.h"

namespace KGraphViewer {

namespace {

class DotGraphicsItem : public QGraphicsItem {
public:
    DotGraphicsItem(const DotRenderOpVec & ops, QGraphicsItem* parent = 0, QGraphicsScene* scene = 0);

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

    const QPen & pen() const {return pen_;}
    void setPen(const QPen & pen);

private:
    void computeBoundingRect();
    QPainterPath pathForSpline(const DotRenderOp& op) const;

    DotRenderOpVec ops_;
    QPen pen_;
    QRectF boundingRect_;
};

DotGraphicsItem::DotGraphicsItem(const DotRenderOpVec& ops, QGraphicsItem* parent, QGraphicsScene* scene) :
    QGraphicsItem(parent, scene),
    ops_(ops)
{
    computeBoundingRect();
}

QRectF DotGraphicsItem::boundingRect() const
{
    return boundingRect_;
}


void DotGraphicsItem::computeBoundingRect()
{
    boundingRect_ = QRectF();

    float penWidth = pen().widthF();

    Q_FOREACH(const DotRenderOp & dro, ops_) {
        if (dro.renderop == "e" || dro.renderop == "E") {
            float x = dro.integers[0];
            float y = -dro.integers[1];
            float w = dro.integers[2];
            float h = dro.integers[3];
            QRectF rect(x - w, y - h, 2 * w, 2 * h);
            rect.adjust(-penWidth, -penWidth, penWidth, penWidth);
            boundingRect_ |= rect;
        } else if (dro.renderop == "p" || dro.renderop == "P" || dro.renderop == "L") {
            for (int i = 0; i < dro.integers[0]; i++) {
                QPointF pt(dro.integers[2*i + 1], -dro.integers[2*i + 2]);
                boundingRect_ |= QRectF(pt, QSizeF(0, 0));
            }
        } else if (dro.renderop == "b" || dro.renderop == "B") {
            boundingRect_ |= pathForSpline(dro).boundingRect();
        }
    }
}

void DotGraphicsItem::setPen(const QPen& pen)
{
    pen_ = pen;
    update();
}

QPainterPath DotGraphicsItem::pathForSpline(const DotRenderOp& op) const
{
    Q_ASSERT(op.integers.size() >= 1);
    int num = op.integers[0];
    int segments = (num - 1) / 3;
    QPainterPath path;
    if (segments < 1)
        return path;
    if (op.integers.size() < 3 + 6 * segments) {
        kWarning() << "DotGraphicsItem::pathForSpline: insufficient parameters";
        return path;
    }

    path.moveTo(QPointF(op.integers[1], -op.integers[2]));
    for (int j = 0; j < segments; ++j) {
        QPointF c1(op.integers[3 + 6*j + 0], -op.integers[3 + 6*j + 1]);
        QPointF c2(op.integers[3 + 6*j + 2], -op.integers[3 + 6*j + 3]);
        QPointF c3(op.integers[3 + 6*j + 4], -op.integers[3 + 6*j + 5]);
        path.cubicTo(c1, c2, c3);
    }
    return path;
}

void DotGraphicsItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->save();
    painter->setPen(pen());

    Q_FOREACH(const DotRenderOp & dro, ops_) {
        if (dro.renderop == "c") {
            QColor c(dro.str.mid(0,7));
            bool ok;
            c.setAlpha(255-dro.str.mid(8).toInt(&ok,16));
            QPen pen = painter->pen();
            pen.setColor(c);
            painter->setPen(pen);
        } else if (dro.renderop == "C") {
            QColor c(dro.str.mid(0,7));
            bool ok;
            c.setAlpha(255-dro.str.mid(8).toInt(&ok,16));
            QBrush brush = painter->brush();
            brush.setColor(c);
            painter->setBrush(brush);
        } else if (dro.renderop == "e" || dro.renderop == "E") {
            // Draw an ellipse
            float x = dro.integers[0];
            float y = -dro.integers[1];
            float w = dro.integers[2];
            float h = dro.integers[3];
            QRectF rect(x - w, y - h, 2 * w, 2 * h);
            if (dro.renderop == "e") {
                painter->save();
                painter->setBrush(Qt::NoBrush);
            }
            painter->drawEllipse(rect);
            if (dro.renderop == "e") {
                painter->restore();
            }
        } else if (dro.renderop == "p" || dro.renderop == "P") {
            // Draw a polygon
            QPolygonF points(dro.integers[0]);
            for (int i = 0; i < dro.integers[0]; i++) {
                points[i] = QPointF(dro.integers[2*i + 1], -dro.integers[2*i + 2]);
            }
            if (dro.renderop == "p") {
                painter->save();
                painter->setBrush(Qt::NoBrush);
            }
            painter->drawPolygon(points);
            if (dro.renderop == "p") {
                painter->restore();
            }
        } else if (dro.renderop == "L") {
            // Draw polyline
            QPolygonF points(dro.integers[0]);
            for (int i = 0; i < dro.integers[0]; i++) {
                points[i] = QPointF(dro.integers[2*i + 1], -dro.integers[2*i + 2]);
            }
            painter->drawPolyline(points);
        } else if (dro.renderop == "B" || dro.renderop == "b") {
            if (dro.renderop == "B") {
                painter->save();
                painter->setBrush(Qt::NoBrush);
            }
            painter->drawPath(pathForSpline(dro));
            if (dro.renderop == "B") {
                painter->restore();
            }
        } else if (dro.renderop == "T") {
            float x = dro.integers[0];
            float y = -dro.integers[1];
            int align = dro.integers[2];
            if (align >= 0) {
                QFontMetrics fm = painter->fontMetrics();
                float width = fm.width(dro.str);
                x -= width * 0.5 * (align + 1);
            }
            painter->drawText(x, y, dro.str);
        } else if (dro.renderop == "F") {
            QFont font = *FontsCache::changeable().fromName(dro.str);
            font.setPointSize(dro.integers[0]);
            painter->setFont(font);
        } else if (dro.renderop == "S") {
            if (dro.str == "solid") {
                QPen pen = painter->pen();
                pen.setStyle(Qt::SolidLine);
                painter->setPen(pen);
            } else if (dro.str == "dashed") {
                QPen pen = painter->pen();
                pen.setStyle(Qt::DashLine);
                painter->setPen(pen);
            } else if (dro.str == "dotted") {
                QPen pen = painter->pen();
                pen.setStyle(Qt::DotLine);
                painter->setPen(pen);
            } else {
                kDebug() << "DotGraphDelegate::paint: unknown style " << dro.str;
            }
        } else {
            kDebug() << "DotGraphDelegate::paint: unhandled renderop " << dro.renderop;
        }
    }

    painter->restore();
}

} // anonymous namespace

DotGraphDelegate::DotGraphDelegate(QObject* parent) : AbstractItemDelegate(parent)
{
}

DotGraphDelegate::~DotGraphDelegate()
{
}

QGraphicsItem* DotGraphDelegate::createNodeItem(const NodeIndex& node)
{
    const DotGraphModel * model = dynamic_cast<const DotGraphModel *>(node.model());

    DotRenderOpVec ops;
    parse_renderop(model->nodeKey(node, "_draw_").toStdString(), ops);
    parse_renderop(model->nodeKey(node, "_ldraw_").toStdString(), ops);

    DotGraphicsItem * item = new DotGraphicsItem(ops);
    return item;
}

QGraphicsItem* DotGraphDelegate::createEdgeItem(const EdgeIndex& edge)
{
    static const char * const ATTRS[] = {
        "_draw_",
        "_hdraw_",
        "_tdraw_",
        "_ldraw_",
        "_hldraw_",
        "_tldraw_",
        0
    };

    const DotGraphModel * model = dynamic_cast<const DotGraphModel *>(edge.model());
    DotRenderOpVec ops;
    for (const char * const * pp = ATTRS; *pp; ++pp) {
        parse_renderop(model->edgeKey(edge, *pp).toStdString(), ops);
    }

    DotGraphicsItem * item = new DotGraphicsItem(ops);
    return item;
}

} // namespace KGraphViewer

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
