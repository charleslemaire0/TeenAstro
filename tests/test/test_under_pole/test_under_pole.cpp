/*
 * test_under_pole.cpp — Under-pole limit geometry (GEM MountLimits::checkPole)
 *
 * SYNC: Logic and constants must match TeenAstroMainUnit/MountLimits.cpp (checkPole).
 *       If you change MountLimits::checkPole, update this file in the same commit.
 *
 * Includes: 12 h limit disables the check (OnStep / UniversalMainUnit parity).
 */

#include <unity.h>
#include <cmath>
#include <cstdint>

#include "TeenAstroMath.h"

namespace {

// --- Mirror MountLimits.cpp (anonymous namespace) -----------------------------
constexpr double UNDER_POLE_TRACKING_MARGIN = 5.0 / 60.0;
constexpr double UNDER_POLE_OFFSET = 6.0;
constexpr double HOURS_PER_DEG_RA = 15.0;

enum class CheckMode { GOTO, TRACKING };

/**
 * Same predicate as MountLimits::checkPole for known poleDef, steps/deg, and side.
 * Uses double threshold comparisons like the firmware (long axis1 promotes to double).
 */
bool checkPoleMirror(long axis1, long poleDef, double stepsPerDegree, PoleSide side,
  double underPoleLimitGOTO, CheckMode mode)
{
  // SYNC MountLimits.cpp:12h disables under-pole check (OnStep / Universal parity).
  if (underPoleLimitGOTO >= 12.0)
    return true;

  double underPoleLimit = (mode == CheckMode::GOTO)
    ? underPoleLimitGOTO
    : underPoleLimitGOTO + UNDER_POLE_TRACKING_MARGIN;

  switch (side)
  {
  case POLE_UNDER:
    return static_cast<double>(axis1)
      < static_cast<double>(poleDef)
          + (underPoleLimit - UNDER_POLE_OFFSET) * HOURS_PER_DEG_RA * stepsPerDegree;
  case POLE_OVER:
    return static_cast<double>(axis1)
      > static_cast<double>(poleDef)
          - (underPoleLimit - UNDER_POLE_OFFSET) * HOURS_PER_DEG_RA * stepsPerDegree;
  default:
    return false;
  }
}

double thresholdUnder(long poleDef, double spd, double underPoleLimitGOTO, CheckMode mode)
{
  double underPoleLimit = (mode == CheckMode::GOTO)
    ? underPoleLimitGOTO
    : underPoleLimitGOTO + UNDER_POLE_TRACKING_MARGIN;
  return static_cast<double>(poleDef)
    + (underPoleLimit - UNDER_POLE_OFFSET) * HOURS_PER_DEG_RA * spd;
}

double thresholdOver(long poleDef, double spd, double underPoleLimitGOTO, CheckMode mode)
{
  double underPoleLimit = (mode == CheckMode::GOTO)
    ? underPoleLimitGOTO
    : underPoleLimitGOTO + UNDER_POLE_TRACKING_MARGIN;
  return static_cast<double>(poleDef)
    - (underPoleLimit - UNDER_POLE_OFFSET) * HOURS_PER_DEG_RA * spd;
}

}  // namespace

// ------------------------------------------------------------------------------
// POLE_UNDER: axis1 must stay strictly below poleDef + margin
// ------------------------------------------------------------------------------

void test_pole_under_9h_goto_inside_one_step_below_threshold()
{
  const long poleDef = 1000000L;
  const double spd = 12800.0;
  const double lim9 = 9.0;
  double thr = thresholdUnder(poleDef, spd, lim9, CheckMode::GOTO);
  long axisOk = (long)std::floor(thr - 1.0);
  TEST_ASSERT_TRUE(checkPoleMirror(axisOk, poleDef, spd, POLE_UNDER, lim9, CheckMode::GOTO));
}

void test_pole_under_9h_goto_outside_at_threshold()
{
  const long poleDef = 1000000L;
  const double spd = 12800.0;
  const double lim9 = 9.0;
  double thr = thresholdUnder(poleDef, spd, lim9, CheckMode::GOTO);
  long axisBad = (long)std::ceil(thr);
  TEST_ASSERT_FALSE(checkPoleMirror(axisBad, poleDef, spd, POLE_UNDER, lim9, CheckMode::GOTO));
}

void test_pole_under_11h_goto_wider_margin_than_9h()
{
  const long poleDef = 500000L;
  const double spd = 10000.0;
  double thr9 = thresholdUnder(poleDef, spd, 9.0, CheckMode::GOTO);
  double thr11 = thresholdUnder(poleDef, spd, 11.0, CheckMode::GOTO);
  TEST_ASSERT_TRUE(thr11 > thr9);
  long between = (long)std::floor(thr9 + (thr11 - thr9) * 0.5);
  TEST_ASSERT_FALSE(checkPoleMirror(between, poleDef, spd, POLE_UNDER, 9.0, CheckMode::GOTO));
  TEST_ASSERT_TRUE(checkPoleMirror(between, poleDef, spd, POLE_UNDER, 11.0, CheckMode::GOTO));
}

