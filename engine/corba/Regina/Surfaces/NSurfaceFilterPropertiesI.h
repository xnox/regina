
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

#ifndef __NSURFACEFILTERPROPERTIESI_H
#define __NSURFACEFILTERPROPERTIESI_H

#include "config.h"

#ifdef __NO_INCLUDE_PATHS
    #include "sfproperties.h"
    #include "corbatools.h"
#else
    #include "engine/surfaces/sfproperties.h"
    #include "corba/corbatools.h"
#endif

#include "NSurfaceFilterPropertiesIDL.h"
#include "NSurfaceFilterI.h"

class NSurfaceFilterProperties_i :
        public virtual Regina::Surfaces::_sk_NSurfaceFilterProperties,
        public NSurfaceFilter_i {
    protected:
        NSurfaceFilterProperties_i(::NSurfaceFilterProperties* newCppPtr) :
                NSurfaceFilter_i(newCppPtr) {
        }
    public:
        STANDARD_NEW_WRAPPER(NSurfaceFilterProperties,
            NSurfaceFilterProperties_i,
            Regina::Surfaces::NSurfaceFilterProperties_ptr)
        
        virtual CORBA::Long getNumberOfECs();
        virtual char* getEC(CORBA::Long index);
        virtual CORBA::Char getOrientability();
        virtual CORBA::Char getCompactness();
        virtual CORBA::Char getRealBoundary();
        virtual void addEC(const char* ec);
        virtual void removeEC(const char* ec);
        virtual void removeAllECs();
        virtual void setOrientability(CORBA::Char value);
        virtual void setCompactness(CORBA::Char value);
        virtual void setRealBoundary(CORBA::Char value);
};

#endif

