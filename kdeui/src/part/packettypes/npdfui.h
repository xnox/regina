
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  KDE User Interface                                                    *
 *                                                                        *
 *  Copyright (c) 1999-2008, Ben Burton                                   *
 *  For further details contact Ben Burton (bab@debian.org).              *
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
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,       *
 *  MA 02110-1301, USA.                                                   *
 *                                                                        *
 **************************************************************************/

/* end stub */

/*! \file npdfui.h
 *  \brief Provides an interface for viewing PDF packets.
 */

#ifndef __NPDFUI_H
#define __NPDFUI_H

#include "../packetui.h"

#include <ktempfile.h>

class QWidgetStack;

namespace KParts {
    class ReadOnlyPart;
};

namespace regina {
    class NPacket;
    class NPDF;
};

/**
 * A packet interface for viewing text packets.
 */
class NPDFUI : public QObject, public PacketReadOnlyUI {
    Q_OBJECT

    private:
        /**
         * Packet details
         */
        regina::NPDF* pdf;

        /**
         * Temporary PDF storage
         */
        KTempFile temp;

        /**
         * Internal components
         */
        QWidget* ui;
        QWidgetStack* stack;
        KParts::ReadOnlyPart* viewer;
        QWidget* layerInfo;
        QWidget* layerError;
        QLabel* msgInfo;
        QLabel* msgError;

    public:
        /**
         * Constructor and destructor.
         */
        NPDFUI(regina::NPDF* packet, PacketPane* newEnclosingPane);

        /**
         * PacketUI overrides.
         */
        regina::NPacket* getPacket();
        QWidget* getInterface();
        QString getPacketMenuText() const;
        void refresh();

    private:
        /**
         * Set up internal components.
         */
        QWidget* messageLayer(QLabel*& text, const char* icon);
        void showInfo(const QString& msg);
        void showError(const QString& msg);
};

#endif
