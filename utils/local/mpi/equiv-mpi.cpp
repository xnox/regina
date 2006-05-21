
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  Attempt to find triangulations related by few elementary moves        *
 *                                                                        *
 *  Copyright (c) 2005-2006, Ben Burton                                   *
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

/**
 * This utility is a parallelised version of equiv.cpp, found in the
 * parent directory.
 *
 * Processes communicate through MPI; the process with rank 0 is the
 * controller and the remainder are slaves.  Each triangulation is
 * farmed out to a slave for processing, where the slave attempts to
 * manipulate it as described in equiv.cpp, producing either a smaller
 * triangulation or equivalent triangulations of the same size.
 *
 * TODO: Talk about output and logging.
 */

#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <popt.h>

#include "file/nxmlfile.h"
#include "packet/ncontainer.h"
#include "triangulation/ntriangulation.h"

#include "mpi.h"

// MPI constants:
#define TAG_REQUEST_TASK 10
#define TAG_RESULT 20
#define TAG_RESULT_DATA 20

// Task result codes:
#define RESULT_OK 1
#define RESULT_NON_MINIMAL 2
#define RESULT_HAS_NEW 3
#define RESULT_ERR 10

// Time constants:
#define MIN_SEC 60
#define HOUR_SEC (60 * MIN_SEC)
#define DAY_SEC (24 * HOUR_SEC)

// MPI constraints:
// TODO: Verify label uniqueness and lengths
#define MAX_TRI_LABEL_LEN 250

using namespace regina;



/**
 * General globals for both controller and slaves.
 * Includes all command-line options (which are read by all processes).
 */

// Maximum number of moves of each type to make.
int argUp = 1;
int argAcross = 1;
int argDown = 1;

// The input file.
std::string inFile;

// The output file (if output is required).
const char* outFile = 0;

// The input packet tree.
NPacket* tree = 0;

// A set of triangulations that have been found to be homeomorphic to
// each other.  A set of this type forms the solution to a single slave task.
typedef std::set<NTriangulation*> TriSet;
TriSet equivs;



/**
 * Controller-specific globals.
 */

// MPI and housekeeping.
int nSlaves;
int nRunningSlaves = 0;
std::ofstream logger;
const char* logFile = "equiv.log";

// The output packet tree.
NPacket* newTree = 0;

// A mapping from triangulations to equivalence classes.
typedef std::map<NTriangulation*, int> ClassMap;
ClassMap eClass;
int nextClass = 0;

// Overall statistics.
unsigned long nTris = 0;
unsigned long nClasses = 0;
unsigned long nNonMin = 0;
unsigned long nHasNew = 0;



/**
 * Slave-specific globals.
 */

// The original triangulation currently being processed.
NTriangulation* orig;

// Facts we have discovered about the original triangulation.
bool nonMin, hasNew;



/**
 * Generic helper routine.
 *
 * Parse command-line arguments for move and filename options.
 */
