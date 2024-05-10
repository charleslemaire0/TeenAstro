
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

#ifndef __TeenAstroCoord_HO_hpp__
#define __TeenAstroCoord_HO_hpp__



#include "TeenAstroCoord.hpp"

class Coord_EQ;
class Coord_IN;

class Coord_HO : protected Coord3R {
private:
	bool mIsApparent;
public:
	Coord_HO(double FrH, double Alt, double Az, bool IsApparent );
	Coord_HO ToApparent(RefrOpt opt);
	Coord_HO ToTopocentric(RefrOpt opt);
	Coord_EQ To_Coord_EQ(double Lat);
	Coord_IN To_Coord_IN(const double(&missaligment)[3][3]);
	double FrH();
	double Alt();
	double Az();
	double direct_Az_S();
};

#endif // __TeenAstroCoord_hpp__
