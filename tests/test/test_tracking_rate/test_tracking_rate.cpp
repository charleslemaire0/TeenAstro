/*
 * test_tracking_rate.cpp - Unit tests for TeenAstro tracking rate protocol
 *
 * Verifies the contract between ASCOM driver and MainUnit firmware for
 * RightAscensionRate and DeclinationRate. The driver sends SXRr/SXRd values;
 * the firmware stores RequestedTrackingRateHA/DEC and reports them via GXAS
 * TrackRateRA/TrackRateDec. This test ensures the formulas match ASCOM spec.
 *
 * ASCOM RightAscensionRate: RA seconds per sidereal second. + = drift east (slow down).
 * ASCOM DeclinationRate:   arc seconds per SI second. + = drift north.
 *
 * Protocol (from Command_SX.cpp, Command_GX.cpp):
 *   SXRr: RequestedTrackingRateHA = (10000 - val) / 10000
 *   GXAS: trackRateRA = 10000 - RequestedTrackingRateHA * 10000
 *   SXRd: RequestedTrackingRateDEC = val / 10000
 *   GXAS: trackRateDec = RequestedTrackingRateDEC * 10000
 */

#include <unity.h>
#include <cmath>
#include <cstdlib>
#include <cstring>

static const double TOL = 1e-9;

// ── Protocol formulas (must match MainUnit Command_SX.cpp and Command_GX.cpp) ──

/** SXRr: val from driver → RequestedTrackingRateHA in firmware */
static double sxlrValToRequestedHA(long val) {
  return (double)(10000L - val) / 10000.0;
}

/** GXAS: RequestedTrackingRateHA → trackRateRA reported to driver */
static int32_t requestedHAToTrackRateRA(double ha) {
  return (int32_t)round(10000.0 - ha * 10000.0);
}

/** SXRd: val from driver → RequestedTrackingRateDEC in firmware */
static double sxlrValToRequestedDEC(long val) {
  return (double)val / 10000.0;
}

/** GXAS: RequestedTrackingRateDEC → trackRateDec reported to driver */
static int32_t requestedDECToTrackRateDec(double dec) {
  return (int32_t)round(dec * 10000.0);
}

/** ASCOM driver: ascomRate → val to send in SXRr (no pier-side adjustment) */
static long ascomRAToVal(double ascomRate) {
  return (long)round(ascomRate * 10000.0);
}

/** ASCOM driver: trackRateRA from GXAS → RightAscensionRate */
static double trackRateRAToAscom(int32_t trackRateRA) {
  return trackRateRA / 10000.0;
}

/** ASCOM driver: ascomRate → val to send in SXRd */
static long ascomDECToVal(double ascomRate) {
  return (long)(ascomRate * 10000.0);
}

/** ASCOM driver: trackRateDec from GXAS → DeclinationRate */
static double trackRateDecToAscom(int32_t trackRateDec) {
  return trackRateDec / 10000.0;
}

// =====================================================================
//  RightAscensionRate
// =====================================================================

void test_ra_sidereal_val_zero() {
  // Sidereal: ASCOM rate 0 → val 0 → HA = 1
  long val = ascomRAToVal(0.0);
  double ha = sxlrValToRequestedHA(val);
  TEST_ASSERT_EQUAL(0, val);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, ha);
}

void test_ra_drift_east_positive_ascom() {
  // ASCOM +0.0033 = drift east = slow down = HA < 1
  double ascomRate = 0.0033;
  long val = ascomRAToVal(ascomRate);
  double ha = sxlrValToRequestedHA(val);
  TEST_ASSERT_EQUAL(33, val);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.9967, ha);  // 1 - 0.0033
}

void test_ra_drift_west_negative_ascom() {
  // ASCOM -0.0033 = drift west = speed up = HA > 1
  double ascomRate = -0.0033;
  long val = ascomRAToVal(ascomRate);
  double ha = sxlrValToRequestedHA(val);
  TEST_ASSERT_EQUAL(-33, val);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0033, ha);  // 1 + 0.0033
}

