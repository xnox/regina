
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  Computational Engine                                                  *
 *                                                                        *
 *  Copyright (c) 1999-2018, Ben Burton                                   *
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
#include "maths/laurent2.h"
#include "utilities/bitmask.h"
#include "utilities/bitmanip.h"
#include "utilities/sequence.h"

// #define DUMP_STATES
#define IDENTIFY_NONVIABLE_KEYS

namespace regina {

const char* Link::homflyAZVarX = "\u03B1"; // alpha
const char* Link::homflyAZVarY = "z";

const char* Link::homflyLMVarX = "\U0001D4C1"; // mathematical script small l
const char* Link::homflyLMVarY = "m";

const char* Link::homflyVarX = homflyAZVarX;
const char* Link::homflyVarY = homflyAZVarY;

namespace {
    /**
     * Possible states of crossings.  Used by Kauffman's algorithm.
     */
    enum CrossingState {
        /**
         * Not yet visited.  Moreover, this state indicates that - if there is
         * a decision to make - we should first attempt to switch this crossing.
         */
        CROSSING_UNSEEN = 0,

        /**
         * Not yet visited.  Moreover, this state indicates that - if there
         * is a decision to make - we have already attempted switching the
         * crossing, and we should now try to splice instead.
         */
        CROSSING_TRIED = 1,

        /**
         * First seen on the upper strand, and so the crossing was kept intact.
         * Visited only once so far.
         */
        CROSSING_KEEP_1 = 2,

        /**
         * First seen on the upper strand, and so the crossing was kept intact.
         * Visited twice (first upper, then lower).
         */
        CROSSING_KEEP_2 = 3,

        /**
         * First seen on the lower strand, and the decision was made to
         * switch the crossing.  Visited only once so far.
         */
        CROSSING_SWITCH_1 = 4,

        /**
         * First seen on the lower strand, and the decision was made to
         * switch the crossing.  Visited twice.
         */
        CROSSING_SWITCH_2 = 5,

        /**
         * First seen on the lower strand, and the decision was made to
         * splice.  Visited only once so far.
         */
        CROSSING_SPLICE_1 = 6,

        /**
         * First seen on the lower strand, and the decision was made to
         * splice.  Visited twice.
         */
        CROSSING_SPLICE_2 = 7
    };

    /**
     * Helper data used by the treewidth-based algorithm to test whether a
     * key is viable.  In other words, this tests whether the data from
     * a given key \e might survive all the way up to the root of the
     * tree decomposition.
     */
    struct ViabilityData {
        const Link* link;

        /**
         * An array that gives the bag index at which each crossing is
         * eventually forgotten.
         *
         * It is assumed that the underlying tree decomposition is nice.
         *
         * This array is a function of the link only, and is initialised
         * in the ViabilityData constructor.
         */
        int* forgetCrossing;

        /**
         * An array that, for each strand ID, records which of the two
         * crossings is forgotten last.  This array stores crossing IDs.
         *
         * This array is a function of the link only, and is initialised
         * in the ViabilityData constructor.
         */
        int* lastCrossing;

        /**
         * An array that, for each strand ID, records the bag index at
         * which the entire strand is forgotten.
         *
         * This array is a function of the link only, and is initialised
         * in the ViabilityData constructor.
         */
        int* forgetStrand;

        /**
         * The first few steps of any link traversal are forced (for as
         * long as we perform only pass moves).  For each strand ID, this
         * array stores the position of the strand in these forced initial
         * steps, or -1 if it does not appear amongst these forced steps.
         *
         * This array is a function of the link only, and is initialised
         * in the ViabilityData constructor.
         */
        int* prefix;

        /**
         * For a crossing at index i that lives in the current bag,
         * mask[i] is a bitwise combination of:
         *
         * * 1 if the lower incoming strand comes from the forgotten zone;
         * * 2 if the upper incoming strand comes from the forgotten zone;
         * * 4 if the lower outgoing strand goes into the forgotten zone;
         * * 8 if the upper outgoing strand goes into the forgotten zone.
         *
         * This array is a function of the bag being processed, and is
         * initialised by initForgetBag() and initJoinBag().
         */
        char* mask;

        /**
         * The number of pairs in each key at a join bag (that is, half
         * the key length).
         *
         * This is only available for join bags, and is initialised by
         * initJoinBag().
         */
        int pairs;

        /**
         * An array that tracks the values of maxForget, a single variable used
         * in keyViable(), over successive calls to partialKeyViable().
         * By using this array, we are able to access previous values of
         * maxForget when backtracking.
         *
         * The role of this variable is documented within keyViable().
         *
         * This is only available for join bags, and is initialised by
         * initJoinBag().
         */
        int *maxForget;

        /**
         * An array that tracks the values of needStartLoop, an integer variable
         * used in keyViable(), over successive calls to partialKeyViable().
         * By using this array, we are able to access previous values of
         * needStartLoop when backtracking.
         *
         * The role of this variable is documented within keyViable().
         *
         * This is only available for join bags, and is initialised by
         * initJoinBag().
         */
        int *needStartLoop;

        /**
         * An array that tracks the values of couldEndLoop, an integer variable
         * used in keyViable(), over successive calls to partialKeyViable().
         * By using this array, we are able to access previous values of
         * couldEndLoop when backtracking.
         *
         * The role of this variable is documented within keyViable().
         *
         * This is only available for join bags, and is initialised by
         * initJoinBag().
         */
        int *couldEndLoop;

#ifdef IDENTIFY_NONVIABLE_KEYS
        /**
         * Marked as \c true when a key is determined to be viable.
         * This allows diagnostic code to determine whether \e any of a set
         * of potential keys was found to be viable.
         *
         * This class never initialises \a foundViable or marks it as \c false;
         * that is the job of whatever diagnostic code uses it.
         */
        bool foundViable;
#endif

        ViabilityData(const Link* l, const TreeDecomposition& d) :
                link(l),
                forgetCrossing(new int[l->size()]),
                lastCrossing(new int[2 * l->size()]),
                forgetStrand(new int[2 * l->size()]),
                prefix(new int[2 * l->size()]),
                mask(new char[l->size()]),
                maxForget(nullptr),
                needStartLoop(nullptr),
                couldEndLoop(nullptr) {
            const TreeBag* b;
            for (b = d.first(); b; b = b->next())
                if (b->type() == NICE_FORGET)
                    forgetCrossing[b->children()->element(b->subtype())] =
                        b->index();

            int from, to;
            for (int i = 0; i < 2 * l->size(); ++i) {
                from = i / 2;
                to = l->strand(i).next().crossing()->index();
                if (forgetCrossing[from] >= forgetCrossing[to]) {
                    lastCrossing[i] = from;
                    forgetStrand[i] = forgetCrossing[from];
                } else {
                    lastCrossing[i] = to;
                    forgetStrand[i] = forgetCrossing[to];
                }
            }

            // Fill prefix[] by following from the upper strand of the root
            // crossing for as far as we can go using only pass moves.
            std::fill(prefix, prefix + 2 * l->size(), -1);

            b = d.root();
            StrandRef start(
                link->crossing(b->children()->element(b->subtype())), 1);

            StrandRef s = start;
            int pos = 0;
            do {
                prefix[s.id()] = pos++;
                ++s;
            } while (! (s == start /* finished a link component */ ||
                (s.strand() == 0 && prefix[s.id() | 1] < 0) /* not a pass */));
        }

        ~ViabilityData() {
            delete[] couldEndLoop;
            delete[] needStartLoop;
            delete[] maxForget;
            delete[] mask;
            delete[] prefix;
            delete[] forgetStrand;
            delete[] lastCrossing;
            delete[] forgetCrossing;
        }

        void initForgetBag(const TreeBag* bag,
                const LightweightSequence<int>* childKey,
                const Crossing* forget) {
            std::fill(mask, mask + link->size(), 0);

            // Identify all strands where one crossing is forgotten and
            // the other is not.
            int strandID;
            StrandRef from, to;
            for (int i = 0; i < childKey->size(); ++i) {
                strandID = (*childKey)[i];

                // In the child bag, this strand ran between the bag and the
                // forgotten zone.

                from = link->strand(strandID);
                to = from.next();

                if (from.crossing() == forget || to.crossing() == forget) {
                    // The entire strand is lost in this (the parent bag).
                    continue;
                }

                // The strand survives through to this bag.
                if (lastCrossing[strandID] == from.crossing()->index()) {
                    // The strand runs from the bag into the forgotten zone.
                    if (from.strand() == 0)
                        mask[from.crossing()->index()] |= 4;
                    else
                        mask[from.crossing()->index()] |= 8;
                } else {
                    // The strand runs from the forgotten zone into the bag.
                    if (to.strand() == 0)
                        mask[to.crossing()->index()] |= 1;
                    else
                        mask[to.crossing()->index()] |= 2;
                }
            }

            // We also need to collect strands that run between the
            // newly-forgotten crossing and the bag.

            for (int i = 0; i < 2; ++i) {
                // From newly-forgotten crossing into the bag:
                to = forget->next(i);
                if (forgetCrossing[to.crossing()->index()] > bag->index()) {
                    if (to.strand() == 0)
                        mask[to.crossing()->index()] |= 1;
                    else
                        mask[to.crossing()->index()] |= 2;
                }

                // From the bag into the newly-forgotten crossing:
                from = forget->prev(i);
                if (forgetCrossing[from.crossing()->index()] > bag->index()) {
                    if (from.strand() == 0)
                        mask[from.crossing()->index()] |= 4;
                    else
                        mask[from.crossing()->index()] |= 8;
                }
            }
        }

        void initJoinBag(const LightweightSequence<int>* leftChildKey,
                const LightweightSequence<int>* rightChildKey) {
            std::fill(mask, mask + link->size(), 0);

            // Identify all strands where one crossing is forgotten and
            // the other is not.
            const LightweightSequence<int>* childKey;
            int strandID;
            StrandRef from, to;
            int side, i;
            for (side = 0; side < 2; ++side) {
                childKey = (side == 0 ? leftChildKey : rightChildKey);
                for (i = 0; i < childKey->size(); ++i) {
                    strandID = (*childKey)[i];

                    // This strand runs between the bag and the forgotten zone.

                    from = link->strand(strandID);
                    to = from.next();

                    if (lastCrossing[strandID] == from.crossing()->index()) {
                        // The strand runs from the bag into the forgotten zone.
                        if (from.strand() == 0)
                            mask[from.crossing()->index()] |= 4;
                        else
                            mask[from.crossing()->index()] |= 8;
                    } else {
                        // The strand runs from the forgotten zone into the bag.
                        if (to.strand() == 0)
                            mask[to.crossing()->index()] |= 1;
                        else
                            mask[to.crossing()->index()] |= 2;
                    }
                }
            }

            // Compute the number of pairs in the join key.
            pairs = (leftChildKey->size() + rightChildKey->size()) / 2;

            // Set up our stacks of partially computed values.
            delete[] couldEndLoop;
            delete[] needStartLoop;
            delete[] maxForget;

            maxForget = new int[pairs + 1];
            needStartLoop = new int[pairs + 1];
            couldEndLoop = new int[pairs + 1];
            maxForget[pairs] = needStartLoop[pairs] = couldEndLoop[pairs] = -1;
        }

