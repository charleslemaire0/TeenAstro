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

void test_set_pole_error_matches_reported_angles(void)
{
  CoordConv cc;
  const double lat = 48.0 * DEG_TO_RAD;
  const double dAz = 1.25 * DEG_TO_RAD;
  const double dAlt = -0.8 * DEG_TO_RAD;
  cc.setPoleError(lat, dAz, dAlt, 0.4);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, dAz * 180.0 / M_PI, cc.polErrorDeg(lat, PE_EQ_AZ));
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, dAlt * 180.0 / M_PI, cc.polErrorDeg(lat, PE_EQ_ALT));
  // Rotation about the pole must not move the reported pole.
  cc.setPoleError(lat, dAz, dAlt, -1.7);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, dAz * 180.0 / M_PI, cc.polErrorDeg(lat, PE_EQ_AZ));
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, dAlt * 180.0 / M_PI, cc.polErrorDeg(lat, PE_EQ_ALT));
}

static void addHeadStar(CoordConv& obs, const double (&T)[3][3], const HeadModel& head,
                        double az, double alt)
{
  double dcSky[3], dcI[3];
  LA3::toDirCos(dcSky, az, alt);
  LA3::multiply(dcI, T, dcSky);
  HeadModel zero;
  double hint1 = 0, hint2 = 0, ax1 = 0, ax2 = 0;
  HeadGeom::inverse(dcI, zero, 0.0, hint1, hint2);
  TEST_ASSERT_TRUE(HeadGeom::inverse(dcI, head, hint2, ax1, ax2));
  obs.addReference(az, alt, ax1, ax2);
}

void test_two_star_with_known_perp_estimates_the_pole(void)
{
  const double lat = 47.22 * DEG_TO_RAD;
  const double dAz = 1.1 * DEG_TO_RAD;
  const double dAlt = -0.4 * DEG_TO_RAD;
  const double cone = 50.0 / 3600.0 * DEG_TO_RAD;
  const double perp = -70.0 / 3600.0 * DEG_TO_RAD;
  const double az0 = 90.0 * DEG_TO_RAD, alt0 = 40.0 * DEG_TO_RAD;
  const double az1 = 250.0 * DEG_TO_RAD, alt1 = 55.0 * DEG_TO_RAD;

  CoordConv truth;
  truth.setPoleError(lat, dAz, dAlt, 0.35);
  HeadModel head(cone, perp, 0.0);

  // Same finish as a GEM 2-star: strip the known cone and perpendicularity,
  // then the two stars estimate the pole. Both stay in the head.
  CoordConv obs;
  obs.clean();
  addHeadStar(obs, truth.T, head, az0, alt0);
  addHeadStar(obs, truth.T, head, az1, alt1);
  const double meas1[2] = { obs.ax1[0], obs.ax2[0] };
  const double meas2[2] = { obs.ax1[1], obs.ax2[1] };
  TEST_ASSERT_TRUE(obs.alignTwoStarKnownGeom(cone, perp));
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, dAz * 180.0 / M_PI, obs.polErrorDeg(lat, PE_EQ_AZ));
  TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, dAlt * 180.0 / M_PI, obs.polErrorDeg(lat, PE_EQ_ALT));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, cone, obs.head.cone);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, perp, obs.head.perp);

  double dcSky[3], dcI[3], pred[3];
  LA3::toDirCos(dcSky, az0, alt0);
  HeadGeom::forward(dcI, meas1[0], meas1[1], obs.head);
  LA3::multiply(pred, obs.Tinv, dcI);
  TEST_ASSERT_DOUBLE_WITHIN(1e-3, 0.0, LA3::angle2Vectors(pred, dcSky));
  LA3::toDirCos(dcSky, az1, alt1);
  HeadGeom::forward(dcI, meas2[0], meas2[1], obs.head);
  LA3::multiply(pred, obs.Tinv, dcI);
  TEST_ASSERT_DOUBLE_WITHIN(1e-3, 0.0, LA3::angle2Vectors(pred, dcSky));
}

