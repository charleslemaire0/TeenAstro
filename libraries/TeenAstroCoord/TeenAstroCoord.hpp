
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
class Coord3R : protected LA3 {
protected:
	LA3::SingleRotation m_Eulers[3];
public:
	Coord3R(LA3::SingleRotation R0, LA3::SingleRotation R1, LA3::SingleRotation R2)
	{
		m_Eulers[0] = R0;
		m_Eulers[1] = R1;
		m_Eulers[2] = R2;
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
	};
};

class Coord4R : protected LA3 {
protected:
	LA3::SingleRotation m_Eulers[4];
public:
	Coord4R(LA3::SingleRotation R0, LA3::SingleRotation R1, LA3::SingleRotation R2, LA3::SingleRotation R3)
	{
		m_Eulers[0] = R0;
		m_Eulers[1] = R1;
		m_Eulers[2] = R2;
		m_Eulers[3] = R3;
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
	};
	LA3::SingleRotation getE3()
	{
		return m_Eulers[3];
	};
};




#endif // __TeenAstroCoord_hpp__