int parseCmdLine(int argc, const char* argv[]) {
    // Set up the command-line arguments.
    poptOption opts[] = {
        { "up", 'u', POPT_ARG_INT, &argUp, 0,
            "Number of initial 2-3 moves (default is 1).", "<moves>" },
        { "across", 'a', POPT_ARG_INT, &argAcross, 0,
            "Number of subsequent 4-4 moves (default is 1).", "<moves>" },
        { "down", 'd', POPT_ARG_INT, &argDown, 0,
            "Number of final reduction moves before the greedy simplification (default is 1).", "<moves>" },
        { "output", 'o', POPT_ARG_STRING, &outFile, 0,
            "Output equivalence classes (plus new triangulations) to the given file.", "<output-file>" },
        POPT_AUTOHELP
        { 0, 0, 0, 0, 0, 0, 0 }
    };

    poptContext optCon = poptGetContext(0, argc, argv, opts, 0);
    poptSetOtherOptionHelp(optCon, "<file.rga>");

    // Parse the command-line arguments.
    int rc = poptGetNextOpt(optCon);
    if (rc != -1) {
        fprintf(stderr, "%s: %s\n\n",
            poptBadOption(optCon, POPT_BADOPTION_NOALIAS), poptStrerror(rc));
        poptPrintHelp(optCon, stderr, 0);
        poptFreeContext(optCon);
        return 1;
    }

    const char** otherOpts = poptGetArgs(optCon);
    if (otherOpts && otherOpts[0]) {
        inFile = otherOpts[0];
        if (otherOpts[1]) {
            fprintf(stderr, "Only one input filename may be supplied.\n\n");
            poptPrintHelp(optCon, stderr, 0);
            poptFreeContext(optCon);
            return 1;
        }
    } else {
        fprintf(stderr, "No input filename was supplied.\n\n");
        poptPrintHelp(optCon, stderr, 0);
        poptFreeContext(optCon);
        return 1;
    }

    // Run a sanity check on the command-line arguments.
    bool broken = false;
    if (argUp < 0) {
        fprintf(stderr, "The number of 2-3 moves up may not be negative.\n");
        broken = true;
    }
    if (argAcross < 0) {
        fprintf(stderr, "The number of 4-4 moves across may not be "
            "negative.\n");
        broken = true;
    }
    if (argDown < 0) {
        fprintf(stderr, "The number of reduction moves down may not be "
            "negative.\n");
        broken = true;
    }

    if (broken) {
        fprintf(stderr, "\n");
        poptPrintHelp(optCon, stderr, 0);
        poptFreeContext(optCon);
        return 1;
    }

    // Done parsing the command line.
    poptFreeContext(optCon);
    return 0;
}

/**
 * Generic helper routine.
 *
 * Write the given time in human-readable form to the given output stream.
 */
void writeTime(std::ostream& out, long seconds) {
    bool started = false;
    if (seconds >= DAY_SEC) {
        out << (seconds / DAY_SEC) << " days ";
        seconds = seconds % DAY_SEC;
        started = true;
    }
    if (started || seconds >= HOUR_SEC) {
        out << (seconds / HOUR_SEC) << " hrs ";
        seconds = seconds % HOUR_SEC;
        started = true;
    }
    if (started || seconds >= MIN_SEC) {
        out << (seconds / MIN_SEC) << " min ";
        seconds = seconds % MIN_SEC;
        started = true;
    }
    out << seconds << " sec";
}

/**
 * Controller helper routine.
 *
 * Write the current date and time plus whitespace to the log stream and
 * return a reference to the log stream.
 */
std::ostream& ctrlLogStamp() {
    time_t t = time(0);
    std::string time = asctime(localtime(&t));
    // Remove the trailing newline.
    if (time[time.length() - 1] == '\n')
        time.resize(time.length() - 1);
    return logger << time << "  ";
}

/**
 * Controller helper routine.
 *
 * Wait for the next running slave to finish a task.  Note that if no
 * slaves are currently processing tasks, this routine will block forever!
 */