        /**
         * Determines whether the strands at positions pos-1 and pos in
         * the key could follow immediately one after another in some
         * traversal of the link (allowing for potential switches and/or
         * splices where these are legal).
         *
         * PRE: pos is even, and is between 0 and key.size() inclusive.
         */
        bool couldConnect(const LightweightSequence<int>& key, int pos) {
            if (pos <= 0 || pos >= key.size())
                return false;
            int enter = key[pos] / 2;
            if (lastCrossing[key[pos - 1]] != enter)
                return false;
            if ((mask[enter] != 6) &&
                    link->strand(key[pos - 1]).next().strand() == 1 &&
                    (! (key[pos] & 1))) {
                // We enter the crossing from the upper strand, but exit
                // from the lower strand.  This can only happen if this
                // is the second pass through the crossing, *and* the first
                // pass was not the beginning of a closed loop.  This means we
                // must not see that crossing again later in the key.
                //
                // If mask[enter] == 6 then the only two strands in the
                // key that touch this crossing are the two we're looking at
                // now.  So we avoid the linear-time test in this case.
                for (int i = pos + 1; i < key.size(); ++i)
                    if (lastCrossing[key[i]] == enter)
                        return false;
            }
            return true;
        }

        /**
         * Examines where key[pos] appears, if at all, amongst the initial
         * steps of any link traversal that are forced (due to only using
         * pass moves, and therefore not making any branching decisions).
         *
         * Returns \c true if everything looks okay, or \c false if an
         * inconsistency is found that makes the entire key non-viable.
         */
        bool verifyPrefix(const LightweightSequence<int>& key, int pos) {
            // If key[pos] is not forced, then key[pos+1] must not be
            // forced either.
            if (prefix[key[pos]] < 0)
                return (pos == key.size() - 1 || prefix[key[pos + 1]] < 0);

            // From here on, we know that key[pos] is forced.

            // If key[pos] is forced, then we must be able to fill the
            // remainder of the key before key[pos] with strands that
            // we know are forced to appear before key[pos].
            if (prefix[key[pos]] < pos)
                return false;

            // If both key[pos] and key[pos+1] are forced, then key[pos]
            // must be forced to appear *earlier* than key[pos+1].
            return (pos == key.size() - 1 || prefix[key[pos + 1]] < 0 ||
                prefix[key[pos + 1]] > prefix[key[pos]]);
        }

        /**
         * Tests whether the data from the given key might survive all the way
         * up to the root of the tree decomposition.
         */
        bool keyViable(const LightweightSequence<int>& key) {
            int n = key.size();

            // Of all the strands passed so far that run between a crossing c
            // and the forgotten zone, what is the highest bag at which we
            // forget such a crossing c?
            int maxForget = -1;

            // If needStartLoop is non-negative, this means we are looking for
            // the beginning of a closed-off loop that starts and ends at that
            // crossing.
            //
            // The crossing number is shifted left one bit; the last bit
            // is 0 if any strand will do, or 1 if the loop must start on the
            // upper strand.
            int needStartLoop = -1;

            // If couldEndLoop is non-negative, then it represents the
            // (unique) crossing that could potentially end a closed-off
            // loop that starts and ends at that crossing.
            //
            // The crossing number is shifted left one bit; the last bit
            // is 0 if any strand will do, or 1 if the loop must start on the
            // upper strand.
            int couldEndLoop = -1;

            int c;
            for (int i = n - 2; i >= 0; i -= 2) {
                if (! verifyPrefix(key, i + 1))
                    return false;
                if (! verifyPrefix(key, i))
                    return false;

                // Examine the connection between the strands at
                // positions i+1 and i+2 in the key.
                if (! couldConnect(key, i + 2)) {
                    // The strand at key[i + 1] is the last that we see
                    // from some closed loop in the link traversal, and
                    // the strand at key[i + 2] is the first that we see
                    // from some later closed loop in the link traversal.
                    if (i < n - 2) {
                        c = lastCrossing[key[i + 2]];
                        if ((mask[c] & 3) == 3) {
                            // We enter the forgotten zone from crossing c, and
                            // both incoming strands at c come back from the
                            // forgotten zone.  Since couldConnect() was false,
                            // it follows that c must begin a closed loop,
                            // and we should have already seen the loop
                            // end again at c later in the key.
                            if (couldEndLoop >> 1 == c) {
                                if (needStartLoop >= 0) {
                                    // We are still looking for the beginning
                                    // of a different loop (which ends
                                    // at key[i+3] or beyond, and starts
                                    // at key[i] or before).  Since loops
                                    // cannot overlap, this key is non-viable.
                                    return false;
                                }
                                if ((couldEndLoop & 1) && ! (key[i + 2] & 1))
                                    return false;

                                couldEndLoop = -1;
                            } else
                                return false;
                        }
                    }

                    c = lastCrossing[key[i + 1]];
                    if ((mask[c] & 12) == 12) {
                        // We exit the forgotten zone back into crossing c,
                        // and both outgoing strands at c lead back into the
                        // forgotten zone.  Since couldConnect() was false,
                        // it follows that c must end a closed loop,
                        // and we should expect to see the loop begin
                        // also at c, at an earlier position in the key.
                        if (needStartLoop >= 0) {
                            // We are already looking for the start of some
                            // other loop.  Since loops cannot be nested,
                            // this key is not viable.
                            return false;
                        }
                        if (maxForget > forgetCrossing[c]) {
                            // We cannot have a loop to/from c, since subsequent
                            // strands involve later-forgotten crossings.
                            return false;
                        }
                        if (maxForget == forgetCrossing[c]) {
                            // The start and of this loop will be our
                            // first traversal through c, which means it
                            // must be a pass from upper to upper.
                            if (link->strand(key[i + 1]).next().strand() == 0)
                                return false;
                            needStartLoop = (c << 1) | 1;
                        } else
                            needStartLoop = c << 1;

                        // Now we are looking for a loop beginning, which
                        // means we are in a new loop that must end at key[i+1].
                        // Any previous couldEndLoop (which would have ended
                        // at key[i+3] or later) is therefore unusable.
                        couldEndLoop = -1;
                    }
                }

                if (maxForget < forgetStrand[key[i + 1]]) {
                    maxForget = forgetStrand[key[i + 1]];
                    couldEndLoop = lastCrossing[key[i + 1]] << 1;
                } else if (maxForget == forgetStrand[key[i + 1]]) {
                    // If this ends a loop, then the matching loop start will
                    // be the first time we see this crossing.  Therefore
                    // it must happen at a pass over the upper strand,
                    // and the loop must finish here again on the upper strand.
                    if (link->strand(key[i + 1]).next().strand() == 1)
                        couldEndLoop = (lastCrossing[key[i + 1]] << 1) | 1;
                    else if (couldEndLoop == (lastCrossing[key[i + 1]] << 1)) {
                        // We cannot offer ourselves as a loop end here, but
                        // the same crossing is still offering itself as a loop
                        // end later on.  However, we can now certify that the
                        // loop start will be the first instance of the
                        // crossing, and so we insist that the loop starts on
                        // the upper strand.
                        couldEndLoop ^= 1;
                    }
                }
                if (maxForget < forgetStrand[key[i]]) {
                    maxForget = forgetStrand[key[i]];
                    couldEndLoop = -1;
                }

                // See if we have located the start of a loop that we
                // were searching for, and/or if we can certify that we
                // will never find it.
                if (needStartLoop >= 0) {
                    if (maxForget > forgetCrossing[needStartLoop >> 1])
                        return false;
                    if ((needStartLoop >> 1) == lastCrossing[key[i]]) {
                        // We found the crossing we were after.
                        if (needStartLoop & 1) {
                            // We are specifically seeking the upper strand.
                            // The only time this happens is when all
                            // other occurrences of the crossing have
                            // been passed, so if this is not the upper
                            // strand then there is no hope for viability.
                            //
                            // TODO: Test this earlier, when needStartLoop is
                            // set.
                            if (! (key[i] & 1))
                                return false;
                        }
                        needStartLoop = -1;
                    }
                }
            }

            // Are we still waiting on the start of a loop that we never found?
            if (needStartLoop >= 0)
                return false;

            if (key.size() > 0) {
                // Check if the very first crossing must start a closed-off loop
                // whose end should have been seen later in the key.
                c = lastCrossing[key[0]];
                if ((mask[c] & 3) == 3) {
                    if (couldEndLoop >> 1 != c)
                        return false;
                    if ((couldEndLoop & 1) && ! (key[0] & 1))
                        return false;
                }

                // Let c be the first crossing ever seen in the key.
                //
                // If we can guarantee that (a) this is the first of two
                // appearances of c when resolving/traversing the link, and
                // (b) crossing c is the beginning of a closed loop in the
                // entire traversal, then (c) we *must* be making a
                // pass operation on c's upper strand.
                if (! (key[0] & 1)) {
                    // We are exiting from c's lower strand, which
                    // contradicts (c) above.  If both (a) and (b) above are
                    // true then we have a contradiction.
                    //
                    // We can guarantee (a) in different ways:
                    // - If both incoming strands at c appear in the key,
                    //   then this is the first appearance of c.
                    //   This corresponds to testing (mask & 3).
                    // - If both outgoing strands at c appear in the key,
                    //   then this is likewise the first appearance of c.
                    //   We already see the lower outgoing strand, so we
                    //   just need to test upper outgoing (mask & 8).
                    //
                    // If (b) is false, then the only way to exit on c's
                    // lower strand the first time we pass through c is to
                    // enter on c's lower strand from some non-forgotten
                    // crossing and perform a switch.
                    // Therfore we guarantee (b) if we know we enter on
                    // c's lower strand from a *forgotten* crossing.
                    // This corresponds to testing (mask & 1).
                    //
                    // Note: 3 | 1 == 3, and 8 | 1 == 9.  So we actually test
                    // mask against 3 and 9.
                    if ((mask[c] & 3) == 3 || (mask[c] & 9) == 9)
                        return false;
                }
            }

#ifdef IDENTIFY_NONVIABLE_KEYS
            foundViable = true;
#endif
            return true;
        }

