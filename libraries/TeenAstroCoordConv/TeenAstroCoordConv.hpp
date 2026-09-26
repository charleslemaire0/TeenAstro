
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
#include "TeenAstroHeadModel.hpp"

/// Maximum alignment stars retained for the rigid model fit.
#define COORDCONV_MAX_STARS 9
/// Minimum stars accepted for a rigid fit. Six unknowns and two equations per
/// star make three stars sufficient on paper, but that leaves zero redundancy:
/// every measurement error goes straight into the parameters. Four stars give
/// two spare equations, which is what lets the conditioning test below tell a
/// genuinely separable term from noise.
#define COORDCONV_MIN_RIGID_STARS 4
/// Axis2 non-perpendicularity may be solved once a rigid session has this many
/// stars. It does not need a meridian flip: within one pier side it is the
/// observable combination of cone and non-perpendicularity.
#define COORDCONV_MIN_PERP_STARS 4
/// Cone error is solved only when each pier side contributes at least this many
/// stars. Three and three is the smallest such session (six stars). One star
/// past the pole is not enough, and six stars on one side is not either.
#define COORDCONV_MIN_CONE_PER_SIDE 3

/// Bits returned by CoordConv::getRigidMask().
#define COORDCONV_FIT_CONE 0x01
#define COORDCONV_FIT_PERP 0x02
#define COORDCONV_FIT_IDX2 0x04

/// Selectors for polar / horizontal misclosure from \p Tinv (polErrorDeg).
/// Plain enum (no fixed underlying type) so Teensy / Arduino builds without -std=c++11 still compile.
enum PolarErrSel {
  PE_EQ_AZ = 0,
  PE_EQ_ALT = 1,
  PE_POL_W = 2
};

// Compute a 3x3 Matrix that describes rotaions between reference coordinates (angle1/angle2) 
// and axis coordinates (axis1 and axis2)
class CoordConv : public LA3 {
public:
	double ax1[2],ax2[2];
	double T[3][3];		    // Transformation matrix from Horizontal to  intrument axis
	double Tinv[3][3];		// Inverse of the above 
	double u[3][3];
	double v[3][3];
	/// Rigid head geometry (cone / perp / axis2 index). Zero means the legacy T only model.
	HeadModel head;
	CoordConv() { reset(); isready = false;}

  // resets reference stars
  void reset() { refs = 0; }

  // clean — zero T and Tinv together so callers never leave a stale inverse
	void clean() {
		setT(0, 0, 0, 0, 0, 0, 0, 0, 0);
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				Tinv[i][j] = 0;
		refs = 0;
		isready = false;
		anglediff = 0;
		head = HeadModel();
		nstars = 0;
		rigidRms = 0;
		rigidMask = 0;
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

	/// Pole / horizontal misclosure (degrees). \p latRad site latitude (radians); basis matches toDirCos / southern azimuth.
	double polErrorDeg(double latRad, PolarErrSel sel) const;
	
	// add a user-provided reference star (all values in radians)
	void addReference(double angle1, double angle2, double axis1, double axis2);

	// Calculate third reference star from two provided ones. Returns false if more or less than two provided 
	bool calculateThirdReference();
	void minimizeAxis1(double offset);
	void minimizeAxis2();

	// ---------------------------------------------------------------------------
	// Rigid six degree of freedom model: T (three) plus head (three).
	// ---------------------------------------------------------------------------

	/// True when a non zero head geometry is in effect.
	bool hasHead() const { return !head.isZero(); }

	/// Set the head terms directly (radians), e.g. when restoring from EEPROM.
	void setHead(double cone, double perp, double idx2) { head = HeadModel(cone, perp, idx2); }

	/// Read the head terms (radians) for persistence.
	void getHead(float &cone, float &perp, float &idx2) const
	{
		cone = (float)head.cone; perp = (float)head.perp; idx2 = (float)head.idx2;
	}

	/// Drop the head terms, leaving T untouched. Reverts to the legacy model.
	void clearHead() { head = HeadModel(); }

	/// Record an alignment star without touching the two star Taki solution.
	/// \p axis1Direct is the raw Rz Euler angle (Coord_IN::Axis1_direct()).
	void addStar(double angle1, double angle2, double axis1Direct, double axis2);

	/// Number of retained alignment stars.
	unsigned char getStars() const { return nstars; }

	/// How the retained stars split across the two mechanical configurations.
	/// \p nIn is axis2 inside +/-90 deg, \p nOut is axis2 past that (beyond the
	/// pole). Cone error is solved only when both are at least
	/// COORDCONV_MIN_CONE_PER_SIDE.
	void pierSideCounts(int &nIn, int &nOut) const
	{
		nIn = 0;
		nOut = 0;
		for (unsigned char i = 0; i < nstars; i++)
		{
			double a = starAxis[i][1];
			while (a > M_PI) a -= 2.0 * M_PI;
			while (a < -M_PI) a += 2.0 * M_PI;
			const double mag = a < 0.0 ? -a : a;
			if (mag < M_PI_2 - 1e-3) nIn++;
			else if (mag > M_PI_2 + 1e-3) nOut++;
		}
	}

	/// Forget the retained stars. T and the head terms are left alone.
	void resetStars() { nstars = 0; }

	/// Fit T and the head terms to the retained stars by damped Gauss-Newton.
	/// Requires isReady() (T already seeded, normally by the two star Taki pass)
	/// and at least COORDCONV_MIN_RIGID_STARS stars. On success T, Tinv and head
	/// are updated and true is returned; on failure nothing is modified.
	///
	/// Head terms are selected from the data rather than fitted unconditionally:
	/// a term is only solved when this star distribution separates it from the
	/// terms already selected. See getRigidMask().
	bool fitRigidModel(double *rmsOut = 0, int *iterOut = 0);

	/// Which head terms the last fit actually solved, as a bit mask of
	/// COORDCONV_FIT_CONE / _PERP / _IDX2. Terms left out were not separable
	/// from the others given the star distribution and are held at zero.
	unsigned char getRigidMask() const { return rigidMask; }

	/// RMS angular residual of the retained stars under the current model (radians).
	double residualRms() const;

	/// RMS residual recorded by the last successful fitRigidModel() (radians).
	double getRigidRms() const { return rigidRms; }

protected:

	// Build coordinate system transformation matrix
	void buildTransformations();

	/// Sky direction predicted for retained star \p i using \p h and the current Tinv.
	void predictSky(double (&p)[3], unsigned char i, const HeadModel &h) const;

	/// Accumulate the Gauss-Newton normal equations at (\p Tinv_w, \p head_w).
	/// Returns the sum of squared tangent plane residuals.
	double accumulateNormals(const double (&Tinv_w)[3][3], const HeadModel &head_w,
	                         const double (&targets)[COORDCONV_MAX_STARS][3],
	                         double (&N)[6][6], double (&g)[6]) const;

  double dcAARef[3][3];	// axis1/axis2 direction cosine vectors for the three reference stars, indexed by reference first
  double dcHDRef[3][3];	// angle1/angle2l direction cosine vectors for the three reference stars, indexed by reference first

  unsigned char refs=0;	// number of reference stars
  bool isready = false;
	double anglediff = 0;

	// Retained alignment stars for the rigid fit: sky angle1/angle2 and the
	// matching instrument axis1Direct/axis2, all radians.
	double starSky[COORDCONV_MAX_STARS][2];
	double starAxis[COORDCONV_MAX_STARS][2];
	unsigned char nstars = 0;
	double rigidRms = 0;
	unsigned char rigidMask = 0;
};



#endif // __CoordConv_hpp__
