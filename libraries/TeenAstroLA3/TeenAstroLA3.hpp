
/*
 * Title       TeenAstro Linear Algebra
 * by          Markus Noga, Charles Lemaire
 *
 * Copyright (C) Markus Noga
 * Copyright (C) Charles Lemaire
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
 * Lib  Authors: Markus Noga, Charles Lemaire
 */

#pragma once

#ifndef __TeenAstroLA3_hpp__
#define __TeenAstroLA3_hpp__

#ifndef M_PI
#include <math.h> // for M_PI
#endif

// Basic linear algebra operations for 3-vectors and 3x3 matrices 
class LA3 {
public:
	enum RotAxis { ROTAXISX, ROTAXISY, ROTAXISZ };
  struct SingleRotation
  {
		RotAxis axis;
    double angle;
  };

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

	// get getIdentityMatrix
	static void getIdentityMatrix(double(&out)[3][3]);

	// get a Matrix after a single rotation
	static void getSingleRotationMatrix(double(&out)[3][3], const LA3::SingleRotation sr);

	// get a Matrix after a sequence of rotations
	static void getMultipleRotationMatrix(double(&out)[3][3], const LA3::SingleRotation* sr, int n);

	// Calculate Euler Angle RX0RYRX1 (in radians) from Matrix()
	static void getEulerRx0RyRx1(const double(&r)[3][3], double& thetaX0, double& thetaY, double& thetaX1);

	// Calculate Euler Angle (in radians) from Matrix()
	static void getEulerRzRxRy(const double(&r)[3][3], double& thetaZ, double& thetaX, double& thetaY);

	// Calculate Euler Angle (in radians) from Matrix()
	static void getEulerRzRyRx(const double(&r)[3][3], double& thetaZ, double& thetaY, double& thetaX);

	// Calculate Euler Angle (in radians) from Matrix()
	static void getEulerRxRyRz(const double(&r)[3][3], double& thetaX, double& thetaY, double& thetaZ);

	// Calculate determinant of a 3x3 matrix
	static double determinant(const double (&m)[3][3]);

	// Print a 3-vector to cout, with given label
	static void printV(const char *label, const double (&v)[3]);

	// Print a 3x3 matrix to cout, with given label
	static void printV(const char *label, const double (&m)[3][3]);
};

#endif // __TeenAstroLA3_hpp__
