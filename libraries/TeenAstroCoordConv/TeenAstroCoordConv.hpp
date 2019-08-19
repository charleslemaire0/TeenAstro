
/*
 * Title       TeenAstro coordinate Conversion
 * by          Markus Noga, Charles Lemaire
 *
 * Copyright (C) Markus Noga
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *
 * Revision History, see GitHub
 *
 *
 * Math Author:  Toshimi Taki, 
 * Lib  Authors: Markus Noga, Charles Lemaire
 *
 * Description:
 *
 * Conversion from instrumental to sky coordinates
 *
 */
#pragma once

#ifndef __TeenAstroCoordConv_hpp__
#define __TeenAstroCoordConv_hpp__

#ifndef M_PI
#include <math.h> // for M_PI
#endif

// Basic linear algebra operations for 3-vectors and 3x3 matrices 
class LA3 {
public:
	// Multiply two 3-vectors
	static void crossProduct(double (&out)[3], const double (&a)[3], const double (&b)[3]);

	// Calculate the 2-norm of a 3-vector, i.e. its Euclidean length
	static double norm(const double (&in)[3]);

	// Normalize a 3-vector to unit length 
	static void normalize(double (&out)[3], const double (&in)[3]);

	// Calculate cosine direction vector from two given polar angles (in radians)
	static void toDirCos(double (&dc)[3], double ang1, double ang2);

	// Calculate polar angles (in radians) from given cosine direction vector
	static void toAngles(double &ang1, double &ang2, const double (&dc)[3]);

	// Convert degrees to radians
	static double toRad(double degrees) { return degrees*M_PI/180; }

	// Convert radians to degrees
	static double toDeg(double radians) { return radians*180/M_PI; }

	// Normalize angle in radians to [0..2PI)
	static double normalizeRads(double radians) { return radians - floor(radians/(2*M_PI))*2*M_PI; }

	// Copy a 3x3 matrix 
	static void copy(double (&out)[3][3], const double (&in)[3][3]);

	// Transpose a 3x3 matrix 
	static void transpose(double (&out)[3][3], const double (&in)[3][3]);

	// Calculate the inverse of a given invertable 3x3 matrix
	static void invert(double (&out)[3][3], const double (&m)[3][3]);

	// Multiply two 3x3 matrices
	static void multiply(double (&out)[3][3], const double (&a)[3][3], const double (&b)[3][3]);

	// Multiply 3x3 matrix with 3-vector
	static void multiply(double (&out)[3], const double (&m)[3][3], const double (&v)[3]);

	// Calculate determinant of a 3x3 matrix
	static double determinant(const double (&m)[3][3]);

	// Print a 3-vector to cout, with given label
	static void printV(const char *label, const double (&v)[3]);

	// Print a 3x3 matrix to cout, with given label
	static void printV(const char *label, const double (&m)[3][3]);
};


// Converts local time and right ascension to local hour angle, and vice versa
class HourAngleConv : public LA3 {
public:
	HourAngleConv() : t0(0) {}

	// set t0, in seconds
	void setT0(unsigned long t) { t0=t; }

	// convert given right ascension to hour angle (both in radians), for given time (in seconds)
	double toHourAngle(double ra, unsigned long t) const { return normalizeRads( (t-t0)*toSiderial - ra ); }

	// convert given hour angle to right ascension (both in radians), for given time (in seconds)
	double toRightAsc( double ha, unsigned long t) const { return normalizeRads( (t-t0)*toSiderial - ha ); }

protected:
	// conversion factor from local time in seconds, to siderial time in radians
	static const double toSiderial;  

	// zero time, in seconds
	unsigned long t0;
};


// Converts between equatorial coordinates (right ascension/declination) 
// and instrumental coordinates (altitude and azimuth), for a given timeStamp in seconds
class CoordConv : public HourAngleConv {
public:
	CoordConv() { reset(); }

	// resets reference stars
	void reset() { refs=0; }

	// returns true if all required reference stars are set (need three)
	bool isReady() const { return refs==3; }

	// add a user-provided reference star (all values in degrees, except time in seconds). adding more than three has no effect
	void addReferenceDeg(double ra, double dec, double az, double alt, unsigned long t);

	// Calculate third reference star from two provided ones. Returns false if more or less than two provided 
	bool calculateThirdReference();

	// Convert equatorial right ascension/dec coordinates to instrumental az/alt coordinates (all values in degrees, except time in seconds) 
	void toHorizontalDeg(double &az, double &alt,  double ra, double dec, unsigned long t) const;

	// Convert instrumental az/alt coordinates to  equatorial right ascension/dec coordinates (all values in degrees, except time in seconds) 
	void toEquatorialDeg(double &ra,  double &dec, double az, double alt, unsigned long t) const;


protected:
	// add a user-provided reference star (all values in radians)
	void addReference(double ha, double dec, double az, double alt);

	// Build coordinate system transformation matrix
	void buildTransformations();

	// Convert equatorial hour angle/dec coordinates to instrumental az/alt coordinates (all values in radians) 
	void toHorizontal(double &az, double &alt,  double ha,  double dec) const;

	// Convert instrumental az/alt coordinates to  equatorial hour angle/dec coordinates (all values in radians) 
	void toEquatorial(double &ha,  double &dec, double az, double alt) const;


	double T   [3][3];		// Transformation matrix from equatorial ha/dec cosines to instrumental axis2/axis1 cosines 
  double Tinv[3][3];		// Inverse of the above 

  double dcAARef[3][3];	// axis1/axis2 direction cosine vectors for the three reference stars, indexed by reference first
  double dcHDRef[3][3];	// ha/decl direction cosine vectors for the three reference stars, indexed by reference first

  unsigned char refs=0;	// number of reference stars
};

// A reference star, with name, right ascension and declination coordinates
struct RefStar {
	const char *name;
	double ra;
	double dec;
};

#endif // __CoordConv_hpp__
