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

#include "dotgraphmodel.h"

#include <KDebug>
#include <QRect>

#ifdef WITH_CGRAPH
#error Check that everything (in particular, subgraphs) work correctly in cgraph mode
#endif

namespace KGraphViewer {

/*

EdgeIndex internal pointer is the Agedge_t.

NodeIndex internal pointer is the Agnode_t or Agraph_t.

*/

class DotGraphModel::Data {
public:
    DotGraphModel & model;
    Agraph_t * graph_p;
    GVC_t * gvc;
    unsigned int automaticNameCounter;

    Data(DotGraphModel & m);
    ~Data();

    void clear();
    void clearLayout();
    Agraph_t * graph();
    void setGraph(Agraph_t * agraph);

    void * agobjFromIndex(const NodeIndex & idx);
    Agnode_t * node_cast(void * obj);
    Agraph_t * graph_cast(void * obj);
    NodeIndex nodeToIndex(Agnode_t * node);
    NodeIndex nodeToIndex(Agraph_t * node);

    Agedge_t * edgeFromIndex(const EdgeIndex & idx) {return idx.isValid() ? static_cast<Agedge_t *>(model.edgeIndexInternalPtr(idx)) : 0;}
    EdgeIndex edgeToIndex(Agedge_t * edge) {return edge ? model.makeEdgeIndex(edge) : EdgeIndex();}

    void emitAllDataChanged();
    void emitNodeChanged(const NodeIndex & node);