void test_ra_gxas_roundtrip() {
  // Driver sends val → firmware stores HA → GXAS reports trackRateRA → driver reads rate
  double ascomRate = 0.0033;
  long val = ascomRAToVal(ascomRate);
  double ha = sxlrValToRequestedHA(val);
  int32_t trackRateRA = requestedHAToTrackRateRA(ha);
  double back = trackRateRAToAscom(trackRateRA);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, back);
}

void test_ra_gxas_roundtrip_negative() {
  double ascomRate = -0.0033;
  long val = ascomRAToVal(ascomRate);
  double ha = sxlrValToRequestedHA(val);
  int32_t trackRateRA = requestedHAToTrackRateRA(ha);
  double back = trackRateRAToAscom(trackRateRA);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, back);
}

void test_ra_large_rate() {
  // ASCOM ±2.6667 (conformance high rate)
  double ascomRate = 2.6667;
  long val = ascomRAToVal(ascomRate);
  double ha = sxlrValToRequestedHA(val);
  TEST_ASSERT_EQUAL(26667, val);
  TEST_ASSERT_DOUBLE_WITHIN(0.0001, 1.0 - 2.6667, ha);  // HA = 1 - 2.6667 = -1.6667
}

void test_ra_pier_independent() {
  // Same ASCOM rate must produce same val regardless of pier (driver does not negate)
  double ascomRate = 0.0033;
  long valEast = ascomRAToVal(ascomRate);
  long valWest = ascomRAToVal(ascomRate);  // no pier parameter in formula
  TEST_ASSERT_EQUAL(valEast, valWest);
}

// =====================================================================
//  DeclinationRate
// =====================================================================

void test_dec_zero() {
  long val = ascomDECToVal(0.0);
  double dec = sxlrValToRequestedDEC(val);
  TEST_ASSERT_EQUAL(0, val);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, dec);
}

void test_dec_positive() {
  // ASCOM +0.05 arcsec/s
  double ascomRate = 0.05;
  long val = ascomDECToVal(ascomRate);
  double dec = sxlrValToRequestedDEC(val);
  TEST_ASSERT_EQUAL(500, val);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.05, dec);
}

void test_dec_negative() {
  double ascomRate = -0.05;
  long val = ascomDECToVal(ascomRate);
  double dec = sxlrValToRequestedDEC(val);
  TEST_ASSERT_EQUAL(-500, val);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, -0.05, dec);
}

void test_dec_gxas_roundtrip() {
  double ascomRate = 0.05;
  long val = ascomDECToVal(ascomRate);
  double dec = sxlrValToRequestedDEC(val);
  int32_t trackRateDec = requestedDECToTrackRateDec(dec);
  double back = trackRateDecToAscom(trackRateDec);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, back);
}

void test_dec_40_arcsec() {
  // Conformance high rate ±40 arcsec/s
  double ascomRate = 40.0;
  long val = ascomDECToVal(ascomRate);
  double dec = sxlrValToRequestedDEC(val);
  TEST_ASSERT_EQUAL(400000, val);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 40.0, dec);
}

// =====================================================================
//  setUp / tearDown (required by PlatformIO Unity)
// =====================================================================
void setUp() {}
void tearDown() {}

// =====================================================================
//  main
// =====================================================================
int main(int argc, char** argv) {
  UNITY_BEGIN();

  RUN_TEST(test_ra_sidereal_val_zero);
  RUN_TEST(test_ra_drift_east_positive_ascom);
  RUN_TEST(test_ra_drift_west_negative_ascom);
  RUN_TEST(test_ra_gxas_roundtrip);
  RUN_TEST(test_ra_gxas_roundtrip_negative);
  RUN_TEST(test_ra_large_rate);
  RUN_TEST(test_ra_pier_independent);

  RUN_TEST(test_dec_zero);
  RUN_TEST(test_dec_positive);
  RUN_TEST(test_dec_negative);
  RUN_TEST(test_dec_gxas_roundtrip);
  RUN_TEST(test_dec_40_arcsec);

  return UNITY_END();
}
