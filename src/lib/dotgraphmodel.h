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

#ifndef KGRAPHVIEWER_DOTGRAPHMODEL_H
#define KGRAPHVIEWER_DOTGRAPHMODEL_H

#include "abstractgraphmodel.h"

#include <graphviz/gvc.h>

namespace KGraphViewer {

/**
 * A graph model whose backing store is a Cgraph Agraph_t
 * and (optionally) a Graphviz layout.
 *
 * \warning This class calls the library function aginit() and sets various default
 * attributes on the graph even when the graph is supplied by the library user.
 */
class KGRAPHVIEWER_EXPORT DotGraphModel : public AbstractGraphModel {
public:
    explicit DotGraphModel(QObject * parent = 0);

    /**
     * Initialize the model with the given graph.
     *
     * DotGraphModel takes ownership of the graph and will free it
     * in its destructor.
     */
    explicit DotGraphModel(Agraph_t * agraph, QObject * parent = 0);
    virtual ~DotGraphModel();

    /**
     * Return the internal graph representation. Never returns 0.
     *
     * Note that if you make manual changes to the graph via this
     * function, it is your responsibility to ensure that users of
     * the model are informed of changes.
     *
     * Manually adding or removing nodes and edges will lead to
     * undefined behavior.
     */
    Agraph_t * graph();

    /**
     * Reset the graph model to use the given graph.
     *
     * The previously contained graph is freed, and DotGraphModel takes
     * ownership of the given graph.
     */
    void setGraph(Agraph_t * agraph);

    /**
     * Give up ownership of the current graph and returns it; the model
     * becomes empty.
     *
     * May return 0 if the graph is empty.
     */
    Agraph_t * releaseGraph();

    virtual Attributes attributes() const;

    virtual QVariant nodeData(const NodeIndex & node, int role) const;
    virtual QVariant edgeData(const EdgeIndex & edge, int role) const;

    virtual NodeIndex firstNode(const NodeIndex & node = NodeIndex()) const;
    virtual NodeIndex nextNode(const NodeIndex & node) const;
    virtual NodeIndex parent(const NodeIndex & node) const;

    virtual EdgeIndex firstEdge() const;
    virtual EdgeIndex nextEdge(const EdgeIndex & node) const;
    virtual QList<EdgeIndex> incidentEdges(const NodeIndex & node) const;
    virtual QList<EdgeIndex> outgoingEdges(const NodeIndex & node) const;
    virtual QList<EdgeIndex> incomingEdges(const NodeIndex & node) const;
    virtual NodeIndex head(const EdgeIndex & edge) const;
    virtual NodeIndex tail(const EdgeIndex & edge) const;

    /**
     * Create a new node under the given parent, which must have been created via the \ref addSubgraph
     * method.
     *
     * \param name the identifier string of the node; if no string is given, a NULL identifier will be used.
     * Using the same identifier for multiple nodes is undefined behavior.
     *
     * \return the index of the newly created node
     */
    NodeIndex addNode(const NodeIndex & parent, const QString & name = QString());

    /**
     * Create a new subgraph -- that is, a node that can have sub-nodes attached to it --
     * under the given parent, which must have been created via \ref addSubgraph as well.
     *
     * \param name the identifier string of the subgraph; if no string is given, a NULL identifier will be used.
     * Using the same identifier for multiple subgraphs is undefined behavior.
     *
     * \return the index of the newly created subgraph
     */
    NodeIndex addSubgraph(const NodeIndex & parent, const QString & name = QString());

    /**
     * Remove a node or subgraph.
     *
     * Note that, for compatibility with the overall AbstractGraphModel concept of nodes,
     * removing a subgraph will recursively remove all nodes within that subgraph.
     */
    void removeNode(const NodeIndex & node);

    /**
     * Add an edge from \p tail to \p head. Both nodes must have been created via \ref addNode.
     *
     * \param name the identifier string of the edge; if no string is given, a NULL identifier will be used.
     * Using the same identifier for multiple edges is undefined behavior.
     */
    EdgeIndex addEdge(const NodeIndex & tailidx, const NodeIndex & headidx);

    void removeEdge(const EdgeIndex & edge);

    QString nodeKey(const NodeIndex& node, const QString& key) const;
    QString edgeKey(const EdgeIndex& idx, const QString& key) const;
    void setNodeKey(const NodeIndex & node, const QString & key, const QString & value);
    void setEdgeKey(const EdgeIndex & edge, const QString & key, const QString & value);
    void setNodeData(const NodeIndex & node, int role, const QVariant & data);
    void setEdgeData(const EdgeIndex & edge, int role, const QVariant & data);

    /**
     * Calls the given Graphviz \p layoutcommand to (re-)calculate a layout for the graph.
     */
    void layout(const QString & layoutcommand);

private:
    class Data;
    const QScopedPointer<Data> d;
};

} // namespace KGraphViewer

#endif // KGRAPHVIEWER_DOTGRAPHMODEL_H

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
