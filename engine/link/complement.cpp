
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  Computational Engine                                                  *
 *                                                                        *
 *  Copyright (c) 1999-2016, Ben Burton                                   *
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

#include "link/link.h"
#include "triangulation/ntriangulation.h"

namespace regina {

Triangulation<3>* Link::complement(bool simplify) const {
    // This implementation produces an oriented triangluation.
    // The orientation follows a right-hand rule, where the thumb points
    // from vertices 0 to 1, and the fingers point from vertice 2 to 3.

    // Our algorithm follows Jeff Weeks' method, which is described in
    // marvellous detail in the comments of link_complement.c from the
    // SnapPea kernel (which you can find in Regina's source tree as
    // engine/snappea/kernel/unused/link_complement.c).
    //
    // We do however make some changes:
    //
    // - We ignore zero-crossing components completely; instead we
    //   connect sum with the appropriate number of unknot complements before
    //   we return the final triangulation.
    //
    // - Jeff's documentation insists that each component has an over- and
    //   under-crossing, and he fixes this in his code by adding spurious
    //   crossings where necessary.
    //
    //   We take a different approach here - we leave the link unchanged,
    //   which has the following side-effect: for each link component with
    //   only over-crossings or only under-crossings, the component (which
    //   must be a separated unknot) is effectively forgotten, and our
    //   triangulation obtains two additional internal vertices instead.
    //   We fix this immediately before returning the final triangulation
    //   by connect summing with additional unknot complements as required.
    //
    // - Jeff's documentation also insists that the underlying 4-valent
    //   graph is connected, and he performs spurious R2 moves where it is
    //   necessary to ensure this.  Again we ignore the issue here; the
    //   side-effect is that our triangulation might be disconnected, and
    //   we fix this before returning by connect summing the pieces together.

    NTriangulation* ans = new NTriangulation();

    // Empty link?  Just return the 3-sphere.
    if (components_.empty()) {
        NTetrahedron* t = ans->newTetrahedron();
        t->join(0, t, NPerm4(0,1));
        t->join(2, t, NPerm4(2,3));
        return ans;
    }

    struct CrossingTet {
        NTetrahedron* tet[4];
        /**
         *
         * Tetrahedra, for -ve crossing:
         *   tet[0]: upper forward -> lower forward
         *   tet[1]: lower forward -> upper backward
         *   tet[2]: upper backward -> lower backward
         *   tet[3]: lower backward -> upper forward
         *
         * Tetrahedra, for +ve crossing:
         *   replace upper <-> lower in the list above
         *
         * Tetrahedron vertices:
         *   0 = north pole
         *   1 = south pole
         *   2->3 represents the arrow in the tetrahedron list above
         */
    };

    size_t n = size();
    CrossingTet* ctet = new CrossingTet[n];

    size_t i, j;

    // Create the local structure around each crossing:
    for (i = 0; i < n; ++i) {
        for (j = 0; j < 4; ++j)
            ctet[i].tet[j] = ans->newTetrahedron();
        if (crossing(i)->sign() > 0) {
            ctet[i].tet[0]->join(0, ctet[i].tet[1], NPerm4(2,3));
            ctet[i].tet[1]->join(1, ctet[i].tet[2], NPerm4(2,3));
            ctet[i].tet[2]->join(0, ctet[i].tet[3], NPerm4(2,3));
            ctet[i].tet[3]->join(1, ctet[i].tet[0], NPerm4(2,3));
        } else {
            ctet[i].tet[0]->join(1, ctet[i].tet[1], NPerm4(2,3));
            ctet[i].tet[1]->join(0, ctet[i].tet[2], NPerm4(2,3));
            ctet[i].tet[2]->join(1, ctet[i].tet[3], NPerm4(2,3));
            ctet[i].tet[3]->join(0, ctet[i].tet[0], NPerm4(2,3));
        }
    }

    // Connect the structures for adjacent crossings:
    const Crossing *cr, *adj;
    StrandRef s, t;
    for (i = 0; i < n; ++i) {
        cr = crossing(i);
        // Make s the strand that follows forwards on the left, and
        // make t the strand that follows forwards on the right.
        if (cr->sign() > 0) {
            s = cr->next(0);
            t = cr->next(1);
        } else {
            s = cr->next(1);
            t = cr->next(0);
        }

        adj = s.crossing();
        if ((adj->sign() > 0 && s.strand() == 1) ||
                (adj->sign() < 0 && s.strand() == 0)) {
            ctet[i].tet[3]->join(2, ctet[adj->index()].tet[3], NPerm4(2,3));
            ctet[i].tet[0]->join(3, ctet[adj->index()].tet[2], NPerm4(2,3));
        } else {
            ctet[i].tet[3]->join(2, ctet[adj->index()].tet[2], NPerm4(2,3));
            ctet[i].tet[0]->join(3, ctet[adj->index()].tet[1], NPerm4(2,3));
        }

        adj = t.crossing();
        if ((adj->sign() > 0 && t.strand() == 1) ||
                (adj->sign() < 0 && t.strand() == 0)) {
            ctet[i].tet[0]->join(2, ctet[adj->index()].tet[3], NPerm4(2,3));
            ctet[i].tet[1]->join(3, ctet[adj->index()].tet[2], NPerm4(2,3));
        } else {
            ctet[i].tet[0]->join(2, ctet[adj->index()].tet[2], NPerm4(2,3));
            ctet[i].tet[1]->join(3, ctet[adj->index()].tet[1], NPerm4(2,3));
        }
    }

    delete[] ctet;

    // Fix any issues involving missing unknot complements and/or
    // disconnected triangluations, as discussed in the comments at the
    // beginning of this routine.

    // TODO: Fix missing unknots / disconnectedness.

    if (simplify)
        ans->intelligentSimplify();
    return ans;
}

} // namespace regina
