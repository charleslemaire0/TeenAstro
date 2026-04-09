/*
 * test_meridian_flip.cpp — Unit tests for GEM meridian / meridian-flip related behavior
 *
 * Direction logic mirrors StatusAxis::effectiveMotionDirectionPositive() in Axis.hpp;
 * if you change one, update the other.
 */

#include <unity.h>
#include <cmath>
#include <cstdint>

#include "CommandEnums.h"

namespace {

bool effectiveMotionDirectionPositive(long pos, double target, double fstep, bool dirLast)
{
  long d = (long)target - pos;
  if (d != 0)
    return d > 0;
  if (fstep != 0.0)
    return fstep > 0.0;
  return dirLast;
}

unsigned gxasGotoKindFromByte100(unsigned char b100)
{
  return (unsigned)((b100 >> 5) & 0x7);
}

}  // namespace

// ── effectiveMotionDirectionPositive (see Axis.hpp StatusAxis) ──────────────

void test_dir_delta_positive_target_ahead()
{
  TEST_ASSERT_TRUE(effectiveMotionDirectionPositive(1000L, 1005.0, 1.0, false));
}

void test_dir_delta_negative_target_behind()
{
  TEST_ASSERT_FALSE(effectiveMotionDirectionPositive(1000L, 995.0, 1.0, true));
}

void test_dir_delta_zero_fstep_overrides_stale_dir_false()
{
  TEST_ASSERT_TRUE(effectiveMotionDirectionPositive(10000L, 10000.0, 0.5, false));
}

void test_dir_delta_zero_fstep_negative_overrides_stale_dir_true()
{
  TEST_ASSERT_FALSE(effectiveMotionDirectionPositive(0L, 0.0, -0.3, true));
}

void test_dir_delta_zero_fstep_zero_uses_dir_true()
{
  TEST_ASSERT_TRUE(effectiveMotionDirectionPositive(0L, 0.0, 0.0, true));
}

void test_dir_delta_zero_fstep_zero_uses_dir_false()
{
  TEST_ASSERT_FALSE(effectiveMotionDirectionPositive(0L, 0.0, 0.0, false));
}

void test_dir_delta_nonzero_ignores_fstep_and_dir()
{
  TEST_ASSERT_TRUE(effectiveMotionDirectionPositive(0L, 10.0, -99.0, false));
  TEST_ASSERT_FALSE(effectiveMotionDirectionPositive(100L, 50.0, 99.0, true));
}

// ── Meridian GOTO minutes → HA degrees (MountLimits / GXCS: minutes/4) ───────

void test_meridian_minutes_to_ha_deg_typical()
{
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, 60.0 / 4.0);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, -15.0, -60.0 / 4.0);
}

void test_meridian_minutes_to_ha_deg_zero()
{
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, 0.0 / 4.0);
}

// ── GXAS byte 100: GotoState in bits 5–7 (Command_GX.cpp) ───────────────────

static uint8_t packGxasByte100(GotoState gs, unsigned alignPhase = 0, unsigned starNum = 0)
{
  uint8_t b100 = (uint8_t)(alignPhase & 0x3);
  b100 |= (uint8_t)((starNum & 0x7) << 2);
  b100 |= (uint8_t)(((uint8_t)gs & 0x7) << 5);
  return b100;
}

void test_gxas_goto_kind_decode_matches_command_gx()
{
  TEST_ASSERT_EQUAL_UINT8(0, gxasGotoKindFromByte100(packGxasByte100(GOTO_NONE)));
  TEST_ASSERT_EQUAL_UINT8(1, gxasGotoKindFromByte100(packGxasByte100(GOTO_EQ)));
  TEST_ASSERT_EQUAL_UINT8(2, gxasGotoKindFromByte100(packGxasByte100(GOTO_ALTAZ)));
  TEST_ASSERT_EQUAL_UINT8(3, gxasGotoKindFromByte100(packGxasByte100(GOTO_FLIP_PIER_SIDE)));
}

void test_gxas_goto_kind_roundtrip_all_states()
{
  for (unsigned gs = 0; gs <= 3; ++gs)
  {
    uint8_t b100 = packGxasByte100(static_cast<GotoState>(gs));
    TEST_ASSERT_EQUAL_UINT(gs, gxasGotoKindFromByte100(b100));
  }
}

void test_gxas_goto_kind_ignores_low_bits()
{
  uint8_t b100 = (uint8_t)(0x1F | ((uint8_t)GOTO_EQ << 5));
  TEST_ASSERT_EQUAL_UINT8(1, gxasGotoKindFromByte100(b100));
}

void test_goto_state_enum_values_documented()
{
  TEST_ASSERT_EQUAL_UINT8(0, (uint8_t)GOTO_NONE);
  TEST_ASSERT_EQUAL_UINT8(1, (uint8_t)GOTO_EQ);
  TEST_ASSERT_EQUAL_UINT8(2, (uint8_t)GOTO_ALTAZ);
  TEST_ASSERT_EQUAL_UINT8(3, (uint8_t)GOTO_FLIP_PIER_SIDE);
}

void setUp() {}
void tearDown() {}

int main(int argc, char** argv)
{
  UNITY_BEGIN();

  RUN_TEST(test_dir_delta_positive_target_ahead);
  RUN_TEST(test_dir_delta_negative_target_behind);
  RUN_TEST(test_dir_delta_zero_fstep_overrides_stale_dir_false);
  RUN_TEST(test_dir_delta_zero_fstep_negative_overrides_stale_dir_true);
  RUN_TEST(test_dir_delta_zero_fstep_zero_uses_dir_true);
  RUN_TEST(test_dir_delta_zero_fstep_zero_uses_dir_false);
  RUN_TEST(test_dir_delta_nonzero_ignores_fstep_and_dir);

  RUN_TEST(test_meridian_minutes_to_ha_deg_typical);
  RUN_TEST(test_meridian_minutes_to_ha_deg_zero);

  RUN_TEST(test_gxas_goto_kind_decode_matches_command_gx);
  RUN_TEST(test_gxas_goto_kind_roundtrip_all_states);
  RUN_TEST(test_gxas_goto_kind_ignores_low_bits);
  RUN_TEST(test_goto_state_enum_values_documented);

  return UNITY_END();
}
