#include <unity.h>
#include "AlignCatalogRule.h"

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

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_german_eq_and_fork_eq_keep_one_pier_side);
  RUN_TEST(test_altaz_four_stars_cover_both_sides);
  return UNITY_END();
}
