
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

#include "regina-config.h"
#include "file/nxmlfile.h"
#include "surfaces/nnormalsurfacelist.h"

#include "examplesaction.h"
#include "recentfilesaction.h"
#include "reginaabout.h"
#include "reginafilter.h"
#include "reginamain.h"
#include "reginapref.h"
#include "reginasupport.h"
#include "../part/reginapart.h"

#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSize>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWhatsThis>

ReginaMain::ReginaMain(ReginaManager* parent, bool showAdvice) {
    // Track the parent manager.
    manager = parent;

    setAttribute(Qt::WA_DeleteOnClose);

    if (ReginaPrefSet::global().windowMainSize.isValid())
        resize(ReginaPrefSet::global().windowMainSize);

    // Accept drag and drop.
    setAcceptDrops(true);

    // Set up our actions.
    setupActions();

    aboutApp = 0;

    // Mark actions as missing for now.
    editAct = 0;
    saveAct = 0;
    saveAsAct = 0;
    treeMenu = 0;
    packetMenu = 0;
    packetTreeToolBar = 0;

    currentPart = 0;

    setWindowTitle(tr("Regina"));

    if (showAdvice) {
        // Until we actually have a part loaded, give the user something
        // helpful to start with.
        QLabel* advice = new QLabel(tr("<qt>To start, try:<p>"
            "File&nbsp;&rarr;&nbsp;Open Example&nbsp;&rarr;&nbsp;"
            "Introductory Examples</qt>"));
        advice->setAlignment(Qt::AlignCenter);
        advice->setWhatsThis(tr("<qt>If you select "
            "<i>File&nbsp;&rarr;&nbsp;Open Example&nbsp;&rarr;&nbsp;"
            "Introductory Examples</i> from the menu, "
            "Regina will open a sample data file that you can "
            "play around with.<p>"
            "You can also read the Regina Handbook, which walks "
            "you through what Regina can do.  Just press F1, or select "
            "<i>Help&nbsp;&rarr;&nbsp;Regina Handbook</i> from the "
            "menu.</qt>"));
        setCentralWidget(advice);
    }
}

ReginaMain::~ReginaMain() {
    if (aboutApp)
      delete aboutApp;
}

void ReginaMain::dragEnterEvent(QDragEnterEvent *event) {
    if( event->mimeData()->hasUrls() )
      event->acceptProposedAction();
    
}

void ReginaMain::dropEvent(QDropEvent *event) {
    // Accept a dropped URL (or URLs).

    // See if we can decode a URI.  If not, just ignore it.
    if (event->mimeData()->hasUrls()) {
        // We have a URI, so process it.
        // Load in each file.
        const QList<QUrl>& urls(event->mimeData()->urls());
        for (QList<QUrl>::const_iterator it = urls.begin();
                it != urls.end(); ++it)
            openUrl(*it);
    }
}

void ReginaMain::closeEvent(QCloseEvent *event) {
    consoles.closeAllConsoles();

    // Do we really want to close?
    if (currentPart && ! currentPart->closeUrl())
        event->ignore();
    else {
        ReginaPrefSet::global().windowMainSize = size();
        ReginaPrefSet::save();
        manager->aboutToClose(this);
        event->accept();
    }
}

void ReginaMain::newTopology() {
    // If we already have a document open, make a new window.
    ReginaMain* useWindow = (currentPart ? manager->newWindow() : this);
    useWindow->newTopologyPart();
}

bool ReginaMain::openUrl(const QUrl& url) {
    // Can we read data from the file?
    QString localFile = url.toLocalFile();
    if (localFile.isEmpty()) {
        QMessageBox::warning(this, tr("Empty filename"), tr(
            "<qt>I cannot open this data file, since the filename is empty.  "
            "Please report this error to the developers at <i>%1</i>.</qt>").
            arg(PACKAGE_BUGREPORT));
        return false;
    }

    regina::NPacket* packetTree = regina::readFileMagic(
        static_cast<const char*>(QFile::encodeName(localFile)));

    if (! packetTree) {
        QMessageBox::warning(this, tr("Could not open data"), tr(
            "The topology data file %1 could not be opened.  Perhaps "
            "it is not a Regina data file?").arg(localFile));
        return false;
    }

    // All good, we have some real data.  Let's go.
    // If we already have a document open, make a new window.
    ReginaMain* useWindow = (currentPart ? manager->newWindow() : this);
    useWindow->newTopologyPart();
    return useWindow->currentPart->initData(packetTree, localFile, QString());
}

