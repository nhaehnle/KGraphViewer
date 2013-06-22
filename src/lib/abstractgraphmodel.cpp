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

#include "abstractgraphmodel.h"

#include <KDebug>

namespace KGraphViewer {

class AbstractGraphModel::Data {
public:
    enum TwoPhaseSignal {
        NoSignal,
        ModelResetSignal,
        NodeInsertedSignal,
        NodeRemovedSignal,
        EdgeInsertedSignal,
        EdgeRemovedSignal
    };

    TwoPhaseSignal twoPhaseSignal;
    NodeIndex signalParent;
    int signalIndex;

    Data() : twoPhaseSignal(NoSignal), signalParent(), signalIndex(-1) {}
    void beginSignal(TwoPhaseSignal sig);
    void endSignal(TwoPhaseSignal sig);
};

void AbstractGraphModel::Data::beginSignal(AbstractGraphModel::Data::TwoPhaseSignal sig)
{
    Q_ASSERT(twoPhaseSignal == NoSignal);
    if (twoPhaseSignal != NoSignal) {
        kWarning() << "AbstractGraphModel::beginSignal: called without closing the previous signal";
    }

    twoPhaseSignal = sig;
}

void AbstractGraphModel::Data::endSignal(AbstractGraphModel::Data::TwoPhaseSignal sig)
{
    Q_ASSERT(twoPhaseSignal == sig);
    if (twoPhaseSignal != sig) {
        kWarning() << "AbstractGraphModel::endSignal: called for bad signal";
    }

    twoPhaseSignal = NoSignal;
}


AbstractGraphModel::AbstractGraphModel(QObject * parent) :
    QObject(parent),
    d(new Data)
{
}

AbstractGraphModel::~AbstractGraphModel()
{
}

AbstractGraphModel::Attributes AbstractGraphModel::attributes() const
{
    return NoAttributes;
}

Qt::ItemFlags AbstractGraphModel::nodeFlags(const NodeIndex& node) const
{
    Q_UNUSED(node);
    return Qt::NoItemFlags;
}

Qt::ItemFlags AbstractGraphModel::edgeFlags(const EdgeIndex& edge) const
{
    Q_UNUSED(edge);
    return Qt::NoItemFlags;
}

NodeIndex AbstractGraphModel::leastCommonAncestor(const NodeIndex& a_in, const NodeIndex& b_in) const
{
    QList<NodeIndex> as, bs;
    NodeIndex a = a_in;
    NodeIndex b = b_in;
    while (a.isValid()) {
        as.append(a);
        a = parent(a);
    }
    while (b.isValid()) {
        bs.append(b);
        b = parent(b);
    }

    if (as.size() < bs.size()) {
        bs.erase(bs.begin(), as.begin() + (bs.size() - as.size()));
    } else if (bs.size() < as.size()) {
        as.erase(as.begin(), as.begin() + (as.size() - bs.size()));
    }

    for (QList<NodeIndex>::const_iterator itas = as.begin(), itbs = bs.begin(); itas != as.end(); ++itas, ++itbs) {
        Q_ASSERT(itbs != bs.end());
        if (*itas == *itbs)
            return *itas;
    }

    return NodeIndex();
}

QList<EdgeIndex> AbstractGraphModel::incidentEdges(const NodeIndex & node) const
{
    QList<EdgeIndex> edges = outgoingEdges(node);
    edges.append(incomingEdges(node));
    return edges;
}

void AbstractGraphModel::beginResetModel()
{
    d->beginSignal(Data::ModelResetSignal);
    emit modelAboutToBeReset();
}

void AbstractGraphModel::endResetModel()
{
    d->endSignal(Data::ModelResetSignal);
    emit modelReset();
}

void AbstractGraphModel::beginInsertNode(const NodeIndex & parent, int idx)
{
    d->beginSignal(Data::NodeInsertedSignal);
    d->signalParent = parent;
    d->signalIndex = idx;
    emit nodeAboutToBeInserted(d->signalParent, d->signalIndex);
}

void AbstractGraphModel::endInsertNode()
{
    d->endSignal(Data::NodeInsertedSignal);
    emit nodeInserted(d->signalParent, d->signalIndex);
}

void AbstractGraphModel::beginRemoveNode(const NodeIndex& parent, int idx)
{
    d->beginSignal(Data::NodeRemovedSignal);
    d->signalParent = parent;
    d->signalIndex = idx;
    emit nodeAboutToBeRemoved(d->signalParent, d->signalIndex);
}

void AbstractGraphModel::endRemoveNode()
{
    d->endSignal(Data::NodeRemovedSignal);
    emit nodeRemoved(d->signalParent, d->signalIndex);
}

void AbstractGraphModel::beginInsertEdge(int idx)
{
    d->beginSignal(Data::EdgeInsertedSignal);
    d->signalIndex = idx;
    emit edgeAboutToBeInserted(d->signalIndex);
}

void AbstractGraphModel::endInsertEdge()
{
    d->endSignal(Data::EdgeInsertedSignal);
    emit edgeInserted(d->signalIndex);
}

void AbstractGraphModel::beginRemoveEdge(int idx)
{
    d->beginSignal(Data::EdgeRemovedSignal);
    d->signalIndex = idx;
    emit edgeAboutToBeRemoved(d->signalIndex);
}

void AbstractGraphModel::endRemoveEdge()
{
    d->endSignal(Data::EdgeRemovedSignal);
    emit edgeRemoved(d->signalIndex);
}

NodeIndex AbstractGraphModel::makeNodeIndex(int index, void * p) const
{
    return NodeIndex(this, index, p);
}

EdgeIndex AbstractGraphModel::makeEdgeIndex(int index, void* p) const
{
    return EdgeIndex(this, index, p);
}

} // namespace KGraphViewer

// kate: space-indent on;indent-width 4;replace-tabs on
