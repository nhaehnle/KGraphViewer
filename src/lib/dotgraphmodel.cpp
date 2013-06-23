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

namespace KGraphViewer {

/*

EdgeIndex internal pointer is the Agedge_t.

For NodeIndex, we have to translate between the cgraph concepts of subgraphs and nodes,
and the AbstractGraphModel concept of a tree of nodes. This is achieved by adding a
Node struct to every Agnode_t and every Agraph_t as an internal attribute.


*/

namespace {

static const char NODE[] = "KGraphViewer::Node";
struct Node {
    Agrec_t hdr;
    void * prev; // either Agnode_t or Agraph_t
    void * next;
    void * firstChild;
};

} // anonymous namespace

class DotGraphModel::Data {
public:
    DotGraphModel & model;
    Agraph_t * graph_p;
    GVC_t * gvc;

    Data(DotGraphModel & m);
    ~Data();

    void clear();
    void clearLayout();
    Agraph_t * graph();
    void setGraph(Agraph_t * agraph);
    void registerGraph(Agraph_t * graph);
    void doRegisterAgraph(Agraph_t * graph);
    void doRegisterAgnode(Agnode_t * node);

    void * nodeAgobj(const NodeIndex & idx);
    Node * nodeRecord(const NodeIndex & idx);
    Node * nodeRecord(void * agobj);
    NodeIndex nodeObjToIndex(void * agobj);

    Agedge_t * edgeFromIndex(const EdgeIndex & idx) {return idx.isValid() ? model.edgeIndexInternalPtr(idx) : 0;}
    EdgeIndex edgeToIndex(Agedge_t * edge) {return edge ? model.makeEdgeIndex(edge) : EdgeIndex();}

    void emitAllDataChanged();
    void emitNodeChanged(const NodeIndex & node);
};

DotGraphModel::Data::Data(DotGraphModel & m) :
    model(m),
    graph_p(0),
    gvc(0)
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
        graph_p = agopen("", AGDIGRAPH);
        registerGraph(graph_p);
    }
    return graph_p;
}

void DotGraphModel::Data::setGraph(Agraph_t* agraph)
{
    clear();
    graph_p = agraph;
    registerGraph(agraph);
}

void DotGraphModel::Data::registerGraph(Agraph_t* g)
{
    doRegisterAgraph(g);

    for (Agnode_t * node = agfstnode(g); node; node = agnxtnode(g, node))
        doRegisterAgnode(node);
}

void DotGraphModel::Data::doRegisterAgraph(Agraph_t* g)
{
    Q_ASSERT(aggetrec(g, NODE, FALSE) == 0);
    Node * node = agbindrec(g, NODE, sizeof(Node), FALSE);
    node->firstChild = 0;
    node->prev = 0;
    node->next = 0;

    Agraph_t * parent = agparent(g);
    if (parent && parent != g) {
        Node * parentRec = nodeRecord(parent);
        if (parentRec->firstChild) {
            node->next = parentRec->firstChild;
            nodeRecord(node->next)->prev = g;
        }
        parentRec->firstChild = g;
    }

    for (Agraph_t * subg = agfstsubg(g); subg; subg = agnxtsubg(subg))
        doRegisterAgraph(subg);
}

void DotGraphModel::Data::doRegisterAgnode(Agnode_t* node)
{
    Q_ASSERT(aggetrec(g, NODE, FALSE) == 0);
    Node * nodeRec = agbindrec(node, NODE, sizeof(Node), FALSE);
    nodeRec->firstChild = 0;
    nodeRec->prev = 0;

    Agraph_t * graph = agraphof(node);
    Node * parentRec = nodeRecord(graph);
    nodeRec->next = parentRec->firstChild;
    if (nodeRec->next)
        nodeRecord(nodeRec->next)->prev = node;
    parentRec->firstChild = node;
}

void* DotGraphModel::Data::nodeAgobj(const NodeIndex & idx)
{
    return idx.isValid() ? model.nodeIndexInternalPtr(idx) : graph();
}

Node * DotGraphModel::Data::nodeRecord(void * agobj)
{
    Node * node = aggetrec(agobj, NODE, FALSE);
    Q_ASSERT(node != 0);
    return node;
}

Node * DotGraphModel::Data::nodeRecord(const NodeIndex & idx)
{
    return nodeRecord(nodeAgobj(idx));
}

NodeIndex DotGraphModel::Data::nodeObjToIndex(void* agobj)
{
    return (agobj && agobj != graph_p) ? model.makeNodeIndex(agobj) : NodeIndex();
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


DotGraphModel::DotGraphModel(QObject* parent) :
    AbstractGraphModel(parent),
    d(new Data)
{
}

DotGraphModel::DotGraphModel(Agraph_t* agraph, QObject* parent) :
    AbstractGraphModel(parent),
    d(new Data)
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
    Node * node = d->nodeRecord(idx);
    return d->nodeObjToIndex(node->firstChild);
}

