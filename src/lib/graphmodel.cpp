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
    QList<Edge *> edges;
    QList<Node *> children;
    QHash<int, QVariant> data;

    Node() : parent(0) {}
    ~Node();
};

Node::~Node()
{
    Q_ASSERT(edges.empty());
    Q_ASSERT(children.empty());
}

/**
 * Internal representation of an edge. Edges are allocated on the heap,
 * and pointers to them are stable.
 */
struct Edge {
    Node * head;
    Node * tail;
    QHash<int, QVariant> data;
};

} // anonymous namespace

class GraphModel::Data {
public:
    GraphModel & model;
    QList<Node *> topLevelNodes;
    QList<Edge *> edges;

    Data(GraphModel & m) : model(m) {}
    ~Data();

    void clear();
    void removeNode(const NodeIndex & parentidx, const NodeIndex & idx);
    void removeEdge(const EdgeIndex & idx);

    Node * parentOfIndex(const NodeIndex & idx);
    Node * nodeOfIndex(const NodeIndex & idx);
    QList<Node *> & children(Node * node);
    NodeIndex nodeToIndex(Node * node);

    Edge * edgeOfIndex(const EdgeIndex & idx);
    EdgeIndex edgeToIndex(Edge * edge);

    NodeIndex makeNodeIndex(int index, Node * parent);
};

GraphModel::Data::~Data()
{
    clear();
}

NodeIndex GraphModel::Data::makeNodeIndex(int index, Node * parent)
{
    return model.makeNodeIndex(index, static_cast<void *>(parent));
}

void GraphModel::Data::clear()
{
    while (!topLevelNodes.empty()) {
        removeNode(NodeIndex(), model.makeNodeIndex(0, 0));
    }

    Q_ASSERT(edges.empty());
}

void GraphModel::Data::removeNode(const NodeIndex & parentidx, const NodeIndex & idx)
{
    QList<Node *> & list = children(parentOfIndex(idx));
    int index = idx.index();
    Node * node = list[index];

    while (!node->children.empty())
        removeNode(idx, model.makeNodeIndex(0, node));
    while (!node->edges.empty())
        removeEdge(edgeToIndex(node->edges[0]));

    model.beginRemoveNode(parentidx, index);
    list.removeAt(index);
    delete node;
    model.endRemoveNode();
}

void GraphModel::Data::removeEdge(const EdgeIndex & idx)
{
    if (!idx.isValid())
        return;

    int index = idx.index();
    Edge * edge = edges.at(index);
    model.beginRemoveEdge(index);
    edge->head->edges.removeAll(edge);
    edge->tail->edges.removeAll(edge);
    edges.removeAt(index);
    delete edge;
    model.endRemoveEdge();
}

Node * GraphModel::Data::parentOfIndex(const NodeIndex & idx)
{
    return static_cast<Node *>(model.nodeIndexInternalPtr(idx));
}

Node * GraphModel::Data::nodeOfIndex(const NodeIndex & idx)
{
    if (!idx.isValid())
        return 0;
    return children(parentOfIndex(idx)).at(idx.index());
}

QList<Node*>& GraphModel::Data::children(Node * node)
{
    if (node)
        return node->children;
    else
        return topLevelNodes;
}

NodeIndex GraphModel::Data::nodeToIndex(Node * node)
{
    QList<Node *> & list = children(node->parent);
    int index = list.indexOf(node);
    Q_ASSERT(index >= 0 && index < list.size());
    return model.makeNodeIndex(index, node->parent);
}

Edge * GraphModel::Data::edgeOfIndex(const EdgeIndex & idx)
{
    if (!idx.isValid())
        return 0;
    return edges.at(idx.index());
}

EdgeIndex GraphModel::Data::edgeToIndex(Edge * edge)
{
    int index = edges.indexOf(edge);
    Q_ASSERT(index >= 0 && index < edges.size());
    return model.makeEdgeIndex(index);
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
    if (node) {
        if (node->children.empty())
            return NodeIndex();
    } else {
        if (d->topLevelNodes.empty())
            return NodeIndex();
    }

    return makeNodeIndex(0, node);
}

