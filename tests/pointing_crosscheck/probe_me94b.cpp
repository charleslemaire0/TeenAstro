#include <cmath>
#include <cstdio>
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"

static void mul(double o[3], const double M[3][3], const double v[3])
{
  for (int i = 0; i < 3; i++)
    o[i] = M[i][0] * v[0] + M[i][1] * v[1] + M[i][2] * v[2];
}

static void seedDirect(CoordConv &cc, double lat)
{
  Coord_HO HO1(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
  Coord_EQ EQ1 = HO1.To_Coord_EQ(lat);
  Coord_IN IN1(0, EQ1.Dec(), EQ1.Ha() - M_PI_2);
  Coord_HO HO2(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
  Coord_EQ EQ2 = HO2.To_Coord_EQ(lat);
  Coord_IN IN2(0, EQ2.Dec(), EQ2.Ha() - M_PI_2);
  cc.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), IN1.Axis2());
  cc.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), IN2.Axis2());
}

int main()
{
  const double lat = 47.0 * DEG_TO_RAD;
  LA3::RefrOpt off = { false, 0.0, 0.0 };
  double pole[3] = { cos(lat), 0.0, sin(lat) };
  double npole[3] = { -pole[0], -pole[1], -pole[2] };

  CoordConv cc;
  seedDirect(cc, lat);

  double Tp[3], Tmp[3];
  mul(Tp, cc.T, pole);
  mul(Tmp, cc.T, npole);
  std::printf("T*pole    = %.4f %.4f %.4f\n", Tp[0], Tp[1], Tp[2]);
  std::printf("T*(-pole) = %.4f %.4f %.4f\n", Tmp[0], Tmp[1], Tmp[2]);

  double dcHome[3];
  LA3::toDirCos(dcHome, 0.0, M_PI_2);
  std::printf("toDirCos(0,90)= %.4f %.4f %.4f\n", dcHome[0], dcHome[1], dcHome[2]);

  Coord_HO homeHO = Coord_IN(0, M_PI_2, 0).To_Coord_HO(cc.T, off);
  double dcH[3];
  LA3::toDirCos(dcH, homeHO.direct_Az_S(), homeHO.Alt());
  std::printf("home sky DC %.4f %.4f %.4f  dot(+pole)=%.4f  Dec=%.4f\n",
              dcH[0], dcH[1], dcH[2],
              dcH[0] * pole[0] + dcH[1] * pole[1] + dcH[2] * pole[2],
              Coord_IN(0, M_PI_2, 0).To_Coord_EQ(cc.T, off, lat).Dec() * RAD_TO_DEG);

  // Seed with Axis1() (negated HA angle) for comparison
  CoordConv ccA;
  {
    Coord_HO HO1(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
    Coord_EQ EQ1 = HO1.To_Coord_EQ(lat);
    Coord_IN IN1(0, EQ1.Dec(), EQ1.Ha() - M_PI_2);
    Coord_HO HO2(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
    Coord_EQ EQ2 = HO2.To_Coord_EQ(lat);
    Coord_IN IN2(0, EQ2.Dec(), EQ2.Ha() - M_PI_2);
    ccA.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1(), IN1.Axis2());
    ccA.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1(), IN2.Axis2());
  }
  std::printf("Axis1 seed ME %.4f homeDec %.4f Tinv2 %.4f %.4f %.4f\n",
              -ccA.polErrorDeg(lat, PE_EQ_ALT),
              Coord_IN(0, M_PI_2, 0).To_Coord_EQ(ccA.T, off, lat).Dec() * RAD_TO_DEG,
              ccA.Tinv[0][2], ccA.Tinv[1][2], ccA.Tinv[2][2]);

  // Fix attempt: after Axis1_direct seed, also run GEM close-out minimisers
  CoordConv m = cc;
  m.minimizeAxis2();
  m.minimizeAxis1(M_PI_2);
  std::printf("direct+minimize ME %.4f W %.4f homeDec %.4f\n",
              -m.polErrorDeg(lat, PE_EQ_ALT), m.polErrorDeg(lat, PE_POL_W),
              Coord_IN(0, M_PI_2, 0).To_Coord_EQ(m.T, off, lat).Dec() * RAD_TO_DEG);

  // Fix attempt: seed Axis1_direct but pass -Axis2 (flip pier)
  CoordConv flip2;
  {
    Coord_HO HO1(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
    Coord_EQ EQ1 = HO1.To_Coord_EQ(lat);
    Coord_IN IN1(0, EQ1.Dec(), EQ1.Ha() - M_PI_2);
    Coord_HO HO2(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
    Coord_EQ EQ2 = HO2.To_Coord_EQ(lat);
    Coord_IN IN2(0, EQ2.Dec(), EQ2.Ha() - M_PI_2);
    flip2.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), -IN1.Axis2());
    flip2.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), -IN2.Axis2());
  }
  std::printf("neg Axis2 seed ME %.4f homeDec %.4f\n",
              -flip2.polErrorDeg(lat, PE_EQ_ALT),
              Coord_IN(0, M_PI_2, 0).To_Coord_EQ(flip2.T, off, lat).Dec() * RAD_TO_DEG);

  // Fix attempt: use Axis1_direct in addReference but negate it (store Axis1())
  // already tested. Try: keep Axis1_direct seed, then reflect Tinv col2 by
  // rebuilding with setPoleError from a corrected measurement.
  // Measured instrument pole sky = home DC (correct +pole). Force that into T.
  CoordConv forced;
  forced.setPoleError(lat, 0.0, 0.0, 0.0);
  // setPoleError gives homeDec=4. Need index = something for GEM.
  // Try index = pi/2 - lat? or use minimizeAxis1 after setPoleError
  forced.minimizeAxis1(M_PI_2);
  std::printf("setPole0+minA1(90) ME %.4f homeDec %.4f\n",
              -forced.polErrorDeg(lat, PE_EQ_ALT),
              Coord_IN(0, M_PI_2, 0).To_Coord_EQ(forced.T, off, lat).Dec() * RAD_TO_DEG);

  // Does setPoleError leave refs for minimizeAxis1?
  std::printf("forced ready %d refs %d\n", (int)forced.isReady(), (int)forced.getRefs());

  return 0;
}