void test_twelve_hours_deactivates_under_pole_check()
{
  const long poleDef = 100000L;
  const double spd = 5000.0;
  const long absurd = poleDef + 500000000L;
  TEST_ASSERT_TRUE(checkPoleMirror(absurd, poleDef, spd, POLE_UNDER, 12.0, CheckMode::GOTO));
  TEST_ASSERT_TRUE(checkPoleMirror(absurd, poleDef, spd, POLE_UNDER, 12.0, CheckMode::TRACKING));
  const long absurdOver = poleDef - 500000000L;
  TEST_ASSERT_TRUE(checkPoleMirror(absurdOver, poleDef, spd, POLE_OVER, 12.0, CheckMode::GOTO));
}

void test_just_below_twelve_hours_still_restricts()
{
  const long poleDef = 0L;
  const double spd = 1.0;
  TEST_ASSERT_FALSE(checkPoleMirror(119L, poleDef, spd, POLE_UNDER, 11.99, CheckMode::GOTO));
}

// ------------------------------------------------------------------------------
// POLE_OVER: axis1 must stay strictly above poleDef - margin
// ------------------------------------------------------------------------------

void test_pole_over_9h_goto_inside_one_step_above_threshold()
{
  const long poleDef = 2000000L;
  const double spd = 6400.0;
  const double lim9 = 9.0;
  double thr = thresholdOver(poleDef, spd, lim9, CheckMode::GOTO);
  long axisOk = (long)std::ceil(thr + 1.0);
  TEST_ASSERT_TRUE(checkPoleMirror(axisOk, poleDef, spd, POLE_OVER, lim9, CheckMode::GOTO));
}

void test_pole_over_9h_goto_outside_at_threshold()
{
  const long poleDef = 2000000L;
  const double spd = 6400.0;
  const double lim9 = 9.0;
  double thr = thresholdOver(poleDef, spd, lim9, CheckMode::GOTO);
  long axisBad = (long)std::floor(thr);
  TEST_ASSERT_FALSE(checkPoleMirror(axisBad, poleDef, spd, POLE_OVER, lim9, CheckMode::GOTO));
}

// ------------------------------------------------------------------------------
// Tracking mode uses a slightly larger effective limit (more permissive)
// ------------------------------------------------------------------------------

void test_tracking_more_permissive_than_goto_pole_under()
{
  const long poleDef = 800000L;
  const double spd = 9000.0;
  const double lim = 10.0;
  double thrGoto = thresholdUnder(poleDef, spd, lim, CheckMode::GOTO);
  double thrTrk = thresholdUnder(poleDef, spd, lim, CheckMode::TRACKING);
  TEST_ASSERT_TRUE(thrTrk > thrGoto);
  long mid = (long)std::floor(thrGoto + (thrTrk - thrGoto) * 0.5);
  TEST_ASSERT_FALSE(checkPoleMirror(mid, poleDef, spd, POLE_UNDER, lim, CheckMode::GOTO));
  TEST_ASSERT_TRUE(checkPoleMirror(mid, poleDef, spd, POLE_UNDER, lim, CheckMode::TRACKING));
}

// ------------------------------------------------------------------------------
// Invalid pier / side
// ------------------------------------------------------------------------------

void test_pole_notvalid_always_false()
{
  const long poleDef = 0L;
  const double spd = 10000.0;
  TEST_ASSERT_FALSE(
    checkPoleMirror(0L, poleDef, spd, POLE_NOTVALID, 11.0, CheckMode::GOTO));
}

// ------------------------------------------------------------------------------
// Legacy OnStep-style hour limit (9..12) maps to (limit-6)*15 degrees of RA axis span
// ------------------------------------------------------------------------------

void test_margin_degrees_match_classic_onstep_scaling()
{
  const double spd = 1.0;  // 1 step per degree → threshold delta = (u-6)*15 steps
  const long poleDef = 0L;
  for (int u10 = 90; u10 <= 110; u10 += 10)
  {
    double u = u10 / 10.0;
    double marginDeg = (u - UNDER_POLE_OFFSET) * HOURS_PER_DEG_RA;
    double thr = thresholdUnder(poleDef, spd, u, CheckMode::GOTO);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, static_cast<double>(poleDef) + marginDeg, thr);
  }
}

void setUp() {}
void tearDown() {}

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  UNITY_BEGIN();

  RUN_TEST(test_pole_under_9h_goto_inside_one_step_below_threshold);
  RUN_TEST(test_pole_under_9h_goto_outside_at_threshold);
  RUN_TEST(test_pole_under_11h_goto_wider_margin_than_9h);
  RUN_TEST(test_twelve_hours_deactivates_under_pole_check);
  RUN_TEST(test_just_below_twelve_hours_still_restricts);
  RUN_TEST(test_pole_over_9h_goto_inside_one_step_above_threshold);
  RUN_TEST(test_pole_over_9h_goto_outside_at_threshold);
  RUN_TEST(test_tracking_more_permissive_than_goto_pole_under);
  RUN_TEST(test_pole_notvalid_always_false);
  RUN_TEST(test_margin_degrees_match_classic_onstep_scaling);

  return UNITY_END();
}
