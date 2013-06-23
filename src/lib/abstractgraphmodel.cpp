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

AbstractGraphModel::AbstractGraphModel(QObject * parent) :
    QObject(parent)
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

} // namespace KGraphViewer

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
