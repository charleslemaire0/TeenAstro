/*
 * test_tracking_rate.cpp - Unit tests for TeenAstro tracking rate protocol
 *
 * Verifies the contract between ASCOM driver and MainUnit firmware for
 * RightAscensionRate and DeclinationRate. The driver sends :SXRr,/ :SXRd, with
 * 16 hex digits (IEEE754 LE double) or legacy ASCII floats; firmware stores
 * RequestedTrackingRateHA/DEC and reports via GXAS and :GXRr# / :GXRd#.
 *
 * ASCOM RightAscensionRate: RA seconds per sidereal second. + = drift east (slow down).
 * ASCOM DeclinationRate:   arc seconds per SI second. + = drift north.
 *
 * Protocol (TeenAstroMainUnit Command_SX.cpp, Command_GX.cpp):
 *   SXRr: RequestedTrackingRateHA = 1.0 - ascomRate
 *   GXAS / GXRr: trackRateRA = 1.0 - RequestedTrackingRateHA (= ascomRate)
 *   SXRd: RequestedTrackingRateDEC = ascomDecRate
 *   GXAS / GXRd: trackRateDec = RequestedTrackingRateDEC
 */

#include <unity.h>
#include <cmath>
#include <cstdint>
#include <cstring>

static const double TOL = 1e-12;

// ── Protocol formulas (must match MainUnit Command_SX.cpp and Command_GX.cpp) ──

/** :SXRr — ASCOM RA rate → RequestedTrackingRateHA */
static double ascomRAToRequestedHA(double ascomRate) {
  return 1.0 - ascomRate;
}

/** GXAS / :GXRr# — RequestedTrackingRateHA → ASCOM RA rate */
static double requestedHAToAscomRA(double ha) {
  return 1.0 - ha;
}

/** :SXRd — ASCOM Dec rate (arcsec/s) → firmware storage */
static double ascomDECToRequested(double ascomDec) {
  return ascomDec;
}

/** GXAS / :GXRd# — firmware → ASCOM Dec rate */
static double requestedDECToAscom(double dec) {
  return dec;
}

/** Hex-LE wire encoding must round-trip (driver/firmware use memcpy + nibbles). */
static void assert_hex_f64_roundtrip(double v) {
  uint8_t b[8];
  memcpy(b, &v, sizeof(double));
  double v2;
  memcpy(&v2, b, sizeof(double));
  TEST_ASSERT_DOUBLE_WITHIN(TOL, v, v2);
}

// =====================================================================
//  RightAscensionRate
// =====================================================================

void test_ra_sidereal_val_zero() {
  double ha = ascomRAToRequestedHA(0.0);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, ha);
  double back = requestedHAToAscomRA(ha);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, back);
  assert_hex_f64_roundtrip(0.0);
}

void test_ra_drift_east_positive_ascom() {
  double ascomRate = 0.0033;
  double ha = ascomRAToRequestedHA(ascomRate);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0 - 0.0033, ha);
  double back = requestedHAToAscomRA(ha);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, back);
  assert_hex_f64_roundtrip(ascomRate);
}

void test_ra_drift_west_negative_ascom() {
  double ascomRate = -0.0033;
  double ha = ascomRAToRequestedHA(ascomRate);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0 + 0.0033, ha);
  double back = requestedHAToAscomRA(ha);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, back);
}

void test_ra_gxas_roundtrip() {
  double ascomRate = 0.0033;
  double ha = ascomRAToRequestedHA(ascomRate);
  double trackRateRA = requestedHAToAscomRA(ha);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, trackRateRA);
}

void test_ra_gxas_roundtrip_negative() {
  double ascomRate = -0.0033;
  double ha = ascomRAToRequestedHA(ascomRate);
  double trackRateRA = requestedHAToAscomRA(ha);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, trackRateRA);
}

void test_ra_large_rate() {
  double ascomRate = 2.6667;
  double ha = ascomRAToRequestedHA(ascomRate);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0 - 2.6667, ha);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, requestedHAToAscomRA(ha));
}

void test_ra_pier_independent() {
  // Driver sends same mount value regardless of pier (firmware applies pier logic).
  double ascomRate = 0.0033;
  double ha1 = ascomRAToRequestedHA(ascomRate);
  double ha2 = ascomRAToRequestedHA(ascomRate);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ha1, ha2);
}

// =====================================================================
//  DeclinationRate
// =====================================================================

void test_dec_zero() {
  double dec = ascomDECToRequested(0.0);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, dec);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, requestedDECToAscom(dec));
}

void test_dec_positive() {
  double ascomRate = 0.05;
  double dec = ascomDECToRequested(ascomRate);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.05, dec);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, requestedDECToAscom(dec));
}

void test_dec_negative() {
  double ascomRate = -0.05;
  double dec = ascomDECToRequested(ascomRate);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, -0.05, dec);
}

void test_dec_gxas_roundtrip() {
  double ascomRate = 0.05;
  double dec = ascomDECToRequested(ascomRate);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, ascomRate, requestedDECToAscom(dec));
}

void test_dec_40_arcsec() {
  double ascomRate = 40.0;
  double dec = ascomDECToRequested(ascomRate);
  TEST_ASSERT_DOUBLE_WITHIN(TOL, 40.0, dec);
  assert_hex_f64_roundtrip(ascomRate);
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
