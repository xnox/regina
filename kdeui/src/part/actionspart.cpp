
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  KDE User Interface                                                    *
 *                                                                        *
 *  Copyright (c) 1999-2011, Ben Burton                                   *
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

#include "reginapart.h"
#include "reginamain.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QMenuBar>
#include <QStyle>
#include <QToolBar>

// TODO: Undo/redo are not yet implemented.


void ReginaPart::setupActions() {
    QAction* act;

    editMenu = new QMenu(tr("&Edit"));
    treeMenu = new QMenu(tr("&Packet Tree"));
    
    // File actions:
    actSave = new QAction(QIcon::fromTheme("document-save"), tr("&Save"), this);
    actSave->setShortcuts(QKeySequence::Save);
    actSave->setWhatsThis(tr("Save the current data file."));
    connect(actSave, SIGNAL(triggered()), this, SLOT(fileSave()));
    allActions.append(actSave);

    act = new QAction(QIcon::fromTheme("document-save-as"), tr("Save &as"), this);
    act->setShortcuts(QKeySequence::SaveAs);
    act->setWhatsThis(tr(
        "Save the current data file, but give it a different name."));
    connect(act, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    allActions.append(act);


    importMenu = new QMenu(tr("&Import"));
    exportMenu = new QMenu(tr("&Export"));

    // Edit actions:
    actCut = new QAction(QIcon::fromTheme("edit-cut"), tr("Cu&t"), this);
    actCut->setWhatsThis(tr("Cut out the current selection and store it "
        "in the clipboard."));
    actCut->setShortcuts(QKeySequence::Cut);
    editMenu->addAction(actCut);
    allActions.append(actCut);

    actCopy = new QAction(QIcon::fromTheme("edit-copy"), tr("&Copy"), this);
    actCopy->setWhatsThis(tr("Copy the current selection to the clipboard."));
    actCopy->setShortcuts(QKeySequence::Copy);
    editMenu->addAction(actCopy);
    allActions.append(actCopy);

    actPaste = new QAction(QIcon::fromTheme("edit-paste"), tr("&Paste"), this);
    actPaste->setWhatsThis(tr("Paste the contents of the clipboard."));
    actPaste->setShortcuts(QKeySequence::Paste);
    editMenu->addAction(actPaste);
    allActions.append(act);
  
    parent->editMenu(editMenu);

    parent->setActions(actSave, act, actCut, actCopy, actPaste);
    

    // New packets:
    QAction *actAngleStructure = new QAction(this);
    actAngleStructure->setText(tr("New &Angle Structure Solutions"));
    actAngleStructure->setIcon(QIcon("packet_angles"));
    actAngleStructure->setShortcut(tr("Alt+a"));
    actAngleStructure->setToolTip(tr("New angle structure solutions"));
    actAngleStructure->setWhatsThis(tr("Create a new list of vertex angle structures "
        "for a triangulation."));
    connect(actAngleStructure, SIGNAL(triggered()), this, SLOT(newAngleStructures()) );
    treeGeneralEditActions.append(actAngleStructure);
    treeMenu->addAction(actAngleStructure);
    allActions.append(actAngleStructure);

    QAction* actContainer = new QAction(this);
    actContainer->setText(tr("New &Container"));
    actContainer->setIcon(QIcon("packet_container"));
    actContainer->setShortcut(tr("Alt+c"));
    actContainer->setToolTip(tr("New container"));
    actContainer->setWhatsThis(tr("Create a new container packet.  Containers "
        "are used to help keep the packet tree organised &ndash; "
        "they serve no purpose other than to store child packets."));
    connect(actContainer, SIGNAL(triggered()), this, SLOT(newContainer()) );
    treeGeneralEditActions.append(actContainer);
    treeMenu->addAction(actContainer);
    allActions.append(actContainer);

    QAction* actFilter = new QAction(this);
    actFilter->setText(tr("New &Filter"));
    actFilter->setIcon(QIcon("packet_filter"));
    actFilter->setShortcut(tr("Alt+f"));
    actFilter->setToolTip(tr("New surface filter"));
    actFilter->setWhatsThis(tr("Create a new normal surface filter.  Surface "
        "filters can be used to sort through normal surface lists and "
        "display only surfaces of particular interest."));
    connect(actFilter, SIGNAL(triggered()), this, SLOT(newFilter()) );
    treeGeneralEditActions.append(actFilter);
    treeMenu->addAction(actFilter);
    allActions.append(actFilter);

    QAction* actSurfaces = new QAction(this);
    actSurfaces->setText(tr("New &Normal Surface List"));
    actSurfaces->setIcon(QIcon("packet_surfaces"));
    actSurfaces->setShortcut(tr("Alt+n"));
    actSurfaces->setToolTip(tr("New normal surface list"));
    actSurfaces->setWhatsThis(tr("Create a new list of vertex normal surfaces "
        "for a triangulation."));
    connect(actSurfaces, SIGNAL(triggered()), this, SLOT(newNormalSurfaces()) );
    treeGeneralEditActions.append(actSurfaces);
    treeMenu->addAction(actSurfaces);
    allActions.append(actSurfaces);

    QAction* actPDF = new QAction(this);
    actPDF->setText(tr("New &PDF Document"));
    actPDF->setIcon(QIcon("packet_pdf"));
    actPDF->setShortcut(tr("Alt+p"));
    actPDF->setToolTip(tr("New PDF document"));
    actPDF->setWhatsThis(tr("Create a new PDF packet containing a copy of "
        "an external PDF document."));
    connect(actPDF, SIGNAL(triggered()), this, SLOT(newPDF()) );
    treeGeneralEditActions.append(actPDF);
    treeMenu->addAction(actPDF);
    allActions.append(actPDF);

    QAction* actScript = new QAction(this);
    actScript->setText(tr("New &Script"));
    actScript->setIcon(QIcon("packet_script"));
    actScript->setShortcut(tr("Alt+s"));
    actScript->setToolTip(tr("New script packet"));
    actScript->setWhatsThis(tr("Create a new Python script that can work "
        "directly with this data file."));
    connect(actScript, SIGNAL(triggered()), this, SLOT(newScript()) );
    treeGeneralEditActions.append(actScript);
    treeMenu->addAction(actScript);
    allActions.append(actScript);

    QAction* actText = new QAction(this);
    actText->setText(tr("New Te&xt"));
    actText->setIcon(QIcon("packet_text"));
    actText->setShortcut(tr("Alt+x"));
    actText->setToolTip(tr("New text packet"));
    actText->setWhatsThis(tr("Create a new piece of text to store within "
        "the packet tree."));
    connect(actText, SIGNAL(triggered()), this, SLOT(newText()) );
    treeGeneralEditActions.append(actText);
    treeMenu->addAction(actText);
    allActions.append(actText);

    QAction* actTriangulation = new QAction(this);
    actTriangulation->setText(tr("New &Triangulation"));
    actTriangulation->setIcon(QIcon("packet_triangulation"));
    actTriangulation->setShortcut(tr("Alt+t"));
    actTriangulation->setToolTip(tr("New triangulation"));
    actTriangulation->setWhatsThis(tr("Create a new 3-manifold triangulation."));
    connect(actTriangulation, SIGNAL(triggered()), this, SLOT(newTriangulation()) );
    treeGeneralEditActions.append(actTriangulation);
    treeMenu->addAction(actTriangulation);
    allActions.append(actTriangulation);

    treeMenu->addSeparator();

    act = new QAction(this);
    act->setText(tr("Form &Census"));
    act->setIcon(QIcon("view-list-text"));
    act->setToolTip(tr("Form a new census of triangulations"));
    act->setWhatsThis(tr("Create a new census of 3-manifold "
        "triangulations according to some set of census constraints."));
    connect(act, SIGNAL(triggered()), this, SLOT(newCensus()) );
    treeGeneralEditActions.append(act);
    treeMenu->addAction(act);
    allActions.append(act);

    treeMenu->addSeparator();

    // Basic packet actions:
    QAction* actView = new QAction(this);
    actView->setText(tr("&View/Edit"));
    actView->setIcon(QIcon("packet_view"));
    actView->setShortcut(tr("Alt+v"));
    actView->setToolTip(tr("View or edit the selected packet"));
    actView->setWhatsThis(tr("View or edit the packet currently selected "
        "in the tree."));
    connect(actView, SIGNAL(triggered()), this, SLOT(packetView()) );
    treePacketViewActions.append(actView);
    treeMenu->addAction(actView);
    allActions.append(actView);

    QAction* actRename = new QAction(this);
    actRename->setText(tr("&Rename"));
    actRename->setIcon(QIcon("edit-rename"));
    actRename->setShortcut(tr("Alt+r"));
    actRename->setToolTip(tr("Rename the selected packet"));
    actRename->setWhatsThis(tr("Rename the packet currently selected "
        "in the tree."));
    connect(actRename, SIGNAL(triggered()), this, SLOT(packetRename()) );
    treePacketEditActions.append(actRename);
    treeMenu->addAction(actRename);
    allActions.append(actRename);

    QAction *actDelete = new QAction(this);
    actDelete->setText(tr("&Delete"));
    actDelete->setIcon(QIcon("edit-delete"));
    actDelete->setShortcut(tr("Delete"));
    actDelete->setToolTip(tr("Delete the selected packet"));
    actDelete->setWhatsThis(tr("Delete the packet currently selected "
        "in the tree."));
    connect(actDelete, SIGNAL(triggered()), this, SLOT(packetDelete()) );
    treePacketEditActions.append(actDelete);
    treeMenu->addAction(actDelete);
    allActions.append(actDelete);

    treeNavMenu = treeMenu->addMenu(tr("&Move"));

    // Tree reorganisation:
    act = new QAction(this);
    act->setText(tr("&Higher Level"));
    act->setIcon(QIcon("arrow-left"));
    act->setShortcut(tr("Alt+Left"));
    act->setToolTip(tr("Move packet to a higher (shallower) level "
        "in the tree"));
    act->setWhatsThis(tr("Move the currently selected packet "
        "one level higher (shallower) in the packet tree.  The packet will "
        "abandon its current parent, and move one level closer to the root "
        "of the tree."));
    connect(act, SIGNAL(triggered()), this, SLOT(moveShallow()) );
    treePacketEditActions.append(act);
    treeNavMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&Lower Level"));
    act->setIcon(QIcon("arrow-right"));
    act->setShortcut(tr("Alt+Right"));
    act->setToolTip(tr("Move packet to a lower (deeper) level in the tree"));
    act->setWhatsThis(tr("Move the currently selected packet "
        "one level lower (deeper) in the packet tree.  The packet will "
        "abandon its current parent, and instead become a child of its "
        "next sibling."));
    connect(act, SIGNAL(triggered()), this, SLOT(moveDeep()) );
    treePacketEditActions.append(act);
    treeNavMenu->addAction(act);
    allActions.append(act);
    
    treeNavMenu->addSeparator();

    act = new QAction(this);
    act->setText(tr("&Up"));
    act->setIcon(QIcon("arrow-up"));
    act->setShortcut(tr("Alt+Up"));
    act->setToolTip(tr("Move packet up through its siblings"));
    act->setWhatsThis(tr("Move the currently selected packet "
        "one step up in the packet tree.  The packet will keep the "
        "same parent."));
    connect(act, SIGNAL(triggered()), this, SLOT(moveUp()) );
    treePacketEditActions.append(act);
    treeNavMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("Jump U&p"));
    act->setIcon(QIcon("arrow-up-double"));
    act->setShortcut(tr("Alt+Shift+Up"));
    act->setToolTip(tr("Jump packet up through its siblings"));
    act->setWhatsThis(tr("Move the currently selected packet "
        "several steps up in the packet tree.  The packet will keep the "
        "same parent."));
    connect(act, SIGNAL(triggered()), this, SLOT(movePageUp()) );
    treePacketEditActions.append(act);
    treeNavMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&Top"));
    act->setIcon(QIcon("go-top"));
    act->setShortcut(tr("Alt+Home"));
    act->setToolTip(tr("Move packet above all its siblings"));
    act->setWhatsThis(tr("Move the currently selected packet "
        "up as far as possible amongst its siblings in the packet tree.  "
        "The packet will keep the same parent, but it will become the "
        "first child of this parent."));
    connect(act, SIGNAL(triggered()), this, SLOT(moveTop()) );
    treePacketEditActions.append(act);
    treeNavMenu->addAction(act);
    allActions.append(act);
    
    treeNavMenu->addSeparator();

    act = new QAction(this);
    act->setText(tr("&Down"));
    act->setIcon(QIcon("arrow-down"));
    act->setShortcut(tr("Alt+Down"));
    act->setToolTip(tr("Move packet down through its siblings"));
    act->setWhatsThis(tr("Move the currently selected packet "
        "one step down in the packet tree.  The packet will keep the "
        "same parent."));
    connect(act, SIGNAL(triggered()), this, SLOT(moveDown()) );
    treePacketEditActions.append(act);
    treeNavMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("Jump Do&wn"));
    act->setIcon(QIcon("arrow-down-double"));
    act->setShortcut(tr("Alt+Shift+Down"));
    act->setToolTip(tr("Jump packet down through its siblings"));
    act->setWhatsThis(tr("Move the currently selected packet "
        "several steps down in the packet tree.  The packet will keep the "
        "same parent."));
    connect(act, SIGNAL(triggered()), this, SLOT(movePageDown()) );
    treePacketEditActions.append(act);
    treeNavMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&Bottom"));
    act->setIcon(QIcon("go-bottom"));
    act->setShortcut(tr("Alt+End"));
    act->setToolTip(tr("Move packet below all its siblings"));
    act->setWhatsThis(tr("Move the currently selected packet "
        "down as far as possible amongst its siblings in the packet tree.  "
        "The packet will keep the same parent, but it will become the "
        "last child of this parent."));
    connect(act, SIGNAL(triggered()), this, SLOT(moveBottom()) );
    treePacketEditActions.append(act);
    treeNavMenu->addAction(act);
    allActions.append(act);
    
    treeMenu->addSeparator();

    act = new QAction(this);
    act->setText(tr("C&lone Packet"));
    act->setIcon(QIcon("edit-copy"));
    act->setShortcut(tr("Alt+l"));
    act->setToolTip(tr("Clone the selected packet only"));
    act->setWhatsThis(tr("Clone the packet currently selected in "
        "the tree.  The new clone will be placed alongside the original "
        "packet."));
    connect(act, SIGNAL(triggered()), this, SLOT(clonePacket()) );
    treePacketEditActions.append(act);
    treeMenu->addAction(act);
    allActions.append(act);


    act = new QAction(this);
    act->setText(tr("Clone Su&btree"));
    act->setToolTip(tr("Clone the subtree beneath the selected packet"));
    act->setWhatsThis(tr("Clone the packet currently selected in "
        "the tree, as well as all of its descendants in the tree.  The new "
        "cloned subtree will be placed alongside the original packet."));
    connect(act, SIGNAL(triggered()), this, SLOT(cloneSubtree()) );
    treePacketEditActions.append(act);
    treeMenu->addAction(act);
    allActions.append(act);

    treeMenu->addSeparator();

    QAction* actRefresh = new QAction(this);
    actRefresh->setText(tr("Refres&h Subtree"));
    actRefresh->setIcon(QIcon("view-refresh"));
    actRefresh->setShortcut(tr("F5"));
    actRefresh->setToolTip(tr("Refresh the subtree beneath the selected packet"));
    actRefresh->setWhatsThis(tr("Refresh the packet "
        "currently selected in the tree, as well as all of its descendants "
        "within the tree.<p>"
        "This should not normally be necessary, but it is a possible "
        "fix-up in case the tree is out of sync with what is happening "
        "elsewhere.  Note that the file is <i>not</i> reloaded from "
        "disc; the tree is just resynced with packet editors and so on."));
    connect(actRefresh, SIGNAL(triggered()), this, SLOT(subtreeRefresh()) );
    treePacketViewActions.append(actRefresh);
    treeMenu->addAction(actRefresh);
    allActions.append(actRefresh);
    
    // Imports and exports:
    act = new QAction(this);
    act->setText(tr("&Regina Data File"));
    act->setIcon(QIcon("regina"));
    act->setToolTip(tr("Import a Regina data file"));
    act->setWhatsThis(tr("Import an external Regina data file.  The "
        "imported packet tree will be grafted into this packet tree."));
    connect(act, SIGNAL(triggered()), this, SLOT(importRegina()) );
    treeGeneralEditActions.append(act);
    importMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&SnapPea Triangulation"));
    act->setIcon(QIcon("snappea"));
    act->setToolTip(tr("Import a SnapPea triangulation"));
    act->setWhatsThis(tr("Import an external SnapPea file as a new "
        "triangulation in this packet tree."));
    connect(act, SIGNAL(triggered()), this, SLOT(importSnapPea()) );
    treeGeneralEditActions.append(act);
    importMenu->addAction(act);
    allActions.append(act);

    act= new QAction(this);
    act->setText(tr("&Orb / Casson Triangulation"));
    act->setIcon(QIcon("orb"));
    act->setToolTip(tr("Import an Orb / Casson triangulation"));
    act->setWhatsThis(tr("Import an external Orb / Casson file as a new "
        "triangulation in this packet tree."));
    connect(act, SIGNAL(triggered()), this, SLOT(importOrb()) );
    treeGeneralEditActions.append(act);
    importMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&Isomorphism Signature List"));
    act->setIcon(QIcon("document-sign"));
    act->setToolTip(tr("Import an isomorphism signature list "
        "for 3-manifold triangulations"));
    act->setWhatsThis(tr("Import an external text file containing "
        "isomorphism signatures for 3-manifold triangulations.  "
        "For each isomorphism signature, "
        "a new 3-manifold triangulation will be created in this packet tree."));
    connect(act, SIGNAL(triggered()), this, SLOT(importIsoSig3()) );
    treeGeneralEditActions.append(act);
    importMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&Dehydrated Triangulation List"));
    act->setIcon(QIcon("dehydrated"));
    act->setToolTip(tr("Import a dehydrated triangulation list"));
    act->setWhatsThis(tr("Import an external text file containing "
        "dehydrated triangulation strings.  For each dehydration string, "
        "a new triangulation will be created in this packet tree."));
    connect(act, SIGNAL(triggered()), this, SLOT(importDehydration()) );
    treeGeneralEditActions.append(act);
    importMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&PDF Document"));
    act->setIcon(QIcon("packet_pdf"));
    act->setToolTip(tr("Import a PDF document"));
    act->setWhatsThis(tr("Import an external PDF document as a new PDF "
        "packet in this tree."));
    connect(act, SIGNAL(triggered()), this, SLOT(importPDF()) );
    treeGeneralEditActions.append(act);
    importMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("P&ython Script"));
    act->setIcon(QIcon("packet_script"));
    act->setToolTip(tr("Import a Python script"));
    act->setWhatsThis(tr("Import an external Python file as a new script "
        "packet in this tree."));
    connect(act, SIGNAL(triggered()), this, SLOT(importPython()) );
    treeGeneralEditActions.append(act);
    importMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&Regina Data File"));
    act->setIcon(QIcon("regina"));
    act->setToolTip(tr("Export a compressed Regina data file"));
    act->setWhatsThis(tr("Export all or part of this packet tree "
        "to a separate Regina data file.  The separate data file will "
        "be saved as compressed XML (the default format)."));
    connect(act, SIGNAL(triggered()), this, SLOT(exportRegina()) );
    exportMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("Regina Data File (&Uncompressed)"));
    act->setIcon(QIcon("regina"));
    act->setToolTip(tr("Export an uncompressed Regina data file"));
    act->setWhatsThis(tr("Export all or part of this packet tree "
        "to a separate Regina data file.  The separate data file will "
        "be saved as uncompressed XML."));
    connect(act, SIGNAL(triggered()), this, SLOT(exportReginaUncompressed()) );
    exportMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&SnapPea Triangulation"));
    act->setIcon(QIcon("snappea"));
    act->setToolTip(tr("Export a SnapPea triangulation"));
    act->setWhatsThis(tr("Export a triangulation from this packet tree "
        "to a separate SnapPea file."));
    connect(act, SIGNAL(triggered()), this, SLOT(exportSnapPea()) );
    exportMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&C++ Source"));
    act->setIcon(QIcon("text-x-c++src"));
    act->setToolTip(tr("Export a triangulation as C++ source"));
    act->setWhatsThis(tr("Export a triangulation from this packet tree "
        "to a C++ source file.<p>"
        "The exported C++ code will reconstruct the original triangulation.  "
        "See the users' handbook for further information on using Regina "
        "in your own code."));
    connect(act, SIGNAL(triggered()), this, SLOT(exportSource()) );
    exportMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("CS&V Surface List"));
    act->setIcon(QIcon("csvexport"));
    act->setToolTip(tr("Export a normal surface list as a "
        "text file with comma-separated values"));
    act->setWhatsThis(tr("Export a normal surface list from this packet tree "
        "to a CSV file (a text file with comma-separated values).  Files of "
        "this type are suitable for importing into spreadsheets and "
        "databases.<p>"
        "Individual disc coordinates as well as various properties of the "
        "normal surfaces (such as orientability and Euler characteristic) "
        "will all be stored as separate fields in the CSV file."));
    connect(act, SIGNAL(triggered()), this, SLOT(exportCSVSurfaceList()) );
    exportMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("&PDF Document"));
    act->setIcon(QIcon("packet_pdf"));
    act->setToolTip(tr("Export a PDF document"));
    act->setWhatsThis(tr("Export a PDF packet from this packet tree "
        "to a separate PDF document."));
    connect(act, SIGNAL(triggered()), this, SLOT(exportPDF()) );
    exportMenu->addAction(act);
    allActions.append(act);

    act = new QAction(this);
    act->setText(tr("P&ython Script"));
    act->setIcon(QIcon("packet_script"));
    act->setToolTip(tr("Export a Python script"));
    act->setWhatsThis(tr("Export a script packet from this packet tree "
        "to a separate Python file."));
    connect(act, SIGNAL(triggered()), this, SLOT(exportPython()) );
    exportMenu->addAction(act);
    allActions.append(act);

    parent->importsExports(importMenu, exportMenu);

    QToolBar* packet = parent->createToolBar(tr("Packet Tree Toolbar"));
    packet->addAction(actView);
    packet->addAction(actRefresh);
    packet->addAction(actRename);
    packet->addAction(actDelete);
    packet->addSeparator();
    packet->addAction(actContainer);
    packet->addAction(actTriangulation);
    packet->addAction(actSurfaces);
    packet->addAction(actAngleStructure);
    packet->addAction(actFilter);
    packet->addAction(actText);
    packet->addAction(actScript);
    packet->addAction(actPDF);
}

