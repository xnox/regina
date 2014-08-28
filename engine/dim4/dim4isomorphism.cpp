
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

#include "dim4/dim4isomorphism.h"
#include "dim4/dim4triangulation.h"
#include "generic/ngenericisomorphism-impl.h"

namespace regina {

// Instatiate all templates from the -impl.h file.
template void NGenericIsomorphism<4>::writeTextShort(std::ostream&) const;
template void NGenericIsomorphism<4>::writeTextLong(std::ostream&) const;
template bool NGenericIsomorphism<4>::isIdentity() const;
template NGenericIsomorphism<4>::NGenericIsomorphism(
    const NGenericIsomorphism<4>&);
template Dim4Isomorphism* NGenericIsomorphism<4>::random(unsigned);
template Dim4Triangulation* NGenericIsomorphism<4>::apply(
        const Dim4Triangulation*) const;
template void NGenericIsomorphism<4>::applyInPlace(Dim4Triangulation*) const;

} // namespace regina
