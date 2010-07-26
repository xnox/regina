
/**************************************************************************
 *                                                                        *
 *  Regina - A Normal Surface Theory Calculator                           *
 *  Computational Engine                                                  *
 *                                                                        *
 *  Copyright (c) 1999-2009, Ben Burton                                   *
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

/*! \file algebra/NSVPolynomialRing.h
 *  \brief A single variable polynomial ring object, implemented sparsely.
 */

#ifndef __NSVPolynomialRing_H
#ifndef __DOXYGEN
#define __NSVPolynomialRing_H
#endif

#include <sstream>
#include <map>

#include "maths/nlargeinteger.h"
#include "utilities/ptrutils.h"

namespace regina {

/**
 * \weakgroup algebra
 * @{
 */

/**
 * This is a class for dealing with elements of a single-variable polynomial ring, 
 * implemented sparsely.
 *
 * \testpart
 *
 * \todo allow for rational coefficients. Template ring.
 * \todo Allow for Laurent polynomials.
 *
 * @author Ryan Budney
 */
class NSVPolynomialRing {
    public:
	static const NSVPolynomialRing zero;
        static const NSVPolynomialRing one;
        static const NSVPolynomialRing pvar;
    private:
       // sparse storage of coefficients
       std::map< signed long, NLargeInteger* > cof;

    public:
        /**
         * Creates an element of the polynomial ring, zero by default. 
         */
        NSVPolynomialRing();

        /**
         * Creates an element of the polynomial ring, of the form at^k 
         */
        NSVPolynomialRing(const NLargeInteger &a, signed long k);

        /**
         * Creates an element of the polynomial ring, of the form "a" 
         */
        NSVPolynomialRing(signed long a);

	/**
	 * Destructor.
 	 */
	virtual ~NSVPolynomialRing();

        /**
         * Creates a polynomial that is a clone of the given
         * polynomial.
         *
         * @param cloneMe the permutation to clone.
         */
        NSVPolynomialRing(const NSVPolynomialRing& cloneMe);

        /**
         * Sets this polynomial to the given polynomial.
         */
        NSVPolynomialRing& operator = (const NSVPolynomialRing& cloneMe);

        /**
	 * Set a coefficient.
         */
        void setCoefficient (signed long i, const NLargeInteger& c);

        /**
	 * Returns the product of two polynomials.
         */
        NSVPolynomialRing operator * (const NSVPolynomialRing& q) const;

        /**
	 * Scalar multiplication of a polynomial.
         */
        friend NSVPolynomialRing operator * (const NLargeInteger &k, const NSVPolynomialRing& q);

	/**
	 * Returns the sum of two polynomials.
	 */
	NSVPolynomialRing operator + (const NSVPolynomialRing& q) const;

	/**
	 * Returns the difference of two polynomials.
	 */
	NSVPolynomialRing operator - (const NSVPolynomialRing& q) const;

	NSVPolynomialRing& operator -=(const NSVPolynomialRing& q);
	NSVPolynomialRing& operator +=(const NSVPolynomialRing& q);
	NSVPolynomialRing operator -() const;

        /**
	 * Returns the the coefficient of t^i for this polynomial.
         */
        const NLargeInteger& operator[](signed long i) const;

        /**
         * Determines if two polynomials are equal.
         */
        bool operator == (const NSVPolynomialRing& other) const;

        /**
	 * Determines if two polynomials are not equal.
         */
        bool operator != (const NSVPolynomialRing& other) const;

        /**
         * Determines if this polynomial is equal to the multiplicative identity
         * element.
         */
        bool isIdentity() const;

       /**
         * Determines if this polynomial is equal to the additive identity
         * element.
         */
        bool isZero() const;

	/**
	 * Returns the number of sign changes in coefficients of the polynomial P(t) - number of sign changes of P(-t) 
         * This is the number of + roots - number of - roots, provided all real.
         */
	long int descartesNo() const;

        /**
         * Returns a string representation of this polynomial.
	 * <tt>a + bt + ct^2 + ... </tt>
         */
        std::string toString() const;

	/**
         * Stream version of toString()
         */
        virtual void writeTextShort(std::ostream& out) const;

	/**
	 * Returns TeX string formatting of toString()
	 */
	std::ostream& writeTeX(std::ostream &out) const;

	/**
         * string version of writeTeX.
	 */
        std::string toTeX() const;

