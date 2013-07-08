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


#include "graphscene.h"

#include "abstractgraphmodel.h"

#include <QGraphicsItem>
#include <QSet>
#include <QTimer>

namespace KGraphViewer {

AbstractItemDelegate::AbstractItemDelegate(QObject * parent) : QObject(parent)
{
}

AbstractItemDelegate::~AbstractItemDelegate()
{
}

namespace {

/**
 * Default rendering of graph nodes and edges.
 */
class DefaultItemDelegate : public AbstractItemDelegate {
public:
    explicit DefaultItemDelegate(QObject * parent = 0) : AbstractItemDelegate(parent) {}

    virtual QGraphicsItem* createNodeItem(const NodeIndex& node);
    virtual QGraphicsItem* createEdgeItem(const EdgeIndex& edge);
};

QGraphicsItem * DefaultItemDelegate::createNodeItem(const NodeIndex & node)
{
    QRectF bbox = node.data(BoundingBoxRole).toRectF();
    QGraphicsRectItem * rect = new QGraphicsRectItem(bbox);

    QString text = node.data(Qt::DisplayRole).toString();
    if (!text.isEmpty()) {
        QGraphicsSimpleTextItem * label = new QGraphicsSimpleTextItem(text, rect);
        QRectF label_bbox = label->boundingRect();
        label->setPos(bbox.center() - label_bbox.center());
    }

    return rect;
}

QGraphicsItem* DefaultItemDelegate::createEdgeItem(const EdgeIndex& edge)
{
    QLineF line;
    line.setP1(edge.data(TailPosRole).toPointF());
    line.setP2(edge.data(HeadPosRole).toPointF());
    QGraphicsLineItem * item = new QGraphicsLineItem(line);
    return item;
}

} // anonymous namespace

class GraphScene::Data : public QObject {
    Q_OBJECT

public:
    AbstractGraphModel * model;
    AbstractItemDelegate * customDelegate;
    AbstractItemDelegate * defaultDelegate;
    QHash<NodeIndex, QGraphicsItem *> nodeItems;
    QHash<EdgeIndex, QGraphicsItem *> edgeItems;
    QSet<NodeIndex> updateNodes;
    QSet<EdgeIndex> updateEdges;
    QTimer updateTimer;

    Data(GraphScene * scene);

    GraphScene * scene() const {return static_cast<GraphScene *>(parent());}
    AbstractItemDelegate * delegate();

    void setModel(AbstractGraphModel * model);
    void setCustomDelegate(AbstractItemDelegate * delegate);