        /**
         * Used to call keyViable() in pieces, which is useful when
         * recursively constructing different keys.
         *
         * To test whether a key is viable, you must call
         * partialKeyViable(p-1), partialKeyViable(p-2), ...,
         * partialKeyViable(0), partialKeyViable(-1), in that order,
         * where p is the number of pairs in the key (i.e., half the key
         * length).  For each i >= 0, when calling partialKeyViable(i) you
         * must have filled in the key elements key[2*i, ..., 2*p-1].
         * When calling partialKeyViable(-1) the key must be complete.
         *
         * Calling partialKeyViable(i) does not overwrite any internal
         * data used to compute partialKeyViable(i+1, i+2, ...), which
         * essentially means that you can backtrack without needing to restore
         * internal variables.
         *
         * PRE: The key cannot have zero length (i.e., p > 0).
         */
        bool partialKeyViable(const LightweightSequence<int>& key, int pos) {
            // This code mirrors keyViable(); see that routine for
            // further documentation.

            if (pos < 0) {
                // Finish the analysis of a fully-completed key.
                // This code mirrors what happens after the main loop in
                // keyViable().
                if (needStartLoop[0] >= 0)
                    return false;

                int c = lastCrossing[key[0]];
                if ((mask[c] & 3) == 3) {
                    if (couldEndLoop[0] >> 1 != c)
                        return false;
                    if ((couldEndLoop[0] & 1) && ! (key[0] & 1))
                        return false;
                }

                if (! (key[0] & 1))
                    if ((mask[c] & 3) == 3 || (mask[c] & 9) == 9)
                        return false;

#ifdef IDENTIFY_NONVIABLE_KEYS
                foundViable = true;
#endif
                return true;
            }

            // Continue the analysis of an only partially-completed key.
            // This code mirrors what happens in a single iteration of the
            // main loop in keyViable().
            if (! verifyPrefix(key, 2 * pos + 1))
                return false;
            if (! verifyPrefix(key, 2 * pos))
                return false;

            couldEndLoop[pos] = couldEndLoop[pos + 1];
            needStartLoop[pos] = needStartLoop[pos + 1];

            int c;
            if (! couldConnect(key, 2 * pos + 2)) {
                if (pos < pairs - 1) {
                    c = lastCrossing[key[2 * pos + 2]];
                    if ((mask[c] & 3) == 3) {
                        if (couldEndLoop[pos + 1] >> 1 == c) {
                            if (needStartLoop[pos + 1] >= 0)
                                return false;
                            if ((couldEndLoop[pos + 1] & 1) &&
                                    ! (key[2 * pos + 2] & 1))
                                return false;
                            couldEndLoop[pos] = -1;
                        } else
                            return false;
                    }
                }

                c = lastCrossing[key[2 * pos + 1]];
                if ((mask[c] & 12) == 12) {
                    if (needStartLoop[pos + 1] >= 0)
                        return false;
                    if (maxForget[pos + 1] > forgetCrossing[c])
                        return false;
                    if (maxForget[pos + 1] == forgetCrossing[c]) {
                        if (link->strand(key[2 * pos + 1]).next().strand() == 0)
                            return false;
                        needStartLoop[pos] = (c << 1) | 1;
                    } else
                        needStartLoop[pos] = c << 1;

                    couldEndLoop[pos] = -1;
                }
            }

            if (maxForget[pos + 1] < forgetStrand[key[2 * pos + 1]]) {
                maxForget[pos] = forgetStrand[key[2 * pos + 1]];
                couldEndLoop[pos] = lastCrossing[key[2 * pos + 1]] << 1;
            } else if (maxForget[pos + 1] == forgetStrand[key[2 * pos + 1]]) {
                maxForget[pos] = forgetStrand[key[2 * pos + 1]];
                if (link->strand(key[2 * pos + 1]).next().strand() == 1)
                    couldEndLoop[pos] =
                        (lastCrossing[key[2 * pos + 1]] << 1) | 1;
                else if (couldEndLoop[pos] ==
                        (lastCrossing[key[2 * pos + 1]] << 1))
                    couldEndLoop[pos] ^= 1;
            } else
                maxForget[pos] = maxForget[pos + 1];

            if (maxForget[pos] < forgetStrand[key[2 * pos]]) {
                maxForget[pos] = forgetStrand[key[2 * pos]];
                couldEndLoop[pos] = -1;
            }

            if (needStartLoop[pos] >= 0) {
                if (maxForget[pos] > forgetCrossing[needStartLoop[pos] >> 1])
                    return false;
                if ((needStartLoop[pos] >> 1) == lastCrossing[key[2 * pos]]) {
                    if (needStartLoop[pos] & 1)
                        if (! (key[2 * pos] & 1))
                            return false;
                    needStartLoop[pos] = -1;
                }
            }

            return true;
        }
    };

    // Convenience functions for the treewidth HOMFLY algorithm:

    inline void aggregate(
            std::map<LightweightSequence<int>*, Laurent2<Integer>*,
                LightweightSequence<int>::Less>* solns,
            LightweightSequence<int>* key, Laurent2<Integer>* value) {
        auto existingSoln = solns->emplace(key, value);
        if (! existingSoln.second) {
            *(existingSoln.first->second) += *value;
            delete key;
            delete value;
        }
    }

    inline Laurent2<Integer>* passValue(const Laurent2<Integer>* from) {
        return new Laurent2<Integer>(*from);
    }

    inline Laurent2<Integer>* switchValue(const Laurent2<Integer>* from,
            Crossing* c) {
        return new Laurent2<Integer>(*from, (c->sign() > 0 ? -2 : 2), 0);
    }

