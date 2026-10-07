#include <cmath>
#include <cstdio>
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"

static void homeAltAz(const CoordConv& cc, double lat, double& alt, double& az)
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

static void addGem(CoordConv& cc, double Lat, double dAz, double dAlt, double azDeg, double altDeg, bool useDirect)
{
  Coord_HO HO_true(0, altDeg * DEG_TO_RAD, azDeg * DEG_TO_RAD, false);
  Coord_HO HO_mech(0, altDeg * DEG_TO_RAD, azDeg * DEG_TO_RAD + dAz, false);
  Coord_EQ EQ = HO_mech.To_Coord_EQ(Lat + dAlt);
  Coord_IN IN(0, EQ.Dec(), EQ.Ha() - M_PI_2);
  double a1 = useDirect ? IN.Axis1_direct() : IN.Axis1();
  cc.addReference(HO_true.direct_Az_S(), HO_true.Alt(), a1, IN.Axis2());
}

int main() {
  const double Lat = 47.22 * DEG_TO_RAD;
  for (int direct = 0; direct < 2; direct++) {
    CoordConv cc;
    addGem(cc, Lat, 5*DEG_TO_RAD, 5*DEG_TO_RAD, 90, 45, direct!=0);
    addGem(cc, Lat, 5*DEG_TO_RAD, 5*DEG_TO_RAD, 270, 45, direct!=0);
    cc.minimizeAxis2();
    cc.minimizeAxis1(M_PI_2);
    double alt, az;
    homeAltAz(cc, Lat, alt, az);
    std::printf("%s: oldAz %.3f oldAlt %.3f oldW %.3f | newAz %.3f newAlt-err %.3f newW(from altaz) %.3f | Dec %.3f\n",
      direct ? "Axis1d" : "Axis1 ",
      cc.polErrorDeg(Lat, PE_EQ_AZ), cc.polErrorDeg(Lat, PE_EQ_ALT), cc.polErrorDeg(Lat, PE_POL_W),
      az*RAD_TO_DEG, (alt-Lat)*RAD_TO_DEG,
      acos(sin(alt)*sin(Lat)+cos(alt)*cos(Lat)*cos(az))*RAD_TO_DEG,
      Coord_IN(0,M_PI_2,0).To_Coord_EQ(cc.T,{false,0,0},Lat).Dec()*RAD_TO_DEG);
  }
  return 0;
}