NodeIndex DotGraphModel::nextNode(const NodeIndex & idx) const
{
    Node * node = d->nodeRecord(idx);
    return d->nodeObjToIndex(node->next);
}

NodeIndex DotGraphModel::parent(const NodeIndex & idx) const
{
    void * agobj = d->nodeAgobj(idx);
    int kind = agobjkind(agobj);
    Q_ASSERT(kind == AGNODE || kind == AGGRAPH);

    if (kind == AGNODE) {
        return d->nodeObjToIndex(agraphof(agobj));
    } else {
        return d->nodeObjToIndex(agparent(static_cast<Agraph_t *>(agobj)));
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

    Agedge_t * nextedge = agnxtin(graph, edge);
    if (nextedge)
        return d->edgeToIndex(nextedge);

    // Remember that all nodes are contained in the root graph as far as the underlying Agraph_t is concerned
    Agraph_t * graph = d->graph();
    for (Agnode_t * node = agnxtnode(graph, aghead(edge)); node; node = agnxtnode(graph, node)) {
        Agedge_t * first = agfstin(graph, node);
        if (first)
            return d->edgeToIndex(first);
    }
    return EdgeIndex;
}

QList<EdgeIndex> DotGraphModel::incidentEdges(const NodeIndex & idx) const
{
    void * agobj = d->nodeAgobj(idx);
    if (agobjkind(agobj) != AGNODE)
        return QList<EdgeIndex>();

    Agnode_t * node = static_cast<Agnode_t *>(agobj);
    Agraph_t * graph = d->graph();
    QList<EdgeIndex> edges;
    for (Agedge_t * edge = agfstedge(graph, node); edge; edge = agnxtedge(graph, edge, node)) {
        edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

QList<EdgeIndex> DotGraphModel::outgoingEdges(const NodeIndex & idx) const
{
    void * agobj = d->nodeAgobj(idx);
    if (agobjkind(agobj) != AGNODE)
        return QList<EdgeIndex>();

    Agnode_t * node = static_cast<Agnode_t *>(agobj);
    Agraph_t * graph = d->graph();
    QList<EdgeIndex> edges;
    for (Agedge_t * edge = agfstout(graph, node); edge; edge = agnxtout(graph, edge)) {
        edges.push_back(d->edgeToIndex(edge));
    }
    return edges;
}

QList<EdgeIndex> DotGraphModel::incomingEdges(const NodeIndex & idx) const
{
    void * agobj = d->nodeAgobj(idx);
    if (agobjkind(agobj) != AGNODE)
        return QList<EdgeIndex>();

    Agnode_t * node = static_cast<Agnode_t *>(agobj);
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
    return d->nodeObjToIndex(edge ? aghead(edge) : 0);
}

NodeIndex DotGraphModel::tail(const EdgeIndex & idx) const
{
    Agedge_t * edge = d->edgeFromIndex(idx);
    return d->nodeObjToIndex(edge ? agtail(edge) : 0);
}

EdgeIndex DotGraphModel::addEdge(const NodeIndex & tailidx, const NodeIndex & headidx, const QString & name)
{
    void * tailobj = d->nodeAgobj(tailidx);
    void * headobj = d->nodeAgobj(headidx);

    if (agobjkind(tailobj) != AGNODE || agobjkind(headobj) != AGNODE) {
        Q_ASSERT(false && "Calling DotGraphModel::addEdge on subgraphs");
        kWarning() << "Calling DotGraphModel::addEdge on subgraphs";
        return EdgeIndex();
    }

    Agraph_t * g = d->graph();
    Agnode_t * tail = static_cast<Agnode_t *>(tailobj);
    Agnode_t * head = static_cast<Agnode_t *>(headobj);

    QByteArray nameBytes = name.toUtf8();
    const char * name = name.isEmpty() ? 0 : nameBytes.data();
    if (name && agedge(g, tail, head, name, FALSE) != NULL) {
        Q_ASSERT(false && "DotGraphModel::addEdge: name already exists");
        kWarning() << "DotGraphModel::addEdge: name already exists";
        return EdgeIndex();
    }

    Agedge_t * edge = agedge(g, tail, head, name, TRUE);
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
    agdeledge(d->graph(), edge);
}

NodeIndex DotGraphModel::addNode(const NodeIndex & parentidx, const QString & name)
{
    void * parentobj = d->nodeAgobj(parentidx);

    if (agobjkind(parentobj) != AGGRAPH) {
        Q_ASSERT(false && "DotGraphModel::addNode: parent is a node");
        kWarning() << "DotGraphModel::addNode: parent is a node";
        return NodeIndex();
    }

    Agraph_t * parent = static_cast<Agraph_t *>(parentobj);

    QByteArray nameBytes = name.toUtf8();
    const char * name = name.isEmpty() ? 0 : nameBytes.data();
    if (name && agnode(d->graph(), name, FALSE) != NULL) {
        Q_ASSERT(false && "DotGraphModel::addNode: name already exists");
        kWarning() << "DotGraphModel::addNode: name already exists";
        return NodeIndex();
    }

    Agnode_t * node = agnode(parent, name, TRUE);
    d->doRegisterAgnode(node);
    NodeIndex idx = d->nodeObjToIndex(node);
    emit nodeInserted(idx);
    return idx;
}

NodeIndex DotGraphModel::addSubgraph(const NodeIndex & parentidx, const QString & name)
{
    void * parentobj = d->nodeAgobj(parentidx);

    if (agobjkind(parentobj) != AGGRAPH) {
        Q_ASSERT(false && "DotGraphModel::addSubgraph: parent is a node");
        kWarning() << "DotGraphModel::addSubgraph: parent is a node";
        return NodeIndex();
    }

    Agraph_t * parent = static_cast<Agraph_t *>(parentobj);

    QByteArray nameBytes = name.toUtf8();
    const char * name = name.isEmpty() ? 0 : nameBytes.data();
    if (name && agsubg(d->graph(), name, FALSE) != NULL) {
        Q_ASSERT(false && "DotGraphModel::addSubgraph: name already exists");
        kWarning() << "DotGraphModel::addSubgraph: name already exists";
        return NodeIndex();
    }

    Agraph_t * g = agsubg(parent, name, TRUE);
    d->doRegisterAgraph(g);
    NodeIndex idx = d->nodeObjToIndex(g);
    emit nodeInserted(idx);
    return idx;
}

void DotGraphModel::removeNode(const NodeIndex & node)
{
    if (!node.isValid())
        return;

    while (NodeIndex firstChild = firstNode(node)) {
        removeNode(firstChild);
    }
    Q_FOREACH(const EdgeIndex & edge, incidentEdges(node)) {
        removeEdge(edge);
    }

    emit nodeAboutToBeRemoved(node);

    void * nodeObj = d->nodeAgobj(node);
    Node * nodeRec = d->nodeRecord(nodeObj);
    Q_ASSERT(!nodeRec->firstChild);

    if (nodeRec->next)
        d->nodeRecord(nodeRec->next)->prev = nodeRec->prev;
    if (nodeRec->prev) {
        d->nodeRecord(nodeRec->prev)->next = nodeRec->next;
    } else {
        Node * parentRec = d->nodeRecord(parent(node));
        Q_ASSERT(parentRec && parentRec != nodeRec);
        Q_ASSERT(parentRec->firstChild == nodeObj);
        parentRec->firstChild = nodeRec->next;
    }
    agdelete(d->graph(), nodeObj);
}

QString DotGraphModel::nodeKey(const NodeIndex& node, const QString& key)
{
    if (!node.isValid())
        return QString();

    void * agobj = d->nodeObjToIndex(node);
    const char * result = agget(agobj, key.toUtf8().data());
    if (result)
        return QString(result);
    else
        return QString();
}

QString DotGraphModel::edgeKey(const EdgeIndex& idx, const QString& key)
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

    //TODO: consider speeding this up via agxget
    void * agobj = d->nodeAgobj(node);
    switch (role) {
    case Qt::DisplayRole: return nodeKey(node, "label");
    case BoundingBoxRole: {
        float w = ND_width(agobj);
        float h = ND_height(agobj);
        float cx = ND_coord(agobj).c;
        float cy = ND_coord(agobj).c;
        return QRectF(cx - w/2, cy - h/2, w, h);
    }
    default:
        kWarning() << "DotGraphModel::nodeData: role " << role << " is not supported.";
        return QVariant();
    }
}

QVariant DotGraphModel::edgeData(const EdgeIndex & idx, int role) const
{
    if (!idx.isValid())
        return QVariant();

    //TODO: consider speeding this up via agxget
    Agedge_t * edge = d->edgeFromIndex(idx);
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

    void * node = d->nodeAgobj(idx);
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

    // Emit loads of data change signals rather than modelReset,
    // because we want to indicate to listeners that node and edge index
    // remain unchaged. This should allow listeners to implement fancy animations
    // to reflect position changes and other nice things.
    d->emitAllDataChanged();
}

} // namespace KGraphViewer

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
