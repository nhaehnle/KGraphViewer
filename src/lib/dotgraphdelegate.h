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
//TODO clarify licenses

#ifndef KGRAPHVIEWER_DOTGRAPHDELEGATE_H
#define KGRAPHVIEWER_DOTGRAPHDELEGATE_H

#include "graphscene.h"

namespace KGraphViewer {

class DotGraphModel;

/**
 * Delegate that renders nodes and edges from a \ref DotGraphModel using
 * the rendering data provided by GraphViz.
 *
 * \warning DotGraphDelegate is not thread-safe in any way because
 * of the indirect use of global variables in dotgrammar.cpp
 */
class KGRAPHVIEWER_EXPORT DotGraphDelegate : public AbstractItemDelegate {
public:
    explicit DotGraphDelegate(QObject* parent = 0);
    virtual ~DotGraphDelegate();

    virtual QGraphicsItem* createNodeItem(const NodeIndex& node);
    virtual QGraphicsItem* createEdgeItem(const EdgeIndex& edge);
};

} // namespace KGraphViewer

#endif // KGRAPHVIEWER_DOTGRAPHDELEGATE_H

// kate: space-indent on;indent-width 4;replace-tabs on;remove-trailing-space true
