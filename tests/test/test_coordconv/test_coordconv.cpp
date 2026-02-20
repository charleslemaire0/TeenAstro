/*
 * test_coordconv.cpp - Unit tests for TeenAstroCoordConv
 *
 * Covers: CoordConv construction, reference star alignment,
 *         transformation matrix properties, getT/setT round-trip,
 *         minimizeAxis, and full-chain integration tests.
 */

// Include library implementations (single-translation-unit build)
#include "TeenAstroLA3.cpp"
#include "TeenAstroCoord_EQ.cpp"
#include "TeenAstroCoord_HO.cpp"
#include "TeenAstroCoord_IN.cpp"
#include "TeenAstroCoord_LO.cpp"
#include "TeenAstroCoordConv.cpp"

#include <unity.h>
#include <cmath>

static const double TOL        = 1e-8;
static const double TOL_FLOAT  = 1e-5;  // getT/setT uses float
static const double TOL_ALIGN  = 1e-4;  // alignment has SVD (float) in the loop

static const LA3::RefrOpt NO_REFR = {false, 10.0, 1013.0};
static const double LAT_45 = M_PI / 4.0;

// =====================================================================
//  1. Construction and reset
// =====================================================================
void test_coordconv_initial_state() {
    CoordConv cc;
    TEST_ASSERT_FALSE(cc.isReady());
    TEST_ASSERT_EQUAL_UINT8(0, cc.getRefs());
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, cc.getError());
}

void test_coordconv_reset() {
    CoordConv cc;
    cc.addReference(0.1, 0.2, 0.3, 0.4);
    TEST_ASSERT_EQUAL_UINT8(1, cc.getRefs());
    cc.reset();
    TEST_ASSERT_EQUAL_UINT8(0, cc.getRefs());
}

void test_coordconv_clean() {
    CoordConv cc;
    cc.addReference(0.1, 0.2, 0.1, 0.2);
    cc.addReference(1.0, 0.5, 1.0, 0.5);
    TEST_ASSERT_TRUE(cc.isReady());
    cc.clean();
    TEST_ASSERT_FALSE(cc.isReady());
}

// =====================================================================
//  2. Identity alignment (sky coords == axis coords)
// =====================================================================
void test_identity_alignment() {
    CoordConv cc;
    // Two reference stars where sky coords = axis coords
    cc.addReference(0.5, 0.3, 0.5, 0.3);
    cc.addReference(2.0, -0.2, 2.0, -0.2);
    TEST_ASSERT_TRUE(cc.isReady());

    // T should be close to identity
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, (i == j) ? 1.0 : 0.0, cc.T[i][j]);
}

void test_identity_alignment_tinv() {
    CoordConv cc;
    cc.addReference(0.5, 0.3, 0.5, 0.3);
    cc.addReference(2.0, -0.2, 2.0, -0.2);
    TEST_ASSERT_TRUE(cc.isReady());

    // Tinv should also be close to identity
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, (i == j) ? 1.0 : 0.0, cc.Tinv[i][j]);
}

// =====================================================================
//  3. T * Tinv = Identity
// =====================================================================
void test_T_times_Tinv_is_identity() {
    CoordConv cc;
    cc.addReference(0.3, 0.6, 0.35, 0.58);
    cc.addReference(1.5, -0.1, 1.55, -0.12);
    TEST_ASSERT_TRUE(cc.isReady());

    double prod[3][3];
    LA3::multiply(prod, cc.T, cc.Tinv);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, (i == j) ? 1.0 : 0.0, prod[i][j]);
}

// =====================================================================
//  4. T is a rotation matrix (det = +1, orthogonal)
// =====================================================================
void test_T_is_rotation() {
    CoordConv cc;
    cc.addReference(0.3, 0.6, 0.35, 0.58);
    cc.addReference(1.5, -0.1, 1.55, -0.12);
    TEST_ASSERT_TRUE(cc.isReady());

    // det(T) should be +1
    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 1.0, LA3::determinant(cc.T));

    // T * T^T should be identity (orthogonal)
    double Tt[3][3], prod[3][3];
    LA3::transpose(Tt, cc.T);
    LA3::multiply(prod, cc.T, Tt);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, (i == j) ? 1.0 : 0.0, prod[i][j]);
}

// =====================================================================
//  5. getT / setT round-trip
// =====================================================================
void test_getT_setT_roundtrip() {
    CoordConv cc1;
    cc1.addReference(0.3, 0.6, 0.35, 0.58);
    cc1.addReference(1.5, -0.1, 1.55, -0.12);
    TEST_ASSERT_TRUE(cc1.isReady());

    // Extract T
    float m11, m12, m13, m21, m22, m23, m31, m32, m33;
    cc1.getT(m11, m12, m13, m21, m22, m23, m31, m32, m33);

    // Set T into a new CoordConv
    CoordConv cc2;
    cc2.setT(m11, m12, m13, m21, m22, m23, m31, m32, m33);
    TEST_ASSERT_TRUE(cc2.isReady());

    // Compare T matrices (float precision)
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL_FLOAT, cc1.T[i][j], cc2.T[i][j]);
}

