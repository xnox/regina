
/**************************************************************************
 *                                                                        *
 *  Regina - A normal surface theory calculator                           *
 *  Computational engine                                                  *
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

#ifndef __NVERTEXI_H
#define __NVERTEXI_H

#include "config.h"

#ifdef __NO_INCLUDE_PATHS
    #include "nvertex.h"
    #include "corbatools.h"
#else
    #include "engine/triangulation/nvertex.h"
    #include "corba/corbatools.h"
#endif

#include "NTetrahedronIDL.h"
#include "ShareableObjectI.h"

class NVertex_i : public virtual Regina::Triangulation::_sk_NVertex,
        public ShareableObject_i {
    protected:
        NVertex_i(::NVertex* newCppPtr) : ShareableObject_i(newCppPtr) {
        }
    public:
        STANDARD_NEW_WRAPPER(NVertex, NVertex_i,
            Regina::Triangulation::NVertex_ptr)

        virtual Regina::Triangulation::NComponent_ptr getComponent();
        virtual Regina::Triangulation::NBoundaryComponent_ptr
            getBoundaryComponent();
        virtual CORBA::Long getLink();
        virtual CORBA::Boolean isLinkClosed();
        virtual CORBA::Boolean isIdeal();
        virtual CORBA::Boolean isBoundary();
        virtual CORBA::Boolean isStandard();
        virtual CORBA::Boolean isLinkOrientable();
        virtual CORBA::Long getLinkEulerCharacteristic();
        virtual CORBA::Long getNumberOfEmbeddings();
        virtual void getEmbedding(Regina::Triangulation::NTetrahedron_ptr& tet,
            CORBA::Long& vertex, CORBA::Long index);
};

#endif

