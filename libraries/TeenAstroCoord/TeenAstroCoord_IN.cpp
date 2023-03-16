#include "TeenAstroCoord_IN.hpp"
#include "TeenAstroCoord_EQ.hpp"
#include "TeenAstroCoord_HO.hpp"


Coord_IN::Coord_IN(double Axis3, double Axis2, double Axis1) : Coord({ LA3::RotAxis::ROTAXISX, Axis3 }, { LA3::RotAxis::ROTAXISY, Axis2 }, { LA3::RotAxis::ROTAXISZ, Axis1 })
{
};
Coord_HO Coord_IN::To_Coord_HO(const double(&invmissaligment)[3][3])
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
  return Coord_HO(frh, alt, az);
};
Coord_EQ Coord_IN::To_Coord_EQ(const double(&invmissaligment)[3][3], double Lat)
{
  return To_Coord_HO(invmissaligment).To_Coord_EQ(Lat);
};
double Coord_IN::Axis3()
{
  return m_Eulers[0].angle;
};
double Coord_IN::Axis2()
{
  return m_Eulers[1].angle;
};
double Coord_IN::Axis1()
{
  return m_Eulers[2].angle;
};