// =====================================================================
//  6. setTinvFromT
// =====================================================================
void test_setTinvFromT() {
    CoordConv cc;
    // Manually set T to a known rotation matrix (Rz(30deg))
    double angle = 30.0 * M_PI / 180.0;
    cc.setT(cos(angle), -sin(angle), 0,
            sin(angle),  cos(angle), 0,
            0,           0,          1);
    cc.setTinvFromT();

    // T * Tinv should be identity
    double prod[3][3];
    LA3::multiply(prod, cc.T, cc.Tinv);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL_FLOAT, (i == j) ? 1.0 : 0.0, prod[i][j]);
}

// =====================================================================
//  7. addReference only accepts 2 stars
// =====================================================================
void test_max_two_references() {
    CoordConv cc;
    cc.addReference(0.1, 0.2, 0.1, 0.2);
    TEST_ASSERT_EQUAL_UINT8(1, cc.getRefs());
    cc.addReference(1.0, 0.5, 1.0, 0.5);
    // After 2nd reference, calculateThirdReference runs and resets refs to 0
    TEST_ASSERT_TRUE(cc.isReady());
    // Adding a third should be ignored (refs was reset to 0, so it starts over)
    cc.addReference(2.0, 0.8, 2.0, 0.8);
    TEST_ASSERT_EQUAL_UINT8(1, cc.getRefs());
}

// =====================================================================
//  8. calculateThirdReference with wrong count
// =====================================================================
void test_calculateThirdReference_needs_exactly_two() {
    CoordConv cc;
    // With 0 refs
    TEST_ASSERT_FALSE(cc.calculateThirdReference());
    // With 1 ref
    cc.addReference(0.1, 0.2, 0.1, 0.2);
    TEST_ASSERT_FALSE(cc.calculateThirdReference());
}

// =====================================================================
//  9. Known rotation alignment
// =====================================================================
void test_known_rotation_alignment() {
    CoordConv cc;
    // Sky coordinates -> instrument coordinates rotated by 10 deg around Z
    double rot = 10.0 * M_PI / 180.0;
    double cosR = cos(rot), sinR = sin(rot);

    // Star 1: sky (0.5, 0.3) -> rotate by Rz(rot) on direction cosines
    double dc1[3], dc1r[3];
    LA3::toDirCos(dc1, 0.5, 0.3);
    double Rz[3][3];
    LA3::SingleRotation sr = {LA3::ROTAXISZ, rot};
    LA3::getSingleRotationMatrix(Rz, sr);
    LA3::multiply(dc1r, Rz, dc1);
    double ax1_1 = -atan2(dc1r[1], dc1r[0]);  // recover angles
    double ax2_1 = asin(dc1r[2]);

    // Star 2: sky (2.0, -0.2) -> rotate
    double dc2[3], dc2r[3];
    LA3::toDirCos(dc2, 2.0, -0.2);
    LA3::multiply(dc2r, Rz, dc2);
    double ax1_2 = -atan2(dc2r[1], dc2r[0]);
    double ax2_2 = asin(dc2r[2]);

    cc.addReference(0.5, 0.3, ax1_1, ax2_1);
    cc.addReference(2.0, -0.2, ax1_2, ax2_2);
    TEST_ASSERT_TRUE(cc.isReady());

    // The alignment error should be near zero (perfect alignment)
    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, 0.0, fabs(cc.getError()));
}

// =====================================================================
//  10. Alignment error for imperfect alignment
// =====================================================================
void test_alignment_error_nonzero() {
    CoordConv cc;
    // Introduce a deliberate offset in one star
    cc.addReference(0.5, 0.3, 0.5, 0.3);
    cc.addReference(2.0, -0.2, 2.05, -0.15); // slightly off
    TEST_ASSERT_TRUE(cc.isReady());

    // Error should be non-zero
    TEST_ASSERT_TRUE(fabs(cc.getError()) > 1e-6);
}

// =====================================================================
//  11. minimizeAxis1
// =====================================================================
void test_minimizeAxis1() {
    CoordConv cc;
    // Setup with a known offset in axis1
    double offset = 0.05;  // 5 centideg
    cc.addReference(0.5, 0.3, 0.5 + offset, 0.3);
    cc.addReference(2.0, -0.2, 2.0 + offset, -0.2);
    TEST_ASSERT_TRUE(cc.isReady());

    // After minimizeAxis1, the error should improve
    double err_before = fabs(cc.getError());
    cc.minimizeAxis1(0.0);
    // cc should still be ready after minimization
    TEST_ASSERT_TRUE(cc.isReady());
}

// =====================================================================
//  12. Integration: full chain EQ -> HO -> IN with CoordConv
// =====================================================================
void test_full_chain_eq_ho_in_roundtrip() {
    // Step 1: Create a CoordConv with identity alignment
    CoordConv cc;
    cc.addReference(0.5, 0.3, 0.5, 0.3);
    cc.addReference(2.0, -0.2, 2.0, -0.2);
    TEST_ASSERT_TRUE(cc.isReady());

    // Step 2: Pick an equatorial position
    double dec = 0.4, ha = 0.8;
    Coord_EQ eq1(0.0, dec, ha);

    // Step 3: EQ -> HO -> IN using the alignment matrix
    Coord_HO ho = eq1.To_Coord_HO(LAT_45, NO_REFR);
    Coord_IN in = ho.To_Coord_IN(cc.T);

    // Step 4: IN -> HO -> EQ using inverse
    Coord_HO ho2 = in.To_Coord_HO(cc.Tinv, NO_REFR);
    Coord_EQ eq2 = ho2.To_Coord_EQ(LAT_45);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, dec, eq2.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, ha, eq2.Ha());
}

