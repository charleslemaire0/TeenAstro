/*
 * test_polar_align_math.cpp — polErrorDeg / polar misclosure helpers
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

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_pol_error_zero_when_not_ready);
    RUN_TEST(test_pol_error_finite_identity_matrix);
    RUN_TEST(test_ap_cold_baseline_eq_gem_north_is_ready);
    RUN_TEST(test_ap_cold_baseline_eq_gem_south_is_ready);
    return UNITY_END();
}