NodeIndex GraphModel::nextNode(const NodeIndex& node) const
{
    if (!node.isValid())
        return NodeIndex();

    Node * np = d->parentOfIndex(node);
    QList<Node *> & children = d->children(np);
    int nextindex = node.index() + 1;

    if (nextindex < children.size())
        return makeNodeIndex(nextindex, np);
    else
        return NodeIndex();
}

NodeIndex GraphModel::parent(const NodeIndex & node) const
{
    Node * np = d->parentOfIndex(node);
    return d->nodeToIndex(np);
}

EdgeIndex GraphModel::firstEdge() const
{
    if (!d->edges.empty())
        return makeEdgeIndex(0);
    else
        return EdgeIndex();
}

EdgeIndex GraphModel::nextEdge(const EdgeIndex & edge) const
{
    if (!edge.isValid())
        return EdgeIndex();

    int nextindex = edge.index();
    if (nextindex < d->edges.size())
        return makeEdgeIndex(nextindex);
    else
        return EdgeIndex();
}

QList<EdgeIndex> GraphModel::incidentEdges(const NodeIndex& node) const
{
    Node * n = d->nodeOfIndex(node);
    if (!n)
        return QList<EdgeIndex>();

    QList<EdgeIndex> edges;
    edges.reserve(n->edges.size());
    Q_FOREACH(Edge * edge, n->edges) {
        edges.push_back(d->edgeToIndex(edge));
    }

    return edges;
}

QList<EdgeIndex> GraphModel::outgoingEdges(const NodeIndex& node) const
{
    Node * n = d->nodeOfIndex(node);
    if (!n)
        return QList<EdgeIndex>();

    QList<EdgeIndex> edges;
    edges.reserve(n->edges.size());
    Q_FOREACH(Edge * edge, n->edges) {
        if (edge->tail == n)
            edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

QList<EdgeIndex> GraphModel::incomingEdges(const NodeIndex& node) const
{
    Node * n = d->nodeOfIndex(node);
    if (!n)
        return QList<EdgeIndex>();

    QList<EdgeIndex> edges;
    edges.reserve(n->edges.size());
    Q_FOREACH(Edge * edge, n->edges) {
        if (edge->head == n)
            edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

NodeIndex GraphModel::head(const EdgeIndex & idx) const
{
    Edge * edge = d->edgeOfIndex(idx);
    if (!edge)
        return NodeIndex();
    return d->nodeToIndex(edge->head);
}

NodeIndex GraphModel::tail(const EdgeIndex& idx) const
{
    Edge * edge = d->edgeOfIndex(idx);
    if (!edge)
        return NodeIndex();
    return d->nodeToIndex(edge->tail);
}

NodeIndex GraphModel::addNode(const NodeIndex & parentidx)
{
    Node * parent = d->nodeOfIndex(parentidx);
    QList<Node *> & list = d->children(parent);
    int index = list.size();
    beginInsertNode(parentidx, index);
    list.push_back(new Node);
    endInsertNode();
    return d->makeNodeIndex(index, parent);
}

void GraphModel::removeNode(const NodeIndex & node)
{
    if (!node.isValid())
        return;
    d->removeNode(parent(node), node);
}

EdgeIndex GraphModel::addEdge(const NodeIndex & tailidx, const NodeIndex & headidx)
{
    Q_ASSERT(tailidx.isValid() && headidx.isValid());

    Node * tail = d->nodeOfIndex(tailidx);
    Node * head = d->nodeOfIndex(headidx);
    int index = d->edges.size();
    beginInsertEdge(index);
    Edge * edge = new Edge;
    edge->head = head;
    edge->tail = tail;
    tail->edges.push_back(edge);
    head->edges.push_back(edge);
    d->edges.push_back(edge);
    endInsertEdge();
    return makeEdgeIndex(index);
}

void GraphModel::removeEdge(const EdgeIndex & edge)
{
    d->removeEdge(edge);
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

// kate: space-indent on;indent-width 4;replace-tabs on
