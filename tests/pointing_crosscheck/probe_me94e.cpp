#include <cmath>
#include <cstdio>
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"

static void homeHO(const CoordConv& cc, double lat, double& alt, double& az)
{
  const double axis2 = lat >= 0 ? M_PI_2 : -M_PI_2;
  double R[3][3], M[3][3];
  LA3::SingleRotation rots[3] = {
    {LA3::RotAxis::ROTAXISX, 0}, {LA3::RotAxis::ROTAXISY, axis2}, {LA3::RotAxis::ROTAXISZ, 0}
  };
  LA3::getMultipleRotationMatrix(R, rots, 3);
  LA3::multiply(M, R, cc.T);
  double frh, azs;
  LA3::getEulerRxRyRz(M, frh, alt, azs);
  az = -azs - M_PI;
  while (az > M_PI) az -= 2*M_PI;
  while (az < -M_PI) az += 2*M_PI;
}

int main() {
  const double lat = 47*DEG_TO_RAD;
  LA3::RefrOpt off={false,0,0};
  struct C { const char* n; CoordConv cc; };
  // direct seed
  CoordConv d;
  {
    Coord_HO HO1(0, 45*DEG_TO_RAD, 90*DEG_TO_RAD, false);
    Coord_EQ EQ1 = HO1.To_Coord_EQ(lat);
    Coord_IN IN1(0, EQ1.Dec(), EQ1.Ha() - M_PI_2);
    Coord_HO HO2(0, 45*DEG_TO_RAD, 270*DEG_TO_RAD, false);
    Coord_EQ EQ2 = HO2.To_Coord_EQ(lat);
    Coord_IN IN2(0, EQ2.Dec(), EQ2.Ha() - M_PI_2);
    d.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), IN1.Axis2());
    d.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), IN2.Axis2());
  }
  double alt, az;
  homeHO(d, lat, alt, az);
  std::printf("direct: alt %.4f az %.4f ME_new %.4f MA_new %.4f Dec %.4f oldME %.4f\n",
    alt*RAD_TO_DEG, az*RAD_TO_DEG, -(alt-lat)*RAD_TO_DEG, az*RAD_TO_DEG*cos(lat),
    Coord_IN(0,M_PI_2,0).To_Coord_EQ(d.T,off,lat).Dec()*RAD_TO_DEG,
    -d.polErrorDeg(lat, PE_EQ_ALT));

  CoordConv p0; p0.setPoleError(lat, 0, 0, 0);
  homeHO(p0, lat, alt, az);
  std::printf("pole0: alt %.4f az %.4f ME_new %.4f Dec %.4f oldME %.4f\n",
    alt*RAD_TO_DEG, az*RAD_TO_DEG, -(alt-lat)*RAD_TO_DEG,
    Coord_IN(0,M_PI_2,0).To_Coord_EQ(p0.T,off,lat).Dec()*RAD_TO_DEG,
    -p0.polErrorDeg(lat, PE_EQ_ALT));

  CoordConv p10; p10.setPoleError(lat, 2*DEG_TO_RAD, -3*DEG_TO_RAD, 0);
  homeHO(p10, lat, alt, az);
  std::printf("pole dAz=2 dAlt=-3(ME=+3): alt %.4f az %.4f ME_new %.4f MA_new %.4f oldME %.4f oldMA %.4f\n",
    alt*RAD_TO_DEG, az*RAD_TO_DEG, -(alt-lat)*RAD_TO_DEG, az*RAD_TO_DEG*cos(lat),
    -p10.polErrorDeg(lat, PE_EQ_ALT), p10.polErrorDeg(lat, PE_EQ_AZ)*cos(lat));

  // Compare Coord_HO path
  Coord_HO h = Coord_IN(0,M_PI_2,0).To_Coord_HO(d.T, off);
  std::printf("Coord_HO direct: Alt %.4f Az %.4f AzS %.4f\n",
    h.Alt()*RAD_TO_DEG, h.Az()*RAD_TO_DEG, h.direct_Az_S()*RAD_TO_DEG);
  return 0;
}
