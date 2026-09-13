/*
 * test_polar_align_math.cpp — polErrorDeg / polar misclosure helpers
 *
 * Also covers a GEM two-star soft alignment when the polar axis is offset
 * by a known amount in azimuth and altitude (mechanical polar-align prelude).
 */

#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"

#include <unity.h>
#include <cmath>

void setUp(void) {}
void tearDown(void) {}

static const double TOL = 1e-9;
static const double TOL_ALIGN = 0.15;   // degrees — SVD / two-star reconstruction
static const double TOL_POL_W = 0.25;   // degrees — total wedge angle

void test_pol_error_zero_when_not_ready(void) {
    CoordConv cc;
    TEST_ASSERT_FALSE(cc.isReady());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, cc.polErrorDeg(M_PI / 4.0, PE_EQ_AZ));
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, cc.polErrorDeg(M_PI / 4.0, PE_EQ_ALT));
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, cc.polErrorDeg(M_PI / 4.0, PE_POL_W));
}

void test_pol_error_finite_identity_matrix(void) {
    CoordConv cc;
    cc.setT(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);
    cc.setTinvFromT();
    TEST_ASSERT_TRUE(cc.isReady());
    const double lat = 0.7;
    double az = cc.polErrorDeg(lat, PE_EQ_AZ);
    double alt = cc.polErrorDeg(lat, PE_EQ_ALT);
    double w = cc.polErrorDeg(lat, PE_POL_W);
    TEST_ASSERT_TRUE(std::isfinite(az));
    TEST_ASSERT_TRUE(std::isfinite(alt));
    TEST_ASSERT_TRUE(std::isfinite(w));
}

// Replicate the synthetic-reference seeding done by initTransformation(reset=true)
// in EEPROM.cpp for an EQ-GEM mount. This is the cold-boot baseline produced
// by :AP# (mechanical pole pass complete): two synthetic refs computed from an
// ideal polar mount at the given latitude, plus calculateThirdReference().
static void seedEqGemSyntheticRefs(CoordConv& cc, double Lat, double sign)
{
    Coord_HO HO1 = Coord_HO(0, 45 * DEG_TO_RAD, 90 * DEG_TO_RAD, false);
    Coord_EQ EQ1 = HO1.To_Coord_EQ(Lat);
    Coord_IN IN1 = Coord_IN(0, sign * EQ1.Dec(), sign * EQ1.Ha() - M_PI_2);

    Coord_HO HO2 = Coord_HO(0, 45 * DEG_TO_RAD, 270 * DEG_TO_RAD, false);
    Coord_EQ EQ2 = HO2.To_Coord_EQ(Lat);
    Coord_IN IN2 = Coord_IN(0, sign * EQ2.Dec(), sign * EQ2.Ha() - M_PI_2);

    cc.addReference(HO1.direct_Az_S(), HO1.Alt(), IN1.Axis1_direct(), IN1.Axis2());
    cc.addReference(HO2.direct_Az_S(), HO2.Alt(), IN2.Axis1_direct(), IN2.Axis2());
}

// :AP# finalize uses initTransformation(true) to rebuild the cold-boot baseline
// (synthetic ideal-polar refs, hasValid=false). Property: the conv ends up in
// "ready" state with finite, deterministic Tinv -- the same state a cold boot
// without saved EEPROM would produce. Subsequent :MS# pointing math runs on
// this synthetic conv (effectively trusts the now-mechanical pole).
void test_ap_cold_baseline_eq_gem_north_is_ready(void) {
    CoordConv cc;
    cc.clean();
    const double lat = 48.85 * DEG_TO_RAD;
    seedEqGemSyntheticRefs(cc, lat, 1.0);
    TEST_ASSERT_TRUE(cc.isReady());
    TEST_ASSERT_TRUE(std::isfinite(cc.polErrorDeg(lat, PE_EQ_AZ)));
    TEST_ASSERT_TRUE(std::isfinite(cc.polErrorDeg(lat, PE_EQ_ALT)));
    TEST_ASSERT_TRUE(std::isfinite(cc.polErrorDeg(lat, PE_POL_W)));
}

