
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  Computational Engine                                                  *
 *                                                                        *
 *  Copyright (c) 1999-2001, Ben Burton                                   *
 *  For further details contact Ben Burton (benb@acm.org).                *
 *                                                                        *
 *  This program is free software; you can redistribute it and/or         *
 *  modify it under the terms of the GNU General Public License as        *
 *  published by the Free Software Foundation; either version 2 of the    *
 *  License, or (at your option) any later version.                       *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful, but   *
 *  WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *  General Public License for more details.                              *
 *                                                                        *
 *  You should have received a copy of the GNU General Public             *
 *  License along with this program; if not, write to the Free            *
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,        *
 *  MA 02111-1307, USA.                                                   *
 *                                                                        *
 **************************************************************************/

/* end stub */

#include "NAugTriSolidTorusI.h"
#include "NLayeredSolidTorusI.h"
#include "NTriSolidTorusI.h"
#include "NSFSI.h"

Regina::Subcomplex::NAugTriSolidTorus_ptr NAugTriSolidTorus_i::cloneMe() {
    return NAugTriSolidTorus_i::newWrapper(MY_ENGINE_OBJECT->clone());
}
Regina::Subcomplex::NTriSolidTorus_ptr NAugTriSolidTorus_i::getCore() {
    return NTriSolidTorus_i::newWrapper((NTriSolidTorus*)
        &MY_ENGINE_OBJECT->getCore());
}
Regina::Subcomplex::NLayeredSolidTorus_ptr NAugTriSolidTorus_i::getAugTorus(
        CORBA::Long annulus) {
    return NLayeredSolidTorus_i::newWrapper((NLayeredSolidTorus*)
        MY_ENGINE_OBJECT->getAugTorus(annulus));
}
CORBA::Char NAugTriSolidTorus_i::getEdgeGroupRoles(CORBA::Long annulus) {
    return MY_ENGINE_OBJECT->getEdgeGroupRoles(annulus).getPermCode();
}
CORBA::Long NAugTriSolidTorus_i::getChainLength() {
    return MY_ENGINE_OBJECT->getChainLength();
}
CORBA::Long NAugTriSolidTorus_i::getTorusAnnulus() {
    return MY_ENGINE_OBJECT->getTorusAnnulus();
}
CORBA::Boolean NAugTriSolidTorus_i::hasLayeredChain() {
    return MY_ENGINE_OBJECT->hasLayeredChain();
}
Regina::Subcomplex::NSFS_ptr NAugTriSolidTorus_i::getSeifertStructure() {
    return NSFS_i::newWrapper((NSFS*)&MY_ENGINE_OBJECT->getSeifertStructure());
}