bool ReginaMain::openExample(const QUrl& url, const QString& description) {
    // Same as openUrl(), but give a pleasant message if the file
    // doesn't seem to exist.
    QString localFile = url.toLocalFile();
    if (! QFile(localFile).exists()) {
        QMessageBox::warning(this, tr("Could not find example file"),
            tr("<qt>The example file \"%1\" could not be found."
            "This may be because the examples have not been installed "
            "properly.  Please contact <i>%2</i> for assistance.").
            arg(description).arg(PACKAGE_BUGREPORT));
        return false;
    }

    regina::NPacket* packetTree = regina::readXMLFile(
        static_cast<const char*>(QFile::encodeName(localFile)));

    if (! packetTree) {
        QMessageBox::warning(this, tr("Could not open example file"),
            tr("<qt>The example file \"%1\" could not be read."
            "This may be because the examples have not been installed "
            "properly.  Please contact <i>%2</i> for assistance.").
            arg(description).arg(PACKAGE_BUGREPORT));
        return false;
    }

    // All good, we have some real data.  Let's go.
    // If we already have a document open, make a new window.
    ReginaMain* useWindow = (currentPart ? manager->newWindow() : this);
    useWindow->newTopologyPart();
    return useWindow->currentPart->initData(packetTree,
        QString(), description);
}

void ReginaMain::pythonConsole() {
    consoles.launchPythonConsole(this);
}

void ReginaMain::pythonReference() {
    PythonManager::openPythonReference(this);
}

void ReginaMain::fileOpen() {
    QString file = QFileDialog::getOpenFileName(this, tr("Open Data File"),
        QString(), tr(FILTER_SUPPORTED));
    if (! file.isNull())
        openUrl(QUrl::fromLocalFile(file));
}

void ReginaMain::optionsPreferences() {
    ReginaPreferences dlg(this);
    dlg.exec();
}

void ReginaMain::helpAboutApp() {
    if (! aboutApp)
        aboutApp = new ReginaAbout(this);
    aboutApp->show();
}

void ReginaMain::helpHandbook() {
    ReginaPrefSet::openHandbook("index", 0, this);
}

void ReginaMain::helpXMLRef() {
    ReginaPrefSet::openHandbook("index", "regina-xml", this);
}

void ReginaMain::helpWhatsThis() {
    QWhatsThis::enterWhatsThisMode();
}

void ReginaMain::helpTipOfDay() {
    // TODO
    //KTipDialog::showTip(this, QString::null, true);
}

void ReginaMain::helpTrouble() {
    ReginaPrefSet::openHandbook("troubleshooting", 0, this);
}

void ReginaMain::helpNoHelp() {
    QMessageBox::information(this, tr("TODO change this title"),
        tr("<qt>If you cannot view the Regina Handbook, it is possibly "
            "because you do not have the KDE Help Center installed.<p>"
            "Try editing Regina's preferences: in the General Options "
            "panel, uncheck the box "
            "<i>\"Open handbook in KDE Help Center\"</i>.  "
            "This will make the handbook open in your default web browser "
            "instead.<p>"
            "If all else fails, remember that you can always read the "
            "Regina Handbook online at "
            "<a href=\"http://regina.sourceforge.net/\">regina.sourceforge.net</a>.  "
            "Just follow the <i>Documentation</i> links.</qt>"),
        tr("Handbook won't open?"));
}

void ReginaMain::changeCaption(const QString& text) {
    setWindowTitle(text);
}

void ReginaMain::newToolbarConfig() {
    // Work around a bug that messes up the newly created GUI.
    /*
    createGUI(0);
    createShellGUI(false);
    createGUI(currentPart);
    */

    //applyMainWindowSettings(KConfigGroup(KGlobal::config(),
    //    QString::fromLatin1("MainWindow")));
}

