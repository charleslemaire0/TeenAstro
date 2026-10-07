#include <cmath>
#include <cstdio>
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"

// Candidate polErrorDeg: sky direction of Coord_IN polar home through T.
static void mountPoleDir(const CoordConv &cc, double latRad, double x[3])
{
  const double axis2 = latRad >= 0.0 ? M_PI_2 : -M_PI_2;
  LA3::RefrOpt off = { false, 0.0, 0.0 };
  Coord_HO ho = Coord_IN(0.0, axis2, 0.0).To_Coord_HO(cc.T, off);
  // Same DC frame as addReference sky side: direct_Az_S, Alt.
  LA3::toDirCos(x, ho.direct_Az_S(), ho.Alt());
  LA3::normalize(x, x);
}

static double peAlt(const double x[3], double latRad)
{
  // Match existing formula but with atan2 for correct quadrant.
  if (fabs(x[0]) < 1e-15 && fabs(x[1]) < 1e-15)
    return (x[2] > 0.0 ? 90.0 - latRad * RAD_TO_DEG : -90.0 - latRad * RAD_TO_DEG);
  return (atan2(x[2], x[0]) - latRad) * RAD_TO_DEG;
}

static double peAz(const double x[3])
{
  if (fabs(x[0]) < 1e-15 && fabs(x[1]) < 1e-15)
    return 0.0;
  return atan2(x[1], x[0]) * RAD_TO_DEG;
}

static double peW(const double x[3], double latRad)
{
  double id[3] = { cos(latRad), 0.0, sin(latRad) };
  double c = x[0] * id[0] + x[1] * id[1] + x[2] * id[2];
  if (c > 1) c = 1;
  if (c < -1) c = -1;
  return acos(c) * RAD_TO_DEG;
}

static void report(const char *tag, CoordConv &cc, double lat)
{
  LA3::RefrOpt off = { false, 0.0, 0.0 };
  double x[3];
  mountPoleDir(cc, lat, x);
  double dec = Coord_IN(0, M_PI_2, 0).To_Coord_EQ(cc.T, off, lat).Dec() * RAD_TO_DEG;
  std::printf("%s  oldME %.3f oldW %.3f  newME %.3f newMA %.3f newW %.3f  homeDec %.3f  ready %d\n",
              tag,
              -cc.polErrorDeg(lat, PE_EQ_ALT), cc.polErrorDeg(lat, PE_POL_W),
              -peAlt(x, lat), peAz(x) * cos(lat), peW(x, lat),
              dec, (int)cc.isReady());
}

int main()
{
  const double lat = 47.0 * DEG_TO_RAD;

  CoordConv direct;
  {
    Coord_HO HO1(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
    Coord_EQ EQ1 = HO1.To_Coord_EQ(lat);
    Coord_IN IN1(0, EQ1.Dec(), EQ1.Ha() - M_PI_2);
    Coord_HO HO2(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
    Coord_EQ EQ2 = HO2.To_Coord_EQ(lat);
    Coord_IN IN2(0, EQ2.Dec(), EQ2.Ha() - M_PI_2);
    direct.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), IN1.Axis2());
    direct.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), IN2.Axis2());
  }
  report("Axis1d seed", direct, lat);

  CoordConv axis1;
  {
    Coord_HO HO1(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
    Coord_EQ EQ1 = HO1.To_Coord_EQ(lat);
    Coord_IN IN1(0, EQ1.Dec(), EQ1.Ha() - M_PI_2);
    Coord_HO HO2(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
    Coord_EQ EQ2 = HO2.To_Coord_EQ(lat);
    Coord_IN IN2(0, EQ2.Dec(), EQ2.Ha() - M_PI_2);
    axis1.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1(), IN1.Axis2());
    axis1.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1(), IN2.Axis2());
  }
  report("Axis1  seed", axis1, lat);

  CoordConv pole0;
  pole0.setPoleError(lat, 0, 0, 0);
  report("setPole0   ", pole0, lat);

  // 10° ME via setPoleError (dAlt = -ME in internal PE_EQ_ALT sign)
  CoordConv pole10;
  pole10.setPoleError(lat, 0.0, -10.0 * DEG_TO_RAD, 0.0);
  report("setPole ME10", pole10, lat);

  // Two-star with small Wallace-like offset on second star using Axis1_direct path
  // Star1: sync-like exact; star2: +3' ME equivalent via setPoleError dials — skip,
  // instead feed two ideal stars with Axis1_direct and confirm newME~0.
  CoordConv two;
  {
    LA3::RefrOpt off = { false, 0.0, 0.0 };
    // Use direct seed T to generate instrument axes for two skies, then refit
    double az1 = 90 * DEG_TO_RAD, alt1 = 45 * DEG_TO_RAD;
    double az2 = 270 * DEG_TO_RAD, alt2 = 45 * DEG_TO_RAD;
    double a1, a2;
    // inverse: sky -> instrument via T from direct seed
    double dc[3], di[3];
    LA3::toDirCos(dc, az1, alt1);
    // T * sky = instrument DC in toDirCos space
    for (int i = 0; i < 3; i++)
      di[i] = direct.T[i][0] * dc[0] + direct.T[i][1] * dc[1] + direct.T[i][2] * dc[2];
    a2 = asin(di[2] > 1 ? 1 : di[2] < -1 ? -1 : di[2]);
    a1 = atan2(-di[1], di[0]); // rough inverse of toDirCos
    two.addReference(az1, alt1, a1, a2);
    LA3::toDirCos(dc, az2, alt2);
    for (int i = 0; i < 3; i++)
      di[i] = direct.T[i][0] * dc[0] + direct.T[i][1] * dc[1] + direct.T[i][2] * dc[2];
    a2 = asin(di[2] > 1 ? 1 : di[2] < -1 ? -1 : di[2]);
    a1 = atan2(-di[1], di[0]);
    two.addReference(az2, alt2, a1, a2);
  }
  report("refit ideal", two, lat);

  return 0;
}
