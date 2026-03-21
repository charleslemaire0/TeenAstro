/*
 * CoordDemo - Integrated example for TeenAstro coordinate libraries
 *
 * Libraries: TeenAstroLA3, TeenAstroCoord, TeenAstroCoordConv
 *
 * Demonstrates:
 *   1. EQ <-> HO conversion (with and without refraction)
 *   2. Two-star alignment and the full EQ -> HO -> IN -> HO -> EQ chain
 *   3. EQ <-> LO conversion through a transformation matrix
 *
 * All angles are in radians unless noted otherwise.
 *
 * Build: pio run -d libraries/TeenAstroCoord/examples/CoordDemo
 */

#include <cstdio>
#include <cmath>

// Library implementations (single-translation-unit build)
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"

static const double DEG = M_PI / 180.0;

static void printDeg(const char* label, double rad) {
  printf("  %-12s %+9.4f deg\n", label, rad / DEG);
}

static void printError(const char* label, double expected, double actual) {
  double err_arcsec = fabs(actual - expected) / DEG * 3600.0;
  printf("  %-12s error: %.2f arcsec\n", label, err_arcsec);
}

// -----------------------------------------------------------------------
//  1. Equatorial <-> Horizontal (no refraction)
// -----------------------------------------------------------------------
static void demoEqHo() {
  printf("=== 1. EQ <-> HO (no refraction) ===\n");

  double lat = 48.8 * DEG;   // Paris
  double ha  = 2.0 * 15 * DEG; // HA = 2 h
  double dec = 30.0 * DEG;
  LA3::RefrOpt noRefr = {false, 10.0, 1013.0};

  Coord_EQ eq(0.0, dec, ha);
  Coord_HO ho = eq.To_Coord_HO(lat, noRefr);

  printf("  EQ -> HO:\n");
  printDeg("Alt", ho.Alt());
  printDeg("Az",  ho.Az());

  Coord_EQ eq2 = ho.To_Coord_EQ(lat);
  printf("  HO -> EQ (round-trip):\n");
  printError("Dec", dec, eq2.Dec());
  printError("HA",  ha,  eq2.Ha());
  printf("\n");
}

// -----------------------------------------------------------------------
//  2. Equatorial <-> Horizontal (with refraction)
// -----------------------------------------------------------------------
static void demoRefraction() {
  printf("=== 2. EQ <-> HO (with refraction) ===\n");

  double lat = 48.8 * DEG;
  double ha  = 1.0 * 15 * DEG;
  double dec = 45.0 * DEG;
  LA3::RefrOpt refr = {true, 10.0, 1010.0};
  LA3::RefrOpt noRefr = {false, 10.0, 1013.0};

  Coord_EQ eq(0.0, dec, ha);

  Coord_HO topo = eq.To_Coord_HO(lat, noRefr);
  Coord_HO app  = eq.To_Coord_HO(lat, refr);

  printf("  Topocentric vs Apparent altitude:\n");
  printDeg("Topo Alt", topo.Alt());
  printDeg("App  Alt", app.Alt());

  double diff_arcsec = (app.Alt() - topo.Alt()) / DEG * 3600.0;
  printf("  Refraction:  %.1f arcsec\n", diff_arcsec);

  Coord_HO back = app.ToTopocentric(refr);
  Coord_EQ eq2  = back.To_Coord_EQ(lat);
  printf("  Round-trip through refraction:\n");
  printError("Dec", dec, eq2.Dec());
  printError("HA",  ha,  eq2.Ha());
  printf("\n");
}

// -----------------------------------------------------------------------
//  3. Two-star alignment + full chain EQ -> HO -> IN -> HO -> EQ
// -----------------------------------------------------------------------
static void demoAlignment() {
  printf("=== 3. Two-star alignment (CoordConv) ===\n");

  double lat = 45.0 * DEG;
  LA3::RefrOpt noRefr = {false, 10.0, 1013.0};

  double misRot = 2.0 * DEG;
  double Rz[3][3];
  LA3::SingleRotation sr = {LA3::ROTAXISZ, misRot};
  LA3::getSingleRotationMatrix(Rz, sr);

  struct { double ha; double dec; } stars[2] = {
    { 0.5, 0.3 },
    { 2.0, -0.2 }
  };

  CoordConv conv;
  for (int i = 0; i < 2; i++) {
    Coord_EQ eq(0.0, stars[i].dec, stars[i].ha);
    Coord_HO ho = eq.To_Coord_HO(lat, noRefr);
    double az  = ho.Az();
    double alt = ho.Alt();

    double dc[3], dcr[3];
    LA3::toDirCos(dc, az, alt);
    LA3::multiply(dcr, Rz, dc);
    double instAz  = -atan2(dcr[1], dcr[0]);
    double instAlt =  asin(dcr[2]);

    conv.addReference(az, alt, instAz, instAlt);
  }

  printf("  Alignment ready: %s\n", conv.isReady() ? "yes" : "no");
  printf("  Alignment error: %.2f arcsec\n",
         fabs(conv.getError()) / DEG * 3600.0);

  double ha = 1.0, dec = 0.5;
  Coord_EQ eq1(0.0, dec, ha);
  Coord_HO ho1 = eq1.To_Coord_HO(lat, noRefr);
  Coord_IN in  = ho1.To_Coord_IN(conv.T);
  Coord_HO ho2 = in.To_Coord_HO(conv.Tinv, noRefr);
  Coord_EQ eq2 = ho2.To_Coord_EQ(lat);

  printf("  Full chain round-trip (EQ->HO->IN->HO->EQ):\n");
  printError("Dec", dec, eq2.Dec());
  printError("HA",  ha,  eq2.Ha());
  printf("\n");
}

// -----------------------------------------------------------------------
//  4. EQ <-> LO (logical axes)
// -----------------------------------------------------------------------
static void demoEqLo() {
  printf("=== 4. EQ <-> LO ===\n");

  double ha = 1.2, dec = 0.3;
  Coord_EQ eq1(0.0, dec, ha);

  double I[3][3];
  LA3::getIdentityMatrix(I);

  Coord_LO lo = eq1.To_Coord_LO(I);
  printf("  LO axes (identity transform):\n");
  printDeg("Axis1", lo.Axis1());
  printDeg("Axis2", lo.Axis2());
  printDeg("Axis3", lo.Axis3());

  Coord_EQ eq2 = lo.To_Coord_EQ(I);
  printf("  LO -> EQ round-trip:\n");
  printError("Dec", dec, eq2.Dec());
  printError("HA",  ha,  eq2.Ha());
  printf("\n");
}

// -----------------------------------------------------------------------
int main() {
  printf("\nTeenAstro Coordinate Libraries - Integrated Demo\n");
  printf("================================================\n\n");

  demoEqHo();
  demoRefraction();
  demoAlignment();
  demoEqLo();

  printf("Done.\n");
  return 0;
}
