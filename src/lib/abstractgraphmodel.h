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

#ifndef KGRAPHVIEWER_ABSTRACTGRAPHMODEL_H
#define KGRAPHVIEWER_ABSTRACTGRAPHMODEL_H

#include "kgraphviewer_export.h"

#include <QObject>
#include <QVariant>

namespace KGraphViewer {

enum DataRole {
    BoundingBoxRole = Qt::UserRole,
    HeadPosRole = Qt::UserRole + 1,
    TailPosRole = Qt::UserRole + 2,

    UserRole = Qt::UserRole + 16
};

class AbstractGraphModel;

class KGRAPHVIEWER_EXPORT NodeIndex {
public:
    NodeIndex() : m(0), i(-1), p(0) {}
    ~NodeIndex() {m = 0; p = 0;}

    bool isValid() const {return (i >= 0) && (m != 0);}
    const AbstractGraphModel * model() const {return m;}
    QVariant data(int role) const;
    NodeIndex parent() const;
    bool operator==(const NodeIndex & rhs) const {
        return (rhs.i == i) && (rhs.p == p) && (rhs.m == m);
    }
    bool operator!=(const NodeIndex & rhs) const {
        return !(*this == rhs);
    }

    int index() const {return i;}

private:
    friend class AbstractGraphModel;
    NodeIndex(const AbstractGraphModel * m_, int i_, void *p_) : m(m_), i(i_), p(p_) {}

    const AbstractGraphModel * m;
    int i;
    void * p;
};

class KGRAPHVIEWER_EXPORT EdgeIndex {
public:
    EdgeIndex() : m(0), i(-1), p(0) {}
    ~EdgeIndex() {m = 0; p = 0;}
    bool isValid() const {return (i >= 0) && (m != 0);}
    const AbstractGraphModel * model() const {return m;}
    QVariant data(int role) const;
    bool operator==(const EdgeIndex & rhs) const {
        return (rhs.i == i) && (rhs.p == p) && (rhs.m == m);
    }
    bool operator!=(const EdgeIndex & rhs) const {
        return !(*this == rhs);
    }

    int index() const {return i;}

private:
    friend class AbstractGraphModel;
    EdgeIndex(const AbstractGraphModel * m_, int i_, void * p_) : m(m_), i(i_), p(p_) {}

    const AbstractGraphModel * m;
    int i;
    void * p;
};

/**
 * Model of a (directed, multi-) graph, possibly with nested nodes.
 *
 * Node positions are given by their bounding boxes.
 * The head and tail position of edges is relative to the top-left corner of the bounding box
 * of the corresponding node.
 *
 * \warning EXPERIMENTAL! Absolutely no binary compatibility guarantees.
 */
class KGRAPHVIEWER_EXPORT AbstractGraphModel : public QObject {
    Q_OBJECT

public:
    enum Attribute {
        NoAttributes = 0x0,
        IsEditable = 0x1,
    };
    Q_DECLARE_FLAGS(Attributes, Attribute);

    explicit AbstractGraphModel(QObject * parent = 0);
    virtual ~AbstractGraphModel();

    virtual Attributes attributes() const;

    virtual QVariant nodeData(const NodeIndex & node, int role) const = 0;
    virtual Qt::ItemFlags nodeFlags(const NodeIndex & node) const;
    virtual QVariant edgeData(const EdgeIndex & edge, int role) const = 0;
    virtual Qt::ItemFlags edgeFlags(const EdgeIndex & edge) const;

    /**
     * Returns the first child of the given node,
     * or the first top-level node if \p node is invalid.
     */
    virtual NodeIndex firstNode(const NodeIndex & node) const = 0;

    /**
     * Return the next child in the given node's direct parent.
     *
     * The behavior of this function is undefined if the model changes between calls.
     */
    virtual NodeIndex nextNode(const NodeIndex & node) const = 0;

    /**
     * Return the parent of the given node.
     */
    virtual NodeIndex parent(const NodeIndex & node) const = 0;

    virtual NodeIndex leastCommonAncestor(const NodeIndex & a, const NodeIndex & b) const;

    /**
     * Return the first edge from the global list of edges.
     */
    virtual EdgeIndex firstEdge() const = 0;

    /**
     * Return the next edge from the global list of edges.
     */
    virtual EdgeIndex nextEdge(const EdgeIndex & node) const = 0;

    virtual QList<EdgeIndex> incidentEdges(const NodeIndex & node) const;
    virtual QList<EdgeIndex> outgoingEdges(const NodeIndex & node) const = 0;
    virtual QList<EdgeIndex> incomingEdges(const NodeIndex & node) const = 0;
    virtual NodeIndex head(const EdgeIndex & edge) const = 0;
    virtual NodeIndex tail(const EdgeIndex & edge) const = 0;

    //TODO: editing functions

Q_SIGNALS:
    void modelAboutToBeReset();
    void modelReset();

    void nodeDataChanged(const NodeIndex & node);
    void nodeAboutToBeRemoved(const NodeIndex & parent, int idx);
    void nodeRemoved(const NodeIndex & parent, int idx);
    void nodeAboutToBeInserted(const NodeIndex & parent, int idx);
    void nodeInserted(const NodeIndex & parent, int idxt);

    void edgeDataChanged(const EdgeIndex & edge);
    void edgeAboutToBeRemoved(int idx);
    void edgeRemoved(int idx);
    void edgeAboutToBeInserted(int idx);
    void edgeInserted(int idx);

protected:
    void beginResetModel();
    void endResetModel();
    void beginRemoveNode(const NodeIndex & parent, int idx);
    void endRemoveNode();
    void beginInsertNode(const NodeIndex & parent, int idx);
    void endInsertNode();
    void beginRemoveEdge(int idx);
    void endRemoveEdge();
    void beginInsertEdge(int idx);
    void endInsertEdge();

    void * nodeIndexInternalPtr(const NodeIndex & node) const {return node.p;}
    void * edgeIndexInternalPtr(const EdgeIndex & edge) const {return edge.p;}
    NodeIndex makeNodeIndex(int index, void * p = 0) const;
    EdgeIndex makeEdgeIndex(int index, void * p = 0) const;

private:
    class Data;
    QScopedPointer<Data> d;
};

/* **************************************************************************

    INLINE IMPLEMENTATIONS

************************************************************************** */

inline QVariant NodeIndex::data(int role) const
{
    if (m)
        return m->nodeData(*this, role);
    else
        return QVariant();
}

inline NodeIndex NodeIndex::parent() const
{
    if (m)
        return m->parent(*this);
    else
        return NodeIndex();
}

inline QVariant EdgeIndex::data(int role) const
{
    if (m)
        return m->edgeData(*this, role);
    else
        return QVariant();
}

} // namespace KGraphViewer

#endif // KGRAPHVIEWER_ABSTRACTGRAPHMODEL_H

// kate: space-indent on;indent-width 4;replace-tabs on

struct D;
