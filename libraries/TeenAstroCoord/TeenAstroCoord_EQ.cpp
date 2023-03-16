#include "TeenAstroCoord_EQ.hpp"
#include "TeenAstroCoord_HO.hpp"
#include "TeenAstroCoord_IN.hpp"

Coord_EQ::Coord_EQ(double FrE, double Dec, double Ha) : Coord({ LA3::RotAxis::ROTAXISX, FrE }, { LA3::RotAxis::ROTAXISY, Dec }, { LA3::RotAxis::ROTAXISY, Dec })
{
};
Coord_HO Coord_EQ::To_Coord_HO(double Lat)
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
};
Coord_IN Coord_EQ::To_Coord_IN(double Lat, const double(&missaligment)[3][3])
{
  return To_Coord_HO(Lat).To_Coord_IN(missaligment);
};
double Coord_EQ::FrE()
{
  return m_Eulers[0].angle;
};
double Coord_EQ::Dec()
{
  return m_Eulers[1].angle;
};
double Coord_EQ::Ha()
{
  return m_Eulers[2].angle;
};