int ctrlWaitForSlave() {
    long result;
    MPI_Status status;
    MPI_Recv(&result, 1, MPI_LONG, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD,
        &status);
    nRunningSlaves--;

    int slave = status.MPI_SOURCE;
    ctrlLogStamp() << "Task completed by slave " << slave << "." << std::endl;

    char triLabel[MAX_TRI_LABEL_LEN + 1];

    // TODO: Mark whether an error occurred.
    if (result == RESULT_OK) {
        equivs.clear();

        NPacket* p;
        NTriangulation* tri;
        while (1) {
            MPI_Recv(triLabel, MAX_TRI_LABEL_LEN + 1, MPI_CHAR, slave,
                TAG_RESULT_DATA, MPI_COMM_WORLD, &status);
            if (*triLabel == 0)
                break;

            p = tree->findPacketLabel(triLabel);
            if (! p)
                ctrlLogStamp() << "ERROR: Equivalent [" << triLabel
                    << "] not found" << std::endl;
            else {
                tri = dynamic_cast<NTriangulation*>(p);
                if (! tri)
                    ctrlLogStamp() << "ERROR: Equivalent [" << triLabel
                        << "] is not a triangulation!" << std::endl;
                else
                    equivs.insert(tri);
            }
        }
        ctrlLogStamp() << "Resulting set contains "
            << equivs.size() << " equivalent(s)." << std::endl;

        // In equivs we now have a list of all triangulations
        // equivalent to orig.

        // Is this an equivalence class we're already seen?
        TriSet::iterator tit;
        ClassMap::iterator cit, cit2;
        for (tit = equivs.begin(); tit != equivs.end(); tit++) {
            cit = eClass.find(*tit);
            if (cit != eClass.end())
                break;
        }
        if (tit != equivs.end()) {
            // We found an equivalence class.  Insert everything we
            // haven't seen yet, and merge the classes of everything
            // we have.
            int c, cOld;
            c = cit->second;
            for (tit = equivs.begin(); tit != equivs.end(); tit++) {
                cit = eClass.find(*tit);
                if (cit == eClass.end())
                    eClass.insert(std::make_pair(*tit, c));
                else if (cit->second != c) {
                    // Merge the two equivalence classes.
                    cOld = cit->second;
                    for (cit = eClass.begin(); cit != eClass.end(); cit++)
                        if (cit->second == cOld)
                            cit->second = c;
                    nClasses--;
                }
            }
        } else {
            // No such equivalence class.  Insert everything.
            int c = nextClass++;
            for (tit = equivs.begin(); tit != equivs.end(); tit++)
                eClass.insert(std::make_pair(*tit, c));
            nClasses++;
        }
    } else if (result == RESULT_NON_MINIMAL) {
        MPI_Recv(triLabel, MAX_TRI_LABEL_LEN + 1, MPI_CHAR, slave,
            TAG_RESULT_DATA, MPI_COMM_WORLD, &status);
        ctrlLogStamp() << "Non-minimal triangulation: " << triLabel
            << std::endl;

        nNonMin++;
    } else if (result == RESULT_HAS_NEW) {
        MPI_Recv(triLabel, MAX_TRI_LABEL_LEN + 1, MPI_CHAR, slave,
            TAG_RESULT_DATA, MPI_COMM_WORLD, &status);
        ctrlLogStamp() << "WARNING: Has unseen equivalent: " << triLabel
            << std::endl;

        nHasNew++;
    } else if (result == RESULT_ERR) {
        // TODO: Send error message.
        ctrlLogStamp() << "ERROR: Slave signalled an error." << std::endl;
    } else {
        ctrlLogStamp() << "ERROR: Unknown result code " << result
            << " received from slave." << std::endl;
    }

    return slave;
}

void ctrlFarmTri(NTriangulation* tri) {
    int slave;
    if (tri == 0 || nRunningSlaves == nSlaves) {
        // We need to wait for somebody to stop first.
        slave = ctrlWaitForSlave();
    } else
        slave = nRunningSlaves + 1;

    if (tri) {
        ctrlLogStamp() << "Farmed [" << tri->getPacketLabel()
            << "] to slave " << slave << "." << std::endl;

        MPI_Send(const_cast<char*>(tri->getPacketLabel().c_str()),
            tri->getPacketLabel().length() + 1, MPI_CHAR, slave,
            TAG_REQUEST_TASK, MPI_COMM_WORLD);
        nRunningSlaves++;
    } else {
        ctrlLogStamp() << "Stopping slave " << slave << "." << std::endl;

        char null = 0;
        MPI_Send(&null, 1, MPI_CHAR, slave, TAG_REQUEST_TASK, MPI_COMM_WORLD);
    }
}

/**
 * Main routine for the controller.
 */
