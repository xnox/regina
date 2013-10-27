
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  Computational Engine                                                  *
 *                                                                        *
 *  Copyright (c) 1999-2013, Ben Burton                                   *
 *  For further details contact Ben Burton (bab@debian.org).              *
 *                                                                        *
 *  This program is free software; you can redistribute it and/or         *
 *  modify it under the terms of the GNU General Public License as        *
 *  published by the Free Software Foundation; either version 2 of the    *
 *  License, or (at your option) any later version.                       *
 *                                                                        *
 *  As an exception, when this program is distributed through (i) the     *
 *  App Store by Apple Inc.; (ii) the Mac App Store by Apple Inc.; or     *
 *  (iii) Google Play by Google Inc., then that store may impose any      *
 *  digital rights management, device limits and/or redistribution        *
 *  restrictions that are required by its terms of service.               *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful, but   *
 *  WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *  General Public License for more details.                              *
 *                                                                        *
 *  You should have received a copy of the GNU General Public             *
 *  License along with this program; if not, write to the Free            *
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,       *
 *  MA 02110-1301, USA.                                                   *
 *                                                                        *
 **************************************************************************/

/* end stub */

#include "dim4/dim4pentachoron.h"
#include "dim4/dim4triangulation.h"
#include <algorithm>

namespace regina {

Dim4Pentachoron::Dim4Pentachoron(Dim4Triangulation* tri) : tri_(tri) {
    std::fill(adj_, adj_ + 5, static_cast<Dim4Pentachoron*>(0));
}

Dim4Pentachoron::Dim4Pentachoron(const std::string& desc,
        Dim4Triangulation* tri) : desc_(desc), tri_(tri) {
    std::fill(adj_, adj_ + 5, static_cast<Dim4Pentachoron*>(0));
}

bool Dim4Pentachoron::hasBoundary() const {
    for (int i=0; i<5; ++i)
        if (adj_[i] == 0)
            return true;
    return false;
}

void Dim4Pentachoron::joinTo(int myFacet, Dim4Pentachoron* you, NPerm5 gluing) {
    NPacket::ChangeEventSpan span(tri_);

    adj_[myFacet] = you;
    adjPerm_[myFacet] = gluing;
    int yourFacet = gluing[myFacet];
    you->adj_[yourFacet] = this;
    you->adjPerm_[yourFacet] = gluing.inverse();

    tri_->clearAllProperties();
}

Dim4Pentachoron* Dim4Pentachoron::unjoin(int myFacet) {
    NPacket::ChangeEventSpan span(tri_);

    Dim4Pentachoron* you = adj_[myFacet];
    int yourFacet = adjPerm_[myFacet][myFacet];
    you->adj_[yourFacet] = 0;
    adj_[myFacet] = 0;

    tri_->clearAllProperties();

    return you;
}

void Dim4Pentachoron::isolate() {
    for (int i=0; i<5; ++i)
        if (adj_[i])
            unjoin(i);
}

void Dim4Pentachoron::writeTextLong(std::ostream& out) const {
    writeTextShort(out);
    out << std::endl;
    for (int i = 4; i >= 0; --i) {
        out << Dim4Tetrahedron::ordering[i].trunc4() << " -> ";
        if (! adj_[i])
            out << "boundary";
        else
            out << adj_[i]->markedIndex() << " ("
                << (adjPerm_[i] * Dim4Tetrahedron::ordering[i]).trunc4()
                << ')';
        out << std::endl;
    }
}

} // namespace regina