void test_ap_cold_baseline_eq_gem_south_is_ready(void) {
    CoordConv cc;
    cc.clean();
    const double lat = -33.87 * DEG_TO_RAD;
    seedEqGemSyntheticRefs(cc, lat, -1.0);
    TEST_ASSERT_TRUE(cc.isReady());
    TEST_ASSERT_TRUE(std::isfinite(cc.polErrorDeg(lat, PE_EQ_AZ)));
    TEST_ASSERT_TRUE(std::isfinite(cc.polErrorDeg(lat, PE_EQ_ALT)));
    TEST_ASSERT_TRUE(std::isfinite(cc.polErrorDeg(lat, PE_POL_W)));
}

// ---------------------------------------------------------------------------
// GEM 2-star alignment with polar axis offset 5° in Az and 5° in Alt
// ---------------------------------------------------------------------------
//
// Geometry (matches polErrorDeg / toDirCos):
//   True NCP in the HO dir-cos frame is toDirCos(0, Lat) = {cos Lat, 0, sin Lat}.
//   Misaligned mechanical pole: toDirCos(dAz, Lat + dAlt).
//
// Instrument axes follow EEPROM.cpp GEM seeding (Axis2 = Dec, Axis1 = Ha − π/2)
// but HA/Dec are computed in the mechanical-pole frame: shift az by +dAz so the
// offset pole sits on the meridian, then To_Coord_EQ(Lat + dAlt).
// Catalog HO of each star stays true-sky.
//
// Note: addReference uses Axis1() (not Axis1_direct) so the instrument angle
// matches polErrorDeg's Tinv column-2 convention (ideal pole → ~0 misclosure).

static void addGemStarObservation(
  CoordConv& cc,
  double Lat,
  double dAzRad,
  double dAltRad,
  double azDeg,
  double altDeg)
{
  Coord_HO HO_true(0, altDeg * DEG_TO_RAD, azDeg * DEG_TO_RAD, false);

  // Mount polar-aligned to the offset pole: put that pole on az_S=0, convert at its alt.
  Coord_HO HO_mech(0, altDeg * DEG_TO_RAD, azDeg * DEG_TO_RAD + dAzRad, false);
  Coord_EQ EQ = HO_mech.To_Coord_EQ(Lat + dAltRad);
  Coord_IN IN(0, EQ.Dec(), EQ.Ha() - M_PI_2);

  cc.addReference(HO_true.direct_Az_S(), HO_true.Alt(), IN.Axis1(), IN.Axis2());
}

static double sphereAngleDeg(double az1, double alt1, double az2, double alt2)
{
  double a[3], b[3];
  LA3::toDirCos(a, az1, alt1);
  LA3::toDirCos(b, az2, alt2);
  double c = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  if (c > 1.0) c = 1.0;
  if (c < -1.0) c = -1.0;
  return acos(c) * 180.0 / M_PI;
}

void test_gem_2star_pole_offset_5deg_az_alt(void)
{
  const double Lat = 47.22 * DEG_TO_RAD; // Nantes-ish
  const double dAzDeg = 5.0;
  const double dAltDeg = 5.0;
  const double dAz = dAzDeg * DEG_TO_RAD;
  const double dAlt = dAltDeg * DEG_TO_RAD;

  CoordConv cc;
  cc.clean();

  // Two well-separated stars (east / west of meridian), both well above horizon.
  addGemStarObservation(cc, Lat, dAz, dAlt, 90.0, 45.0);
  TEST_ASSERT_FALSE(cc.isReady()); // one ref only
  addGemStarObservation(cc, Lat, dAz, dAlt, 270.0, 45.0);
  TEST_ASSERT_TRUE(cc.isReady());

  // Same post-processing as Command_A after the second star on a GEM.
  cc.minimizeAxis2();
  cc.minimizeAxis1(M_PI_2);
  TEST_ASSERT_TRUE(cc.isReady());

  const double errAz = cc.polErrorDeg(Lat, PE_EQ_AZ);
  const double errAlt = cc.polErrorDeg(Lat, PE_EQ_ALT);
  const double errW = cc.polErrorDeg(Lat, PE_POL_W);

  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, dAzDeg, errAz);
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, dAltDeg, errAlt);

  // Total wedge = angle between true NCP and mechanical pole on the sphere.
  const double expectW = sphereAngleDeg(0.0, Lat, dAz, Lat + dAlt);
  TEST_ASSERT_DOUBLE_WITHIN(TOL_POL_W, expectW, errW);
}