    inline Laurent2<Integer>* spliceValue(const Laurent2<Integer>* from,
            Crossing* c) {
        Laurent2<Integer>* ans = new Laurent2<Integer>(*from,
            (c->sign() > 0 ? -1 : 1), 1);
        if (c->sign() < 0)
            ans->negate();
        return ans;
    }
}

Laurent2<Integer>* Link::homflyKauffman() const {
    // Throughout this code, delta = (alpha - alpha^-1) / z.

    // We know from the preconditions that there is at least one crossing.
    size_t n = crossings_.size();

    // We order the arcs as follows:
    // - crossing 0, entering lower strand
    // - crossing 0, entering upper strand
    // - crossing 1, entering lower strand
    // - crossing 1, entering upper strand
    // - ...
    // followed by all zero-crossing unknot components (which we never
    // need to process explicitly).

    long comp = 0;
    long splices = 0;
    long splicesNeg = 0;
    long writheAdj = 0;

    // Count the number of 0-crossing unknot components separately.
    size_t unknots = 0;
    for (StrandRef s : components_)
        if (! s)
            ++unknots;

    // The final polynomial will be sum_i (coeff[i] * delta^(i + unknots)).
    //
    // Here i represents one less than the number of link components in
    // a state, not counting any 0-crossing unknot components.
    // Since we are assured at least one crossing at this point,
    // we have 0 <= i <= #components + #crossings - 1.
    size_t maxComp = 0;
    Laurent2<Integer>* coeff = new Laurent2<Integer>[
        n + components_.size()];

    // Iterate through a tree of states:
    CrossingState* state = new CrossingState[n];
    std::fill(state, state + n, CROSSING_UNSEEN);

    StrandRef* first = new StrandRef[n + components_.size()];
    std::fill(first, first + n + components_.size(), StrandRef());

    bool* seen = new bool[2 * n]; // index = 2 * crossing_index + strand
    std::fill(seen, seen + 2 * n, false);

    Laurent2<Integer> term;
    StrandRef s = StrandRef(crossings_.front(), 0);
    long pos = 0;
    bool backtrack;
    while (pos >= 0) {
        // We prepare to follow the (pos)th arc.

#ifdef DUMP_STATES
        std::cerr << "=> " << pos << ", s" << s << ", c" << comp
            << ", w" << writheAdj << " : ";
        for (size_t i = 0; i < n; ++i)
            std::cerr << (int)state[i];
        std::cerr << ' ';
        for (size_t i = 0; i < comp; ++i)
            std::cerr << first[i];
        std::cerr << ' ';
        for (size_t i = 0; i < 2*n; ++i)
            std::cerr << (seen[i] ? 'X' : '_');
        if (pos == 2 * n)
            std::cerr << "  ***";
        std::cerr << std::endl;
#endif

        if (seen[2 * s.crossing()->index() + s.strand()]) {
            // We have closed off a component of the (possibly spliced) link.
            first[comp] = s;
            ++comp;

            if (pos == 2 * n) {
                // We have completely determined a state.
                // The contribution to the HOMFLY polynomial is:
                //     (-1)^splicesNeg * z^splices * alpha^writheAdj *
                //     delta^(#components-1).
                // Note that delta^(#components-1) will be computed later;
                // here we just store the rest of the term in coeff[comp-1].
                term.init(writheAdj, splices);
                if (splicesNeg % 2)
                    term.negate();

                coeff[comp - 1] += term;
                if (comp > maxComp)
                    maxComp = comp;

                // Backtrack!
                backtrack = true;
                --comp;
                while (backtrack) {
#ifdef DUMP_STATES
                    std::cerr << "<- " << pos << ", s" << s << ", c" << comp
                        << ", w" << writheAdj << " : ";
                    for (size_t i = 0; i < n; ++i)
                        std::cerr << (int)state[i];
                    std::cerr << ' ';
                    for (size_t i = 0; i < comp; ++i)
                        std::cerr << first[i];
                    std::cerr << ' ';
                    for (size_t i = 0; i < 2*n; ++i)
                        std::cerr << (seen[i] ? 'X' : '_');
                    std::cerr << std::endl;
#endif

                    --pos;
                    if (pos < 0)
                        break;

                    --s;
                    if (state[s.crossing()->index()] == CROSSING_SPLICE_1 ||
                            state[s.crossing()->index()] == CROSSING_SPLICE_2)
                        s.jump();

                    if (! seen[2 * s.crossing()->index() + s.strand()]) {
                        --comp;
                        s = first[comp];

                        // We have to step backwards again from first[comp].
                        ++pos;
                        continue;
                    } else
                        seen[2 * s.crossing()->index() + s.strand()] = false;

                    switch (state[s.crossing()->index()]) {
                        case CROSSING_KEEP_1:
                            state[s.crossing()->index()] = CROSSING_UNSEEN;
                            break;
                        case CROSSING_SWITCH_1:
                            // We switched this crossing the first time around.
                            // Set things up so that we splice this time.
                            writheAdj += 2 * s.crossing()->sign();
                            state[s.crossing()->index()] = CROSSING_TRIED;

                            // Resume iteration from here.
                            backtrack = false;
                            break;
                        case CROSSING_SPLICE_1:
                            --splices;
                            if (s.crossing()->sign() < 0)
                                --splicesNeg;
                            writheAdj += s.crossing()->sign();

                            state[s.crossing()->index()] = CROSSING_UNSEEN;
                            break;
                        case CROSSING_KEEP_2:
                            state[s.crossing()->index()] = CROSSING_KEEP_1;
                            break;
                        case CROSSING_SWITCH_2:
                            state[s.crossing()->index()] = CROSSING_SWITCH_1;
                            break;
                        case CROSSING_SPLICE_2:
                            state[s.crossing()->index()] = CROSSING_SPLICE_1;
                            break;
                        case CROSSING_UNSEEN:
                        case CROSSING_TRIED:
                            // Should never happen.
                            std::cerr << "ERROR: homfly() is backtracking "
                                "through a crossing that does not seem to have "
                                "been visited." << std::endl;
                            break;
                    }
                }

                continue;
            } else {
                // Move to the next component.
                // Note that s should at this point be equal to the starting
                // strand of the component we just closed off.
                for (size_t i = 2 * s.crossing()->index() + s.strand() + 1;
                        i < 2 * n; ++i)
                    if (! seen[i]) {
                        s = StrandRef(crossings_[i / 2], i % 2);
                        break;
                    }
            }
        }

        seen[2 * s.crossing()->index() + s.strand()] = true;

        switch (state[s.crossing()->index()]) {
            case CROSSING_UNSEEN:
                if (s.strand() == 1) {
                    // We first visit this crossing on the upper strand.
                    // There is nothing to do.
                    // Just pass through the crossing.
                    state[s.crossing()->index()] = CROSSING_KEEP_1;
                } else {
                    // We first visit this crossing on the lower strand.
                    // Our first option is to switch.
                    // Following this, we pass through the crossing.
                    state[s.crossing()->index()] = CROSSING_SWITCH_1;

                    writheAdj -= 2 * s.crossing()->sign();
                }
                break;
            case CROSSING_TRIED:
                // We previously switched this crossing.
                // Splice, and then jump to the other strand and
                // continue through the crossing.
                state[s.crossing()->index()] = CROSSING_SPLICE_1;

                ++splices;
                if (s.crossing()->sign() < 0)
                    ++splicesNeg;
                writheAdj -= s.crossing()->sign();

                s.jump();
                break;
            case CROSSING_KEEP_1:
                // Pass through the crossing.
                state[s.crossing()->index()] = CROSSING_KEEP_2;
                break;
            case CROSSING_SWITCH_1:
                // Pass through the crossing.
                state[s.crossing()->index()] = CROSSING_SWITCH_2;
                break;
            case CROSSING_SPLICE_1:
                // Jump to the other strand and continue through the
                // crossing.
                state[s.crossing()->index()] = CROSSING_SPLICE_2;
                s.jump();
                break;

            case CROSSING_KEEP_2:
            case CROSSING_SWITCH_2:
            case CROSSING_SPLICE_2:
                // Should never happen.
                std::cerr << "ERROR: homfly() is visiting a "
                    "crossing for the third time." << std::endl;
                break;
        }
        ++s;
        ++pos;
    }
    delete[] seen;
    delete[] first;
    delete[] state;

    // Piece together the final polynomial.

    Laurent2<Integer>* ans = new Laurent2<Integer>;

    Laurent2<Integer> delta(1, -1);
    delta.set(-1, -1, -1);

    Laurent2<Integer> deltaPow(0, 0); // Initialises to delta^0 == 1.
    for (size_t i = 0; i < unknots; ++i)
        deltaPow *= delta;
    for (size_t i = 0; i < maxComp; ++i) {
        if (! coeff[i].isZero()) {
            coeff[i] *= deltaPow;
            (*ans) += coeff[i];
        }
        deltaPow *= delta;
    }

    delete[] coeff;
    return ans;
}

Laurent2<Integer>* Link::homflyTreewidth() const {
    // We know from the precondition that there is at least one crossing.

    Laurent2<Integer> delta(1, -1);
    delta.set(-1, -1, -1);

    // Build a nice tree decomposition.
    const TreeDecomposition& d = niceTreeDecomposition();
    size_t nBags = d.size();

    const TreeBag *bag, *child, *sibling;
    int index;

    // Each partial solution is a key-value map.
    //
    // Each key is an ordered sequence of strands s_1 s_2 ... s_{2k}.
    // For odd i, s_i is the ID of a strand that runs from the bag into
    // the forgotten zone.  For even i, s_i is the ID of a strand that
    // runs from the forgotten zone back into the bag.
    //
    // The key represents those strands of a full link traversal that cross
    // the border of the forgotten zone, in the order in which we traverse
    // them.  Here "full link traversal" is the kind of traversal used in
    // Kauffman's skein template algorithm, including switches and/or splices.
    // The ordering of crossings that we use with Kauffman's algorithm is
    // according to where in the nice tree decomposition a crossing is
    // forgotten - crossings that are forgotten in later bags (closer to the
    // root) are considered first as potential traversal starting points.
    //
    // Each corresponding value represents a "partial HOMFLY polynomial",
    // aggregated over all "partial traversals" that follow the strands
    // seen in the key in and out of the forgotten zone in the same order.
    // Only the actions taken within the forgotten zone are factored into
    // this polynomial.
    //
    // An important fact: each bag is guaranteed to have at least one solution,
    // since there is always some way to traverse the link.

    typedef LightweightSequence<int> Key;
    typedef Laurent2<Integer> Value;

    typedef std::map<Key*, Value*, Key::Less> SolnSet;

    SolnSet** partial = new SolnSet*[nBags];
    std::fill(partial, partial + nBags, nullptr);

    ViabilityData vData(this, d);

    for (bag = d.first(); bag; bag = bag->next()) {
        index = bag->index();
        std::cerr << "Bag " << index << " [" << bag->size() << "] ";

        if (bag->isLeaf()) {
            // Leaf bag.
            std::cerr << "LEAF" << std::endl;

            partial[index] = new SolnSet;
            partial[index]->emplace(new Key(), new Laurent2<Integer>(0, 0));
        } else if (bag->type() == NICE_INTRODUCE) {
            // Introduce bag.
            child = bag->children();
            std::cerr << "INTRODUCE" << std::endl;

            // When introducing a new crossing, all of its arcs must
            // lead to unseen crossings or crossings already in the bag.
            // Therefore the keys and values remain unchanged.

            partial[index] = partial[child->index()];
            partial[child->index()] = nullptr;
        } else if (bag->type() == NICE_FORGET) {
            // Forget bag.
            child = bag->children();
            std::cerr << "FORGET -> " <<
                partial[child->index()]->size() << std::endl;

            Crossing* c = crossings_[child->element(bag->subtype())];

            vData.initForgetBag(bag,
                partial[child->index()]->begin()->first, c);

            if (c->next(0).crossing() == c && c->next(1).crossing() == c) {
                // The crossing is part of two loops.
                // This means that we are forgetting a complete
                // 1-crossing unknot component.
                //
                // Steal the list of solutions directly from the child
                // bag, and just factor the extra unknot into each value.
                partial[index] = partial[child->index()];
                partial[child->index()] = nullptr;

                // We do *not* factor in the extra unknot if this is the
                // last crossing to ever be forgotten.  This is because
                // the HOMFLY formula requires us to subtract 1 from the
                // total number of loops.
                if (index != nBags - 1)
                    for (auto& soln : *(partial[index]))
                        (*soln.second) *= delta;

                continue;
            }

            partial[index] = new SolnSet;

            const Key *kChild;
            const Value *vChild;
            Value *vNew;

            // Identify if/where the four strands touching this
            // crossing appear in the key:
            //   - id[0:lower, 1:upper][0:in, 1:out] is the unique
            //     strand ID (2 * crossing + strand);
            //   - pos[0:lower, 1:upper][0:in, 1:out] is index,
            //     or -1 if not present.
            // We also make a bitmask indicating which of these
            // four strands head into the forgotten zone.
            //
            // Both id and mask are independent of which partial solution we're
            // looking at, so we just extract them from the first key in
            // the child bag.
            // However, pos depends on the key, and so we compute that
            // every time.
            int id[2][2];
            int pos[2][2];
            int i, j;
            char mask;

            id[0][0] = c->prev(0).id();
            id[0][1] = c->lower().id();
            id[1][0] = c->prev(1).id();
            id[1][1] = c->upper().id();

            kChild = partial[child->index()]->begin()->first;

            pos[0][0] = pos[0][1] = pos[1][0] = pos[1][1] = -1;
            mask = 0;
            for (i = 0; i < kChild->size(); ++i) {
                if ((*kChild)[i] == id[0][0]) {
                    pos[0][0] = i;
                    mask |= 1;
                } else if ((*kChild)[i] == id[0][1]) {
                    pos[0][1] = i;
                    mask |= 2;
                } else if ((*kChild)[i] == id[1][0]) {
                    pos[1][0] = i;
                    mask |= 4;
                } else if ((*kChild)[i] == id[1][1]) {
                    pos[1][1] = i;
                    mask |= 8;
                }
            }

            // Compute the size of the new key.
            // We will use the same key object throughout processing to
            // avoid repeated costly new/delete operations.
            Key kNew(kChild->size() +
                (c->next(0).crossing() == c || c->next(1).crossing() == c ?
                    2 : 4) -
                2 * BitManipulator<char>::bits(mask));

            for (auto& soln : *(partial[child->index()])) {
                kChild = soln.first;
                vChild = soln.second;

#ifdef IDENTIFY_NONVIABLE_KEYS
                vData.foundViable = false;
#endif

                // Recompute the pos array.
                // We don't need to reset it, since the same strands
                // will be found each time.
                for (i = 0; i < kChild->size(); ++i) {
                    if ((*kChild)[i] == id[0][0])
                        pos[0][0] = i;
                    else if ((*kChild)[i] == id[0][1])
                        pos[0][1] = i;
                    else if ((*kChild)[i] == id[1][0])
                        pos[1][0] = i;
                    else if ((*kChild)[i] == id[1][1])
                        pos[1][1] = i;
                }

                // There are *many* different cases that we need to deal
                // with here.

                if (c->next(0).crossing() == c) {
                    // TODO: Find a test link that verifies this case.
                    // Case: the crossing is part of one loop (lower -> upper)
                    // Work out which strands to/from the crossing run
                    // into the forgotten zone.
                    // In all of our analysis, we silently untwist the loop at
                    // crossing c, and pretend there is no crossing at all.
                    switch (mask) {
                        case 0:
                            // Neither strand is from the forgotten zone.
                            for (i = 0; i <= kChild->size(); i += 2) {
                                std::copy(kChild->begin(),
                                    kChild->begin() + i,
                                    kNew.begin());
                                kNew[i] = id[0][0];
                                kNew[i + 1] = id[1][1];
                                std::copy(kChild->begin() + i,
                                    kChild->end(),
                                    kNew.begin() + i + 2);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));
                            }
                            break;
                        case 1:
                            // One strand is from a forgotten crossing.
                            // Merge it with the other.
                            std::cerr << "loop1a 1 merge" << std::endl;
                            kNew = *kChild;
                            kNew[pos[0][0]] = id[1][1];

                            if (vData.keyViable(kNew))
                                aggregate(partial[index], new Key(kNew),
                                    passValue(vChild));
                            break;
                        case 8:
                            // One strand is from a forgotten crossing.
                            // Merge it with the other.
                            std::cerr << "loop1a 8 merge" << std::endl;
                            kNew = *kChild;
                            kNew[pos[1][1]] = id[0][0];

                            if (vData.keyViable(kNew))
                                aggregate(partial[index], new Key(kNew),
                                    passValue(vChild));
                            break;
                        case 9:
                            // Both strands are from the forgotten zone.
                            if (pos[1][1] + 1 == pos[0][0]) {
                                // We are closing off a loop.
                                if (pos[1][1] == kChild->size() - 2) {
                                    std::cerr << "loop1a 9 pass" << std::endl;
                                    std::copy(kChild->begin(),
                                        kChild->end() - 2, kNew.begin());

                                    // This is one of the few cases that
                                    // could describe the last forget bag,
                                    // where we must remember to subtract 1
                                    // from the total number of loops.
                                    if (vData.keyViable(kNew)) {
                                        vNew = passValue(vChild);
                                        if (index != nBags - 1)
                                            (*vNew) *= delta;
                                        aggregate(partial[index],
                                            new Key(kNew), vNew);
                                    }
                                }
                            } else {
                                // Just merge the two free ends.
                                if (pos[0][0] + 1 == pos[1][1]) {
                                    std::cerr << "loop1a 9 merge" << std::endl;
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[0][0],
                                        kNew.begin());
                                    std::copy(
                                        kChild->begin() + pos[0][0] + 2,
                                        kChild->end(),
                                        kNew.begin() + pos[0][0]);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            passValue(vChild));
                                }
                            }
                            break;
                    }
                } else if (c->next(1).crossing() == c) {
                    // TODO: Find a test link that verifies this case.
                    // Case: the crossing is part of one loop (upper -> lower)
                    // Work out which strands to/from the crossing run
                    // into the forgotten zone.
                    // In all of our analysis, we silently untwist the loop at
                    // crossing c, and pretend there is no crossing at all.
                    switch (mask) {
                        case 0:
                            // Neither strand is from the forgotten zone.
                            for (i = 0; i <= kChild->size(); i += 2) {
                                std::copy(kChild->begin(),
                                    kChild->begin() + i,
                                    kNew.begin());
                                kNew[i] = id[1][0];
                                kNew[i + 1] = id[0][1];
                                std::copy(kChild->begin() + i,
                                    kChild->end(),
                                    kNew.begin() + i + 2);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));
                            }
                            break;
                        case 2:
                            // One strand is from a forgotten crossing.
                            // Merge it with the other.
                            std::cerr << "loop1b 2 merge" << std::endl;
                            kNew = *kChild;
                            kNew[pos[0][1]] = id[1][0];

