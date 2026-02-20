/*
 * test_coord.cpp - Unit tests for TeenAstroCoord
 *
 * Covers: Coord_EQ, Coord_HO, Coord_IN, Coord_LO
 *         construction, accessors, conversions, round-trips.
 */

// Include library implementations (single-translation-unit build)
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"

#include <unity.h>
#include <cmath>

static const double TOL      = 1e-8;
static const double TOL_CONV = 1e-6;   // coordinate conversions accumulate small errors

// Latitude for tests: 48.8 N (roughly Paris)
static const double LAT_PARIS = 48.8 * M_PI / 180.0;
// Latitude 45 N (simplifies some geometry)
static const double LAT_45    = M_PI / 4.0;

// No refraction for most tests
static const LA3::RefrOpt NO_REFR = {false, 10.0, 1013.0};
// Standard refraction (Meeus standard: 10°C, 1010 mbar)
static const LA3::RefrOpt STD_REFR = {true, 10.0, 1010.0};

// Identity misalignment matrix
static double IDENTITY[3][3];

void initIdentity() {
    LA3::getIdentityMatrix(IDENTITY);
}

// =====================================================================
//  1. Coord_EQ - construction and accessors
// =====================================================================
void test_eq_construction() {
    double fre = 0.1, dec = 0.5, ha = 1.2;
    Coord_EQ eq(fre, dec, ha);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, fre, eq.FrE());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, dec, eq.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, ha, eq.Ha());
}

void test_eq_direct_ha() {
    Coord_EQ eq(0.0, 0.0, 1.5);
    // direct_Ha = -Ha internally stored as -Ha in Euler[2]
    TEST_ASSERT_DOUBLE_WITHIN(TOL, -1.5, eq.direct_Ha());
}

void test_eq_ra() {
    double ha = 1.0, lst = 3.0;
    Coord_EQ eq(0.0, 0.0, ha);
    double ra = eq.Ra(lst);
    // RA = LST - HA
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 2.0, ra);
}

void test_eq_ra_wrap() {
    double ha = 4.0, lst = 1.0;
    Coord_EQ eq(0.0, 0.0, ha);
    double ra = eq.Ra(lst);
    // RA = LST - HA = -3.0 -> should wrap to positive
    TEST_ASSERT_TRUE(ra >= 0.0);
    TEST_ASSERT_TRUE(ra < 2 * M_PI);
}

// =====================================================================
//  2. Coord_HO - construction and accessors
// =====================================================================
void test_ho_construction() {
    double frh = 0.0, alt = 0.7, az = 2.1;
    Coord_HO ho(frh, alt, az, false);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, frh, ho.FrH());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, alt, ho.Alt());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, az, ho.Az());
}

// =====================================================================
//  3. Coord_IN - construction and accessors
// =====================================================================
void test_in_construction() {
    double a3 = 0.1, a2 = 0.2, a1 = 0.3;
    Coord_IN in(a3, a2, a1);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, a3, in.Axis3());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, a2, in.Axis2());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, a1, in.Axis1());
}

void test_in_direct_axis1() {
    Coord_IN in(0.0, 0.0, 1.5);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, -1.5, in.Axis1_direct());
}

// =====================================================================
//  4. Coord_LO - construction and accessors
// =====================================================================
void test_lo_construction() {
    double a3 = 0.1, a2 = 0.2, a1 = 0.3;
    Coord_LO lo(a3, a2, a1);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, a3, lo.Axis3());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, a2, lo.Axis2());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, a1, lo.Axis1());
}

// =====================================================================
//  5. EQ <-> HO round-trip (no refraction)
// =====================================================================
void test_eq_ho_roundtrip() {
    double fre = 0.05, dec = 0.3, ha = 1.0;
    Coord_EQ eq1(fre, dec, ha);
    Coord_HO ho = eq1.To_Coord_HO(LAT_PARIS, NO_REFR);
    Coord_EQ eq2 = ho.To_Coord_EQ(LAT_PARIS);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, dec, eq2.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, ha, eq2.Ha());
}

