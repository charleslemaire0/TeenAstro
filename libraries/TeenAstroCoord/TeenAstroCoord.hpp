
/*
 * Title       TeenAstro coordinate
 * by          Charles Lemaire
 *
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
 * Lib  Authors: Charles Lemaire
 *
 * Description:
 *
 * Object oriented coordinate definition
 *
 */
#pragma once

#ifndef __TeenAstroCoord_hpp__
#define __TeenAstroCoord_hpp__

#ifndef M_PI
#include <math.h> // for M_PI
#endif

#include "TeenAstroLA3.hpp"

// Converts between reference coordinates (angle1/angle2) 
// and axis coordinates (axis1 and axis2)
class Coord : public LA3 {
private:
	LA3::SingleRotation m_Eulers[3];
public:
	Coord(LA3::SingleRotation R0, LA3::SingleRotation R1, LA3::SingleRotation R3)
	{
		m_Eulers[0] = R0;
		m_Eulers[1] = R1;
		m_Eulers[2] = R2;
	};
	Coord(Coord Coo)
	{
		m_Eulers[0] = Coo.getE0();
		m_Eulers[1] = Coo.getE1();
		m_Eulers[2] = Coo.getE2();
	};
	LA3::SingleRotation getE0()
	{
		return m_Eulers[0];
	};
	LA3::SingleRotation getE1()
	{
		return m_Eulers[1];
	};
	LA3::SingleRotation getE2()
	{
		return m_Eulers[2];
	}:
};

class Coord_EQ : public Coord {

	Coord_EQ(double FrE, double Dec, double Ha)
	{
		m_Eulers[0] = { LA3::RotAxis::ROTAXISX, FrE };
		m_Eulers[1] = { LA3::RotAxis::ROTAXISY, Dec };
		m_Eulers[2] = { LA3::RotAxis::ROTAXISZ, Ha };
	};
	Coord_HO To_Coord_HO(double Lat)
	{
		double frh, alt, az;
		double tmp[3][3];
		LA3::SingleRotation rots[4] = {
			m_Eulers[0],
			m_Eulers[1],
			m_Eulers[2],
			{LA3::RotAxis::ROTAXISY, (M_PI_2 - Lat) }
		};
		LA3::getMultipleRotationMatrix(tmp, rots, 4);
		LA3::getEulerRxRyRz(tmp, frh, alt, az);
		return Coord_HO(frh, alt, az);
	}
	Coord_IN To_Coord_IN(double Lat, double missaligment[3][3])
	{
		return To_Coord_HO(double Lat).To_Coord_IN(missaligment);
	}
  double FrE()
  {
    return m_Eulers[0].angle;
  };
	double Dec()
	{
		return m_Eulers[1].angle;
	};
	double Ha()
	{
		return m_Eulers[2].angle;
	};
};

class Coord_HO : public Coord {
	Coord_HO(double FrH, double Alt, double Az)
	{
		m_Eulers[0] = { LA3::RotAxis::ROTAXISX, FrH };
		m_Eulers[1] = { LA3::RotAxis::ROTAXISY, Alt };
		m_Eulers[2] = { LA3::RotAxis::ROTAXISZ, Az };
	};
	Coord_EQ To_Coord_EQ(double Lat)
	{
		double fre, dec, ha;
		double tmp[3][3];
		LA3::SingleRotation rots[4] = {
			m_Eulers[0],
			m_Eulers[1],
			m_Eulers[2],
			{LA3::RotAxis::ROTAXISY, -(M_PI_2 - Lat) }
		};
		LA3::getMultipleRotationMatrix(tmp, rots, 4);
		LA3::getEulerRxRyRz(tmp, fre, dec, ha);
		return Coord_EQ(fre, dec, ha);
	}
	Coord_IN To_Coord_IN(double missaligment[3][3])
	{
		double axis3, axis2, axis1;
		double tmp1[3][3];
		double tmp2[3][3];
		LA3::SingleRotation rots[3] = {
			m_Eulers[0],
			m_Eulers[1],
			m_Eulers[2]
		};
		LA3::getMultipleRotationMatrix(tmp1, rots, 3);
		LA3::multiply(tmp2, tmp1, missaligment);
		LA3::getEulerRxRyRz(tmp2, axis3, axis2, axis1);
	}
	double FrH()
	{
		return m_Eulers[0].angle;
	};
	double Alt()
	{
		return m_Eulers[1].angle;
	};
	double Az()
	{
		return m_Eulers[2].angle;
	};
};

class Coord_IN : public Coord {
	Coord_HO(double Axis3, double Axis2, double Axis1)
	{
		m_Eulers[0] = { LA3::RotAxis::ROTAXISX, Axis3 };
		m_Eulers[1] = { LA3::RotAxis::ROTAXISY, Axis2 };
		m_Eulers[2] = { LA3::RotAxis::ROTAXISZ, Axis1 };
	};
	Coord_HO To_Coord_HO(double invmissaligment[3][3])
	{
		double frh, alt, az;
		double tmp1[3][3];
		double tmp2[3][3];
		LA3::SingleRotation rots[3] = {
			m_Eulers[0],
			m_Eulers[1],
			m_Eulers[2]
		};
		LA3::getMultipleRotationMatrix(tmp1, rots, 3);
		LA3::multiply(tmp2, tmp1, invmissaligment);
		LA3::getEulerRxRyRz(tmp2, frh, alt, az);
	}
	Coord_EQ To_Coord_EQ(double invmissaligment[3][3], double Lat)
	{
		return To_Coord_HO(invmissaligment).To_Coord_EQ(Lat);
	}
};


#endif // __TeenAstroCoord_hpp__
