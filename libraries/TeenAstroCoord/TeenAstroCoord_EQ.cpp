#include "TeenAstroCoord_EQ.hpp"
#include "TeenAstroCoord_HO.hpp"
#include "TeenAstroCoord_IN.hpp"
#include "TeenAstroCoord_LO.hpp"

Coord_EQ::Coord_EQ(double FrE, double Dec, double Ha) : 
  Coord3R(
    { LA3::RotAxis::ROTAXISX, FrE },
    { LA3::RotAxis::ROTAXISY, Dec }, 
    { LA3::RotAxis::ROTAXISZ, -Ha })
{
};
Coord_LO Coord_EQ::To_Coord_LO(const double(&invtrafo)[3][3])
{
  double Axis1, Axis2, Axis3;
  double tmp1[3][3];
  double tmp2[3][3];
  LA3::SingleRotation rots[3] = {
    m_Eulers[0],
    m_Eulers[1],
    m_Eulers[2]
  };
  LA3::getMultipleRotationMatrix(tmp1, rots, 3);
  LA3::multiply(tmp2, tmp1, invtrafo);
  LA3::getEulerRxRyRz(tmp2, Axis3, Axis2, Axis1);
  return Coord_LO(Axis3, Axis2, Axis1);
};
Coord_HO Coord_EQ::To_Coord_HO(double Lat, RefrOpt Opt)
{
  double frh, alt, az_s_direct;
  double tmp[3][3];
  LA3::SingleRotation rots[4] = {
    m_Eulers[0],
    m_Eulers[1],
    m_Eulers[2],
    {LA3::RotAxis::ROTAXISY, (M_PI_2 - Lat) },
  };
  LA3::getMultipleRotationMatrix(tmp, rots, 4);
  LA3::getEulerRxRyRz(tmp, frh, alt, az_s_direct);
  if (Opt.use)
  {
    LA3::Topocentric2Apparent(alt, Opt);
  }
  return Coord_HO(frh, alt, modRad( - az_s_direct + M_PI), Opt.use);
};
Coord_IN Coord_EQ::To_Coord_IN(double Lat, RefrOpt Opt, const double(&missaligment)[3][3])
{
  return To_Coord_HO(Lat, Opt).To_Coord_IN(missaligment);
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
  return -m_Eulers[2].angle;
};
double Coord_EQ::direct_Ha()
{
  return m_Eulers[2].angle;
};
double Coord_EQ::Ra(double LST)
{
  double Ra = modRad(LST - Ha());
  if (Ra < 0) Ra += 2 * M_PI;
  return Ra;
};