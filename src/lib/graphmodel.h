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

#ifndef KGRAPHVIEWER_GRAPHMODEL_H
#define KGRAPHVIEWER_GRAPHMODEL_H

#include "abstractgraphmodel.h"

namespace KGraphViewer {

/**
 * A simple graph model.
 */
class KGRAPHVIEWER_EXPORT GraphModel : public AbstractGraphModel {
public:
    explicit GraphModel(QObject * parent = 0);
    virtual ~GraphModel();

    virtual Attributes attributes() const;

    virtual QVariant nodeData(const NodeIndex & node, int role) const;
    virtual QVariant edgeData(const EdgeIndex & edge, int role) const;

    virtual NodeIndex firstNode(const NodeIndex & node) const;
    virtual NodeIndex nextNode(const NodeIndex & node) const;
    virtual NodeIndex parent(const NodeIndex & node) const;

    virtual EdgeIndex firstEdge() const;
    virtual EdgeIndex nextEdge(const EdgeIndex & node) const;
    virtual QList<EdgeIndex> incidentEdges(const NodeIndex & node) const;
    virtual QList<EdgeIndex> outgoingEdges(const NodeIndex & node) const;
    virtual QList<EdgeIndex> incomingEdges(const NodeIndex & node) const;
    virtual NodeIndex head(const EdgeIndex & edge) const;
    virtual NodeIndex tail(const EdgeIndex & edge) const;

    NodeIndex addNode(const NodeIndex & parent);
    void removeNode(const NodeIndex & node);
    void setNodeData(const NodeIndex & node, int role, const QVariant & data);

    EdgeIndex addEdge(const NodeIndex & tail, const NodeIndex & head);
    void removeEdge(const EdgeIndex & edge);
    void setEdgeData(const EdgeIndex & edge, int role, const QVariant & data);

private:
    class Data;
    const QScopedPointer<Data> d;
};

} // namespace KGraphViewer

#endif // KGRAPHVIEWER_GRAPHMODEL_H

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
