
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
#include "snappea/nsnappeatriangulation.h"
#include "surfaces/nnormalsurfacelist.h"

#include "examplesaction.h"
#include "reginaabout.h"
#include "reginafilter.h"
#include "reginamain.h"
#include "reginapref.h"
#include "reginasupport.h"
#include "../part/reginapart.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSize>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWhatsThis>

ReginaMain::ReginaMain(ReginaManager* parent, bool showAdvice) {

    setAttribute(Qt::WA_DeleteOnClose);

    // Accept drag and drop.
    setAcceptDrops(true);


    // Set up our actions and status bar.
    setupActions();
    // statusBar()->show();

    // Read the configuration.
    readOptions();

    // Don't forget to save toolbar/etc settings.
    //setAutoSaveSettings(QString::fromLatin1("MainWindow"), true);

    aboutApp = 0;

    // Mark actions as missing for now.
    editAct = 0;
    saveAct = 0;
    saveAsAct = 0;
    treeMenu = 0;
    packetMenu = 0;
    packetTreeToolBar = 0;

    currentPart = 0;

    // Track the parent manager.
    manager = parent;

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

void ReginaMain::setPreferences(const ReginaPrefSet& prefs) {
    globalPrefs = prefs;
    emit preferencesChanged(globalPrefs);
    consoles.updatePreferences(globalPrefs);
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
        saveOptions();
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
    consoles.launchPythonConsole(this, &globalPrefs);
}

void ReginaMain::pythonReference() {
    PythonManager::openPythonReference(this);
}

void ReginaMain::close() {
    manager->onClose(this);
    close();
}

void ReginaMain::quit() {
    manager->quit();
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
    globalPrefs.openHandbook("index", 0, this);
}

void ReginaMain::helpXMLRef() {
    globalPrefs.openHandbook("index", "regina-xml", this);
}

void ReginaMain::helpWhatsThis() {
    QWhatsThis::enterWhatsThisMode();
}

void ReginaMain::helpTipOfDay() {
    // TODO
    //KTipDialog::showTip(this, QString::null, true);
}

void ReginaMain::helpTrouble() {
    globalPrefs.openHandbook("troubleshooting", 0, this);
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
    actNew->setShortcut(tr("Ctrl+n"));
    actNew->setWhatsThis(tr("Create a new topology data file.  This is "
        "the standard type of data file used by Regina."));
    connect(actNew, SIGNAL(triggered()), this, SLOT(newTopology()));
    fileMenu->addAction(actNew);
    toolBar->addAction(actNew);

    actOpen = new QAction(this);
    actOpen->setText(tr("&Open..."));
    actOpen->setIcon(ReginaSupport::themeIcon("document-open"));
    actOpen->setShortcut(tr("Ctrl+o"));
    actOpen->setWhatsThis(tr("Open a topology data file."));
    connect(actOpen, SIGNAL(triggered()), this, SLOT(fileOpen()));
    fileMenu->addAction(actOpen);
    toolBar->addAction(actOpen);
   
    fileOpenExample = new ExamplesAction(this);
    fillExamples();
    connect(fileOpenExample, SIGNAL(urlSelected(const QUrl&, const QString&)),
        this, SLOT(openExample(const QUrl&, const QString&)));
    fileMenu->addMenu(fileOpenExample);

    /*
    act = new QAction(this);
    act->setText(tr("&Save"));
    act->setIcon(ReginaSupport::themeIcon("document-save"));
    act->setShortcut(tr("Ctrl+s"));
    act->setWhatsThis(tr("Save the topology data to a file."));
    connect(act, SIGNAL(triggered()), this, SLOT(saveUrl()));
    fileMenu->addAction(act);
    toolBar->addAction(act); 

    act = new QAction(this);
    act->setText(tr("Save &As..."));
    act->setIcon(ReginaSupport::themeIcon("document-save-as"));
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
    act->setShortcut(tr("Ctrl+w"));
    act->setWhatsThis(tr("Close this topology data file."));
    connect(act, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(act);

    act = new QAction(this);
    act->setText(tr("&Quit"));
    act->setIcon(ReginaSupport::themeIcon("application-exit"));
    act->setShortcut(tr("Ctrl+q"));
    act->setWhatsThis(tr("Close all files and quit Regina."));
    connect(act, SIGNAL(triggered()), this, SLOT(quit()));
    fileMenu->addAction(act);

    // Toolbar and status bar:
    //showToolbar = KStandardAction::showToolbar(this,
    //    SLOT(optionsShowToolbar()), actionCollection());
    //setStandardToolBarMenuEnabled(true);
    
    /*
    showStatusbar = KStandardAction::showStatusbar(this,
        SLOT(optionsShowStatusbar()), actionCollection());
    */


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
    act->setWhatsThis(tr("Configure Regina.  Here you can set "
        "your own preferences for how Regina behaves."));
    connect(act, SIGNAL(triggered()), this, SLOT(optionsPreferences()));
    settingsMenu->addAction(act);


    QMenu *helpMenu =  menuBar()->addMenu(tr("&Help"));
    // Help:
    act = new QAction(this);
    act->setText(tr("&About Regina"));
    act->setIcon(ReginaSupport::themeIcon("help-about"));
    act->setWhatsThis(tr("Display information about Regina, such as "
        "the authors, license and website."));
    connect(act, SIGNAL(triggered()), this, SLOT(helpAboutApp()));
    helpMenu->addAction(act);


    act = new QAction(this);
    act->setText(tr("Regina &Handbook"));
    act->setIcon(ReginaSupport::themeIcon("help-contents"));
    act->setShortcut(tr("F1"));
    act->setWhatsThis(tr("Open the Regina handbook.  "
        "This is the main users' guide for how to use Regina."));
    connect(act, SIGNAL(triggered()), this, SLOT(helpHandbook()));
    helpMenu->addAction(act);

    act = new QAction(this);
    act->setText(tr("What's &This?"));
    act->setIcon(ReginaSupport::themeIcon("help-hint"));
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
    if (currentPart && ! currentPart->url().isEmpty()) {
        // TODO Save recent file
        //fileOpenRecent->addUrl(currentPart->url());

        // Save the new file list to the global configuration.
        // Note that the other main windows will be updated because of this.
        saveOptions();
    }
}

void ReginaMain::readOptions() {
    
    // Read in new preferences.
    
    QSettings settings;
    settings.beginGroup("Display");
    globalPrefs.autoDock = settings.value("PacketDocking", true).toBool();
    globalPrefs.displayTagsInTree = 
        settings.value("DisplayTagsInTree", false).toBool();

    settings.endGroup();
    settings.beginGroup("Census");
    QStringList censusStrings = 
        settings.value("Files", QStringList()).value<QStringList>();
    if (censusStrings.empty())
        globalPrefs.censusFiles = ReginaPrefSet::defaultCensusFiles();
    else {
        globalPrefs.censusFiles.clear();

        // Each string must start with + or - (active or inactive).
        // Any other strings will be ignored.
        for (QStringList::const_iterator it = censusStrings.begin();
                it != censusStrings.end(); it++) {
            if ((*it).isEmpty())
                continue;
            switch ((*it)[0].toAscii()) {
                case '+':
                    // Active file.
                    globalPrefs.censusFiles.push_back(ReginaFilePref(
                        (*it).mid(1), true));
                    break;
                case '-':
                    // Inactive file.
                    globalPrefs.censusFiles.push_back(ReginaFilePref(
                        (*it).mid(1), false));
                    break;
            }
        }
    }
    settings.endGroup();
    
    settings.beginGroup("Doc");
    globalPrefs.handbookInKHelpCenter = settings.value(
        "HandbookInKHelpCenter", false).toBool();
    settings.endGroup();


    settings.beginGroup("File");
    globalPrefs.autoFileExtension = settings.value(
        "AutomaticExtension", true).toBool();
    // TODO: Replace this, recentFiles
    // fileOpenRecent->loadEntries(*configGroup);
    settings.endGroup();

    settings.beginGroup("PDF");
    globalPrefs.pdfAutoClose = settings.value("AutoClose", true).toBool();
#ifdef __APPLE__
    // On MacOSX, use an external viewer by default.
    globalPrefs.pdfEmbed = settings.value("Embed", false).toBool();
#else
    // On Linux, use an embedded viewer if we can.
    globalPrefs.pdfEmbed = settings.value("Embed", true).toBool();
#endif
    globalPrefs.pdfExternalViewer = settings.value("ExternalViewer").
        toString().trimmed();
    settings.endGroup();

    settings.beginGroup("Python");
    globalPrefs.pythonAutoIndent = 
        settings.value("AutoIndent", true).toBool();
    globalPrefs.pythonSpacesPerTab = settings.value("SpacesPerTab", 4).toInt();
    globalPrefs.pythonWordWrap = settings.value("WordWrap", false).toBool();
    settings.endGroup();

    settings.beginGroup("SnapPea");
    globalPrefs.snapPeaClosed = settings.value("AllowClosed", false).toBool();
    regina::NSnapPeaTriangulation::enableKernelMessages(
        settings.value("KernelMessages", false).toBool());
    settings.endGroup();


    settings.beginGroup("Surfaces");
    globalPrefs.surfacesCompatThreshold = settings.value(
        "CompatibilityThreshold", 100).toInt();

    globalPrefs.surfacesCreationCoords = settings.value(
        "CreationCoordinates", regina::NNormalSurfaceList::STANDARD).toInt();

    QString str = settings.value("InitialCompat").toString();
    if (str == "Global")
        globalPrefs.surfacesInitialCompat = ReginaPrefSet::GlobalCompat;
    else
        globalPrefs.surfacesInitialCompat = ReginaPrefSet::LocalCompat;
            /* default */

    str = settings.value("InitialTab").toString();
    if (str == "Coordinates")
        globalPrefs.surfacesInitialTab = ReginaPrefSet::Coordinates;
    else if (str == "Matching")
        globalPrefs.surfacesInitialTab = ReginaPrefSet::Matching;
    else if (str == "Compatibility")
        globalPrefs.surfacesInitialTab = ReginaPrefSet::Compatibility;
    else
        globalPrefs.surfacesInitialTab = ReginaPrefSet::Summary; /* default */
    settings.endGroup();

    settings.beginGroup("Tree");
    globalPrefs.treeJumpSize = settings.value("JumpSize", 10).toInt();
    settings.endGroup();

    settings.beginGroup("Triangulation");

    str = settings.value("InitialTab").toString();
    if (str == "Skeleton")
        globalPrefs.triInitialTab = ReginaPrefSet::Skeleton;
    else if (str == "Algebra")
        globalPrefs.triInitialTab = ReginaPrefSet::Algebra;
    else if (str == "Composition")
        globalPrefs.triInitialTab = ReginaPrefSet::Composition;
    else if (str == "Surfaces")
        globalPrefs.triInitialTab = ReginaPrefSet::Surfaces;
    else if (str == "SnapPea")
        globalPrefs.triInitialTab = ReginaPrefSet::SnapPea;
    else
        globalPrefs.triInitialTab = ReginaPrefSet::Gluings; /* default */

    str = settings.value("InitialSkeletonTab").toString();
    if (str == "FacePairingGraph")
        globalPrefs.triInitialSkeletonTab = ReginaPrefSet::FacePairingGraph;
    else
        globalPrefs.triInitialSkeletonTab = ReginaPrefSet::SkelComp; /* def. */

    str = settings.value("InitialAlgebraTab").toString();
    if (str == "FundGroup")
        globalPrefs.triInitialAlgebraTab = ReginaPrefSet::FundGroup;
    else if (str == "TuraevViro")
        globalPrefs.triInitialAlgebraTab = ReginaPrefSet::TuraevViro;
    else if (str == "CellularInfo")
        globalPrefs.triInitialAlgebraTab = ReginaPrefSet::CellularInfo;
    else
        globalPrefs.triInitialAlgebraTab = ReginaPrefSet::Homology; /* def. */

    globalPrefs.triSurfacePropsThreshold = settings.value(
        "SurfacePropsThreshold", 6).toInt();
    settings.endGroup();

    settings.beginGroup("Extensions");
    globalPrefs.triGAPExec = settings.value("GAPExec", "gap").toString().
        trimmed();
    globalPrefs.triGraphvizExec = settings.value("GraphvizExec", "neato").
        toString().trimmed();
    settings.endGroup();

    globalPrefs.readPythonLibraries();

    settings.beginGroup("ReginaMain");
    resize(settings.value("size", sizeHint()).toSize());
    settings.endGroup();

    emit preferencesChanged(globalPrefs);
    consoles.updatePreferences(globalPrefs);
}

void ReginaMain::saveOptions() {

    QSettings settings;
    settings.beginGroup("Display");
    // Save the current set of preferences.
    settings.setValue("PacketDocking", globalPrefs.autoDock);
    settings.setValue("PacketDocking", globalPrefs.autoDock);
    settings.setValue("DisplayTagsInTree", globalPrefs.displayTagsInTree);
    settings.endGroup();

    settings.beginGroup("Census");
    QStringList censusStrings;
    // Distinguish an empty list from an uninitialised list.
    if (globalPrefs.censusFiles.empty())
        censusStrings.push_back("0");
    else
        for (ReginaFilePrefList::const_iterator it =
                globalPrefs.censusFiles.begin();
                it != globalPrefs.censusFiles.end(); it++)
            censusStrings.push_back(((*it).active ? '+' : '-') +
                (*it).filename);
    settings.setValue("Files", censusStrings);
    settings.endGroup();

    settings.beginGroup("Doc");
    settings.setValue("HandbookInKHelpCenter",
        globalPrefs.handbookInKHelpCenter);
    settings.endGroup();

    settings.beginGroup("File");
    settings.setValue("AutomaticExtension", globalPrefs.autoFileExtension);
    //fileOpenRecent->saveEntries(*configGroup); TODO recent files
    settings.endGroup();

    settings.beginGroup("PDF");
    settings.setValue("AutoClose", globalPrefs.pdfAutoClose);
    settings.setValue("Embed", globalPrefs.pdfEmbed);
    settings.setValue("ExternalViewer", globalPrefs.pdfExternalViewer);
    settings.endGroup();

    settings.beginGroup("Python");
    settings.setValue("AutoIndent", globalPrefs.pythonAutoIndent);
    settings.setValue("SpacesPerTab", globalPrefs.pythonSpacesPerTab);
    settings.setValue("WordWrap", globalPrefs.pythonWordWrap);
    settings.endGroup();

    settings.beginGroup("SnapPea");
    settings.setValue("AllowClosed", globalPrefs.snapPeaClosed);
    settings.setValue("KernelMessages",
        regina::NSnapPeaTriangulation::kernelMessagesEnabled());
    settings.endGroup();

    settings.beginGroup("Surfaces");
    settings.setValue("CompatibilityThreshold",
        globalPrefs.surfacesCompatThreshold);
    settings.setValue("CreationCoordinates",
        globalPrefs.surfacesCreationCoords);

    switch (globalPrefs.surfacesInitialCompat) {
        case ReginaPrefSet::GlobalCompat:
            settings.setValue("InitialCompat", "Global"); break;
        default:
            settings.setValue("InitialCompat", "Local"); break;
    }

    switch (globalPrefs.surfacesInitialTab) {
        case ReginaPrefSet::Coordinates:
            settings.setValue("InitialTab", "Coordinates"); break;
        case ReginaPrefSet::Matching:
            settings.setValue("InitialTab", "Matching"); break;
        case ReginaPrefSet::Compatibility:
            settings.setValue("InitialTab", "Compatibility"); break;
        default:
            settings.setValue("InitialTab", "Summary"); break;
    }
    settings.endGroup();

    settings.beginGroup("Tree");
    settings.setValue("JumpSize", globalPrefs.treeJumpSize);
    settings.endGroup();

    settings.beginGroup("Triangulation");

    switch (globalPrefs.triInitialTab) {
        case ReginaPrefSet::Skeleton:
            settings.setValue("InitialTab", "Skeleton"); break;
        case ReginaPrefSet::Algebra:
            settings.setValue("InitialTab", "Algebra"); break;
        case ReginaPrefSet::Composition:
            settings.setValue("InitialTab", "Composition"); break;
        case ReginaPrefSet::Surfaces:
            settings.setValue("InitialTab", "Surfaces"); break;
        case ReginaPrefSet::SnapPea:
            settings.setValue("InitialTab", "SnapPea"); break;
        default:
            settings.setValue("InitialTab", "Gluings"); break;
    }

    switch (globalPrefs.triInitialSkeletonTab) {
        case ReginaPrefSet::FacePairingGraph:
            settings.setValue("InitialSkeletonTab", "FacePairingGraph"); break;
        default:
            settings.setValue("InitialSkeletonTab", "SkelComp"); break;
    }

    switch (globalPrefs.triInitialAlgebraTab) {
        case ReginaPrefSet::FundGroup:
            settings.setValue("InitialAlgebraTab", "FundGroup"); break;
        case ReginaPrefSet::TuraevViro:
            settings.setValue("InitialAlgebraTab", "TuraevViro"); break;
        case ReginaPrefSet::CellularInfo:
            settings.setValue("InitialAlgebraTab", "CellularInfo"); break;
        default:
            settings.setValue("InitialAlgebraTab", "Homology"); break;
    }

    settings.setValue("SurfacePropsThreshold",
        globalPrefs.triSurfacePropsThreshold);
    settings.endGroup();

    settings.beginGroup("Extensions");
    settings.setValue("GAPExec", globalPrefs.triGAPExec);
    settings.setValue("GraphvizExec", globalPrefs.triGraphvizExec);
    settings.endGroup();

    globalPrefs.writePythonLibraries();

    settings.beginGroup("ReginaMain");
    settings.setValue("size", size());
    settings.endGroup();

    // TODO Call via reginaMain to ensure windows read new options.
    // Make sure other main windows read in and acknowledge the new options.
//    QListIterator<KMainWindow*> it(memberList());
//    while(it.hasNext()) {
//        KMainWindow* otherMain = it.next();
//        if (otherMain != this) {
//            ReginaMain* regina = qobject_cast<ReginaMain*>(otherMain);
//            if (regina) 
//                regina->readOptions();
//        }
//    }
}

void ReginaMain::newTopologyPart() {
    currentPart = new ReginaPart(this, QStringList());

    // Connect up signals and slots.
    connect(this, SIGNAL(preferencesChanged(const ReginaPrefSet&)),
        currentPart, SLOT(updatePreferences(const ReginaPrefSet&)));

    disconnect(actPython, SIGNAL(triggered()), this, SLOT(pythonConsole()));
    connect(actPython, SIGNAL(triggered()), currentPart, SLOT(pythonConsole()));

    // Perform initial setup on the part.
    emit preferencesChanged(globalPrefs);

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
    // Use roughly 2/3 the screen size by default.
    QSize use = QApplication::desktop()->availableGeometry().size();
    use *= 2;
    use /= 3;
    return use;
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

