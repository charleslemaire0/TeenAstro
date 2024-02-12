#include "TeenAstroCoord_EQ.hpp"
#include "TeenAstroCoord_LO.hpp"


Coord_LO::Coord_LO(double Axis3, double Axis2, double Axis1) : Coord3R({ LA3::RotAxis::ROTAXISX, Axis3 }, { LA3::RotAxis::ROTAXISY, Axis2 }, { LA3::RotAxis::ROTAXISZ, Axis1 })
{
};
Coord_EQ Coord_LO::To_Coord_EQ(const double(&trafoinv)[3][3])
{
  double fre, ha, dec;
  double tmp1[3][3];
  double tmp2[3][3];
  LA3::SingleRotation rots[3] = {
    m_Eulers[0],
    m_Eulers[1],
    m_Eulers[2]
  };
  LA3::getMultipleRotationMatrix(tmp1, rots, 3);
  LA3::multiply(tmp2, tmp1, trafoinv);
  LA3::getEulerRxRyRz(tmp2, fre, dec, ha);
  return Coord_EQ(fre, dec, ha );
};
double Coord_LO::Axis3()
{
  return m_Eulers[0].angle;
};
double Coord_LO::Axis2()
{
  return m_Eulers[1].angle;
};
double Coord_LO::Axis1()
{
  return m_Eulers[2].angle;
};