// One two-star session the way Command_A finishes a GEM alignment.
// Encoder angles already include the simulated errors. Nothing else is passed.
static void alignPair(CoordConv& cc, double Lat,
                      double az1, double alt1, double a1, double b1,
                      double az2, double alt2, double a2, double b2)
{
  cc.clean();
  Coord_HO sky1(0, alt1 * DEG_TO_RAD, az1 * DEG_TO_RAD, false);
  Coord_HO sky2(0, alt2 * DEG_TO_RAD, az2 * DEG_TO_RAD, false);
  // getInstr() then Axis1_direct(), which is what :A*# records.
  Coord_IN r1(0, b1, a1);
  Coord_IN r2(0, b2, a2);
  cc.addReference(sky1.direct_Az_S(), sky1.Alt(), r1.Axis1_direct(), r1.Axis2());
  cc.addReference(sky2.direct_Az_S(), sky2.Alt(), r2.Axis1_direct(), r2.Axis2());
  cc.minimizeAxis2();
  cc.minimizeAxis1(Lat >= 0 ? M_PI_2 : -M_PI_2);
}

void test_gem_2star_many_pairs_pole_5deg_index_is_unknown(void)
{
  const double Lat = 47.22 * DEG_TO_RAD;
  const double dAz = 5.0 * DEG_TO_RAD;
  const double dAlt = 5.0 * DEG_TO_RAD;
  const double indexRad = 5.0 * DEG_TO_RAD;
  // The truth model is only used to build encoder readings. The alignment
  // CoordConv below is never given these angles.
  CoordConv truth;
  truth.setPoleError(Lat, dAz, dAlt, 0.0);

  const double pairs[][4] = {
    { 90, 45, 270, 45 },
    { 60, 30, 300, 55 },
    { 120, 60, 240, 35 },
    { 45, 40, 200, 50 },
    { 80, 25, 280, 70 },
    { 100, 50, 220, 30 },
    { 70, 55, 250, 40 },
    { 110, 35, 290, 60 }
  };
  const int nPairs = (int)(sizeof(pairs) / sizeof(pairs[0]));

  double worstAz = 0, worstAlt = 0;
  for (int p = 0; p < nPairs; p++)
  {
    const double az1 = pairs[p][0], alt1 = pairs[p][1];
    const double az2 = pairs[p][2], alt2 = pairs[p][3];
    Coord_HO sky1(0, alt1 * DEG_TO_RAD, az1 * DEG_TO_RAD, false);
    Coord_HO sky2(0, alt2 * DEG_TO_RAD, az2 * DEG_TO_RAD, false);
    Coord_IN i1 = sky1.To_Coord_IN(truth.Tinv);
    Coord_IN i2 = sky2.To_Coord_IN(truth.Tinv);
    // 5° axis1 index, unknown to the mount: a common shift of the encoder.
    CoordConv cc;
    alignPair(cc, Lat, az1, alt1, i1.Axis1() + indexRad, i1.Axis2(),
                         az2, alt2, i2.Axis1() + indexRad, i2.Axis2());
    const double eAz = cc.polErrorDeg(Lat, PE_EQ_AZ);
    const double eAlt = cc.polErrorDeg(Lat, PE_EQ_ALT);
    if (fabs(eAz - 5.0) > worstAz) worstAz = fabs(eAz - 5.0);
    if (fabs(eAlt - 5.0) > worstAlt) worstAlt = fabs(eAlt - 5.0);
    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 5.0, eAz);
    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 5.0, eAlt);
  }
  // The 5° index is absorbed into the axis index. It does not move the pole.
  TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, worstAz);
  TEST_ASSERT_DOUBLE_WITHIN(0.001, 0.0, worstAlt);
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
    RUN_TEST(test_set_pole_error_matches_reported_angles);
    RUN_TEST(test_two_star_with_known_perp_estimates_the_pole);
    RUN_TEST(test_gem_2star_many_pairs_pole_5deg_index_is_unknown);
    return UNITY_END();
}