    void clearItems();
    void buildItems();
    void buildChildren(const NodeIndex & parent);

public Q_SLOTS:
    void modelReset();
    void nodeDataChanged(KGraphViewer::NodeIndex node);
    void nodeInserted(KGraphViewer::NodeIndex node);
    void nodeAboutToBeRemoved(const KGraphViewer::NodeIndex& node);
    void edgeDataChanged(KGraphViewer::EdgeIndex edge);
    void edgeInserted(KGraphViewer::EdgeIndex edge);
    void edgeAboutToBeRemoved(KGraphViewer::EdgeIndex node);
    void delayedUpdates();
};

GraphScene::Data::Data(GraphScene * scene) :
    QObject(scene),
    model(0),
    customDelegate(0),
    defaultDelegate(0)
{
    updateTimer.setSingleShot(true);
    connect(&updateTimer, SIGNAL(timeout()), SLOT(delayedUpdates()));
}

AbstractItemDelegate * GraphScene::Data::delegate()
{
    if (customDelegate)
        return customDelegate;
    if (!defaultDelegate)
        defaultDelegate = new DefaultItemDelegate(this);
    return defaultDelegate;
}

void GraphScene::Data::setCustomDelegate(AbstractItemDelegate * delegate)
{
    if (delegate == customDelegate)
        return;

    customDelegate = delegate;
    if (model) {
        clearItems();
        buildItems();
    }
}

void GraphScene::Data::setModel(AbstractGraphModel * newModel)
{
    if (newModel == model)
        return;

    if (model) {
        clearItems();
        disconnect(model, 0, this, 0);
    }

    model = newModel;

    if (model) {
        buildItems();
        connect(model, SIGNAL(modelReset()), SLOT(modelReset()));
        connect(model, SIGNAL(nodeInserted(KGraphViewer::NodeIndex)), SLOT(nodeInserted(KGraphViewer::NodeIndex)));
        connect(model, SIGNAL(nodeAboutToBeRemoved(KGraphViewer::NodeIndex)), SLOT(nodeAboutToBeRemoved(KGraphViewer::NodeIndex)));
        connect(model, SIGNAL(nodeDataChanged(KGraphViewer::NodeIndex)), SLOT(nodeDataChanged(KGraphViewer::NodeIndex)));
        connect(model, SIGNAL(edgeInserted(KGraphViewer::EdgeIndex)), SLOT(edgeInserted(KGraphViewer::EdgeIndex)));
        connect(model, SIGNAL(edgeAboutToBeRemoved(KGraphViewer::EdgeIndex)), SLOT(edgeAboutToBeRemoved(KGraphViewer::EdgeIndex)));
        connect(model, SIGNAL(edgeDataChanged(KGraphViewer::EdgeIndex)), SLOT(edgeDataChanged(KGraphViewer::EdgeIndex)));
    }
}

void GraphScene::Data::clearItems()
{
    Q_FOREACH (QGraphicsItem * item, edgeItems.values())
        delete item;
    edgeItems.clear();

    Q_FOREACH (QGraphicsItem * item, nodeItems.values())
        delete item;
    nodeItems.clear();

    updateNodes.clear();
    updateEdges.clear();
}

void GraphScene::Data::buildItems()
{
    buildChildren(NodeIndex());

    AbstractItemDelegate * d = delegate();
    GraphScene * s = scene();

    for (EdgeIndex edgeidx = model->firstEdge(); edgeidx.isValid(); edgeidx = model->nextEdge(edgeidx)) {
        QGraphicsItem * item = d->createEdgeItem(edgeidx);
        s->addItem(item);
        edgeItems.insert(edgeidx, item);
    }
}

void GraphScene::Data::buildChildren(const NodeIndex & parent)
{
    AbstractItemDelegate * d = delegate();
    GraphScene * s = scene();

    for (NodeIndex nodeidx = model->firstNode(parent); nodeidx.isValid(); nodeidx = model->nextNode(nodeidx)) {
        QGraphicsItem * item = d->createNodeItem(nodeidx);
        s->addItem(item);
        nodeItems.insert(nodeidx, item);
        buildChildren(nodeidx);
    }
}

void GraphScene::Data::modelReset()
{
    clearItems();
    buildItems();
}

void GraphScene::Data::nodeAboutToBeRemoved(const NodeIndex& node)
{
    if (nodeItems.contains(node)) {
        delete nodeItems[node];
        nodeItems.remove(node);
    }
    updateNodes.remove(node);
}

void GraphScene::Data::nodeInserted(NodeIndex node)
{
    updateNodes.insert(node);
    updateTimer.start();
}

void GraphScene::Data::nodeDataChanged(NodeIndex node)
{
    updateNodes.insert(node);
    updateTimer.start();
}

void GraphScene::Data::edgeAboutToBeRemoved(EdgeIndex edge)
{
    if (edgeItems.contains(edge)) {
        delete edgeItems[edge];
        edgeItems.remove(edge);
    }
    updateEdges.remove(edge);
}

void GraphScene::Data::edgeInserted(EdgeIndex edge)
{
    updateEdges.insert(edge);
    updateTimer.start();
}

void GraphScene::Data::edgeDataChanged(EdgeIndex edge)
{
    updateEdges.insert(edge);
    updateTimer.start();
}

void GraphScene::Data::delayedUpdates()
{
    AbstractItemDelegate * d = delegate();
    GraphScene * s = scene();

    Q_FOREACH (const NodeIndex & node, updateNodes) {
        if (nodeItems.contains(node))
            delete nodeItems[node];
        QGraphicsItem * item = d->createNodeItem(node);
        s->addItem(item);
        nodeItems[node] = item;
    }
    updateNodes.clear();

    Q_FOREACH (const EdgeIndex & edge, updateEdges) {
        if (edgeItems.contains(edge))
            delete edgeItems[edge];
        QGraphicsItem * item = d->createEdgeItem(edge);
        s->addItem(item);
        edgeItems[edge] = item;
    }
    updateEdges.clear();
}


GraphScene::GraphScene(QObject* parent) :
    QGraphicsScene(parent),
    d(new Data(this))
{
}

GraphScene::~GraphScene()
{
}

AbstractGraphModel* GraphScene::model() const
{
    return d->model;
}

void GraphScene::setModel(AbstractGraphModel * model)
{
    if (model == d->model)
        return;

    d->setModel(model);
}

/**
 * Install a custom item delegate.
 *
 * Any existing delegate will be removed but not deleted. GraphScene does not take ownership
 * of the given delegate.
 *
 * Already created items will be re-created, which means that you should
 * set the delegate \em before setting the model when possible.
 */
void GraphScene::setItemDelegate(AbstractItemDelegate * delegate)
{
    d->setCustomDelegate(delegate);
}

AbstractItemDelegate * GraphScene::itemDelegate() const
{
    return d->delegate();
}

} // namespace KGraphViewer

#include "graphscene.moc"

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
