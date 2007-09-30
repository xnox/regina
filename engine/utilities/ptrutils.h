
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  Computational Engine                                                  *
 *                                                                        *
 *  Copyright (c) 1999-2007, Ben Burton                                   *
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

/*! \file ptrutils.h
 *  \brief Provides function objects for use in the Standard Template
 *  Library that take pointers as arguments but work with the pointees
 *  instead.
 */

#ifndef __PTRUTILS_H
#ifndef __DOXYGEN
#define __PTRUTILS_H
#endif

namespace regina {

/**
 * \weakgroup utilities
 * @{
 */

/**
 * An adaptable binary function used to compare the objects to which
 * pointers are pointing.  This class is for use with the Standard Template
 * Library.
 *
 * The first template argument \a T will generally not be a pointer class.
 * Instead, this function will accept two const \e pointers to \a T.  It
 * will then dereference these pointers and compare these dereferenced
 * objects using the given comparison function (which defaults to std::less,
 * but which can be changed by passing a different second template argument).
 *
 * \ifacespython Not present.
 */
template <typename T, typename Comp = std::less<T> >
class LessDeref {
    public:
        typedef const T* first_argument_type;
            /**< The first argument type for this binary function. */
        typedef const T* second_argument_type;
            /**< The second argument type for this binary function. */
        typedef bool result_type;
            /**< The result type for this binary comparison function. */

    private:
        Comp comp_;
            /**< A function object for performing comparisons between
                 dereferenced objects. */

    public:
        /**
         * Compares the objects to which the given pointers are pointing.
         * The two pointers are dereferenced, and then a function of
         * type \a Comp (the second template argument) is used to
         * compare the dereferenced objects.
         *
         * @param ptr1 a pointer to the first object under examination.
         * @param ptr2 a pointer to the second object under examination.
         * @return \c true if the first dereferenced object is less than
         * the second, or \c false otherwise.
         */
        bool operator() (const T* ptr1, const T* ptr2) const {
            return comp_(*ptr1, *ptr2);
        }
};

/*@}*/

} // namespace regina

#endif