    QString unusedName(Agraph_t * graph);
    void initGraph();
};

DotGraphModel::Data::Data(DotGraphModel & m) :
    model(m),
    graph_p(0),
    gvc(0),
    automaticNameCounter(0)
{
}

DotGraphModel::Data::~Data()
{
    clear();
}

AbstractGraphModel::Attributes DotGraphModel::attributes() const
{
    return IsEditable;
}

void DotGraphModel::Data::clear()
{
    if (graph_p) {
        clearLayout();
        agclose(graph_p);
        graph_p = 0;
    }
}

void DotGraphModel::Data::clearLayout()
{
    if (gvc) {
        Q_ASSERT(graph_p != 0);
        gvFreeLayout(gvc, graph_p);
        gvFreeContext(gvc);
    }
}

Agraph_t * DotGraphModel::Data::graph()
{
    if (!graph_p) {
        aginit();
        char dummy[] = ""; // the agopen API is not properly const-safe
        graph_p = agopen(dummy, AGDIGRAPH);
        initGraph();
    }
    return graph_p;
}

void DotGraphModel::Data::setGraph(Agraph_t* agraph)
{
    clear();
    graph_p = agraph;
}

void DotGraphModel::Data::initGraph()
{
    // The graph library is not properly const-safe
    char LABEL[] = "label";
    char EMPTY[] = "";
    agnodeattr(graph(), LABEL, EMPTY);
}

void * DotGraphModel::Data::agobjFromIndex(const NodeIndex & idx)
{
    return idx.isValid() ? model.nodeIndexInternalPtr(idx) : graph();
}

Agnode_t * DotGraphModel::Data::node_cast(void * obj)
{
    Q_ASSERT(agobjkind(obj) == AGNODE);
    return static_cast<Agnode_t *>(obj);
}

Agraph_t * DotGraphModel::Data::graph_cast(void * obj)
{
    Q_ASSERT(agobjkind(obj) == AGGRAPH);
    return static_cast<Agraph_t *>(obj);
}

NodeIndex DotGraphModel::Data::nodeToIndex(Agnode_t * node)
{
    return node ? model.makeNodeIndex(node) : NodeIndex();
}

NodeIndex DotGraphModel::Data::nodeToIndex(Agraph_t * graph)
{
    return (graph && graph != graph_p) ? model.makeNodeIndex(graph) : NodeIndex();
}

void DotGraphModel::Data::emitAllDataChanged()
{
    emitNodeChanged(NodeIndex());
}

void DotGraphModel::Data::emitNodeChanged(const NodeIndex& node)
{
    for (NodeIndex child = model.firstNode(node); child.isValid(); child = model.nextNode(child))
        emitNodeChanged(child);

    if (node.isValid()) {
        emit model.nodeDataChanged(node);

        Q_FOREACH (const EdgeIndex & edge, model.outgoingEdges(node)) {
            emit model.edgeDataChanged(edge);
        }
    }
}

QString DotGraphModel::Data::unusedName(Agraph_t* graph)
{
    QString name;
    do {
        name = QString("kgv%1").arg(++automaticNameCounter);
    } while (agfindnode(graph, name.toUtf8().data()));
    return name;
}

DotGraphModel::DotGraphModel(QObject* parent) :
    AbstractGraphModel(parent),
    d(new Data(*this))
{
}

DotGraphModel::DotGraphModel(Agraph_t* agraph, QObject* parent) :
    AbstractGraphModel(parent),
    d(new Data(*this))
{
    d->setGraph(agraph);
}

DotGraphModel::~DotGraphModel()
{
}

Agraph_t* DotGraphModel::graph()
{
    return d->graph();
}

void DotGraphModel::setGraph(Agraph_t * agraph)
{
    d->setGraph(agraph);
    emit modelReset();
}

Agraph_t * DotGraphModel::releaseGraph()
{
    Agraph_t * g = d->graph_p;
    d->clear();
    emit modelReset();
    return g;
}

NodeIndex DotGraphModel::firstNode(const NodeIndex & idx) const
{
    void * agobj = d->agobjFromIndex(idx);
    if (agobjkind(agobj) == AGNODE) {
        return NodeIndex();
    } else {
        Agraph_t * subgraph = d->graph_cast(agobj);
        Agnode_t * metanode = agmetanode(subgraph);
        Agedge_t * edge = agfstout(metanode->graph, metanode);
        if (edge)
            return d->nodeToIndex(agusergraph(aghead(edge)));
        else
            return d->nodeToIndex(agfstnode(subgraph));
    }
}

NodeIndex DotGraphModel::nextNode(const NodeIndex & idx) const
{
    if (!idx.isValid())
        return NodeIndex();

    void * agobj = d->agobjFromIndex(idx);
    if (agobjkind(agobj) == AGNODE) {
        Agnode_t * node = d->node_cast(agobj);
        Agraph_t * subgraph = node->graph;
        return d->nodeToIndex(agnxtnode(subgraph, node));
    } else {
        Agraph_t * subgraph = d->graph_cast(agobj);
        Agnode_t * metanode = agmetanode(subgraph);
        Agraph_t * metagraph = metanode->graph;
        Agedge_t * edge_from_parent = agfstin(metagraph, metanode);
        Q_ASSERT(edge_from_parent);
        Q_ASSERT(agnxtin(metagraph, edge_from_parent) == 0 && "We do not support cycles in the meta-graph");
        Agedge_t * edge_to_sibling = agnxtout(metagraph, edge_from_parent);
        if (edge_to_sibling)
            return d->nodeToIndex(agusergraph(aghead(edge_to_sibling)));

        Agnode_t * parent_metanode = edge_from_parent->tail;
        return d->nodeToIndex(agfstnode(agusergraph(parent_metanode)));
    }
}

NodeIndex DotGraphModel::parent(const NodeIndex & idx) const
{
    if (!idx.isValid())
        return NodeIndex();

    void * agobj = d->agobjFromIndex(idx);
    if (agobjkind(agobj) == AGNODE) {
        Agnode_t * node = d->node_cast(agobj);
        return d->nodeToIndex(node->graph);
    } else {
        Agraph_t * subgraph = d->graph_cast(agobj);
        Agnode_t * metanode = agmetanode(subgraph);
        Agedge_t * edge_from_parent = agfstin(metanode->graph, metanode);
        if (edge_from_parent)
            return d->nodeToIndex(agusergraph(agtail(edge_from_parent)));
        else
            return NodeIndex();
    }
}

EdgeIndex DotGraphModel::firstEdge() const
{
    Agraph_t * graph = d->graph();
    for (Agnode_t * node = agfstnode(graph); node; node = agnxtnode(graph, node)) {
        Agedge_t * edge = agfstin(graph, node);
        if (edge)
            return d->edgeToIndex(edge);
    }
    return EdgeIndex();
}

EdgeIndex DotGraphModel::nextEdge(const EdgeIndex & idx) const
{
    Agedge_t * edge = d->edgeFromIndex(idx);
    if (!edge)
        return EdgeIndex();

    Agedge_t * nextedge = agnxtin(d->graph(), edge);
    if (nextedge)
        return d->edgeToIndex(nextedge);

    // Remember that all nodes are contained in the root graph as far as the underlying Agraph_t is concerned
    Agraph_t * graph = d->graph();
    for (Agnode_t * node = agnxtnode(graph, aghead(edge)); node; node = agnxtnode(graph, node)) {
        Agedge_t * first = agfstin(graph, node);
        if (first)
            return d->edgeToIndex(first);
    }
    return EdgeIndex();
}

QList<EdgeIndex> DotGraphModel::incidentEdges(const NodeIndex & idx) const
{
    void * agobj = d->agobjFromIndex(idx);
    if (agobjkind(agobj) != AGNODE)
        return QList<EdgeIndex>();

    Agnode_t * node = d->node_cast(agobj);
    Agraph_t * graph = d->graph();
    QList<EdgeIndex> edges;
    for (Agedge_t * edge = agfstedge(graph, node); edge; edge = agnxtedge(graph, edge, node)) {
        edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

QList<EdgeIndex> DotGraphModel::outgoingEdges(const NodeIndex & idx) const
{
    void * agobj = d->agobjFromIndex(idx);
    if (agobjkind(agobj) != AGNODE)
        return QList<EdgeIndex>();

    Agnode_t * node = d->node_cast(agobj);
    Agraph_t * graph = d->graph();
    QList<EdgeIndex> edges;
    for (Agedge_t * edge = agfstout(graph, node); edge; edge = agnxtout(graph, edge)) {
        edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

QList<EdgeIndex> DotGraphModel::incomingEdges(const NodeIndex & idx) const
{
    void * agobj = d->agobjFromIndex(idx);
    if (agobjkind(agobj) != AGNODE)
        return QList<EdgeIndex>();

    Agnode_t * node = d->node_cast(agobj);
    Agraph_t * graph = d->graph();
    QList<EdgeIndex> edges;
    for (Agedge_t * edge = agfstin(graph, node); edge; edge = agnxtin(graph, edge)) {
        edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

NodeIndex DotGraphModel::head(const EdgeIndex & idx) const
{
    Agedge_t * edge = d->edgeFromIndex(idx);
    return d->nodeToIndex(edge ? aghead(edge) : 0);
}

NodeIndex DotGraphModel::tail(const EdgeIndex & idx) const
{
    Agedge_t * edge = d->edgeFromIndex(idx);
    return d->nodeToIndex(edge ? agtail(edge) : 0);
}

EdgeIndex DotGraphModel::addEdge(const NodeIndex & tailidx, const NodeIndex & headidx)
{
    void * tailobj = d->agobjFromIndex(tailidx);
    void * headobj = d->agobjFromIndex(headidx);

    if (agobjkind(tailobj) != AGNODE || agobjkind(headobj) != AGNODE) {
        Q_ASSERT(false && "Calling DotGraphModel::addEdge on subgraphs");
        kWarning() << "Calling DotGraphModel::addEdge on subgraphs";
        return EdgeIndex();
    }

    Agraph_t * g = d->graph();
    Agnode_t * tail = d->node_cast(tailobj);
    Agnode_t * head = d->node_cast(headobj);

    Agedge_t * edge = agedge(g, tail, head);
    EdgeIndex idx = d->edgeToIndex(edge);
    emit edgeInserted(idx);
    return idx;
}

void DotGraphModel::removeEdge(const EdgeIndex & idx)
{
    Agedge_t * edge = d->edgeFromIndex(idx);
    if (!edge)
        return;

    emit edgeAboutToBeRemoved(idx);
    agdelete(d->graph(), edge);
}

NodeIndex DotGraphModel::addNode(const NodeIndex & parentidx, const QString & name)
{
    void * parentobj = d->agobjFromIndex(parentidx);

    if (agobjkind(parentobj) != AGGRAPH) {
        Q_ASSERT(false && "DotGraphModel::addNode: parent is a node");
        kWarning() << "DotGraphModel::addNode: parent is a node";
        return NodeIndex();
    }

    Agraph_t * parent = d->graph_cast(parentobj);
    QString realname;
    if (!name.isEmpty()) {
        if (agfindnode(d->graph(), name.toUtf8().data()) != NULL) {
            Q_ASSERT(false && "DotGraphModel::addNode: name already exists");
            kWarning() << "DotGraphModel::addNode: name already exists";
            return NodeIndex();
        }
        realname = name;
    } else {
        realname = d->unusedName(d->graph());
    }

    Agnode_t * node = agnode(parent, realname.toUtf8().data());
    NodeIndex idx = d->nodeToIndex(node);
    emit nodeInserted(idx);
    return idx;
}

NodeIndex DotGraphModel::addSubgraph(const NodeIndex & parentidx, const QString & name)
{
    void * parentobj = d->agobjFromIndex(parentidx);

    if (agobjkind(parentobj) != AGGRAPH) {
        Q_ASSERT(false && "DotGraphModel::addSubgraph: parent is a node");
        kWarning() << "DotGraphModel::addSubgraph: parent is a node";
        return NodeIndex();
    }

    Agraph_t * parent = d->graph_cast(parentobj);
    QString realname;
    if (!name.isEmpty()) {
        if (agfindsubg(d->graph(), name.toUtf8().data()) != NULL) {
            Q_ASSERT(false && "DotGraphModel::addSubgraph: name already exists");
            kWarning() << "DotGraphModel::addSubgraph: name already exists";
            return NodeIndex();
        }
        realname = name;
    } else {
        realname = d->unusedName(agmetanode(d->graph())->graph);
    }

    Agraph_t * g = agsubg(parent, realname.toUtf8().data());
    NodeIndex idx = d->nodeToIndex(g);
    emit nodeInserted(idx);
    return idx;
}

void DotGraphModel::removeNode(const NodeIndex & node)
{
    if (!node.isValid())
        return;

    for (;;) {
        NodeIndex firstChild = firstNode(node);
        if (!firstChild.isValid())
            break;
        removeNode(firstChild);
    }
    Q_FOREACH(const EdgeIndex & edge, incidentEdges(node)) {
        removeEdge(edge);
    }

    emit nodeAboutToBeRemoved(node);
    agdelete(d->graph(), d->agobjFromIndex(node));
}

QString DotGraphModel::nodeKey(const NodeIndex& node, const QString& key) const
{
    if (!node.isValid())
        return QString();

    void * agobj = d->agobjFromIndex(node);
    const char * result = agget(agobj, key.toUtf8().data());
    if (result)
        return QString(result);
    else
        return QString();
}

QString DotGraphModel::edgeKey(const EdgeIndex& idx, const QString& key) const
{
    if (!idx.isValid())
        return QString();

    Agedge_t * edge = d->edgeFromIndex(idx);
    const char * result = agget(edge, key.toUtf8().data());
    if (result)
        return QString(result);
    else
        return QString();
}

QVariant DotGraphModel::nodeData(const NodeIndex & node, int role) const
{
    if (!node.isValid())
        return QVariant();

    void * agobj = d->agobjFromIndex(node);

    if (agobjkind(agobj) == AGNODE) {
        Agnode_t * node = d->node_cast(agobj);
        switch (role) {
        case BoundingBoxRole: {
            const boxf & bb = ND_bb(node);
            return QRectF(bb.LL.x, bb.LL.y, (bb.UR.x - bb.LL.x), (bb.UR.y - bb.LL.y));
        }
        default:
            // fall-through
            break;
        }
    } else {
        Agraph_t * subgraph = d->graph_cast(agobj);
        switch (role) {
        case BoundingBoxRole: {
            const boxf & bb = GD_bb(subgraph);
            return QRectF(bb.LL.x, bb.LL.y, (bb.UR.x - bb.LL.x), (bb.UR.y - bb.LL.y));
        }
        default:
            // fall-through
            break;
        }
    }

    switch (role) {
    case Qt::DisplayRole: return nodeKey(node, "label");
    default:
        kWarning() << "DotGraphModel::nodeData: role " << role << " is not supported.";
        return QVariant();
    }
}

QVariant DotGraphModel::edgeData(const EdgeIndex & idx, int role) const
{
    if (!idx.isValid())
        return QVariant();

    //Agedge_t * edge = d->edgeFromIndex(idx);
    switch (role) {
    case Qt::DisplayRole: return edgeKey(idx, "label");;
    default:
        kWarning() << "DotGraphModel::edgeData: role " << role << " is not supported.";
        return QVariant();
    }
}

void DotGraphModel::setNodeKey(const NodeIndex & idx, const QString & key, const QString & value)
{
    if (!idx.isValid())
        return;

    void * node = d->agobjFromIndex(idx);
    if (agset(node, key.toUtf8().data(), value.toUtf8().data()) < 0) {
        kWarning() << "DotGraphModel::setNodeKey: failed to set key " << key;
    } else {
        emit nodeDataChanged(idx);
    }
}

void DotGraphModel::setEdgeKey(const EdgeIndex & idx, const QString& key, const QString& value)
{
    if (!idx.isValid())
        return;

    Agedge_t * edge = d->edgeFromIndex(idx);
    if (agset(edge, key.toUtf8().data(), value.toUtf8().data()) < 0) {
        kWarning() << "DotGraphModel::setEdgeKey: failed to set key " << key;
    } else {
        emit edgeDataChanged(idx);
    }
}

void DotGraphModel::setNodeData(const NodeIndex& idx, int role, const QVariant& data)
{
    switch (role) {
    case Qt::DisplayRole:
        setNodeKey(idx, "label", data.toString());
        break;

    default:
        kWarning() << "DotGraphModel::setNodeData: role " << role << " not supported.";
        break;
    }
}

void DotGraphModel::setEdgeData(const EdgeIndex& edge, int role, const QVariant& data)
{
    switch (role) {
    case Qt::DisplayRole:
        setEdgeKey(edge, "label", data.toString());
        break;

    default:
        kWarning() << "DotGraphModel::setEdgeData: role " << role << " not supported";
        break;
    }
}

void DotGraphModel::layout(const QString & layoutcommand)
{
    if (!d->graph_p)
        return;

    d->clearLayout();
    d->gvc = gvContext();
    gvLayout(d->gvc, d->graph(), layoutcommand.toUtf8().data());

    // Emit data change signals rather than modelReset,
    // because we want to indicate to listeners that node and edge index
    // remain unchaged. This should allow listeners to implement fancy animations
    // to reflect position changes and other nice things.
    d->emitAllDataChanged();
}

} // namespace KGraphViewer

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