void test_gem_2star_ideal_pole_reports_near_zero(void)
{
  const double Lat = 47.22 * DEG_TO_RAD;
  CoordConv cc;
  cc.clean();
  addGemStarObservation(cc, Lat, 0.0, 0.0, 90.0, 45.0);
  addGemStarObservation(cc, Lat, 0.0, 0.0, 270.0, 45.0);
  TEST_ASSERT_TRUE(cc.isReady());
  cc.minimizeAxis2();
  cc.minimizeAxis1(M_PI_2);

  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 0.0, cc.polErrorDeg(Lat, PE_EQ_AZ));
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 0.0, cc.polErrorDeg(Lat, PE_EQ_ALT));
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 0.0, cc.polErrorDeg(Lat, PE_POL_W));
}

void test_gem_2star_then_ap_cold_baseline_clears_soft_model(void)
{
  // After the bolt pass, :AP# reseeds the synthetic ideal-polar baseline;
  // polar misclosure from the soft 2-star model must no longer be the 5°/5°
  // soft-model report. (Synthetic EEPROM seeding uses Axis1_direct and is a
  // different convention — here we only require the soft 5° signal is gone
  // after a clean + re-seed that reports a distinct near-ideal model via
  // Axis1()-based observations at zero offset.)
  const double Lat = 47.22 * DEG_TO_RAD;
  CoordConv cc;
  cc.clean();
  addGemStarObservation(cc, Lat, 5.0 * DEG_TO_RAD, 5.0 * DEG_TO_RAD, 90.0, 45.0);
  addGemStarObservation(cc, Lat, 5.0 * DEG_TO_RAD, 5.0 * DEG_TO_RAD, 270.0, 45.0);
  cc.minimizeAxis2();
  cc.minimizeAxis1(M_PI_2);
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 5.0, cc.polErrorDeg(Lat, PE_EQ_AZ));
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 5.0, cc.polErrorDeg(Lat, PE_EQ_ALT));

  // Simulate :AP# trust-the-pole: rebuild from zero-offset observations (ideal pole).
  cc.clean();
  addGemStarObservation(cc, Lat, 0.0, 0.0, 90.0, 45.0);
  addGemStarObservation(cc, Lat, 0.0, 0.0, 270.0, 45.0);
  cc.minimizeAxis2();
  cc.minimizeAxis1(M_PI_2);
  TEST_ASSERT_TRUE(cc.isReady());
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 0.0, cc.polErrorDeg(Lat, PE_EQ_AZ));
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 0.0, cc.polErrorDeg(Lat, PE_EQ_ALT));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_pol_error_zero_when_not_ready);
    RUN_TEST(test_pol_error_finite_identity_matrix);
    RUN_TEST(test_ap_cold_baseline_eq_gem_north_is_ready);
    RUN_TEST(test_ap_cold_baseline_eq_gem_south_is_ready);
    RUN_TEST(test_gem_2star_ideal_pole_reports_near_zero);
    RUN_TEST(test_gem_2star_pole_offset_5deg_az_alt);
    RUN_TEST(test_gem_2star_then_ap_cold_baseline_clears_soft_model);
    return UNITY_END();
}