        /**
         * stream-friendly output
         */
        friend std::ostream& operator << (std::ostream& out, const NSVPolynomialRing& p);
};

/*@}*/

// Inline functions for NSVPolynomialRing

// zero element -- nothing to do
inline NSVPolynomialRing::NSVPolynomialRing() {}

// monomial constructor
inline NSVPolynomialRing::NSVPolynomialRing(const NLargeInteger &a, signed long k)
{ if (a != 0) cof.insert(std::pair<signed long, NLargeInteger*>( k, new NLargeInteger(a) ) ); }

inline NSVPolynomialRing::NSVPolynomialRing(signed long a)
{ if (a != 0) cof.insert(std::pair<signed long, NLargeInteger*>( 0, new NLargeInteger(a) ) ); }

// destructor
inline NSVPolynomialRing::~NSVPolynomialRing()
{
std::map< signed long, NLargeInteger* >::iterator ci;
for (ci = cof.begin(); ci != cof.end(); ci++)
	delete ci->second;
}

// copy constructor
inline NSVPolynomialRing::NSVPolynomialRing(const NSVPolynomialRing& cloneMe)
{
std::map< signed long, NLargeInteger* >::const_iterator ci;
for (ci = cloneMe.cof.begin(); ci != cloneMe.cof.end(); ci++) cof.insert( 
 std::pair< signed long, NLargeInteger* >( ci->first, clonePtr(ci->second) ) );
}

// assignment
inline NSVPolynomialRing& NSVPolynomialRing::operator = (const NSVPolynomialRing& cloneMe)
{
// deallocate everything
std::map< signed long, NLargeInteger* >::iterator ci;
for (ci = cof.begin(); ci != cof.end(); ci++)
	delete ci->second;
cof.clear();

// copy everything
ci = cof.begin();
std::map< signed long, NLargeInteger* >::const_iterator Ci;
for (Ci = cloneMe.cof.begin(); Ci != cloneMe.cof.end(); Ci++)
	ci = cof.insert(ci, std::pair< signed long, NLargeInteger* >( Ci->first, clonePtr(Ci->second) ));

return (*this);
}

inline const NLargeInteger& NSVPolynomialRing::operator[](signed long i) const
{
// is t^i in cof?  if so return coefficient, if not, return 0.
 std::map< signed long, NLargeInteger* >::const_iterator p;
 p = cof.find(i);
 if (p != cof.end()) return (*p->second);
 else return NLargeInteger::zero;
}

inline bool NSVPolynomialRing::operator == (const NSVPolynomialRing& other) const
{
// verify equal...
 if (cof.size() != other.cof.size()) return false;
 std::map< signed long, NLargeInteger* >::const_iterator p1, p2;
 p1 = cof.begin(); p2 = other.cof.begin();
 while (p1 != cof.end() )
  {
   if ( p1->first != p2->first ) return false;
   if ( (*p1->second) != (*p2->second) ) return false;
   p1++; p2++;
  }
 return true;
}

inline bool NSVPolynomialRing::operator != (const NSVPolynomialRing& other) const
{ return ( !((*this)==other) ); }

inline bool NSVPolynomialRing::isIdentity() const
{
// check only nonzero coeff is t^0 with coeff 1.
if (cof.size() != 1) return false;
if (cof.begin()->first != 0) return false;
if (*(cof.begin()->second) != NLargeInteger::one) return false;
return true;
}

inline bool NSVPolynomialRing::isZero() const
{
if (cof.size() != 0) return false;
return true;
}

inline std::string NSVPolynomialRing::toString() const
{
// run through cof, assemble into string
std::string retval; std::stringstream ss;
std::map< signed long, NLargeInteger* >::const_iterator p;
bool outputSomething = false; // keep track of whether or not we've output anything
for (p = cof.begin(); p != cof.end(); p++)
 {
  NLargeInteger mag( p->second->abs() );  bool pos( ((*p->second) > 0) ? true : false );  signed long exp( p->first );
  ss.str("");  ss<<p->first;
  // ensure sensible output, eg:  t^(-5) + 5 - t + 3t^2 + t^3 - t^(12), etc...
  if (mag != 0) // don't output 0t^n
   { 
   if (outputSomething) retval += ( pos ? "+" : "-" ); else if (!pos) retval += "-";
   outputSomething = true;
   if ( (exp==0) || (mag != 1) ) retval += mag.stringValue();
   if (exp == 1) retval += "t"; else 
   if ( (exp<0) || (exp>9) ) retval += "t^(" + ss.str() + ")"; else
    if (exp != 0) retval += "t^" + ss.str();
   }
 }
if (outputSomething == false) retval = "0";
return retval;
}

inline void NSVPolynomialRing::writeTextShort(std::ostream& out) const
{ out<<toString(); }

inline std::ostream& NSVPolynomialRing::writeTeX(std::ostream &out) const
{ out<<toTeX(); return out; }

inline std::string NSVPolynomialRing::toTeX() const
{
// run through cof, assemble into string
std::string retval; std::stringstream ss;
std::map< signed long, NLargeInteger* >::const_iterator p;
bool outputSomething = false; // keep track of whether or not we've output anything
for (p = cof.begin(); p != cof.end(); p++)
 {
  NLargeInteger mag( p->second->abs() );  bool pos( ((*p->second) > 0) ? true : false );  signed long exp( p->first );
  ss.str("");  ss<<p->first;
  // ensure sensible output, eg:  t^{-5} + 5 - t + 3t^2 + t^3 - t^{12}, etc...
  if (mag != 0) // don't output 0t^n
   { 
   if (outputSomething) retval += ( pos ? "+" : "-" ); else if (!pos) retval += "-";
   outputSomething = true;
   if ( (exp==0) || (mag != 1) ) retval += mag.stringValue();
   if (exp == 1) retval += "t"; else 
   if ( (exp<0) || (exp>9) ) retval += "t^{" + ss.str() + "}"; else
    if (exp != 0) retval += "t^" + ss.str();
   }
 }
if (outputSomething == false) retval = "0";
return retval;
}

inline std::ostream& operator << (std::ostream& out, const NSVPolynomialRing& p)
{ p.writeTextShort(out); return out; }

} // namespace regina

#endif