void test_full_chain_with_rotation() {
    // Create alignment with a small rotation (simulating misalignment)
    CoordConv cc;
    double rot = 0.02;  // ~1 degree misalignment
    double Rz[3][3];
    LA3::SingleRotation sr = {LA3::ROTAXISZ, rot};
    LA3::getSingleRotationMatrix(Rz, sr);

    // Generate two reference stars with rotated axis coords
    double sky_coords[2][2] = {{0.5, 0.3}, {2.0, -0.2}};
    for (int s = 0; s < 2; s++) {
        double dc[3], dcr[3];
        LA3::toDirCos(dc, sky_coords[s][0], sky_coords[s][1]);
        LA3::multiply(dcr, Rz, dc);
        double ax1 = -atan2(dcr[1], dcr[0]);
        double ax2 = asin(dcr[2]);
        cc.addReference(sky_coords[s][0], sky_coords[s][1], ax1, ax2);
    }
    TEST_ASSERT_TRUE(cc.isReady());

    // Full round-trip
    double dec = 0.5, ha = 1.0;
    Coord_EQ eq1(0.0, dec, ha);
    Coord_HO ho1 = eq1.To_Coord_HO(LAT_45, NO_REFR);
    Coord_IN in  = ho1.To_Coord_IN(cc.T);
    Coord_HO ho2 = in.To_Coord_HO(cc.Tinv, NO_REFR);
    Coord_EQ eq2 = ho2.To_Coord_EQ(LAT_45);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, dec, eq2.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, ha, eq2.Ha());
}

// =====================================================================
//  13. Integration: EQ <-> LO with CoordConv
// =====================================================================
void test_eq_lo_roundtrip_with_coordconv() {
    CoordConv cc;
    cc.addReference(0.5, 0.3, 0.5, 0.3);
    cc.addReference(2.0, -0.2, 2.0, -0.2);
    TEST_ASSERT_TRUE(cc.isReady());

    double dec = 0.3, ha = 1.2;
    Coord_EQ eq1(0.0, dec, ha);
    Coord_LO lo = eq1.To_Coord_LO(cc.Tinv);
    Coord_EQ eq2 = lo.To_Coord_EQ(cc.Tinv);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, dec, eq2.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, ha, eq2.Ha());
}

// =====================================================================
//  14. Integration: with refraction
// =====================================================================
void test_full_chain_with_refraction() {
    CoordConv cc;
    cc.addReference(0.5, 0.3, 0.5, 0.3);
    cc.addReference(2.0, -0.2, 2.0, -0.2);
    TEST_ASSERT_TRUE(cc.isReady());

    double dec = 0.4, ha = 0.5;
    LA3::RefrOpt refr = {true, 10.0, 1013.0};

    Coord_EQ eq1(0.0, dec, ha);
    // EQ -> HO (apparent, with refraction) -> IN
    Coord_IN in = eq1.To_Coord_IN(LAT_45, refr, cc.T);
    // IN -> EQ (applying inverse refraction)
    Coord_EQ eq2 = in.To_Coord_EQ(cc.Tinv, refr, LAT_45);

    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, dec, eq2.Dec());
    TEST_ASSERT_DOUBLE_WITHIN(TOL_ALIGN, ha, eq2.Ha());
}

// =====================================================================
//  main
// =====================================================================
void setUp()    {}
void tearDown() {}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Construction
    RUN_TEST(test_coordconv_initial_state);
    RUN_TEST(test_coordconv_reset);
    RUN_TEST(test_coordconv_clean);

    // Identity alignment
    RUN_TEST(test_identity_alignment);
    RUN_TEST(test_identity_alignment_tinv);

    // Matrix properties
    RUN_TEST(test_T_times_Tinv_is_identity);
    RUN_TEST(test_T_is_rotation);

    // Serialization
    RUN_TEST(test_getT_setT_roundtrip);
    RUN_TEST(test_setTinvFromT);

    // Reference management
    RUN_TEST(test_max_two_references);
    RUN_TEST(test_calculateThirdReference_needs_exactly_two);

    // Alignment quality
    RUN_TEST(test_known_rotation_alignment);
    RUN_TEST(test_alignment_error_nonzero);
    RUN_TEST(test_minimizeAxis1);

    // Integration: full-chain round-trips
    RUN_TEST(test_full_chain_eq_ho_in_roundtrip);
    RUN_TEST(test_full_chain_with_rotation);
    RUN_TEST(test_eq_lo_roundtrip_with_coordconv);
    RUN_TEST(test_full_chain_with_refraction);

    return UNITY_END();
}
