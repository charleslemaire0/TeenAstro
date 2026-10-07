#include <cmath>
#include <cstdio>
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"
int main() {
  const double lat = 47.0 * DEG_TO_RAD;
  const double sign = 1.0;
  CoordConv cc;
  Coord_HO HO1(0, 45*DEG_TO_RAD, 90*DEG_TO_RAD, false);
  Coord_EQ EQ1 = HO1.To_Coord_EQ(lat);
  Coord_IN IN1(0, sign*EQ1.Dec(), sign*EQ1.Ha() - M_PI_2);
  Coord_HO HO2(0, 45*DEG_TO_RAD, 270*DEG_TO_RAD, false);
  Coord_EQ EQ2 = HO2.To_Coord_EQ(lat);
  Coord_IN IN2(0, sign*EQ2.Dec(), sign*EQ2.Ha() - M_PI_2);
  std::printf("IN1 Axis1=%.4f Axis1d=%.4f Axis2=%.4f deg\n",
    IN1.Axis1()*RAD_TO_DEG, IN1.Axis1_direct()*RAD_TO_DEG, IN1.Axis2()*RAD_TO_DEG);
  std::printf("IN2 Axis1=%.4f Axis1d=%.4f Axis2=%.4f deg\n",
    IN2.Axis1()*RAD_TO_DEG, IN2.Axis1_direct()*RAD_TO_DEG, IN2.Axis2()*RAD_TO_DEG);
  cc.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), IN1.Axis2());
  cc.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), IN2.Axis2());
  std::printf("ready %d\n", (int)cc.isReady());
  std::printf("PE_EQ_ALT %.4f  PE_EQ_AZ %.4f  W %.4f\n",
    cc.polErrorDeg(lat, PE_EQ_ALT), cc.polErrorDeg(lat, PE_EQ_AZ), cc.polErrorDeg(lat, PE_POL_W));
  std::printf("Wallace ME %.4f  MA %.4f\n",
    -cc.polErrorDeg(lat, PE_EQ_ALT), cc.polErrorDeg(lat, PE_EQ_AZ)*cos(lat));
  // Tinv col2
  std::printf("Tinv[:,2] = %.4f %.4f %.4f\n", cc.Tinv[0][2], cc.Tinv[1][2], cc.Tinv[2][2]);
  double ideal[3] = { cos(lat), 0, sin(lat) };
  std::printf("ideal pole = %.4f %.4f %.4f\n", ideal[0], ideal[1], ideal[2]);
  // home pose sky
  LA3::RefrOpt off = {false,0,0};
  Coord_EQ atHome = Coord_IN(0, M_PI_2, 0).To_Coord_EQ(cc.T, off, lat);
  std::printf("home->EQ Dec=%.4f Ha=%.4f deg\n", atHome.Dec()*RAD_TO_DEG, atHome.Ha()*RAD_TO_DEG);
  // what if seed with Axis1() instead
  CoordConv cc2;
  cc2.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1(), IN1.Axis2());
  cc2.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1(), IN2.Axis2());
  std::printf("Axis1 seed: ME %.4f MA %.4f W %.4f homeDec %.4f\n",
    -cc2.polErrorDeg(lat, PE_EQ_ALT), cc2.polErrorDeg(lat, PE_EQ_AZ)*cos(lat),
    cc2.polErrorDeg(lat, PE_POL_W),
    Coord_IN(0,M_PI_2,0).To_Coord_EQ(cc2.T, off, lat).Dec()*RAD_TO_DEG);
  // setPoleError(0,0,0)
  CoordConv cc3;
  cc3.setPoleError(lat, 0, 0, 0);
  std::printf("setPoleError0: ME %.4f MA %.4f W %.4f ready %d\n",
    -cc3.polErrorDeg(lat, PE_EQ_ALT), cc3.polErrorDeg(lat, PE_EQ_AZ)*cos(lat),
    cc3.polErrorDeg(lat, PE_POL_W), (int)cc3.isReady());
  Coord_EQ h3 = Coord_IN(0, M_PI_2, 0).To_Coord_EQ(cc3.T, off, lat);
  std::printf("setPoleError0 home Dec %.4f Ha %.4f\n", h3.Dec()*RAD_TO_DEG, h3.Ha()*RAD_TO_DEG);
  return 0;
}
