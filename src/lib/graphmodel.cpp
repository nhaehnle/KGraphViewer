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

#include "graphmodel.h"

#include <QVariant>

namespace KGraphViewer {

/*

GraphModel stores a tree of nodes and a flat list of all edges.

*/

namespace {

struct Edge;

/**
 * Internal representation of a node. Nodes are allocated on the heap,
 * and pointers to them are stable and used in our NodeIndex representation.
 *
 * Nodes and edges must be created and destroyed explicitly.
 */
struct Node {
    Node * parent;
    Node * next;
    Node ** pprev;
    Node * firstChild;
    QList<Edge *> edges;
    QHash<int, QVariant> data;

    Node() : firstChild(0) {}
    ~Node();
};

Node::~Node()
{
    Q_ASSERT(edges.empty());
    Q_ASSERT(!firstChild);
}

/**
 * Internal representation of an edge. Edges are allocated on the heap,
 * and pointers to them are stable.
 */
struct Edge {
    Node * head;
    Node * tail;
    Edge * next;
    Edge ** pprev;
    QHash<int, QVariant> data;
};

} // anonymous namespace

class GraphModel::Data {
public:
    GraphModel & model;
    Node * firstNode;
    Edge * firstEdge;

    Data(GraphModel & m) : model(m), firstNode(0), firstEdge(0) {}
    ~Data();

    void clear();
    void removeNode(Node * node);
    void removeEdge(Edge * edge);

