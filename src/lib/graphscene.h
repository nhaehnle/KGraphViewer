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

#ifndef KGRAPHVIEWER_GRAPHSCENE_H
#define KGRAPHVIEWER_GRAPHSCENE_H

#include "kgraphviewer_export.h"

#include <QGraphicsScene>

namespace KGraphViewer {

class AbstractGraphModel;
class NodeIndex;

class KGRAPHVIEWER_EXPORT AbstractItemDelegate : public QObject {
public:
    explicit AbstractItemDelegate(QObject* parent = 0);
    virtual ~AbstractItemDelegate();

    virtual QGraphicsItem * createNodeItem(const NodeIndex & node) = 0;
};

/**
 * A graphics scene that automatically adds and removes graphics items corresponding
 * to a graph's nodes and edges.
 */
class KGRAPHVIEWER_EXPORT GraphScene : public QGraphicsScene {
public:
    explicit GraphScene(QObject * parent = 0);
    virtual ~GraphScene();

    void setModel(AbstractGraphModel * model);
    AbstractGraphModel * model() const;
    void setItemDelegate(AbstractItemDelegate * delegate);
    AbstractItemDelegate * itemDelegate() const;

    QGraphicsItem * itemForNode(NodeIndex & node) const;

private:
    class Data;
    const QScopedPointer<Data> d;
};

} // namespace KGraphViewer

#endif // KGRAPHVIEWER_GRAPHSCENE_H

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
