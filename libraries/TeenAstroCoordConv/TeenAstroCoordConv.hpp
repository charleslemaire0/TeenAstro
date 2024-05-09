
/*
 * Title       TeenAstro coordinate Conversion
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
 * Math Author:  Toshimi Taki, 
 * Lib  Authors: Markus Noga, Charles Lemaire
 *
 * Description:
 *
 * Conversion from axis to sky coordinates
 *
 */
#pragma once

#ifndef __TeenAstroCoordConv_hpp__
#define __TeenAstroCoordConv_hpp__

#ifndef M_PI
#include <math.h> // for M_PI
#endif

#include "TeenAstroLA3.hpp"

// Compute a 3x3 Matrix that describes rotaions between reference coordinates (angle1/angle2) 
// and axis coordinates (axis1 and axis2)
class CoordConv : public LA3 {
public:
	double ax1[2],ax2[2];
	double T[3][3];		    // Transformation matrix from Horizontal to  intrument axis
	double Tinv[3][3];		// Inverse of the above 
	double u[3][3];
	double v[3][3];
	CoordConv() { reset(); isready = false;}

  // resets reference stars
  void reset() { refs = 0; }

  // clean
	void clean() {
		setT(0, 0, 0, 0, 0, 0, 0, 0, 0); isready = false; anglediff = 0;
	}
	
	// returns true if all required reference stars are set (need three)
	bool isReady() const { return isready; }

	double getError() const { return isready ? anglediff : 0; }

  // return the number of currently measured refs
  unsigned char getRefs() const { return refs; }

	// get the transformation to be stored into EEPROM
	void getT(float &m11, float &m12, float &m13,float &m21, float &m22, float &m23,float &m31, float &m32, float &m33);
	
	// set the transformation from EEPROM
	void setT(float m11, float m12, float m13,float m21, float m22, float m23,float m31, float m32, float m33);

	void setTinvFromT();
	
	// add a user-provided reference star (all values in radians)
	void addReference(double angle1, double angle2, double axis1, double axis2);

	// Calculate third reference star from two provided ones. Returns false if more or less than two provided 
	bool calculateThirdReference();
	void minimizeAxis1(double offset);
	void minimizeAxis2();
protected:

	// Build coordinate system transformation matrix
	void buildTransformations();

  double dcAARef[3][3];	// axis1/axis2 direction cosine vectors for the three reference stars, indexed by reference first
  double dcHDRef[3][3];	// angle1/angle2l direction cosine vectors for the three reference stars, indexed by reference first

  unsigned char refs=0;	// number of reference stars
  bool isready = false;
	double anglediff = 0;
};



#endif // __CoordConv_hpp__
