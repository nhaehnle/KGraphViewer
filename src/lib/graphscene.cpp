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

    virtual QGraphicsItem * createNodeItem(const NodeIndex& node);
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


class Node;

/**
 * List of shadow data for children of a node, implemented as pointers
 * because we don't want \ref Node to be copyable.
 *
 * This should really be replaced by a C++11 QList<std::unique_ptr<Node> >.
 */
class NodeList {
public:
    NodeList() {}
    ~NodeList();

    void clear();
    Node & createAt(int idx);
    void removeAt(int idx);
    int size() const {return list_.size();}

    Node & operator[](int idx) {return *list_[idx];}

private:
    QList<Node *> list_;

    Q_DISABLE_COPY(NodeList)
};

/**
 * Shadow data for each node, maintained by \ref GraphScene::Data.
 */
class Node {
public:
    QGraphicsItem * item;
    NodeList children;

    Node() : item(0) {}
    ~Node() {delete item; item = 0;}

private:
    Q_DISABLE_COPY(Node)
};

NodeList::~NodeList()
{
    clear();
}

void NodeList::clear()
{
    while (!list_.empty()) {
        delete list_.last();
        list_.pop_back();
    }
}

Node & NodeList::createAt(int idx)
{
    Node * node = new Node;
    list_.insert(idx, node);
    return *node;
}

void NodeList::removeAt(int idx)
{
    delete list_[idx];
    list_.removeAt(idx);
}

} // anonymous namespace

class GraphScene::Data : public QObject {
public:
    AbstractGraphModel * model;
    AbstractItemDelegate * customDelegate;
    AbstractItemDelegate * defaultDelegate;
    NodeList topLevelNodes;

    Data(GraphScene * scene) :
        QObject(scene),
        model(0),
        customDelegate(0),
        defaultDelegate(0)
    {
    }

    GraphScene * scene() const {return static_cast<GraphScene *>(parent());}
    AbstractItemDelegate * delegate();

    void setModel(AbstractGraphModel * model);
    void setCustomDelegate(AbstractItemDelegate * delegate);

    NodeList & childrenOfIndex(const NodeIndex & idx);
    Node & nodeAtIndex(const NodeIndex & idx);

    void clearItems();
    void buildItems();
    void buildChildren(const NodeIndex & parent, NodeList & list);
};

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
    if (model)
        clearItems();

    model = newModel;

    if (model)
        buildItems();
}

NodeList & GraphScene::Data::childrenOfIndex(const NodeIndex & idx)
{
    if (idx.isValid()) {
        return nodeAtIndex(idx).children;
    } else {
        return topLevelNodes;
    }
}

Node & GraphScene::Data::nodeAtIndex(const NodeIndex & node)
{
    Q_ASSERT(node.isValid());
    return childrenOfIndex(node.parent())[node.index()];
}

void GraphScene::Data::clearItems()
{
    topLevelNodes.clear();
}

void GraphScene::Data::buildItems()
{
    buildChildren(NodeIndex(), topLevelNodes);
}

void GraphScene::Data::buildChildren(const NodeIndex & parent, NodeList & list)
{
    AbstractItemDelegate * d = delegate();
    GraphScene * s = scene();

    for (NodeIndex nodeidx = model->firstNode(parent); nodeidx.isValid(); nodeidx = model->nextNode(nodeidx)) {
        Node & node = list.createAt(list.size());
        node.item = d->createNodeItem(nodeidx);
        s->addItem(node.item);
        buildChildren(nodeidx, node.children);
    }
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
 * Install a custom item delegate responsible.
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

// kate: space-indent on;indent-width 4;replace-tabs on