int mainController() {
    NTriangulation* t;

    // Start logging.
    logger.open(logFile);
    if (! logger) {
        fprintf(stderr, "Could not open %s for writing.\n", logFile);
        return 1;
    }

    // Do it.
    for (NPacket* p = tree; p; p = p->nextTreePacket())
        if (p->getPacketType() == NTriangulation::packetType) {
            nTris++;
            ctrlFarmTri(static_cast<NTriangulation*>(p));
        }

    // Kill off any slaves that aren't working, since there are no tasks
    // to give them.
    if (nRunningSlaves < nSlaves) {
        // TODO
    }

    // Wait for remaining slaves to finish.
    while (nRunningSlaves > 0)
        ctrlFarmTri(0);

    // Done!
    logger << "All slaves finished." << std::endl;

    // Write the summary of results.
    if (nClasses) {
        printf("EQUIVALENCE CLASSES:\n\n");

        if (outFile) {
            newTree = new NContainer();
            newTree->setPacketLabel("Equivalence Classes");
        }

        int classNum = 1;
        std::string className;
        NContainer* classCnt = 0;

        ClassMap::iterator cit, cit2;
        int c;
        for (cit = eClass.begin(); cit != eClass.end(); cit++)
            if (cit->second >= 0) {
                // The first triangulation of a new equivalence class.
                c = cit->second;

                std::ostringstream s;
                s << "Class " << classNum << " : " <<
                    cit->first->getHomologyH1().toString();
                className = s.str();
                classNum++;

                printf("%s\n\n", className.c_str());
                if (outFile) {
                    classCnt = new NContainer();
                    classCnt->setPacketLabel(className);
                    newTree->insertChildLast(classCnt);
                }

                // Find the triangulations in this class, and erase the
                // class as we go.
                for (cit2 = cit; cit2 != eClass.end(); cit2++)
                    if (cit2->second == c) {
                        printf("    %s\n",
                            cit2->first->getPacketLabel().c_str());
                        if (outFile) {
                            t = new NTriangulation(*(cit2->first));
                            t->setPacketLabel(cit2->first->getPacketLabel());
                            classCnt->insertChildLast(t);
                        }

                        cit2->second = -1;
                    }

                printf("\n");
            }
    }

    printf("Final statistics:\n");
    printf("    Triangulations read:            %ld\n", nTris);
    printf("    Equivalence classes:            %ld\n", nClasses);
    printf("    Non-minimal triangulations:     %ld\n", nNonMin);
    printf("    Triangulations with new equivs: %ld\n", nHasNew);

    // Are we saving results?
    if (outFile && newTree) {
        fprintf(stderr, "\nSaving results to %s...\n", outFile);
        writeXMLFile(outFile, newTree);
    } else
        fprintf(stderr, "\nNot saving results.\n");

    // Clean up.
    if (newTree)
        delete newTree;

    return 0;
}

/**
 * Slave helper routine.
 *
 * Signal that a fatal error occurred whilst working on the current task.
 */
void slaveBail(const std::string& /* TODO error */) {
    long result = RESULT_ERR;
    MPI_Send(&result, 1, MPI_LONG, 0, TAG_RESULT, MPI_COMM_WORLD);
}

/**
 * Slave helper routine.
 *
 * Signal that the triangulation currently being processed is
 * non-minimal, and can be henceforth ignored.
 */
void slaveSendNonMin() {
    long result = RESULT_NON_MINIMAL;
    MPI_Send(&result, 1, MPI_LONG, 0, TAG_RESULT, MPI_COMM_WORLD);

    MPI_Send(const_cast<char*>(orig->getPacketLabel().c_str()),
        orig->getPacketLabel().length() + 1, MPI_CHAR, 0,
        TAG_RESULT_DATA, MPI_COMM_WORLD);
}

/**
 * Slave helper routine.
 *
 * Signal that the triangulation currently being processed has a
 * homeomorphic triangulation of the same size that we haven't seen.
 */
void slaveSendNew() {
    long result = RESULT_HAS_NEW;
    MPI_Send(&result, 1, MPI_LONG, 0, TAG_RESULT, MPI_COMM_WORLD);

    MPI_Send(const_cast<char*>(orig->getPacketLabel().c_str()),
        orig->getPacketLabel().length() + 1, MPI_CHAR, 0,
        TAG_RESULT_DATA, MPI_COMM_WORLD);
}

/**
 * Slave helper routine.
 *
 * Signal that processing for this task is complete.  A set of
 * homeomorphic triangulations is be returned.
 */
void slaveSendEquivs() {
    long result = RESULT_OK;
    MPI_Send(&result, 1, MPI_LONG, 0, TAG_RESULT, MPI_COMM_WORLD);

    for (TriSet::iterator tit = equivs.begin(); tit != equivs.end(); tit++)
        MPI_Send(const_cast<char*>((*tit)->getPacketLabel().c_str()),
            (*tit)->getPacketLabel().length() + 1, MPI_CHAR, 0,
            TAG_RESULT_DATA, MPI_COMM_WORLD);

    char null = 0;
    MPI_Send(&null, 1, MPI_CHAR, 0, TAG_RESULT, MPI_COMM_WORLD);
}

