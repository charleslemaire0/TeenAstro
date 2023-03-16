#include "TeenAstroCoord_HO.hpp"
#include "TeenAstroCoord_IN.hpp"
#include "TeenAstroCoord_EQ.hpp"


Coord_HO::Coord_HO(double FrH, double Alt, double Az) : Coord({ LA3::RotAxis::ROTAXISX, FrH }, { LA3::RotAxis::ROTAXISY, Alt }, { LA3::RotAxis::ROTAXISZ, Az })
{
};
Coord_EQ Coord_HO::To_Coord_EQ(double Lat)
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
};
Coord_IN Coord_HO::To_Coord_IN(const double(&missaligment)[3][3])
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
  return Coord_IN(axis3, axis2, axis1);
};
double Coord_HO::FrH()
{
  return m_Eulers[0].angle;
};
double Coord_HO::Alt()
{
  return m_Eulers[1].angle;
};
double Coord_HO::Az()
{
  return m_Eulers[2].angle;
};
