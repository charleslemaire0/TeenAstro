#include <unity.h>
#include "AlignCatalogRule.h"
#include "GearCatalogRank.h"

// mountKind: 0 equatorial (German or fork), 1 alt-az (German or fork)
// session:   0 two-star, 1 four-star, 2 first half of 3+3, 3 other half

void test_german_eq_and_fork_eq_keep_one_pier_side(void) {
  TEST_ASSERT_EQUAL(ALIGN_EQ_EITHER, alignCatalogRule(0, 0));
  TEST_ASSERT_EQUAL(ALIGN_EQ_THIS_SIDE, alignCatalogRule(0, 1));
  TEST_ASSERT_EQUAL(ALIGN_EQ_THIS_SIDE, alignCatalogRule(0, 2));
  TEST_ASSERT_EQUAL(ALIGN_EQ_OTHER_SIDE, alignCatalogRule(0, 3));
}

void test_altaz_four_stars_cover_both_sides(void) {
  TEST_ASSERT_EQUAL(ALIGN_AZ_SPREAD, alignCatalogRule(1, 0));
  TEST_ASSERT_EQUAL(ALIGN_AZ_FULL, alignCatalogRule(1, 1));
  TEST_ASSERT_EQUAL(ALIGN_AZ_SECTOR, alignCatalogRule(1, 2));
  TEST_ASSERT_EQUAL(ALIGN_AZ_OPPOSITE, alignCatalogRule(1, 3));
}

static GearStarGeom baseGeom() {
  GearStarGeom g;
  g.mountKind = 0;
  g.leg = 1;
  g.gemSamePier = true;
  g.ha = 40.0;
  g.dec = 20.0;
  g.alt = 42.0;
  g.az = 90.0;
  g.mag = 2.0;
  g.refHa = 40.0;
  g.refDec = 20.0;
  g.refAlt = 42.0;
  g.refAz = 90.0;
  g.hasRef = true;
  g.sideSign = 1;
  return g;
}

void test_gem_drops_the_other_pier_side(void) {
  GearStarGeom g = baseGeom();
  g.ha = -40.0;
  g.dec = 40.0;
  GearStarScore s = gearStarScore(g);
  TEST_ASSERT_FALSE(s.keep);

  g.gemSamePier = false;
  s = gearStarScore(g);
  TEST_ASSERT_TRUE(s.keep);
}

void test_eq_axis1_prefers_hour_angle_over_declination(void) {
  GearStarGeom mostlyHa = baseGeom();
  mostlyHa.ha = 80.0;
  mostlyHa.dec = 22.0;
  GearStarGeom mostlyDec = baseGeom();
  mostlyDec.ha = 50.0;
  mostlyDec.dec = 50.0;
  mostlyDec.leg = 1;
  const GearStarScore ha = gearStarScore(mostlyHa);
  const GearStarScore dec = gearStarScore(mostlyDec);
  TEST_ASSERT_TRUE(ha.keep);
  TEST_ASSERT_TRUE(ha.score > dec.score);
}

void test_eq_axis2_prefers_declination(void) {
  GearStarGeom g = baseGeom();
  g.leg = 2;
  g.ha = 42.0;
  g.dec = 45.0;
  const GearStarScore dec = gearStarScore(g);
  g.ha = 90.0;
  g.dec = 22.0;
  const GearStarScore ha = gearStarScore(g);
  TEST_ASSERT_TRUE(dec.keep);
  TEST_ASSERT_FALSE(ha.keep);
}

void test_altaz_axis1_uses_azimuth_not_hour_angle(void) {
  GearStarGeom g = baseGeom();
  g.mountKind = 1;
  g.gemSamePier = false;
  g.ha = -80.0;
  g.az = 180.0;
  g.alt = 44.0;
  const GearStarScore az = gearStarScore(g);
  TEST_ASSERT_TRUE(az.keep);

  g.az = 100.0;
  g.alt = 75.0;
  const GearStarScore alt = gearStarScore(g);
  TEST_ASSERT_TRUE(alt.keep);
  TEST_ASSERT_TRUE(az.score > alt.score);
}

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_german_eq_and_fork_eq_keep_one_pier_side);
  RUN_TEST(test_altaz_four_stars_cover_both_sides);
  RUN_TEST(test_gem_drops_the_other_pier_side);
  RUN_TEST(test_eq_axis1_prefers_hour_angle_over_declination);
  RUN_TEST(test_eq_axis2_prefers_declination);
  RUN_TEST(test_altaz_axis1_uses_azimuth_not_hour_angle);
  return UNITY_END();
}
