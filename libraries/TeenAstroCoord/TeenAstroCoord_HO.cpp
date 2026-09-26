#include "TeenAstroCoord_HO.hpp"
#include "TeenAstroCoord_IN.hpp"
#include "TeenAstroCoord_EQ.hpp"


Coord_HO::Coord_HO(double FrH, double Alt, double Az, bool IsApparent) : 
  Coord3R(
    { LA3::RotAxis::ROTAXISX, FrH },
    { LA3::RotAxis::ROTAXISY, Alt },
    { LA3::RotAxis::ROTAXISZ, modRad( - Az + M_PI)})
{
  mIsApparent = IsApparent;
};

Coord_HO Coord_HO::ToApparent(RefrOpt opt)
{
  double alt = m_Eulers[1].angle;
  if (!mIsApparent)
  {
    LA3::Topocentric2Apparent(alt, opt);
  }
  return Coord_HO(FrH(), alt, Az(), true);
}

Coord_HO Coord_HO::ToTopocentric(RefrOpt opt)
{
  double alt = m_Eulers[1].angle;
  if (mIsApparent)
  {
    LA3::Apparent2Topocentric(alt, opt);
  }
  return Coord_HO(FrH(), alt, Az(), false);
}

Coord_EQ Coord_HO::To_Coord_EQ(double Lat)
{
  double fre, dec, direct_ha;
  double tmp[3][3];
  LA3::SingleRotation rots[4] = {
    m_Eulers[0],
    m_Eulers[1],
    m_Eulers[2],
    {LA3::RotAxis::ROTAXISY, -(M_PI_2 - Lat)}
  };
  LA3::getMultipleRotationMatrix(tmp, rots, 4);
  LA3::getEulerRxRyRz(tmp, fre, dec, direct_ha);
  return Coord_EQ(fre, dec, -direct_ha);
};

Coord_IN Coord_HO::To_Coord_IN(const double(&missaligment)[3][3])
{
  double axis3, axis2, direct_axis1;
  double tmp1[3][3];
  double tmp2[3][3];
  LA3::SingleRotation rots[3] = {
    m_Eulers[0],
    m_Eulers[1],
    m_Eulers[2]
  };
  LA3::getMultipleRotationMatrix(tmp1, rots, 3);
  LA3::multiply(tmp2, tmp1, missaligment);
  LA3::getEulerRxRyRz(tmp2, axis3, axis2, direct_axis1);
  return Coord_IN(axis3, axis2, -direct_axis1);
};

Coord_IN Coord_HO::To_Coord_IN(const double(&missaligment)[3][3], const HeadModel &head)
{
  if (head.isZero())
    return To_Coord_IN(missaligment);

  double axis3, axis2, direct_axis1;
  double tmp1[3][3];
  double tmp2[3][3];
  LA3::SingleRotation rots[3] = {
    m_Eulers[0],
    m_Eulers[1],
    m_Eulers[2]
  };
  LA3::getMultipleRotationMatrix(tmp1, rots, 3);
  LA3::multiply(tmp2, tmp1, missaligment);

  // Ideal (zero head) solution first: it fixes which of the two valid mount
  // configurations we stay on, so pier side and beyond the pole behaviour match
  // the overload without a head model.
  LA3::getEulerRxRyRz(tmp2, axis3, axis2, direct_axis1);

  // The pointing direction is the first row of the instrument frame matrix.
  const double d[3] = { tmp2[0][0], tmp2[0][1], tmp2[0][2] };
  double headAxis1Direct, headAxis2;
  if (!HeadGeom::inverse(d, head, axis2, headAxis1Direct, headAxis2))
    return Coord_IN(axis3, axis2, -direct_axis1);  // unreachable with this head, keep the ideal solution

  axis3 = HeadGeom::fieldRotation(tmp2, headAxis1Direct, headAxis2, head);
  return Coord_IN(axis3, headAxis2, -headAxis1Direct);
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
  return modRad(-m_Eulers[2].angle + M_PI);
};

double Coord_HO::direct_Az_S()
{
  return m_Eulers[2].angle;
};