                            if (vData.keyViable(kNew))
                                aggregate(partial[index], new Key(kNew),
                                    passValue(vChild));
                            break;
                        case 4:
                            // One strand is from a forgotten crossing.
                            // Merge it with the other.
                            std::cerr << "loop1b 4 merge" << std::endl;
                            kNew = *kChild;
                            kNew[pos[1][0]] = id[0][1];

                            if (vData.keyViable(kNew))
                                aggregate(partial[index], new Key(kNew),
                                    passValue(vChild));
                            break;
                        case 6:
                            // Both strands are from the forgotten zone.
                            if (pos[0][1] + 1 == pos[1][0]) {
                                // We are closing off a loop.
                                if (pos[0][1] == kChild->size() - 2) {
                                    std::cerr << "loop1b 6 pass" << std::endl;
                                    std::copy(kChild->begin(),
                                        kChild->end() - 2, kNew.begin());

                                    // This is one of the few cases that
                                    // could describe the last forget bag,
                                    // where we must remember to subtract 1
                                    // from the total number of loops.
                                    if (vData.keyViable(kNew)) {
                                        vNew = passValue(vChild);
                                        if (index != nBags - 1)
                                            (*vNew) *= delta;
                                        aggregate(partial[index],
                                            new Key(kNew), vNew);
                                    }
                                }
                            } else {
                                // Just merge the two free ends.
                                if (pos[1][0] + 1 == pos[0][1]) {
                                    std::cerr << "loop1b 6 merge" << std::endl;
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[1][0],
                                        kNew.begin());
                                    std::copy(
                                        kChild->begin() + pos[1][0] + 2,
                                        kChild->end(),
                                        kNew.begin() + pos[1][0]);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            passValue(vChild));
                                }
                            }
                            break;
                    }
                } else {
                    // Case: the crossing is part of no loops.
                    // Work out which strands to/from the crossing run
                    // into the forgotten zone.
                    switch (mask) {
                        case 0:
                            // Case verified.
                            // No strands are from forgotten crossings.
                            for (i = 0; i <= kChild->size(); i += 2)
                                for (j = i; j <= kChild->size(); j += 2) {
                                    // Pass:
                                    std::copy(kChild->begin(),
                                        kChild->begin() + i,
                                        kNew.begin());
                                    kNew[i] = id[1][0];
                                    kNew[i + 1] = id[1][1];
                                    std::copy(kChild->begin() + i,
                                        kChild->begin() + j,
                                        kNew.begin() + i + 2);
                                    kNew[j + 2] = id[0][0];
                                    kNew[j + 3] = id[0][1];
                                    std::copy(kChild->begin() + j,
                                        kChild->end(),
                                        kNew.begin() + j + 4);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            passValue(vChild));

                                    // Switch:
                                    std::copy(kChild->begin(),
                                        kChild->begin() + i,
                                        kNew.begin());
                                    kNew[i] = id[0][0];
                                    kNew[i + 1] = id[0][1];
                                    std::copy(kChild->begin() + i,
                                        kChild->begin() + j,
                                        kNew.begin() + i + 2);
                                    kNew[j + 2] = id[1][0];
                                    kNew[j + 3] = id[1][1];
                                    std::copy(kChild->begin() + j,
                                        kChild->end(),
                                        kNew.begin() + j + 4);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            switchValue(vChild, c));

                                    // Splice:
                                    std::copy(kChild->begin(),
                                        kChild->begin() + i,
                                        kNew.begin());
                                    kNew[i] = id[0][0];
                                    kNew[i + 1] = id[1][1];
                                    std::copy(kChild->begin() + i,
                                        kChild->begin() + j,
                                        kNew.begin() + i + 2);
                                    kNew[j + 2] = id[1][0];
                                    kNew[j + 3] = id[0][1];
                                    std::copy(kChild->begin() + j,
                                        kChild->end(),
                                        kNew.begin() + j + 4);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            spliceValue(vChild, c));
                                }
                            break;
                        case 1:
                            // Case verified.
                            for (i = 0; i < pos[0][0]; i += 2) {
                                // Pass:
                                std::copy(kChild->begin(),
                                    kChild->begin() + i,
                                    kNew.begin());
                                kNew[i] = id[1][0];
                                kNew[i + 1] = id[1][1];
                                std::copy(kChild->begin() + i,
                                    kChild->begin() + pos[0][0],
                                    kNew.begin() + i + 2);
                                kNew[pos[0][0] + 2] = id[0][1];
                                std::copy(kChild->begin() + pos[0][0] + 1,
                                    kChild->end(),
                                    kNew.begin() + pos[0][0] + 3);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));
                            }
                            for (i = pos[0][0] + 1; i <= kChild->size();
                                    i += 2) {
                                // Switch:
                                std::copy(kChild->begin(),
                                    kChild->begin() + pos[0][0],
                                    kNew.begin());
                                kNew[pos[0][0]] = id[0][1];
                                std::copy(kChild->begin() + pos[0][0] + 1,
                                    kChild->begin() + i,
                                    kNew.begin() + pos[0][0] + 1);
                                kNew[i] = id[1][0];
                                kNew[i + 1] = id[1][1];
                                std::copy(kChild->begin() + i,
                                    kChild->end(),
                                    kNew.begin() + i + 2);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        switchValue(vChild, c));

                                // Splice:
                                std::copy(kChild->begin(),
                                    kChild->begin() + pos[0][0],
                                    kNew.begin());
                                kNew[pos[0][0]] = id[1][1];
                                std::copy(kChild->begin() + pos[0][0] + 1,
                                    kChild->begin() + i,
                                    kNew.begin() + pos[0][0] + 1);
                                kNew[i] = id[1][0];
                                kNew[i + 1] = id[0][1];
                                std::copy(kChild->begin() + i,
                                    kChild->end(),
                                    kNew.begin() + i + 2);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        spliceValue(vChild, c));
                            }
                            break;
                        case 2:
                            // Case verified.
                            for (i = 0; i <= pos[0][1]; i += 2) {
                                // Pass:
                                std::copy(kChild->begin(),
                                    kChild->begin() + i,
                                    kNew.begin());
                                kNew[i] = id[1][0];
                                kNew[i + 1] = id[1][1];
                                std::copy(kChild->begin() + i,
                                    kChild->begin() + pos[0][1],
                                    kNew.begin() + i + 2);
                                kNew[pos[0][1] + 2] = id[0][0];
                                std::copy(kChild->begin() + pos[0][1] + 1,
                                    kChild->end(),
                                    kNew.begin() + pos[0][1] + 3);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));

                                // Splice:
                                std::copy(kChild->begin(),
                                    kChild->begin() + i,
                                    kNew.begin());
                                kNew[i] = id[0][0];
                                kNew[i + 1] = id[1][1];
                                std::copy(kChild->begin() + i,
                                    kChild->begin() + pos[0][1],
                                    kNew.begin() + i + 2);
                                kNew[pos[0][1] + 2] = id[1][0];
                                std::copy(kChild->begin() + pos[0][1] + 1,
                                    kChild->end(),
                                    kNew.begin() + pos[0][1] + 3);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        spliceValue(vChild, c));
                            }
                            for (i = pos[0][1] + 2; i <= kChild->size();
                                    i += 2) {
                                // Switch:
                                std::copy(kChild->begin(),
                                    kChild->begin() + pos[0][1],
                                    kNew.begin());
                                kNew[pos[0][1]] = id[0][0];
                                std::copy(kChild->begin() + pos[0][1] + 1,
                                    kChild->begin() + i,
                                    kNew.begin() + pos[0][1] + 1);
                                kNew[i] = id[1][0];
                                kNew[i + 1] = id[1][1];
                                std::copy(kChild->begin() + i,
                                    kChild->end(),
                                    kNew.begin() + i + 2);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        switchValue(vChild, c));
                            }
                            break;
                        case 3:
                            // TODO: Find a test link that verifies this case.
                            if (pos[0][1] + 1 == pos[0][0]) {
                                // d=a
                                // Pass:
                                if (pos[0][1] == kChild->size() - 2) {
                                    std::cerr << "3a pass" << std::endl;
                                    for (i = 0; i < kChild->size(); i += 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + i,
                                            kNew.begin());
                                        kNew[i] = id[1][0];
                                        kNew[i + 1] = id[1][1];
                                        std::copy(kChild->begin() + i,
                                            kChild->end() - 2,
                                            kNew.begin() + i + 2);

                                        if (vData.keyViable(kNew)) {
                                            vNew = passValue(vChild);
                                            (*vNew) *= delta;
                                            aggregate(partial[index],
                                                new Key(kNew), vNew);
                                        }
                                    }
                                }
                            } else if (pos[0][0] < pos[0][1]) {
                                // Splice:
                                std::cerr << "3b splice" << std::endl;
                                kNew = *kChild;
                                kNew[pos[0][0]] = id[1][1];
                                kNew[pos[0][1]] = id[1][0];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        spliceValue(vChild, c));

                                if (pos[0][0] + 1 == pos[0][1]) {
                                    // Pass and switch:
                                    std::cerr << "3b pass/switch" << std::endl;
                                    for (i = 0; i < pos[0][0]; i += 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + i,
                                            kNew.begin());
                                        kNew[i] = id[1][0];
                                        kNew[i + 1] = id[1][1];
                                        std::copy(kChild->begin() + i,
                                            kChild->begin() + pos[0][0],
                                            kNew.begin() + i + 2);
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[0][0] + 2);

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                passValue(vChild));
                                    }
                                    for (i = pos[0][1] + 2;
                                            i <= kChild->size(); i += 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->begin() + i,
                                            kNew.begin() + pos[0][0]);
                                        kNew[i - 2] = id[1][0];
                                        kNew[i - 1] = id[1][1];
                                        std::copy(kChild->begin() + i,
                                            kChild->end(),
                                            kNew.begin() + i);

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                switchValue(vChild, c));
                                    }
                                }
                            }
                            break;
                        case 4:
                            // Case verified.
                            for (i = 0; i < pos[1][0]; i += 2) {
                                // Switch:
                                std::copy(kChild->begin(),
                                    kChild->begin() + i,
                                    kNew.begin());
                                kNew[i] = id[0][0];
                                kNew[i + 1] = id[0][1];
                                std::copy(kChild->begin() + i,
                                    kChild->begin() + pos[1][0],
                                    kNew.begin() + i + 2);
                                kNew[pos[1][0] + 2] = id[1][1];
                                std::copy(kChild->begin() + pos[1][0] + 1,
                                    kChild->end(),
                                    kNew.begin() + pos[1][0] + 3);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        switchValue(vChild, c));

                                // Splice:
                                std::copy(kChild->begin(),
                                    kChild->begin() + i,
                                    kNew.begin());
                                kNew[i] = id[0][0];
                                kNew[i + 1] = id[1][1];
                                std::copy(kChild->begin() + i,
                                    kChild->begin() + pos[1][0],
                                    kNew.begin() + i + 2);
                                kNew[pos[1][0] + 2] = id[0][1];
                                std::copy(kChild->begin() + pos[1][0] + 1,
                                    kChild->end(),
                                    kNew.begin() + pos[1][0] + 3);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        spliceValue(vChild, c));
                            }
                            for (i = pos[1][0] + 1; i <= kChild->size();
                                    i += 2) {
                                // Pass:
                                std::copy(kChild->begin(),
                                    kChild->begin() + pos[1][0],
                                    kNew.begin());
                                kNew[pos[1][0]] = id[1][1];
                                std::copy(kChild->begin() + pos[1][0] + 1,
                                    kChild->begin() + i,
                                    kNew.begin() + pos[1][0] + 1);
                                kNew[i] = id[0][0];
                                kNew[i + 1] = id[0][1];
                                std::copy(kChild->begin() + i,
                                    kChild->end(),
                                    kNew.begin() + i + 2);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));
                            }
                            break;
                        case 5:
                            // Case verified.
                            // Both incoming strands are from
                            // forgotten crossings.
                            if (pos[0][0] < pos[1][0]) {
                                // Switch:
                                kNew = *kChild;
                                kNew[pos[0][0]] = id[0][1];
                                kNew[pos[1][0]] = id[1][1];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        switchValue(vChild, c));

                                // Splice:
                                kNew = *kChild;
                                kNew[pos[0][0]] = id[1][1];
                                kNew[pos[1][0]] = id[0][1];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        spliceValue(vChild, c));
                            } else {
                                // Pass:
                                kNew = *kChild;
                                kNew[pos[0][0]] = id[0][1];
                                kNew[pos[1][0]] = id[1][1];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));
                            }
                            break;
                        case 6:
                            // Case verified.
                            if (pos[0][1] + 1 == pos[1][0]) {
                                // d=b
                                // Switch:
                                kNew = *kChild;
                                kNew[pos[0][1]] = id[0][0];
                                kNew[pos[1][0]] = id[1][1];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        switchValue(vChild, c));

                                if (pos[0][1] == kChild->size() - 2) {
                                    // Splice:
                                    for (i = 0; i < kChild->size(); i += 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + i,
                                            kNew.begin());
                                        kNew[i] = id[0][0];
                                        kNew[i + 1] = id[1][1];
                                        std::copy(kChild->begin() + i,
                                            kChild->end() - 2,
                                            kNew.begin() + i + 2);

                                        if (vData.keyViable(kNew)) {
                                            vNew = spliceValue(vChild, c);
                                            (*vNew) *= delta;
                                            aggregate(partial[index],
                                                new Key(kNew), vNew);
                                        }
                                    }
                                }
                            } else {
                                if (pos[1][0] < pos[0][1]) {
                                    // Pass:
                                    kNew = *kChild;
                                    kNew[pos[0][1]] = id[0][0];
                                    kNew[pos[1][0]] = id[1][1];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            passValue(vChild));

                                    if (pos[1][0] + 1 == pos[0][1]) {
                                        // Splice:
                                        for (i = 0; i < pos[1][0]; i += 2) {
                                            std::copy(kChild->begin(),
                                                kChild->begin() + i,
                                                kNew.begin());
                                            kNew[i] = id[0][0];
                                            kNew[i + 1] = id[1][1];
                                            std::copy(kChild->begin() + i,
                                                kChild->begin() + pos[1][0],
                                                kNew.begin() + i + 2);
                                            std::copy(
                                                kChild->begin() + pos[1][0] + 2,
                                                kChild->end(),
                                                kNew.begin() + pos[1][0] + 2);

                                            if (vData.keyViable(kNew))
                                                aggregate(partial[index],
                                                    new Key(kNew),
                                                    spliceValue(vChild, c));
                                        }
                                    }
                                } else {
                                    // Switch:
                                    kNew = *kChild;
                                    kNew[pos[0][1]] = id[0][0];
                                    kNew[pos[1][0]] = id[1][1];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            switchValue(vChild, c));
                                }
                            }
                            break;
                        case 7:
                            // Case verified.
                            if (pos[0][1] + 1 == pos[1][0]) {
                                // d=b
                                // Switch and splice:
                                if (pos[0][0] + 1 == pos[0][1]) {
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[0][0],
                                        kNew.begin());
                                    kNew[pos[0][0]] = id[1][1];
                                    std::copy(kChild->begin() + pos[0][0] + 3,
                                        kChild->end(),
                                        kNew.begin() + pos[0][0] + 1);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            switchValue(vChild, c));
                                }
                                if (pos[0][1] == kChild->size() - 2) {
                                    std::copy(kChild->begin(),
                                        kChild->end() - 2, kNew.begin());
                                    kNew[pos[0][0]] = id[1][1];

                                    if (vData.keyViable(kNew)) {
                                        vNew = spliceValue(vChild, c);
                                        (*vNew) *= delta;
                                        aggregate(partial[index],
                                            new Key(kNew), vNew);
                                    }
                                }
                            } else if (pos[0][1] + 1 == pos[0][0]) {
                                // d=a
                                // Pass:
                                if (pos[0][1] == kChild->size() - 2) {
                                    std::copy(kChild->begin(),
                                        kChild->end() - 2, kNew.begin());
                                    kNew[pos[1][0]] = id[1][1];

                                    if (vData.keyViable(kNew)) {
                                        vNew = passValue(vChild);
                                        (*vNew) *= delta;
                                        aggregate(partial[index],
                                            new Key(kNew), vNew);
                                    }
                                }
                            } else {
                                if (pos[0][0] + 1 == pos[0][1]) {
                                    // Pass and switch:
                                    if (pos[1][0] < pos[0][0]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[0][0]);
                                        kNew[pos[1][0]] = id[1][1];

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                passValue(vChild));
                                    } else {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[0][0]);
                                        kNew[pos[1][0] - 2] = id[1][1];

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                switchValue(vChild, c));
                                    }
                                } else if (pos[1][0] + 1 == pos[0][1] &&
                                        pos[0][0] < pos[1][0]) {
                                    // Splice:
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[1][0],
                                        kNew.begin());
                                    std::copy(
                                        kChild->begin() + pos[1][0] + 2,
                                        kChild->end(),
                                        kNew.begin() + pos[1][0]);
                                    kNew[pos[0][0]] = id[1][1];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            spliceValue(vChild, c));
                                }
                            }
                            break;
                        case 8:
                            // Case verified.
                            for (i = 0; i <= pos[1][1]; i += 2) {
                                // Switch:
                                std::copy(kChild->begin(),
                                    kChild->begin() + i,
                                    kNew.begin());
                                kNew[i] = id[0][0];
                                kNew[i + 1] = id[0][1];
                                std::copy(kChild->begin() + i,
                                    kChild->begin() + pos[1][1],
                                    kNew.begin() + i + 2);
                                kNew[pos[1][1] + 2] = id[1][0];
                                std::copy(kChild->begin() + pos[1][1] + 1,
                                    kChild->end(),
                                    kNew.begin() + pos[1][1] + 3);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        switchValue(vChild, c));
                            }
                            for (i = pos[1][1] + 2; i <= kChild->size();
                                    i += 2) {
                                // Pass:
                                std::copy(kChild->begin(),
                                    kChild->begin() + pos[1][1],
                                    kNew.begin());
                                kNew[pos[1][1]] = id[1][0];
                                std::copy(kChild->begin() + pos[1][1] + 1,
                                    kChild->begin() + i,
                                    kNew.begin() + pos[1][1] + 1);
                                kNew[i] = id[0][0];
                                kNew[i + 1] = id[0][1];
                                std::copy(kChild->begin() + i,
                                    kChild->end(),
                                    kNew.begin() + i + 2);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));

                                // Splice:
                                std::copy(kChild->begin(),
                                    kChild->begin() + pos[1][1],
                                    kNew.begin());
                                kNew[pos[1][1]] = id[0][0];
                                std::copy(kChild->begin() + pos[1][1] + 1,
                                    kChild->begin() + i,
                                    kNew.begin() + pos[1][1] + 1);
                                kNew[i] = id[1][0];
                                kNew[i + 1] = id[0][1];
                                std::copy(kChild->begin() + i,
                                    kChild->end(),
                                    kNew.begin() + i + 2);

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        spliceValue(vChild, c));
                            }
                            break;
                        case 9:
                            // Case verified.
                            if (pos[1][1] + 1 == pos[0][0]) {
                                // c=a
                                // Pass:
                                kNew = *kChild;
                                kNew[pos[1][1]] = id[1][0];
                                kNew[pos[0][0]] = id[0][1];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));
                            } else {
                                if (pos[1][1] < pos[0][0]) {
                                    // Pass:
                                    kNew = *kChild;
                                    kNew[pos[1][1]] = id[1][0];
                                    kNew[pos[0][0]] = id[0][1];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            passValue(vChild));
                                } else {
                                    // Switch:
                                    kNew = *kChild;
                                    kNew[pos[1][1]] = id[1][0];
                                    kNew[pos[0][0]] = id[0][1];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            switchValue(vChild, c));

                                    if (pos[0][0] + 1 == pos[1][1]) {
                                        // Splice:
                                        for (i = pos[1][1] + 2;
                                                i <= kChild->size(); i += 2) {
                                            std::copy(kChild->begin(),
                                                kChild->begin() + pos[0][0],
                                                kNew.begin());
                                            std::copy(
                                                kChild->begin() + pos[0][0] + 2,
                                                kChild->begin() + i,
                                                kNew.begin() + pos[0][0]);
                                            kNew[i - 2] = id[1][0];
                                            kNew[i - 1] = id[0][1];
                                            std::copy(kChild->begin() + i,
                                                kChild->end(),
                                                kNew.begin() + i);

                                            if (vData.keyViable(kNew))
                                                aggregate(partial[index],
                                                    new Key(kNew),
                                                    spliceValue(vChild, c));
                                        }
                                    }
                                }
                            }
                            break;
                        case 10:
                            // Case verified.
                            // Both outgoing strands are to
                            // forgotten crossings.
                            if (pos[0][1] < pos[1][1]) {
                                // Switch:
                                kNew = *kChild;
                                kNew[pos[0][1]] = id[0][0];
                                kNew[pos[1][1]] = id[1][0];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        switchValue(vChild, c));
                            } else {
                                // Pass:
                                kNew = *kChild;
                                kNew[pos[0][1]] = id[0][0];
                                kNew[pos[1][1]] = id[1][0];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        passValue(vChild));

                                // Splice:
                                kNew = *kChild;
                                kNew[pos[0][1]] = id[1][0];
                                kNew[pos[1][1]] = id[0][0];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        spliceValue(vChild, c));
                            }
                            break;
                        case 11:
                            // Case verified.
                            if (pos[0][1] + 1 == pos[0][0]) {
                                // d=a
                                // Pass:
                                if (pos[0][1] == kChild->size() - 2) {
                                    std::copy(kChild->begin(),
                                        kChild->end() - 2, kNew.begin());
                                    kNew[pos[1][1]] = id[1][0];

                                    if (vData.keyViable(kNew)) {
                                        vNew = passValue(vChild);
                                        (*vNew) *= delta;
                                        aggregate(partial[index],
                                            new Key(kNew), vNew);
                                    }
                                }
                            } else if (pos[1][1] + 1 == pos[0][0]) {
                                // c=a
                                // Pass:
                                if (pos[0][0] + 1 == pos[0][1]) {
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[1][1],
                                        kNew.begin());
                                    kNew[pos[1][1]] = id[1][0];
                                    std::copy(kChild->begin() + pos[1][1] + 3,
                                        kChild->end(),
                                        kNew.begin() + pos[1][1] + 1);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            passValue(vChild));
                                }
                            } else {
                                if (pos[0][0] + 1 == pos[0][1]) {
                                    // Pass and switch:
                                    if (pos[1][1] < pos[0][1]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[0][0]);
                                        kNew[pos[1][1]] = id[1][0];

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                passValue(vChild));
                                    } else {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[0][0]);
                                        kNew[pos[1][1] - 2] = id[1][0];

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                switchValue(vChild, c));
                                    }
                                } else if (pos[0][0] + 1 == pos[1][1] &&
                                        pos[1][1] < pos[0][1]) {
                                    // Splice:
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[0][0],
                                        kNew.begin());
                                    std::copy(
                                        kChild->begin() + pos[0][0] + 2,
                                        kChild->end(),
                                        kNew.begin() + pos[0][0]);
                                    kNew[pos[0][1] - 2] = id[1][0];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            spliceValue(vChild, c));
                                }
                            }
                            break;
                        case 12:
                            // TODO: Find a test link that verifies this case.
                            if (pos[1][1] + 1 == pos[1][0]) {
                                // c=b
                                // Splice:
                                std::cerr << "12a splice" << std::endl;
                                kNew = *kChild;
                                kNew[pos[1][1]] = id[0][0];
                                kNew[pos[1][0]] = id[0][1];

                                if (vData.keyViable(kNew))
                                    aggregate(partial[index], new Key(kNew),
                                        spliceValue(vChild, c));

                                if (pos[1][1] == kChild->size() - 2) {
                                    // Switch:
                                    std::cerr << "12a switch" << std::endl;
                                    for (i = 0; i < kChild->size(); i += 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + i,
                                            kNew.begin());
                                        kNew[i] = id[0][0];
                                        kNew[i + 1] = id[0][1];
                                        std::copy(kChild->begin() + i,
                                            kChild->end() - 2,
                                            kNew.begin() + i + 2);

                                        if (vData.keyViable(kNew)) {
                                            vNew = switchValue(vChild, c);
                                            (*vNew) *= delta;
                                            aggregate(partial[index],
                                                new Key(kNew), vNew);
                                        }
                                    }
                                }
                            } else {
                                if (pos[1][1] < pos[1][0]) {
                                    // Splice:
                                    std::cerr << "12b splice" << std::endl;
                                    kNew = *kChild;
                                    kNew[pos[1][1]] = id[0][0];
                                    kNew[pos[1][0]] = id[0][1];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            spliceValue(vChild, c));
                                } else if (pos[1][0] + 1 == pos[1][1]) {
                                    // Pass and switch:
                                    std::cerr << "12b pass/switch" << std::endl;
                                    for (i = 0; i < pos[1][0]; i += 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + i,
                                            kNew.begin());
                                        kNew[i] = id[0][0];
                                        kNew[i + 1] = id[0][1];
                                        std::copy(kChild->begin() + i,
                                            kChild->begin() + pos[1][0],
                                            kNew.begin() + i + 2);
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[1][0] + 2);

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                switchValue(vChild, c));
                                    }
                                    for (i = pos[1][1] + 2;
                                            i <= kChild->size(); i += 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[1][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 2,
                                            kChild->begin() + i,
                                            kNew.begin() + pos[1][0]);
                                        kNew[i - 2] = id[0][0];
                                        kNew[i - 1] = id[0][1];
                                        std::copy(kChild->begin() + i,
                                            kChild->end(),
                                            kNew.begin() + i);

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                passValue(vChild));
                                    }
                                }
                            }
                            break;
                        case 13:
                            // Case verified.
                            if (pos[1][1] + 1 == pos[0][0]) {
                                // c=a
                                // Pass:
                                if (pos[1][0] + 1 == pos[1][1]) {
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[1][0],
                                        kNew.begin());
                                    kNew[pos[1][0]] = id[0][1];
                                    std::copy(kChild->begin() + pos[1][0] + 3,
                                        kChild->end(),
                                        kNew.begin() + pos[1][0] + 1);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            passValue(vChild));
                                }
                            } else if (pos[1][1] + 1 == pos[1][0]) {
                                // c=b
                                // Switch and splice:
                                if (pos[1][1] == kChild->size() - 2) {
                                    std::copy(kChild->begin(),
                                        kChild->end() - 2, kNew.begin());
                                    kNew[pos[0][0]] = id[0][1];

                                    if (vData.keyViable(kNew)) {
                                        vNew = switchValue(vChild, c);
                                        (*vNew) *= delta;
                                        aggregate(partial[index],
                                            new Key(kNew), vNew);
                                    }
                                }
                                if (pos[0][0] + 1 == pos[1][1]) {
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[0][0],
                                        kNew.begin());
                                    kNew[pos[0][0]] = id[0][1];
                                    std::copy(kChild->begin() + pos[0][0] + 3,
                                        kChild->end(),
                                        kNew.begin() + pos[0][0] + 1);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            spliceValue(vChild, c));
                                }
                            } else {
                                if (pos[1][0] + 1 == pos[1][1]) {
                                    // Pass and switch:
                                    if (pos[1][0] < pos[0][0]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[1][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[1][0]);
                                        kNew[pos[0][0] - 2] = id[0][1];

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                passValue(vChild));
                                    } else {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[1][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[1][0]);
                                        kNew[pos[0][0]] = id[0][1];

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                switchValue(vChild, c));
                                    }
                                } else if (pos[0][0] + 1 == pos[1][1] &&
                                        pos[0][0] < pos[1][0]) {
                                    // Splice:
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[0][0],
                                        kNew.begin());
                                    std::copy(
                                        kChild->begin() + pos[0][0] + 2,
                                        kChild->end(),
                                        kNew.begin() + pos[0][0]);
                                    kNew[pos[1][0] - 2] = id[0][1];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            spliceValue(vChild, c));
                                }
                            }
                            break;
                        case 14:
                            // Case verified.
                            if (pos[0][1] + 1 == pos[1][0]) {
                                // d=b
                                // Switch and splice:
                                if (pos[1][0] + 1 == pos[1][1]) {
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[0][1],
                                        kNew.begin());
                                    kNew[pos[0][1]] = id[0][0];
                                    std::copy(kChild->begin() + pos[0][1] + 3,
                                        kChild->end(),
                                        kNew.begin() + pos[0][1] + 1);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            switchValue(vChild, c));
                                } else if (pos[0][1] == kChild->size() - 2) {
                                    std::copy(kChild->begin(),
                                        kChild->end() - 2, kNew.begin());
                                    kNew[pos[1][1]] = id[0][0];

                                    if (vData.keyViable(kNew)) {
                                        vNew = spliceValue(vChild, c);
                                        (*vNew) *= delta;
                                        aggregate(partial[index],
                                            new Key(kNew), vNew);
                                    }
                                }
                            } else if (pos[1][1] + 1 == pos[1][0]) {
                                // c=b
                                // Switch and splice:
                                if (pos[1][0] + 1 == pos[0][1]) {
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[1][1],
                                        kNew.begin());
                                    kNew[pos[1][1]] = id[0][0];
                                    std::copy(kChild->begin() + pos[1][1] + 3,
                                        kChild->end(),
                                        kNew.begin() + pos[1][1] + 1);

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            spliceValue(vChild, c));
                                } else if (pos[1][1] == kChild->size() - 2) {
                                    std::copy(kChild->begin(),
                                        kChild->end() - 2, kNew.begin());
                                    kNew[pos[0][1]] = id[0][0];

                                    if (vData.keyViable(kNew)) {
                                        vNew = switchValue(vChild, c);
                                        (*vNew) *= delta;
                                        aggregate(partial[index],
                                            new Key(kNew), vNew);
                                    }
                                }
                            } else {
                                if (pos[1][0] + 1 == pos[1][1]) {
                                    // Pass and switch:
                                    if (pos[1][1] < pos[0][1]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[1][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[1][0]);
                                        kNew[pos[0][1] - 2] = id[0][0];

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                passValue(vChild));
                                    } else {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[1][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[1][0]);
                                        kNew[pos[0][1]] = id[0][0];

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                switchValue(vChild, c));
                                    }
                                } else if (pos[1][0] + 1 == pos[0][1] &&
                                        pos[1][1] < pos[0][1]) {
                                    // Splice:
                                    std::copy(kChild->begin(),
                                        kChild->begin() + pos[1][0],
                                        kNew.begin());
                                    std::copy(
                                        kChild->begin() + pos[1][0] + 2,
                                        kChild->end(),
                                        kNew.begin() + pos[1][0]);
                                    kNew[pos[1][1]] = id[0][0];

                                    if (vData.keyViable(kNew))
                                        aggregate(partial[index], new Key(kNew),
                                            spliceValue(vChild, c));
                                }
                            }
                            break;
                        case 15:
                            // Case verified.
                            // All strands are from forgotten crossings.
                            if (pos[0][1] + 1 == pos[0][0]) {
                                if (pos[1][1] + 1 == pos[1][0]) {
                                    // d=a, c=b
                                    // Pass:
                                    if (pos[1][1] == kChild->size() - 4 &&
                                            pos[0][1] == kChild->size() - 2) {
                                        std::copy(kChild->begin(),
                                            kChild->end() - 4, kNew.begin());

                                        // This is one of the few cases that
                                        // could describe the last forget bag,
                                        // where we must remember to subtract 1
                                        // from the total number of loops.
                                        if (vData.keyViable(kNew)) {
                                            vNew = passValue(vChild);
                                            (*vNew) *= delta;
                                            if (index != nBags - 1)
                                                (*vNew) *= delta;
                                            aggregate(partial[index],
                                                new Key(kNew), vNew);
                                        }
                                    }
                                } else {
                                    // d=a
                                    // Pass:
                                    if (pos[0][1] == kChild->size() - 2 &&
                                            pos[1][0] + 1 == pos[1][1]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[1][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 2,
                                            kChild->end() - 2,
                                            kNew.begin() + pos[1][0]);

                                        if (vData.keyViable(kNew)) {
                                            vNew = passValue(vChild);
                                            (*vNew) *= delta;
                                            aggregate(partial[index],
                                                new Key(kNew), vNew);
                                        }
                                    }
                                }
                            } else if (pos[0][1] + 1 == pos[1][0]) {
                                if (pos[1][1] + 1 == pos[0][0]) {
                                    // d=b, c=a
                                    // Pass:
                                    if (pos[1][1] == kChild->size() - 4 &&
                                            pos[0][1] == kChild->size() - 2) {
                                        std::copy(kChild->begin(),
                                            kChild->end() - 4, kNew.begin());

                                        // This is one of the few cases that
                                        // could describe the last forget bag,
                                        // where we must remember to subtract 1
                                        // from the total number of loops.
                                        if (vData.keyViable(kNew)) {
                                            vNew = passValue(vChild);
                                            if (index != nBags - 1)
                                                (*vNew) *= delta;
                                            aggregate(partial[index],
                                                new Key(kNew), vNew);
                                        }
                                    }
                                } else {
                                    // d=b
                                    // Switch and splice:
                                    if (pos[1][0] + 1 == pos[1][1] &&
                                            pos[0][0] + 1 == pos[0][1]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 4,
                                            kChild->end(),
                                            kNew.begin() + pos[0][0]);

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                switchValue(vChild, c));
                                    } else if (pos[0][0] + 1 == pos[1][1] &&
                                            pos[0][1] == kChild->size() - 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->end() - 2,
                                            kNew.begin() + pos[0][0]);

                                        if (vData.keyViable(kNew)) {
                                            vNew = spliceValue(vChild, c);
                                            (*vNew) *= delta;
                                            aggregate(partial[index],
                                                new Key(kNew), vNew);
                                        }
                                    }
                                }
                            } else {
                                if (pos[1][1] + 1 == pos[1][0]) {
                                    // c=b
                                    // Switch and splice:
                                    if (pos[0][0] + 1 == pos[0][1] &&
                                            pos[1][1] == kChild->size() - 2) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->end() - 2,
                                            kNew.begin() + pos[0][0]);

                                        if (vData.keyViable(kNew)) {
                                            vNew = switchValue(vChild, c);
                                            (*vNew) *= delta;
                                            aggregate(partial[index],
                                                new Key(kNew), vNew);
                                        }
                                    } else if (pos[0][0] + 1 == pos[1][1] &&
                                            pos[1][0] + 1 == pos[0][1]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 4,
                                            kChild->end(),
                                            kNew.begin() + pos[0][0]);

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                spliceValue(vChild, c));
                                    }
                                } else if (pos[1][1] + 1 == pos[0][0]) {
                                    // c=a
                                    // Pass:
                                    if (pos[1][0] + 1 == pos[1][1] &&
                                            pos[0][0] + 1 == pos[0][1]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[1][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 4,
                                            kChild->end(),
                                            kNew.begin() + pos[1][0]);

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                passValue(vChild));
                                    }
                                } else {
                                    // Pass, switch and splice:
                                    if (pos[0][0] + 1 == pos[0][1] &&
                                            pos[1][0] + 1 == pos[1][1]) {
                                        if (pos[1][0] < pos[0][0]) {
                                            std::copy(kChild->begin(),
                                                kChild->begin() + pos[1][0],
                                                kNew.begin());
                                            std::copy(
                                                kChild->begin() + pos[1][0] + 2,
                                                kChild->begin() + pos[0][0],
                                                kNew.begin() + pos[1][0]);
                                            std::copy(
                                                kChild->begin() + pos[0][0] + 2,
                                                kChild->end(),
                                                kNew.begin() + pos[0][0] - 2);

                                            if (vData.keyViable(kNew))
                                                aggregate(partial[index],
                                                    new Key(kNew),
                                                    passValue(vChild));
                                        } else {
                                            std::copy(kChild->begin(),
                                                kChild->begin() + pos[0][0],
                                                kNew.begin());
                                            std::copy(
                                                kChild->begin() + pos[0][0] + 2,
                                                kChild->begin() + pos[1][0],
                                                kNew.begin() + pos[0][0]);
                                            std::copy(
                                                kChild->begin() + pos[1][0] + 2,
                                                kChild->end(),
                                                kNew.begin() + pos[1][0] - 2);

                                            if (vData.keyViable(kNew))
                                                aggregate(partial[index],
                                                    new Key(kNew),
                                                    switchValue(vChild, c));
                                        }
                                    } else if (pos[0][0] + 1 == pos[1][1] &&
                                            pos[1][0] + 1 == pos[0][1] &&
                                            pos[0][0] < pos[1][0]) {
                                        std::copy(kChild->begin(),
                                            kChild->begin() + pos[0][0],
                                            kNew.begin());
                                        std::copy(
                                            kChild->begin() + pos[0][0] + 2,
                                            kChild->begin() + pos[1][0],
                                            kNew.begin() + pos[0][0]);
                                        std::copy(
                                            kChild->begin() + pos[1][0] + 2,
                                            kChild->end(),
                                            kNew.begin() + pos[1][0] - 2);

                                        if (vData.keyViable(kNew))
                                            aggregate(partial[index],
                                                new Key(kNew),
                                                spliceValue(vChild, c));
                                    }
                                }
                            }
                            break;
                    }
                }

