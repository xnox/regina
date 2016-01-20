
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  Computational Engine                                                  *
 *                                                                        *
 *  Copyright (c) 1999-2014, Ben Burton                                   *
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

#include "triangulation/nedge.h"
#include "triangulation/ntriangle.h"

namespace regina {

NTriangle::Type NTriangle::type() {
    if (type_)
        return type_;

    subtype_ = -1;

    // Determine the triangle type.
    NVertex* v[3];
    NEdge* e[3];
    int i;
    for (i = 0; i < 3; i++) {
        v[i] = vertex(i);
        e[i] = edge(i);
    }

    if (e[0] != e[1] && e[1] != e[2] && e[2] != e[0]) {
        // Three distinct edges.
        if (v[0] == v[1] && v[1] == v[2])
            return (type_ = PARACHUTE);
        for (i = 0; i < 3; i++)
            if (v[(i+1)%3] == v[(i+2)%3]) {
                subtype_ = i;
                return (type_ = SCARF);
            }
        return (type_ = TRIANGLE);
    }

    if (e[0] == e[1] && e[1] == e[2]) {
        // All edges identified.
        if (getEdgeMapping(0).sign() == getEdgeMapping(1).sign() &&
                getEdgeMapping(1).sign() == getEdgeMapping(2).sign())
            return (type_ = L31);

        for (i = 0; i < 3; i++)
            if (getEdgeMapping((i+1)%3).sign() ==
                    getEdgeMapping((i+2)%3).sign()) {
                subtype_ = i;
                return (type_ = DUNCEHAT);
            }
    }

    // Two edges identified.
    for (i = 0; i < 3; i++)
        if (e[(i+1)%3] == e[(i+2)%3]) {
            subtype_ = i;

            if (getEdgeMapping((i+1)%3).sign() ==
                    getEdgeMapping((i+2)%3).sign())
                return (type_ = MOBIUS);

            if (v[0] == v[1] && v[1] == v[2])
                return (type_ = HORN);

            return (type_ = CONE);
        }

    // We should never reach this point.
    return UNKNOWN_TYPE;
}

void NTriangle::writeTextLong(std::ostream& out) const {
    writeTextShort(out);
    out << std::endl;

    out << "Appears as:" << std::endl;
    for (auto& emb : *this)
        out << "  " << emb << std::endl;
}

} // namespace regina