/**
 * Slave helper routine.
 *
 * We have a homeomorphic triangulation of the same size as the original.
 */
void slaveSameSize(NTriangulation* t) {
    // Hunt for it in the packet tree.
    NTriangulation* found = 0;
    for (NPacket* p = tree; p; p = p->nextTreePacket())
        if (p->getPacketType() == NTriangulation::packetType)
            if (static_cast<NTriangulation*>(p)->isIsomorphicTo(*t).get()) {
                found = static_cast<NTriangulation*>(p);
                break;
            }

    if (found == orig)
        return;

    if (! found) {
        hasNew = true;
        return;
    }

    // Yes, it's a set and so the insert should just fail if it's
    // already present.  But this is leading to a small memory leak on
    // my machine which, when multiplied by the _very_ large number of
    // times this is called, has rather unpleasant effects (such as
    // exhausting all swap space).  So let's keep the test here for now.
    if (! equivs.count(found))
        equivs.insert(found);
}

/**
 * Slave helper routine.
 *
 * Do the final greedy simplify and process.
 */
void slaveProcessAlt(NTriangulation* t) {
    t->intelligentSimplify();

    if (t->getNumberOfTetrahedra() < orig->getNumberOfTetrahedra())
        nonMin = true;
    else if (t->getNumberOfTetrahedra() == orig->getNumberOfTetrahedra())
        slaveSameSize(t);
}

/**
 * Slave helper routine.
 *
 * Perform reduction moves.  The given triangulation may be changed.
 */
void slaveTryMovesDown(NTriangulation* t, int maxLevels) {
    if (maxLevels == 0) {
        slaveProcessAlt(t);
        return;
    }

    NTriangulation* alt;
    unsigned i, j;
    bool found = false;

    for (i = 0; i < t->getNumberOfEdges(); i++)
        if (t->twoZeroMove(t->getEdge(i), true, false)) {
            alt = new NTriangulation(*t);
            alt->twoZeroMove(alt->getEdge(i));
            slaveTryMovesDown(alt, maxLevels - 1);
            found = true;
            delete alt;

            if (nonMin)
                return;
        }

    for (i = 0; i < t->getNumberOfEdges(); i++)
        for (j = 0; j < 2; j++)
            if (t->twoOneMove(t->getEdge(i), j, true, false)) {
                alt = new NTriangulation(*t);
                alt->twoOneMove(alt->getEdge(i), j);
                slaveTryMovesDown(alt, maxLevels - 1);
                found = true;
                delete alt;

                if (nonMin)
                    return;
            }

    // Only try 3-2 moves if nothing better has worked so far.
    if (! found)
        for (i = 0; i < t->getNumberOfEdges(); i++)
            if (t->threeTwoMove(t->getEdge(i), true, false)) {
                alt = new NTriangulation(*t);
                alt->threeTwoMove(alt->getEdge(i));
                slaveTryMovesDown(alt, maxLevels - 1);
                found = true;
                delete alt;

                if (nonMin)
                    return;
            }

    // Only try 4-4 moves if nothing else has worked.
    if (! found)
        for (i = 0; i < t->getNumberOfEdges(); i++)
            for (j = 0; j < 2; j++)
                if (t->fourFourMove(t->getEdge(i), j, true, false)) {
                    alt = new NTriangulation(*t);
                    alt->fourFourMove(alt->getEdge(i), j);
                    slaveTryMovesDown(alt, maxLevels - 1);
                    found = true;
                    delete alt;

                    if (nonMin)
                        return;
                }

    // No more moves to make.  Run straight to intelligentSimplify().
    if (! found)
        slaveProcessAlt(t);
}

/**
 * Slave helper routine.
 *
 * Perform 4-4 moves.  The given triangulation may be changed.
 * Moves that revert to prev, prev2 or prev3 will not be considered.
 */