void test_eq_ho_roundtrip_multiple_positions() {
    // Test several HA/Dec combinations
    double positions[][2] = {
        {0.0, M_PI / 4},     // transit, 45 dec
        {M_PI / 2, 0.0},     // HA=6h, equator
        {-M_PI / 3, 0.7},    // east of meridian
        {M_PI / 6, -0.3},    // southern dec
    };
    for (auto& pos : positions) {
        double ha = pos[0], dec = pos[1];
        Coord_EQ eq1(0.0, dec, ha);
        Coord_HO ho = eq1.To_Coord_HO(LAT_45, NO_REFR);
        Coord_EQ eq2 = ho.To_Coord_EQ(LAT_45);
        TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, dec, eq2.Dec());
        TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, ha, eq2.Ha());
    }
}

// =====================================================================
//  6. HO <-> EQ round-trip
// =====================================================================
void test_ho_eq_roundtrip() {
    double alt = 0.5, az = 1.5;
    Coord_HO ho1(0.0, alt, az, false);
    Coord_EQ eq = ho1.To_Coord_EQ(LAT_PARIS);
    Coord_HO ho2 = eq.To_Coord_HO(LAT_PARIS, NO_REFR);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, alt, ho2.Alt());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, az, ho2.Az());
}

// =====================================================================
//  7. HO <-> IN round-trip (identity misalignment)
// =====================================================================
void test_ho_in_roundtrip_identity() {
    initIdentity();
    double alt = 0.6, az = 2.0;
    Coord_HO ho1(0.0, alt, az, false);
    Coord_IN in = ho1.To_Coord_IN(IDENTITY);

    double inv[3][3];
    LA3::invert(inv, IDENTITY);
    Coord_HO ho2 = in.To_Coord_HO(inv, NO_REFR);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, alt, ho2.Alt());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, az, ho2.Az());
}

// =====================================================================
//  8. EQ -> IN -> EQ round-trip (identity misalignment)
// =====================================================================
void test_eq_in_roundtrip_identity() {
    initIdentity();
    double dec = 0.4, ha = 0.8;
    Coord_EQ eq1(0.0, dec, ha);
    Coord_IN in = eq1.To_Coord_IN(LAT_45, NO_REFR, IDENTITY);

    double inv[3][3];
    LA3::invert(inv, IDENTITY);
    Coord_EQ eq2 = in.To_Coord_EQ(inv, NO_REFR, LAT_45);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, dec, eq2.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, ha, eq2.Ha());
}

// =====================================================================
//  9. EQ <-> LO round-trip (identity transformation)
// =====================================================================
void test_eq_lo_roundtrip_identity() {
    initIdentity();
    double dec = 0.3, ha = 1.2;
    Coord_EQ eq1(0.0, dec, ha);

    double Tinv[3][3];
    LA3::invert(Tinv, IDENTITY);  // Tinv = I for identity
    Coord_LO lo = eq1.To_Coord_LO(Tinv);
    Coord_EQ eq2 = lo.To_Coord_EQ(Tinv);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, dec, eq2.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, ha, eq2.Ha());
}

// =====================================================================
//  10. Refraction: ToApparent / ToTopocentric round-trip
// =====================================================================
void test_ho_refraction_roundtrip() {
    double alt = 0.5, az = 1.0;
    Coord_HO topo(0.0, alt, az, false);

    Coord_HO apparent = topo.ToApparent(STD_REFR);
    TEST_ASSERT_TRUE(apparent.Alt() > topo.Alt());  // apparent is higher

    Coord_HO back = apparent.ToTopocentric(STD_REFR);
    // Meeus Saemundsson/Bennett pair: consistent to ~0.4 arcsecond
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, alt, back.Alt());
}