#ifdef IDENTIFY_NONVIABLE_KEYS
                if (! vData.foundViable)
                    std::cerr << "UNUSED: " << *kChild << std::endl;
#endif

                delete kChild;
                delete vChild;
            }

            delete partial[child->index()];
            partial[child->index()] = nullptr;
        } else {
            // Join bag.
            child = bag->children();
            sibling = child->sibling();

            // Extract the sizes of each bag's keys.
            // The key size depends only on the bag, not the particular
            // key-value solution at that bag, and so we get this data
            // by looking at the first solution in each bag.
            int pairs1 = partial[child->index()]->begin()->first->size()/2;
            int pairs2 = partial[sibling->index()]->begin()->first->size()/2;
            int pairs = pairs1 + pairs2;

            std::cerr << "JOIN -> " <<
                partial[child->index()]->size() << " x " <<
                partial[sibling->index()]->size() << " : #pairs = " <<
                pairs1 << " / " << pairs2 << std::endl;

            if (pairs1 == 0) {
                // The keys are exactly the keys from the second child,
                // so we steal the second child solution set entirely
                // without copying solutions individually.
                //
                // The first child should have exactly one key (which is
                // empty), and we just need to multiply all values by
                // the corresponding value.

                auto emptySoln = partial[child->index()]->begin();

                partial[index] = partial[sibling->index()];
                for (auto& soln : *(partial[index]))
                    (*soln.second) *= *(emptySoln->second);

                delete emptySoln->first;
                delete emptySoln->second;
                delete partial[child->index()];

                partial[child->index()] = partial[sibling->index()] = nullptr;
                continue;
            } else if (pairs2 == 0) {
                // As before, but with the two children the other way around.
                auto emptySoln = partial[sibling->index()]->begin();

                partial[index] = partial[child->index()];
                for (auto& soln : *(partial[index]))
                    (*soln.second) *= *(emptySoln->second);

                delete emptySoln->first;
                delete emptySoln->second;
                delete partial[sibling->index()];

                partial[child->index()] = partial[sibling->index()] = nullptr;
                continue;
            }

            // Both child bags have positive length keys.

            vData.initJoinBag(partial[child->index()]->begin()->first,
                partial[sibling->index()]->begin()->first);

            partial[index] = new SolnSet;

            Key kNew(2 * pairs);

            // The bits of choice correspond to the positions of pairs in the
            // final key.  A 0 bit means we take a pair from k1, and a 1 bit
            // means we take a pair from k2.
            Bitmask choice(pairs);

            int pos1, pos2, pos;

            const Key *k1, *k2;
            const Value *v1, *v2;
            // int count = 0;
            for (auto& soln1 : *(partial[child->index()])) {
                k1 = soln1.first;
                v1 = soln1.second;
                // if ((++count) % 100 == 0) std::cerr << count << std::endl;

#ifdef IDENTIFY_NONVIABLE_KEYS
                vData.foundViable = false;
#endif
                for (auto& soln2 : *(partial[sibling->index()])) {
                    k2 = soln2.first;
                    v2 = soln2.second;

                    // Combine the two child keys and values in all
                    // possible ways.
                    Value val(*v1);
                    val *= *v2;

                    // Fill the final key from the end to the beginning, so
                    // that we can more aggressively test for non-viable keys.
                    choice.reset();
                    pos = pairs - 1;
                    pos1 = pairs1 - 1;
                    pos2 = pairs2 - 1;

                    while (pos < pairs) {
                        // We are about to try the current option for
                        // position pos in the final key.
                        if (pos < 0) {
                            // Try the key.
                            if (vData.partialKeyViable(kNew, pos)) {
                                if (! partial[index]->emplace(
                                        new Key(kNew), new Value(val)).second)
                                    std::cerr << "ERROR: Combined keys in join "
                                        "bag are not unique" << std::endl;
                            }
                            // Fall through to the backtrack step.
                        } else if (! choice.get(pos)) {
                            // Try key 1, if we can.
                            if (pos1 >= 0) {
                                kNew[2 * pos] = (*k1)[2 * pos1];
                                kNew[2 * pos + 1] = (*k1)[2 * pos1 + 1];

                                if (vData.partialKeyViable(kNew, pos)) {
                                    --pos1;
                                    --pos;
                                    continue;
                                }
                            }
                            // We cannot use key 1.
                            // Try key 2 instead.
                            choice.set(pos, true);
                            continue;
                        } else {
                            // Try key 2, if we can.
                            if (pos2 >= 0) {
                                kNew[2 * pos] = (*k2)[2 * pos2];
                                kNew[2 * pos + 1] = (*k2)[2 * pos2 + 1];

                                if (vData.partialKeyViable(kNew, pos)) {
                                    --pos2;
                                    --pos;
                                    continue;
                                }
                            }
                            // We cannot use key 2.
                            // Reset this bit, and fall through to the
                            // backtrack step.
                            choice.set(pos, false);
                        }

                        // Backtrack!
                        ++pos;
                        while (pos < pairs) {
                            // Try the next option at this position.
                            if (! choice.get(pos)) {
                                ++pos1;
                                choice.set(pos, true);
                                break;
                            } else {
                                ++pos2;
                                // We are out of options for this bit.
                                // Reset the bit and move further up.
                                choice.set(pos, false);
                                ++pos;
                            }
                        }
                    }
                }

#ifdef IDENTIFY_NONVIABLE_KEYS
                if (! vData.foundViable)
                    std::cerr << "UNUSED: " << *k1 << std::endl;
#endif
            }

            for (auto& soln : *(partial[child->index()])) {
                delete soln.first;
                delete soln.second;
            }
            for (auto& soln : *(partial[sibling->index()])) {
                delete soln.first;
                delete soln.second;
            }

            delete partial[child->index()];
            delete partial[sibling->index()];
            partial[child->index()] = partial[sibling->index()] = nullptr;
        }

        /*
        for (const auto& soln : *(partial[index])) {
            for (int i = 0; i < soln.first->size(); ++i)
                std::cerr << (*soln.first)[i] << ' ';
            std::cerr << "-> " << (*soln.second) << std::endl;
        }
        */
    }

    // Collect the final answer from partial[nBags - 1].
    // std::cerr << "FINISH" << std::endl;
    Value* ans = partial[nBags - 1]->begin()->second;

    for (auto& soln : *(partial[nBags - 1])) {
        delete soln.first;
        if (soln.second != ans)
            delete soln.second;
    }
    delete partial[nBags - 1];

    delete[] partial;

    // Finally, factor in any zero-crossing components.
    for (StrandRef s : components_)
        if (! s)
            (*ans) *= delta;

    return ans;
}