void slaveTryMovesAcross(NTriangulation* t, int maxLevels,
        NTriangulation* prev = 0, NTriangulation* prev2 = 0,
        NTriangulation* prev3 = 0) {
    unsigned i, j;
    NTriangulation* alt;

    if (maxLevels > 0)
        for (i = 0; i < t->getNumberOfEdges(); i++)
            for (j = 0; j < 2; j++)
                if (t->fourFourMove(t->getEdge(i), j, true, false)) {
                    alt = new NTriangulation(*t);
                    alt->fourFourMove(alt->getEdge(i), j);
                    if (prev && alt->isIsomorphicTo(*prev).get()) {
                        // Ignore, reversion.
                    } else if (prev2 && alt->isIsomorphicTo(*prev2).get()) {
                        // Ignore, reversion.
                    } else if (prev3 && alt->isIsomorphicTo(*prev3).get()) {
                        // Ignore, reversion.
                    } else
                        slaveTryMovesAcross(alt, maxLevels - 1, t, prev, prev2);
                    delete alt;

                    if (nonMin)
                        return;
                }

    // Try just going for the simplify.
    slaveTryMovesDown(t, argDown);
}

/**
 * Slave helper routine.
 *
 * Perform 2-3 moves.  The given triangulation will not be changed.
 */
void slaveTryMovesUp(NTriangulation* t, int levelsRemaining) {
    NTriangulation* alt;

    if (levelsRemaining == 0) {
        // We're not allowed to change the original, so clone it.
        alt = new NTriangulation(*t);
        slaveTryMovesAcross(alt, argAcross);
        delete alt;
    } else {
        for (unsigned i = 0; i < t->getNumberOfFaces(); i++) {
            alt = new NTriangulation(*t);
            if (alt->twoThreeMove(alt->getFace(i))) {
                if (levelsRemaining > 1)
                    slaveTryMovesUp(alt, levelsRemaining - 1);
                else
                    slaveTryMovesAcross(alt, argAcross);
            }
            delete alt;

            if (nonMin)
                return;
        }
    }
}

/**
 * Slave helper routine.
 *
 * Process a single triangulation.
 */
void slaveProcessTri(NTriangulation* tri) {
    nonMin = false;
    orig = tri;
    equivs.clear();
    equivs.insert(orig);

    slaveTryMovesUp(orig, argUp);

    if (nonMin)
        slaveSendNonMin();
    else if (hasNew)
        slaveSendNew();
    else
        slaveSendEquivs();
}

/**
 * Main routine for a slave (ranks 1..size).
 */
int mainSlave() {
    char triLabel[MAX_TRI_LABEL_LEN + 1];
    NPacket* p;
    NTriangulation* tri;

    // Keep fetching and processing tasks until there are no more.
    MPI_Status status;
    while (true) {
        // Get the next processing task.
        MPI_Recv(triLabel, MAX_TRI_LABEL_LEN + 1, MPI_CHAR, 0,
            TAG_REQUEST_TASK, MPI_COMM_WORLD, &status);

        if (*triLabel == 0)
            break;

        p = tree->findPacketLabel(triLabel);
        if (! p)
            slaveBail(std::string("Packet ") + triLabel + " not found.");
        else {
            tri = dynamic_cast<NTriangulation*>(p);
            if (! tri)
                slaveBail(std::string("Packet ") + triLabel +
                    " is not a triangulation.");
            else
                slaveProcessTri(tri);
        }
    }

    return 0;
}

/**
 * Main routine for all processors.
 *
 * Parse the command-line arguments for options, then determine whether
 * we are controller or slave and run a specialised main routine accordingly.
 */
int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    // Which processor are we?
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Extract command-line options.
    if (parseCmdLine(argc, (const char**)argv)) {
        MPI_Finalize();
        return 1;
    }

    // Read the input file.
    if (! (tree = readXMLFile(inFile.c_str()))) {
        fprintf(stderr, "ERROR: Could not read data from %s.\n",
            inFile.c_str());
        MPI_Finalize();
        return 1;
    }

    // Controller or slave?
    int retVal;
    if (rank == 0) {
        // We're the controller.
        // How many processors in total?
        int size;
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        if (size <= 1) {
            fprintf(stderr, "ERROR: At least two processors are required "
                "(one controller and one slave).\n");
            retVal = 1;
        } else {
            nSlaves = size - 1;
            retVal = mainController();
        }
    } else {
        // We're one of many slaves.
        retVal = mainSlave();
    }

    // Clean up
    delete tree;

    MPI_Finalize();
    return retVal;
}