void test_ho_refraction_already_apparent() {
    // If already apparent, ToApparent should not change
    Coord_HO apparent(0.0, 0.5, 1.0, true);
    Coord_HO again = apparent.ToApparent(STD_REFR);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, apparent.Alt(), again.Alt());
}

void test_ho_refraction_already_topocentric() {
    // If already topocentric, ToTopocentric should not change
    Coord_HO topo(0.0, 0.5, 1.0, false);
    Coord_HO again = topo.ToTopocentric(STD_REFR);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, topo.Alt(), again.Alt());
}

// =====================================================================
//  11. EQ -> HO with refraction round-trip
// =====================================================================
void test_eq_ho_roundtrip_with_refraction() {
    double fre = 0.0, dec = 0.4, ha = 0.5;
    Coord_EQ eq1(fre, dec, ha);
    Coord_HO ho_app = eq1.To_Coord_HO(LAT_45, STD_REFR);

    // Convert back: HO (apparent) -> topocentric -> EQ
    Coord_HO ho_topo = ho_app.ToTopocentric(STD_REFR);
    Coord_EQ eq2 = ho_topo.To_Coord_EQ(LAT_45);

    // Meeus Saemundsson/Bennett pair: consistent to ~0.4 arcsecond
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, dec, eq2.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, ha, eq2.Ha());
}

// =====================================================================
//  12. Zenith test: at lat=phi, star at Dec=phi, HA=0 -> Alt=90
// =====================================================================
void test_zenith_star() {
    double lat = LAT_45;
    Coord_EQ eq(0.0, lat, 0.0);  // Dec = Lat, HA = 0
    Coord_HO ho = eq.To_Coord_HO(lat, NO_REFR);
    TEST_ASSERT_DOUBLE_WITHIN(TOL_CONV, M_PI / 2, ho.Alt());
}

// =====================================================================
//  13. Celestial pole test: at lat=45, Polaris (Dec~90) -> Alt~45
// =====================================================================
void test_pole_star_altitude() {
    double lat = LAT_45;
    double dec_pole = M_PI / 2 - 0.01;  // nearly 90 deg
    Coord_EQ eq(0.0, dec_pole, 0.0);
    Coord_HO ho = eq.To_Coord_HO(lat, NO_REFR);
    // Alt should be approximately equal to latitude
    TEST_ASSERT_DOUBLE_WITHIN(0.02, lat, ho.Alt());
}

// =====================================================================
//  main
// =====================================================================
void setUp()    {}
void tearDown() {}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Construction & accessors
    RUN_TEST(test_eq_construction);
    RUN_TEST(test_eq_direct_ha);
    RUN_TEST(test_eq_ra);
    RUN_TEST(test_eq_ra_wrap);
    RUN_TEST(test_ho_construction);
    RUN_TEST(test_in_construction);
    RUN_TEST(test_in_direct_axis1);
    RUN_TEST(test_lo_construction);

    // EQ <-> HO round-trips
    RUN_TEST(test_eq_ho_roundtrip);
    RUN_TEST(test_eq_ho_roundtrip_multiple_positions);
    RUN_TEST(test_ho_eq_roundtrip);

    // HO <-> IN round-trip
    RUN_TEST(test_ho_in_roundtrip_identity);

    // EQ <-> IN round-trip
    RUN_TEST(test_eq_in_roundtrip_identity);

    // EQ <-> LO round-trip
    RUN_TEST(test_eq_lo_roundtrip_identity);

    // Refraction
    RUN_TEST(test_ho_refraction_roundtrip);
    RUN_TEST(test_ho_refraction_already_apparent);
    RUN_TEST(test_ho_refraction_already_topocentric);
    RUN_TEST(test_eq_ho_roundtrip_with_refraction);

    // Astronomical identities
    RUN_TEST(test_zenith_star);
    RUN_TEST(test_pole_star_altitude);

    return UNITY_END();
}
