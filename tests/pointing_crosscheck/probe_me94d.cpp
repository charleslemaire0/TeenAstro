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
  CoordConv cc;
  Coord_HO HO1(0, 45*DEG_TO_RAD, 90*DEG_TO_RAD, false);
  Coord_EQ EQ1 = HO1.To_Coord_EQ(lat);
  Coord_IN IN1(0, EQ1.Dec(), EQ1.Ha() - M_PI_2);
  Coord_HO HO2(0, 45*DEG_TO_RAD, 270*DEG_TO_RAD, false);
  Coord_EQ EQ2 = HO2.To_Coord_EQ(lat);
  Coord_IN IN2(0, EQ2.Dec(), EQ2.Ha() - M_PI_2);
  cc.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), IN1.Axis2());
  cc.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), IN2.Axis2());
  LA3::RefrOpt off={false,0,0};
  Coord_IN home(0, M_PI_2, 0);
  Coord_EQ eq = home.To_Coord_EQ(cc.T, off, lat);
  Coord_HO ho = home.To_Coord_HO(cc.T, off);
  std::printf("EQ Dec %.6f Ha %.6f\n", eq.Dec()*RAD_TO_DEG, eq.Ha()*RAD_TO_DEG);
  std::printf("HO Alt %.6f AzS %.6f Az %.6f\n", ho.Alt()*RAD_TO_DEG, ho.direct_Az_S()*RAD_TO_DEG, ho.Az()*RAD_TO_DEG);
  // Build R*T and print third column / row0
  double R[3][3], M[3][3];
  LA3::SingleRotation rots[3]={{LA3::RotAxis::ROTAXISX,0},{LA3::RotAxis::ROTAXISY,M_PI_2},{LA3::RotAxis::ROTAXISZ,0}};
  LA3::getMultipleRotationMatrix(R, rots, 3);
  LA3::multiply(M, R, cc.T);
  std::printf("M col2 (R*T*e_z)= %.4f %.4f %.4f\n", M[0][2], M[1][2], M[2][2]);
  std::printf("M row0 = %.4f %.4f %.4f\n", M[0][0], M[0][1], M[0][2]);
  // Where does instrument -Y go? (polar axis hypothesis)
  double eym[3]={0,-1,0}; double out[3];
  for(int i=0;i<3;i++) out[i]=cc.Tinv[i][0]*0+cc.Tinv[i][1]*(-1)+cc.Tinv[i][2]*0;
  std::printf("Tinv*(-ey)= %.4f %.4f %.4f\n", out[0], out[1], out[2]);
  double pole[3]={cos(lat),0,sin(lat)};
  std::printf("dot pole %.4f\n", out[0]*pole[0]+out[2]*pole[2]);
  // instrument +Y
  for(int i=0;i<3;i++) out[i]=cc.Tinv[i][1];
  std::printf("Tinv*(+ey)= %.4f %.4f %.4f dot %.4f\n", out[0], out[1], out[2], out[0]*pole[0]+out[2]*pole[2]);
  return 0;
}
