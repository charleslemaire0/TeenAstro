/*
 * test_pier_side.cpp - Unsupported pier side error.
 *
 * German alt-az and alt-az fork only use POLE_UNDER. Being on POLE_OVER is
 * ERRT_PIER_SIDE; a goto reports it as ERRGOTO_PIER_SIDE, which clients read
 * as LX200_ERR_PIER_SIDE. An equatorial fork may use POLE_OVER to look under
 * Polaris, the same as a German equatorial uses both sides.
 */
#include <unity.h>
#include "CommandEnums.h"
#include "PierSideRule.h"
#include "RtcBattery.h"

// MainUnit MountTypes.h pulls in Arduino.h; its tracking error values are
// mirrored here. goTo() reports a tracking error as lastError + 10.
static const int ERRT_MERIDIAN_VALUE  = 7;
static const int ERRT_PIER_SIDE_VALUE = 8;

void test_altaz_rejects_over_pole(void) {
  TEST_ASSERT_FALSE(poleSideSupported(true, POLE_OVER));
  TEST_ASSERT_TRUE(poleSideSupported(true, POLE_UNDER));
  TEST_ASSERT_TRUE(poleSideSupported(true, POLE_NOTVALID));
}

void test_equatorial_fork_and_gem_use_both_sides(void) {
  // false = not alt-az: equatorial fork and German equatorial.
  TEST_ASSERT_TRUE(poleSideSupported(false, POLE_OVER));
  TEST_ASSERT_TRUE(poleSideSupported(false, POLE_UNDER));
}

void test_pier_letter_maps_to_pole_side(void) {
  // Alt-az, or equatorial in the north: east is the supported side.
  TEST_ASSERT_EQUAL(POLE_UNDER, poleSideFromPierLetter('E', true));
  TEST_ASSERT_EQUAL(POLE_OVER, poleSideFromPierLetter('W', true));
  // Equatorial in the south: west is the supported side.
  TEST_ASSERT_EQUAL(POLE_OVER, poleSideFromPierLetter('E', false));
  TEST_ASSERT_EQUAL(POLE_UNDER, poleSideFromPierLetter('W', false));
}

void test_sm_west_refused_on_altaz(void) {
  TEST_ASSERT_FALSE(poleSideSupported(true, poleSideFromPierLetter('W', true)));
  TEST_ASSERT_TRUE(poleSideSupported(true, poleSideFromPierLetter('E', true)));
  // Equatorial fork, northern sky: west is the side under the pole, and it is allowed.
  TEST_ASSERT_TRUE(poleSideSupported(false, poleSideFromPierLetter('W', true)));
}

void test_goto_code_follows_tracking_error(void) {
  TEST_ASSERT_EQUAL(ERRT_MERIDIAN_VALUE + 10, (int)ERRGOTO_MERIDIAN);
  TEST_ASSERT_EQUAL(ERRT_PIER_SIDE_VALUE + 10, (int)ERRGOTO_PIER_SIDE);
  TEST_ASSERT_EQUAL('B', '0' + (int)ERRGOTO_PIER_SIDE);
}

void test_goto_code_maps_to_client_error(void) {
  TEST_ASSERT_EQUAL(LX200_ERR_PIER_SIDE, gotoErrorToReturn(ERRGOTO_PIER_SIDE, true));
  TEST_ASSERT_FALSE(isOk(LX200_ERR_PIER_SIDE));
}

void test_rtc_goto_is_its_own_code(void) {
  TEST_ASSERT_EQUAL(19, (int)ERRGOTO_RTC);
  TEST_ASSERT_EQUAL('C', '0' + (int)ERRGOTO_RTC);
  TEST_ASSERT_EQUAL(LX200_ERR_RTC, gotoErrorToReturn(ERRGOTO_RTC, true));
  TEST_ASSERT_FALSE(isOk(LX200_ERR_RTC));
}

void test_rtc_unset_epoch_matches_boot_seed_only(void) {
  const uint32_t seed = 1546300800u;
  TEST_ASSERT_TRUE(rtcTimeIsUnset(seed, 0));
  TEST_ASSERT_TRUE(rtcTimeIsUnset(seed + 2, 1500));
  TEST_ASSERT_FALSE(rtcTimeIsUnset(seed + 3600, 2000));
  TEST_ASSERT_FALSE(rtcTimeIsUnset(1709600000u, 2000));
  TEST_ASSERT_FALSE(rtcTimeIsUnset(seed - 1, 0));
}

void setUp() {}
void tearDown() {}

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;
  UNITY_BEGIN();
  RUN_TEST(test_altaz_rejects_over_pole);
  RUN_TEST(test_equatorial_fork_and_gem_use_both_sides);
  RUN_TEST(test_pier_letter_maps_to_pole_side);
  RUN_TEST(test_sm_west_refused_on_altaz);
  RUN_TEST(test_goto_code_follows_tracking_error);
  RUN_TEST(test_goto_code_maps_to_client_error);
  RUN_TEST(test_rtc_goto_is_its_own_code);
  RUN_TEST(test_rtc_unset_epoch_matches_boot_seed_only);
  return UNITY_END();
}