void ReginaMain::setupActions() {
    QAction* act;
    toolBar = new QToolBar(this);
    toolBar->setWindowTitle(tr("Main"));
    addToolBar(toolBar);
    
    fileMenu =  menuBar()->addMenu(tr("&File"));

    

    // File actions:
    actNew = new QAction(this); 
    actNew->setText(tr("&New Topology Data"));
    actNew->setIcon(ReginaSupport::themeIcon("document-new"));
    actNew->setShortcuts(QKeySequence::New);
    actNew->setWhatsThis(tr("Create a new topology data file.  This is "
        "the standard type of data file used by Regina."));
    connect(actNew, SIGNAL(triggered()), this, SLOT(newTopology()));
    fileMenu->addAction(actNew);
    toolBar->addAction(actNew);

    actOpen = new QAction(this);
    actOpen->setText(tr("&Open..."));
    actOpen->setIcon(ReginaSupport::themeIcon("document-open"));
    actOpen->setShortcuts(QKeySequence::Open);
    actOpen->setWhatsThis(tr("Open a topology data file."));
    connect(actOpen, SIGNAL(triggered()), this, SLOT(fileOpen()));
    fileMenu->addAction(actOpen);
    toolBar->addAction(actOpen);
   
    fileOpenRecent = new RecentFilesAction(this);
    connect(fileOpenRecent, SIGNAL(urlSelected(const QUrl&)),
        this, SLOT(openUrl(const QUrl&)));
    fileMenu->addMenu(fileOpenRecent);

    fileOpenExample = new ExamplesAction(this);
    fillExamples();
    connect(fileOpenExample, SIGNAL(urlSelected(const QUrl&, const QString&)),
        this, SLOT(openExample(const QUrl&, const QString&)));
    fileMenu->addMenu(fileOpenExample);

    /*
    act = new QAction(this);
    act->setText(tr("&Save"));
    act->setIcon(ReginaSupport::themeIcon("document-save"));
    act->setShortcuts(QKeySequence::Save);
    act->setWhatsThis(tr("Save the topology data to a file."));
    connect(act, SIGNAL(triggered()), this, SLOT(saveUrl()));
    fileMenu->addAction(act);
    toolBar->addAction(act); 

    act = new QAction(this);
    act->setText(tr("Save &As..."));
    act->setIcon(ReginaSupport::themeIcon("document-save-as"));
    act->setShortcuts(QKeySequence::SaveAs);
    act->setWhatsThis(tr("Save the topology data to a new file."));
    connect(act, SIGNAL(triggered()), this, SLOT(saveUrlAs()));
    fileMenu->addAction(act);
    toolBar->addAction(act); */

    saveSep = fileMenu->addSeparator();
    importAct = fileMenu->addMenu(tr("&Import"))->menuAction();
    importAct->setEnabled(false);
    exportAct = fileMenu->addMenu(tr("&Export"))->menuAction();
    exportAct->setEnabled(false);
    exportSep = fileMenu->addSeparator();

    act = new QAction(this);
    act->setText(tr("&Close"));
    act->setIcon(ReginaSupport::themeIcon("window-close"));
    act->setShortcuts(QKeySequence::Close);
    act->setWhatsThis(tr("Close this topology data file."));
    connect(act, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(act);

    act = new QAction(this);
    act->setText(tr("&Quit"));
    act->setIcon(ReginaSupport::themeIcon("application-exit"));
    act->setShortcuts(QKeySequence::Quit);
    act->setMenuRole(QAction::QuitRole);
    act->setWhatsThis(tr("Close all files and quit Regina."));
    connect(act, SIGNAL(triggered()), manager, SLOT(closeAllWindows()));
    fileMenu->addAction(act);

    QMenu *toolMenu =  menuBar()->addMenu(tr("&Tools"));
    toolMenuAction = toolMenu->menuAction();
    // Tools:
    actPython = new QAction(this);
    actPython->setText(tr("&Python Console"));
    actPython->setIcon(ReginaSupport::regIcon("python_console"));
    actPython->setShortcut(tr("Alt+y"));
    actPython->setWhatsThis(tr("Open a new Python console.  You can "
        "use a Python console to interact directly with Regina's "
        "mathematical engine."));
    connect(actPython, SIGNAL(triggered()), this, SLOT(pythonConsole()));
    toolMenu->addAction(actPython);
    toolBar->addAction(actPython);
    
    
    QMenu *settingsMenu =  menuBar()->addMenu(tr("&Settings"));
    // Preferences:

    act = new QAction(this);
    act->setText(tr("&Configure Regina"));
    act->setIcon(ReginaSupport::themeIcon("configure"));
    act->setShortcuts(QKeySequence::Preferences);
    act->setMenuRole(QAction::PreferencesRole);
    act->setWhatsThis(tr("Configure Regina.  Here you can set "
        "your own preferences for how Regina behaves."));
    connect(act, SIGNAL(triggered()), this, SLOT(optionsPreferences()));
    settingsMenu->addAction(act);


    QMenu *helpMenu =  menuBar()->addMenu(tr("&Help"));
    // Help:
    act = new QAction(this);
    act->setText(tr("&About Regina"));
    act->setIcon(ReginaSupport::themeIcon("help-about"));
    act->setMenuRole(QAction::AboutRole);
    act->setWhatsThis(tr("Display information about Regina, such as "
        "the authors, license and website."));
    connect(act, SIGNAL(triggered()), this, SLOT(helpAboutApp()));
    helpMenu->addAction(act);


    act = new QAction(this);
    act->setText(tr("Regina &Handbook"));
    act->setIcon(ReginaSupport::themeIcon("help-contents"));
    act->setShortcuts(QKeySequence::HelpContents);
    act->setWhatsThis(tr("Open the Regina handbook.  "
        "This is the main users' guide for how to use Regina."));
    connect(act, SIGNAL(triggered()), this, SLOT(helpHandbook()));
    helpMenu->addAction(act);

    act = new QAction(this);
    act->setText(tr("What's &This?"));
    act->setIcon(ReginaSupport::themeIcon("help-hint"));
    act->setShortcuts(QKeySequence::WhatsThis);
    connect(act, SIGNAL(triggered()), this, SLOT(helpWhatsThis()));
    helpMenu->addAction(act);

    act = new QAction(this);
    act->setText(tr("&Python API Reference"));
    act->setIcon(ReginaSupport::regIcon("python_console"));
    act->setWhatsThis(tr("Open the detailed documentation for Regina's "
        "mathematical engine.  This describes the classes, methods and "
        "routines that Regina makes available to Python scripts.<p>"
        "See the <i>Python Scripting</i> chapter of the user's handbook "
        "for more information (the handbook is "
        "accessed through <i>Regina Handbook</i> in the <i>Help</i> menu)."));
    connect(act, SIGNAL(triggered()), this, SLOT(pythonReference()));
    helpMenu->addAction(act);

    act = new QAction(this);
    act->setText(tr("&File Format Reference"));
    act->setIcon(ReginaSupport::themeIcon("application-xml"));
    act->setWhatsThis(tr("Open the file format reference manual.  "
        "This give full details of the XML file format that Regina "
        "uses to store its data files."));
    connect(act, SIGNAL(triggered()), this, SLOT(helpXMLRef()));
    helpMenu->addAction(act);

    // TODO: Not implemented
    //act = KStandardAction::tipOfDay(this, SLOT(helpTipOfDay()),
    //    actionCollection());
    //act->setWhatsThis(tr("View tips and hints on how to use Regina."));
   
    act = new QAction(this);
    act->setText(tr("Tr&oubleshooting"));
    act->setIcon(ReginaSupport::themeIcon("dialog-warning"));
    connect(act, SIGNAL(triggered()), this, SLOT(helpTrouble()));
    helpMenu->addAction(act);
   
    act = new QAction(this);
    act->setText(tr("Handbook won't open?"));
    act->setIcon(ReginaSupport::themeIcon("dialog-cancel"));
    connect(act, SIGNAL(triggered()), this, SLOT(helpNoHelp()));
    helpMenu->addAction(act);
}

void ReginaMain::fillExamples() {
    fileOpenExample->addUrl("sample-misc.rga",
        tr("Introductory Examples"));
    fileOpenExample->addUrl("closed-hyp-census.rga",
        tr("Closed Hyperbolic Census"));
    fileOpenExample->addUrl("closed-or-census.rga",
        tr("Closed Orientable Census (Small)"));
    fileOpenExample->addUrl("closed-or-census-large.rga",
        tr("Closed Orientable Census (Large)"));
    fileOpenExample->addUrl("closed-nor-census.rga",
        tr("Closed Non-Orientable Census"));
    fileOpenExample->addUrl("snappea-census.rga",
        tr("Cusped Hyperbolic Census"));
    fileOpenExample->addUrl("knot-link-census.rga",
        tr("Knot / Link Complements"));
    fileOpenExample->addUrl("sig-3mfd-census.rga",
        tr("Splitting Surface Sigs (General)"));
    fileOpenExample->addUrl("sig-prime-min-census.rga",
        tr("Splitting Surface Sigs (Prime, Minimal)"));
}

void ReginaMain::addRecentFile() {
    if (currentPart && ! currentPart->url().isEmpty())
        fileOpenRecent->addUrl(currentPart->url());
}

void ReginaMain::newTopologyPart() {
    currentPart = new ReginaPart(this, QStringList());

    // Connect up signals and slots.
    disconnect(actPython, SIGNAL(triggered()), this, SLOT(pythonConsole()));
    connect(actPython, SIGNAL(triggered()), currentPart, SLOT(pythonConsole()));

    setCentralWidget(currentPart->widget());
}


bool ReginaMain::saveUrl() {
    if (! currentPart )
        return false;
    currentPart->fileSave();
    return true;
}

bool ReginaMain::saveUrlAs() {
    if (! currentPart )
        return false;
    currentPart->fileSaveAs();
    return true;
}

QSize ReginaMain::sizeHint() const {
    return ReginaPrefSet::defaultMainSize();
}

void ReginaMain::plugMenu(QMenu *menu) {
    if (packetMenu) {
        menuBar()->removeAction(packetMenu);
    }
    packetMenu = menuBar()->insertMenu(toolMenuAction,menu);
}

void ReginaMain::unplugMenu() {
    if (packetMenu) {
        menuBar()->removeAction(packetMenu);
        packetMenu = NULL;
    }
}

void ReginaMain::plugTreeMenu(QMenu *menu) {
    if (treeMenu) {
        menuBar()->removeAction(treeMenu);
    }
    if (packetMenu) {
        treeMenu = menuBar()->insertMenu(packetMenu,menu);
    } else {
        treeMenu = menuBar()->insertMenu(toolMenuAction,menu);
    }
}

void ReginaMain::unplugTreeMenu() {
    if (treeMenu) {
        menuBar()->removeAction(packetMenu);
        treeMenu = NULL;
    }
}

void ReginaMain::setActions(QAction *save, QAction *saveAs,
        QAction *actCut, QAction *actCopy, QAction *actPaste) {
    // First insert SaveAs before the separator
    if (saveAsAct) {
        fileMenu->removeAction(saveAsAct);
    }
    saveAsAct = saveAs;
    fileMenu->insertAction(saveSep,saveAs);
    // Then insert Save before SaveAs
    if (saveAct) {
        fileMenu->removeAction(saveAct);
    }
    saveAct = save;
    fileMenu->insertAction(saveAs,save);
  
    toolBar->insertAction(actPython, save);
    toolBar->insertSeparator(actPython);
    toolBar->insertAction(actPython, actCut);
    toolBar->insertAction(actPython, actCopy);
    toolBar->insertAction(actPython, actPaste);
    toolBar->insertSeparator(actPython);
}

QToolBar* ReginaMain::createToolBar(QString name) {
    if (packetTreeToolBar) {
        removeToolBar(packetTreeToolBar);
    }
    packetTreeToolBar = addToolBar(name);
    return packetTreeToolBar;
}


void ReginaMain::importsExports(QMenu *imports, QMenu *exports) {
    // First insert Export before the separator
    if (exportAct) {
        fileMenu->removeAction(exportAct);
    }
    exportAct = fileMenu->insertMenu(exportSep,exports);
    // Then insert Import before Export
    if (importAct) {
        fileMenu->removeAction(importAct);
    }
    importAct = fileMenu->insertMenu(exports->menuAction(),imports);

}

void ReginaMain::editMenu(QMenu *menu) {
    if (editAct) {
        menuBar()->removeAction(editAct);
    }

    if (treeMenu) {
        // Insert before "treeMenu" aka Packet Tree
        editAct = menuBar()->insertMenu(treeMenu,menu);
    } else if(packetMenu) {
        // Insert before "plugMenu" aka packet-specific menu
        editAct = menuBar()->insertMenu(packetMenu,menu);
    } else {
        // Insert before Tools
        editAct = menuBar()->insertMenu(toolMenuAction,menu);
    }
}

