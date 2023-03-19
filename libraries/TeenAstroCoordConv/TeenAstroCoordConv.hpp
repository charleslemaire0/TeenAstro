
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

// Converts between reference coordinates (angle1/angle2) 
// and axis coordinates (axis1 and axis2)
class CoordConv : public LA3 {
public:

	double T[3][3];		// Transformation matrix from equatorial angle1/angle2 cosines to axis axis2/axis1 cosines 
	double Tinv[3][3];		// Inverse of the above 

	enum Err { EQ_AZ, EQ_ALT, POL_W };
	CoordConv() { reset(); isready = false;}

  // resets reference stars
  void reset() { refs = 0; }

  // clean
  void clean() { setT(0, 0, 0, 0, 0, 0, 0, 0, 0); isready = false; }
	
	// returns true if all required reference stars are set (need three)
	bool isReady() const { return isready; }

  // return the number of currently measured refs
  unsigned char getRefs() const { return refs; }

	// get the transformation to be stored into EEPROM
	void getT(float &m11, float &m12, float &m13,float &m21, float &m22, float &m23,float &m31, float &m32, float &m33);
	
	// set the transformation from EEPROM
	void setT(float m11, float m12, float m13,float m21, float m22, float m23,float m31, float m32, float m33);

	void setTinvFromT();
	
	// add a user-provided reference star (all values in degrees, except time in seconds). adding more than three has no effect
	void addReferenceDeg(double angle1, double angle2, double axis1, double axis2);

	// Calculate third reference star from two provided ones. Returns false if more or less than two provided 
	bool calculateThirdReference();

	// Convert reference angle1/angle2 coordinates to axis axis1/axis2 coordinates (all values in degrees) 
	void toAxisDeg(double &axis1, double &axis2,  double angle1, double angle2) const;

	// Convert axis axis1/axis2 coordinates to  reference angle1/angle2 coordinates (all values in degrees) 
	void toReferenceDeg(double &angle1,  double &angle2, double axis1, double axis2) const;

	double polErrorDeg(double lat, Err sel);


protected:
	// add a user-provided reference star (all values in radians)
	void addReference(double angle1, double angle2, double axis1, double axis2);

	// Build coordinate system transformation matrix
	void buildTransformations();

	// Convert reference angle1/angle2 coordinates to axis axis1/axis2 coordinates (all values in radians) 
	void toAxis(double &axis1, double &alt,  double angle1,  double angle2) const;

	// Convert axis axis1/axis2 coordinates to  equatorial hour angle/angle2 coordinates (all values in radians) 
	void toReference(double &angle1,  double &angle2, double axis1, double axis2) const;



  double dcAARef[3][3];	// axis1/axis2 direction cosine vectors for the three reference stars, indexed by reference first
  double dcHDRef[3][3];	// angle1/angle2l direction cosine vectors for the three reference stars, indexed by reference first

  unsigned char refs=0;	// number of reference stars
  bool isready=false;
};



#endif // __CoordConv_hpp__