    Node * nodeOfIndex(const NodeIndex & idx) {return idx.isValid() ? static_cast<Node *>(model.nodeIndexInternalPtr(idx)) : 0;}
    Edge * edgeOfIndex(const EdgeIndex & idx) {return idx.isValid() ? static_cast<Edge *>(model.edgeIndexInternalPtr(idx)) : 0;}
    NodeIndex nodeToIndex(Node * node) {return node ? model.makeNodeIndex(node) : NodeIndex();}
    EdgeIndex edgeToIndex(Edge * edge) {return edge ? model.makeEdgeIndex(edge) : EdgeIndex();}
};

GraphModel::Data::~Data()
{
    clear();
}

void GraphModel::Data::clear()
{
    while (firstNode) {
        removeNode(firstNode);
    }

    Q_ASSERT(!firstEdge);
}

void GraphModel::Data::removeNode(Node * node)
{
    Q_ASSERT(node != 0);

    while (node->firstChild)
        removeNode(node->firstChild);
    while (!node->edges.empty())
        removeEdge(node->edges[0]);

    emit model.nodeAboutToBeRemoved(model.makeNodeIndex(node));
    if (node->next)
        node->next->pprev = node->pprev;
    *node->pprev = node->next;
    delete node;
}

void GraphModel::Data::removeEdge(Edge * edge)
{
    Q_ASSERT(edge != 0);

    emit model.edgeAboutToBeRemoved(model.makeEdgeIndex(edge));
    edge->head->edges.removeAll(edge);
    edge->tail->edges.removeAll(edge);
    if (edge->next)
        edge->next->pprev = edge->pprev;
    *edge->pprev = edge->next;
    delete edge;
}

GraphModel::GraphModel(QObject * parent) :
    AbstractGraphModel(parent),
    d(new Data(*this))
{
}

GraphModel::~GraphModel()
{
}

AbstractGraphModel::Attributes GraphModel::attributes() const
{
    return IsEditable;
}

QVariant GraphModel::nodeData(const NodeIndex & idx, int role) const
{
    Node * node = d->nodeOfIndex(idx);
    if (!node)
        return QVariant();

    return node->data.value(role);
}

QVariant GraphModel::edgeData(const EdgeIndex & idx, int role) const
{
    Edge * edge = d->edgeOfIndex(idx);
    if (!edge)
        return QVariant();

    return edge->data.value(role);
}

NodeIndex GraphModel::firstNode(const NodeIndex & idx) const
{
    Node * node = d->nodeOfIndex(idx);
    return d->nodeToIndex(node ? node->firstChild : d->firstNode);
}

NodeIndex GraphModel::nextNode(const NodeIndex & idx) const
{
    Node * node = d->nodeOfIndex(idx);
    return d->nodeToIndex(node ? node->next : 0);
}

NodeIndex GraphModel::parent(const NodeIndex & idx) const
{
    Node * node = d->nodeOfIndex(idx);
    return d->nodeToIndex(node ? node->parent : 0);
}

EdgeIndex GraphModel::firstEdge() const
{
    return d->edgeToIndex(d->firstEdge);
}

EdgeIndex GraphModel::nextEdge(const EdgeIndex & idx) const
{
    Edge * edge = d->edgeOfIndex(idx);
    return d->edgeToIndex(edge ? edge->next : 0);
}

QList<EdgeIndex> GraphModel::incidentEdges(const NodeIndex & idx) const
{
    Node * node = d->nodeOfIndex(idx);
    if (!node)
        return QList<EdgeIndex>();

    QList<EdgeIndex> edges;
    edges.reserve(node->edges.size());
    Q_FOREACH(Edge * edge, node->edges) {
        edges.push_back(d->edgeToIndex(edge));
    }

    return edges;
}

QList<EdgeIndex> GraphModel::outgoingEdges(const NodeIndex & idx) const
{
    Node * node = d->nodeOfIndex(idx);
    if (!node)
        return QList<EdgeIndex>();

    QList<EdgeIndex> edges;
    edges.reserve(node->edges.size());
    Q_FOREACH(Edge * edge, node->edges) {
        if (edge->tail == node)
            edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

QList<EdgeIndex> GraphModel::incomingEdges(const NodeIndex & idx) const
{
    Node * node = d->nodeOfIndex(idx);
    if (!node)
        return QList<EdgeIndex>();

    QList<EdgeIndex> edges;
    edges.reserve(node->edges.size());
    Q_FOREACH(Edge * edge, node->edges) {
        if (edge->head == node)
            edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

NodeIndex GraphModel::head(const EdgeIndex & idx) const
{
    Edge * edge = d->edgeOfIndex(idx);
    return d->nodeToIndex(edge ? edge->head : 0);
}

NodeIndex GraphModel::tail(const EdgeIndex& idx) const
{
    Edge * edge = d->edgeOfIndex(idx);
    return d->nodeToIndex(edge ? edge->tail : 0);
}

NodeIndex GraphModel::addNode(const NodeIndex & parentidx)
{
    Node * parent = d->nodeOfIndex(parentidx);

    Node * node = new Node;
    node->parent = parent;
    node->next = parent ? parent->firstChild : d->firstNode;
    if (node->next)
        node->pprev = &node->next;
    node->pprev = parent ? &parent->firstChild : &d->firstNode;
    *node->pprev = node;

    NodeIndex idx = d->nodeToIndex(node);
    emit nodeInserted(idx);
    return idx;
}

void GraphModel::removeNode(const NodeIndex & idx)
{
    if (idx.isValid())
        d->removeNode(d->nodeOfIndex(idx));
}

EdgeIndex GraphModel::addEdge(const NodeIndex & tailidx, const NodeIndex & headidx)
{
    Q_ASSERT(tailidx.isValid() && headidx.isValid());

    Node * tail = d->nodeOfIndex(tailidx);
    Node * head = d->nodeOfIndex(headidx);

    Edge * edge = new Edge;
    edge->head = head;
    edge->tail = tail;
    tail->edges.push_back(edge);
    head->edges.push_back(edge);
    edge->next = d->firstEdge;
    if (edge->next)
        edge->next->pprev = &edge->next;
    edge->pprev = &d->firstEdge;
    d->firstEdge = edge;

    EdgeIndex idx = d->edgeToIndex(edge);
    emit edgeInserted(idx);
    return idx;
}

void GraphModel::removeEdge(const EdgeIndex & idx)
{
    if (idx.isValid())
        d->removeEdge(d->edgeOfIndex(idx));
}

void GraphModel::setNodeData(const NodeIndex & idx, int role, const QVariant& data)
{
    if (!idx.isValid())
        return;

    Node * node = d->nodeOfIndex(idx);
    node->data.insert(role, data);
    emit nodeDataChanged(idx);
}

void GraphModel::setEdgeData(const EdgeIndex & idx, int role, const QVariant& data)
{
    if (!idx.isValid())
        return;

    Edge * edge = d->edgeOfIndex(idx);
    edge->data.insert(role, data);
    emit edgeDataChanged(idx);
}

} // namespace KGraphViewer

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