const Laurent2<Integer>& Link::homflyAZ(Algorithm alg) const {
    if (homflyAZ_.known())
        return *homflyAZ_.value();

    if (crossings_.empty()) {
        if (components_.empty())
            return *(homflyAZ_ = new Laurent2<Integer>());

        // We have an unlink with no crossings.
        // The HOMFLY polynomial is delta^(#components - 1).
        Laurent2<Integer> delta(1, -1);
        delta.set(-1, -1, -1);

        // The following constructor initialises ans to 1.
        Laurent2<Integer>* ans = new Laurent2<Integer>(0, 0);
        for (size_t i = 1; i < components_.size(); ++i)
            (*ans) *= delta;

        return *(homflyAZ_ = ans);
    }

    switch (alg) {
        case ALG_TREEWIDTH:
            homflyAZ_ = homflyTreewidth();
            break;
        default:
            homflyAZ_ = homflyKauffman();
            break;
    }
    return *homflyAZ_.value();
}

const Laurent2<Integer>& Link::homflyLM(Algorithm alg) const {
    if (homflyLM_.known())
        return *homflyLM_.value();

    Laurent2<Integer>* ans = new Laurent2<Integer>(homflyAZ(alg));

    // Negate all coefficients for a^i z^j where i-j == 2 (mod 4).
    // Note that i-j should always be 0 or 2 (mod 4), never odd.
    for (auto& term : ans->coeff_) {
        if ((term.first.first - term.first.second) % 4 != 0)
            term.second.negate();
    }

    return *(homflyLM_ = ans);
}

} // namespace regina

