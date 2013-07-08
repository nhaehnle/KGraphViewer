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
    /** Bounding box of a node in global coordinates.
     */
    BoundingBoxRole = Qt::UserRole,

    /** Global position of the head of an edge.
     */
    HeadPosRole = Qt::UserRole + 1,

    /** Global position of the tail of an edge.
     */
    TailPosRole = Qt::UserRole + 2,

    UserRole = Qt::UserRole + 16
};

class AbstractGraphModel;

class KGRAPHVIEWER_EXPORT NodeIndex {
public:
    NodeIndex() : m(0), p(0) {}
    ~NodeIndex() {m = 0; p = 0;}

    bool isValid() const {return m != 0;}
    const AbstractGraphModel * model() const {return m;}
    QVariant data(int role) const;
    NodeIndex parent() const;
    bool operator==(const NodeIndex & rhs) const {
        return (rhs.p == p) && (rhs.m == m);
    }
    bool operator!=(const NodeIndex & rhs) const {
        return !(*this == rhs);
    }
    bool operator<(const NodeIndex & rhs) const {
        return (p < rhs.p) || (p == rhs.p && m < rhs.m);
    }
    uint qHash() const {return ::qHash(m) ^ ::qHash(p);}

private:
    friend class AbstractGraphModel;
    NodeIndex(const AbstractGraphModel * m_, void *p_) : m(m_), p(p_) {}
    NodeIndex(const AbstractGraphModel * m_, int i_) : m(m_), p(reinterpret_cast<void *>(i_)) {}

    const AbstractGraphModel * m;
    void * p;
};

inline uint qHash(const KGraphViewer::NodeIndex & idx) {return idx.qHash();}


class KGRAPHVIEWER_EXPORT EdgeIndex {
public:
    EdgeIndex() : m(0), p(0) {}
    ~EdgeIndex() {m = 0; p = 0;}
    bool isValid() const {return (m != 0);}
    const AbstractGraphModel * model() const {return m;}
    QVariant data(int role) const;
    bool operator==(const EdgeIndex & rhs) const {
        return (rhs.p == p) && (rhs.m == m);
    }
    bool operator!=(const EdgeIndex & rhs) const {
        return !(*this == rhs);
    }
    bool operator<(const EdgeIndex & rhs) const {
        return (p < rhs.p) || (p == rhs.p && m < rhs.m);
    }
    uint qHash() const {return ::qHash(m) ^ ::qHash(p);}

private:
    friend class AbstractGraphModel;
    EdgeIndex(const AbstractGraphModel * m_, void * p_) : m(m_), p(p_) {}

    const AbstractGraphModel * m;
    void * p;
};

inline uint qHash(const KGraphViewer::EdgeIndex & idx) {return idx.qHash();}


/**
 * Model of a (directed, multi-) graph, possibly with nested nodes.
 *
 * Implementations must ensure that NodeIndex and EdgeIndex instances are stable
 * and remain valid unless the corresponding node or edge are removed.
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
    void modelReset();

    void nodeDataChanged(const NodeIndex & node);
    void nodeAboutToBeRemoved(const NodeIndex & node);
    void nodeInserted(const NodeIndex & node);

    void edgeDataChanged(const EdgeIndex & edge);
    void edgeAboutToBeRemoved(const EdgeIndex & edge);
    void edgeInserted(const EdgeIndex & edge);

protected:
    void * nodeIndexInternalPtr(const NodeIndex & node) const {Q_ASSERT(node.m == this); return node.p;}
    void * edgeIndexInternalPtr(const EdgeIndex & edge) const {Q_ASSERT(edge.m == this); return edge.p;}
    int nodeIndexInternalInt(const NodeIndex & node) const {Q_ASSERT(node.m == this); return reinterpret_cast<size_t>(&node.p);}
    int edgeIndexInternalInt(const EdgeIndex & edge) const {Q_ASSERT(edge.m == this); return reinterpret_cast<size_t>(&edge.p);}
    NodeIndex makeNodeIndex(void * p) const {return NodeIndex(this, p);}
    NodeIndex makeNodeIndex(int i) const  {return NodeIndex(this, reinterpret_cast<void *>(i));}
    EdgeIndex makeEdgeIndex(void * p) const {return EdgeIndex(this, p);}
    EdgeIndex makeEdgeIndex(int i) const {return EdgeIndex(this, reinterpret_cast<void *>(i));}
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

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
