/*
 * test_la3.cpp - Unit tests for TeenAstroLA3 (linear algebra library)
 *
 * Covers: modRad, vector ops, matrix ops, rotation matrices,
 *         Euler angle extraction (round-trips), refraction, SVD.
 */

// Include library implementation (single-translation-unit build)
#include "TeenAstroLA3.cpp"

#include <unity.h>
#include <cmath>

// ---------------------------------------------------------------------------
//  Tolerance helpers
// ---------------------------------------------------------------------------
static const double TOL     = 1e-10;
static const double TOL_SVD = 1e-4;   // SVD uses float internally

// Compare two angles mod 2*PI
static void assertAngleNear(double expected, double actual, double tol, const char* msg) {
    double diff = fmod(actual - expected + 3 * M_PI, 2 * M_PI) - M_PI;
    char buf[128];
    snprintf(buf, sizeof(buf), "%s  (exp %.8f  got %.8f)", msg, expected, actual);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(tol, 0.0, diff, buf);
}

// =====================================================================
//  1. modRad
// =====================================================================
void test_modRad_zero()       { TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, LA3::modRad(0.0)); }
void test_modRad_2pi()        { TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, LA3::modRad(2 * M_PI)); }
void test_modRad_neg2pi()     { TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, LA3::modRad(-2 * M_PI)); }
void test_modRad_positive()   { TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, LA3::modRad(1.0 + 2 * M_PI)); }
void test_modRad_negative()   { TEST_ASSERT_DOUBLE_WITHIN(TOL, -1.0, LA3::modRad(-1.0 - 2 * M_PI)); }

// =====================================================================
//  2. Vector operations
// =====================================================================
void test_dotProduct_orthogonal() {
    double a[3] = {1, 0, 0};
    double b[3] = {0, 1, 0};
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, LA3::dotProduct(a, b));
}
void test_dotProduct_parallel() {
    double a[3] = {1, 2, 3};
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 14.0, LA3::dotProduct(a, a));
}

void test_crossProduct_ij() {
    double a[3] = {1, 0, 0};
    double b[3] = {0, 1, 0};
    double out[3];
    LA3::crossProduct(out, a, b);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[0]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[1]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, out[2]);
}
void test_crossProduct_anticommutative() {
    double a[3] = {1, 2, 3};
    double b[3] = {4, 5, 6};
    double ab[3], ba[3];
    LA3::crossProduct(ab, a, b);
    LA3::crossProduct(ba, b, a);
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_DOUBLE_WITHIN(TOL, -ab[i], ba[i]);
}

void test_angle2Vectors_same() {
    double a[3] = {1, 0, 0};
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, LA3::angle2Vectors(a, a));
}
void test_angle2Vectors_perpendicular() {
    double a[3] = {1, 0, 0};
    double b[3] = {0, 1, 0};
    TEST_ASSERT_DOUBLE_WITHIN(TOL, M_PI / 2, LA3::angle2Vectors(a, b));
}
void test_angle2Vectors_opposite() {
    double a[3] = {1, 0, 0};
    double b[3] = {-1, 0, 0};
    TEST_ASSERT_DOUBLE_WITHIN(TOL, M_PI, LA3::angle2Vectors(a, b));
}

void test_norm_unit() {
    double a[3] = {1, 0, 0};
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, LA3::norm(a));
}
void test_norm_known() {
    double a[3] = {3, 4, 0};
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 5.0, LA3::norm(a));
}

void test_normalize() {
    double a[3] = {3, 4, 0};
    double out[3];
    LA3::normalize(out, a);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, LA3::norm(out));
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.6, out[0]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.8, out[1]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[2]);
}

// =====================================================================
//  3. toDirCos
// =====================================================================
void test_toDirCos_origin() {
    double dc[3];
    LA3::toDirCos(dc, 0.0, 0.0);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, dc[0]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, dc[1]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, dc[2]);
}
void test_toDirCos_pole() {
    double dc[3];
    LA3::toDirCos(dc, 0.0, M_PI / 2);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, dc[0]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, dc[1]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, dc[2]);
}
void test_toDirCos_unit_length() {
    double dc[3];
    LA3::toDirCos(dc, 1.23, 0.45);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, LA3::norm(dc));
}

// =====================================================================
//  4. toRad / toDeg / normalizeRads
// =====================================================================
void test_toRad()          { TEST_ASSERT_DOUBLE_WITHIN(TOL, M_PI, LA3::toRad(180.0)); }
void test_toDeg()          { TEST_ASSERT_DOUBLE_WITHIN(TOL, 180.0, LA3::toDeg(M_PI)); }
void test_normalizeRads_positive() { TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, LA3::normalizeRads(1.0 + 4 * M_PI)); }
void test_normalizeRads_negative() { TEST_ASSERT_DOUBLE_WITHIN(TOL, 2 * M_PI - 1.0, LA3::normalizeRads(-1.0)); }

// =====================================================================
//  5. Matrix operations
// =====================================================================
void test_identity() {
    double I[3][3];
    LA3::getIdentityMatrix(I);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL, (i == j) ? 1.0 : 0.0, I[i][j]);
}

void test_copy() {
    double A[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
    double B[3][3];
    LA3::copy(B, A);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL, A[i][j], B[i][j]);
}

void test_transpose() {
    double A[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
    double At[3][3];
    LA3::transpose(At, A);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL, A[j][i], At[i][j]);
}

void test_transpose_double_is_original() {
    double A[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
    double At[3][3], Att[3][3];
    LA3::transpose(At, A);
    LA3::transpose(Att, At);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL, A[i][j], Att[i][j]);
}

void test_determinant_identity() {
    double I[3][3];
    LA3::getIdentityMatrix(I);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, LA3::determinant(I));
}

void test_determinant_known() {
    double A[3][3] = {{1,2,3},{0,1,4},{5,6,0}};
    // det = 1*(0-24) - 2*(0-20) + 3*(0-5) = -24 + 40 - 15 = 1
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, LA3::determinant(A));
}

void test_invert_identity() {
    double I[3][3], Iinv[3][3];
    LA3::getIdentityMatrix(I);
    LA3::invert(Iinv, I);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL, (i == j) ? 1.0 : 0.0, Iinv[i][j]);
}

void test_invert_product_is_identity() {
    double A[3][3] = {{1,2,3},{0,1,4},{5,6,0}};
    double Ainv[3][3], prod[3][3];
    LA3::invert(Ainv, A);
    LA3::multiply(prod, A, Ainv);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL, (i == j) ? 1.0 : 0.0, prod[i][j]);
}

void test_multiply_matrices_identity() {
    double A[3][3] = {{1,2,3},{4,5,6},{7,8,9}};
    double I[3][3], prod[3][3];
    LA3::getIdentityMatrix(I);
    LA3::multiply(prod, A, I);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL, A[i][j], prod[i][j]);
}

void test_multiply_matrix_vector_identity() {
    double I[3][3], v[3] = {1, 2, 3}, out[3];
    LA3::getIdentityMatrix(I);
    LA3::multiply(out, I, v);
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_DOUBLE_WITHIN(TOL, v[i], out[i]);
}

void test_multiply_matrix_vector_known() {
    double A[3][3] = {{1,0,0},{0,2,0},{0,0,3}};
    double v[3] = {1, 1, 1}, out[3];
    LA3::multiply(out, A, v);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, out[0]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 2.0, out[1]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 3.0, out[2]);
}

// =====================================================================
//  6. Rotation matrices
// =====================================================================
void test_rotation_X_90() {
    LA3::SingleRotation sr = {LA3::ROTAXISX, M_PI / 2};
    double R[3][3];
    LA3::getSingleRotationMatrix(R, sr);
    // Rx(90) should map (0,1,0) -> (0,0,1)
    double v[3] = {0, 1, 0}, out[3];
    LA3::multiply(out, R, v);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[0]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[1]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, out[2]);
}

void test_rotation_Y_90() {
    LA3::SingleRotation sr = {LA3::ROTAXISY, M_PI / 2};
    double R[3][3];
    LA3::getSingleRotationMatrix(R, sr);
    // Ry(90) should map (1,0,0) -> (0,0,-1)
    double v[3] = {1, 0, 0}, out[3];
    LA3::multiply(out, R, v);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[0]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[1]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, -1.0, out[2]);
}

void test_rotation_Z_90() {
    LA3::SingleRotation sr = {LA3::ROTAXISZ, M_PI / 2};
    double R[3][3];
    LA3::getSingleRotationMatrix(R, sr);
    // Rz(90) should map (1,0,0) -> (0,1,0)
    double v[3] = {1, 0, 0}, out[3];
    LA3::multiply(out, R, v);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[0]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, out[1]);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 0.0, out[2]);
}

void test_rotation_determinant_is_one() {
    LA3::SingleRotation sr = {LA3::ROTAXISZ, 0.789};
    double R[3][3];
    LA3::getSingleRotationMatrix(R, sr);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, 1.0, LA3::determinant(R));
}

void test_multiple_rotation_composition() {
    LA3::SingleRotation rots[2] = {
        {LA3::ROTAXISX, M_PI / 4},
        {LA3::ROTAXISY, M_PI / 3}
    };
    double composed[3][3];
    LA3::getMultipleRotationMatrix(composed, rots, 2);

    // Verify against manual multiplication
    double Rx[3][3], Ry[3][3], manual[3][3];
    LA3::getSingleRotationMatrix(Rx, rots[0]);
    LA3::getSingleRotationMatrix(Ry, rots[1]);
    LA3::multiply(manual, Rx, Ry);

    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL, manual[i][j], composed[i][j]);
}

// =====================================================================
//  7. Euler angle extraction (round-trip tests)
// =====================================================================
void test_euler_RzRxRy_roundtrip() {
    double a1 = 0.3, a2 = 0.5, a3 = -0.7;
    LA3::SingleRotation rots[3] = {
        {LA3::ROTAXISZ, a1},
        {LA3::ROTAXISX, a2},
        {LA3::ROTAXISY, a3}
    };
    double m[3][3];
    LA3::getMultipleRotationMatrix(m, rots, 3);
    double e1, e2, e3;
    LA3::getEulerRzRxRy(m, e1, e2, e3);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a1, e1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a2, e2);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a3, e3);
}

void test_euler_RzRyRx_roundtrip() {
    double a1 = -0.4, a2 = 0.6, a3 = 0.2;
    LA3::SingleRotation rots[3] = {
        {LA3::ROTAXISZ, a1},
        {LA3::ROTAXISY, a2},
        {LA3::ROTAXISX, a3}
    };
    double m[3][3];
    LA3::getMultipleRotationMatrix(m, rots, 3);
    double e1, e2, e3;
    LA3::getEulerRzRyRx(m, e1, e2, e3);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a1, e1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a2, e2);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a3, e3);
}

void test_euler_RxRyRz_roundtrip() {
    double a1 = 0.1, a2 = -0.3, a3 = 0.8;
    LA3::SingleRotation rots[3] = {
        {LA3::ROTAXISX, a1},
        {LA3::ROTAXISY, a2},
        {LA3::ROTAXISZ, a3}
    };
    double m[3][3];
    LA3::getMultipleRotationMatrix(m, rots, 3);
    double e1, e2, e3;
    LA3::getEulerRxRyRz(m, e1, e2, e3);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a1, e1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a2, e2);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a3, e3);
}

void test_euler_Rx0RyRx1_roundtrip() {
    double a0 = 0.2, ay = 0.9, a1 = -0.5;
    LA3::SingleRotation rots[3] = {
        {LA3::ROTAXISX, a0},
        {LA3::ROTAXISY, ay},
        {LA3::ROTAXISX, a1}
    };
    double m[3][3];
    LA3::getMultipleRotationMatrix(m, rots, 3);
    double e0, ey, e1;
    LA3::getEulerRx0RyRx1(m, e0, ey, e1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a0, e0);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, ay, ey);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, a1, e1);
}

// =====================================================================
//  8. Refraction  (Meeus Saemundsson/Bennett pair)
// =====================================================================
// Standard conditions: 10 °C, 1010 mbar  (Meeus standard atmosphere)
static const LA3::RefrOpt STD_REFR_LA3 = {true, 10.0, 1010.0};

// Helper: arcminutes -> radians
static double arcmin2rad(double am) { return am * M_PI / (180.0 * 60.0); }

void test_refraction_roundtrip_45() {
    double alt = LA3::toRad(45.0);
    double original = alt;
    LA3::Topocentric2Apparent(alt, STD_REFR_LA3);
    TEST_ASSERT_TRUE(alt > original);
    LA3::Apparent2Topocentric(alt, STD_REFR_LA3);
    // Saemundsson/Bennett pair: consistent to ~0.4 arcsecond = 7e-6 rad
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, original, alt);
}

void test_refraction_roundtrip_5() {
    double alt = LA3::toRad(5.0);
    double original = alt;
    LA3::Topocentric2Apparent(alt, STD_REFR_LA3);
    LA3::Apparent2Topocentric(alt, STD_REFR_LA3);
    // At low altitude the Saemundsson/Bennett discrepancy grows to ~2-3 arcsec
    TEST_ASSERT_DOUBLE_WITHIN(2e-5, original, alt);
}

void test_refraction_roundtrip_80() {
    double alt = LA3::toRad(80.0);
    double original = alt;
    LA3::Topocentric2Apparent(alt, STD_REFR_LA3);
    LA3::Apparent2Topocentric(alt, STD_REFR_LA3);
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, original, alt);
}

void test_refraction_disabled() {
    LA3::RefrOpt opt = {false, 10.0, 1010.0};
    double alt = LA3::toRad(30.0);
    double original = alt;
    LA3::Topocentric2Apparent(alt, opt);
    TEST_ASSERT_DOUBLE_WITHIN(TOL, original, alt);
}

void test_refraction_low_altitude_larger() {
    double alt5 = LA3::toRad(5.0);
    double orig5 = alt5;
    LA3::Topocentric2Apparent(alt5, STD_REFR_LA3);
    double correction_5 = alt5 - orig5;

    double alt60 = LA3::toRad(60.0);
    double orig60 = alt60;
    LA3::Topocentric2Apparent(alt60, STD_REFR_LA3);
    double correction_60 = alt60 - orig60;

    // At low altitude, refraction is larger
    TEST_ASSERT_TRUE(correction_5 > correction_60);
}

void test_refraction_zenith_zero() {
    // At zenith (90 deg), refraction should be essentially zero
    double alt = LA3::toRad(90.0);
    double original = alt;
    LA3::Topocentric2Apparent(alt, STD_REFR_LA3);
    TEST_ASSERT_DOUBLE_WITHIN(arcmin2rad(0.01), original, alt);
}

void test_refraction_nautical_almanac_horizon() {
    // Nautical Almanac: refraction at the horizon (h=0) ~ 34.5 arcmin
    // Saemundsson at h=0: R = 1.02/tan(10.3/5.11 deg) + 0.0019279 ~ 28.75 arcmin
    // (The 34.5 value is for apparent horizon; Saemundsson gives ~28.8 for true h=0)
    // At standard conditions (P=1010, T=10), Saemundsson gives ~28.8 arcmin at true alt=0
    double alt = 0.0;  // true altitude = 0 radians
    LA3::Topocentric2Apparent(alt, STD_REFR_LA3);
    double refraction_arcmin = alt * (180.0 * 60.0 / M_PI);
    // Should be approximately 28.8 arcminutes (Saemundsson at h=0)
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 28.8, refraction_arcmin);
}

void test_refraction_nautical_almanac_45() {
    // At 45 degrees, refraction ~ 58 arcseconds = 0.97 arcmin (Nautical Almanac)
    double alt = LA3::toRad(45.0);
    double original = alt;
    LA3::Topocentric2Apparent(alt, STD_REFR_LA3);
    double refraction_arcmin = (alt - original) * (180.0 * 60.0 / M_PI);
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 0.97, refraction_arcmin);
}

void test_refraction_pressure_temperature_scaling() {
    // Higher pressure -> more refraction; higher temperature -> less refraction
    double alt_std = LA3::toRad(30.0);
    double alt_high_p = alt_std;
    double alt_high_t = alt_std;
    double orig = alt_std;
    LA3::Topocentric2Apparent(alt_std, STD_REFR_LA3);
    LA3::RefrOpt high_p = {true, 10.0, 1050.0};
    LA3::Topocentric2Apparent(alt_high_p, high_p);
    LA3::RefrOpt high_t = {true, 30.0, 1010.0};
    LA3::Topocentric2Apparent(alt_high_t, high_t);
    TEST_ASSERT_TRUE((alt_high_p - orig) > (alt_std - orig));  // more pressure -> more refraction
    TEST_ASSERT_TRUE((alt_high_t - orig) < (alt_std - orig));  // more temperature -> less refraction
}

// =====================================================================
//  9. SVD (basic smoke test - SVD uses float internally)
// =====================================================================
void test_svd_identity() {
    double I[3][3];
    LA3::getIdentityMatrix(I);
    double u[3][3], v[3][3];
    LA3::getsvd(I, u, v);
    // For identity: U and V should both be close to identity (or reflections)
    // Check that U * V^T ~ I
    double vt[3][3], prod[3][3];
    LA3::transpose(vt, v);
    LA3::multiply(prod, u, vt);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL_SVD, (i == j) ? 1.0 : 0.0, fabs(prod[i][j]));
}

void test_svd_rotation() {
    // SVD of a rotation matrix: U*S*V^T = R, with S = I
    LA3::SingleRotation sr = {LA3::ROTAXISZ, 0.5};
    double R[3][3];
    LA3::getSingleRotationMatrix(R, sr);
    double u[3][3], v[3][3];
    LA3::getsvd(R, u, v);
    // U * V^T should reconstruct R
    double vt[3][3], prod[3][3];
    LA3::transpose(vt, v);
    LA3::multiply(prod, u, vt);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(TOL_SVD, R[i][j], prod[i][j]);
}

// =====================================================================
//  main
// =====================================================================
void setUp()    {}
void tearDown() {}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // modRad
    RUN_TEST(test_modRad_zero);
    RUN_TEST(test_modRad_2pi);
    RUN_TEST(test_modRad_neg2pi);
    RUN_TEST(test_modRad_positive);
    RUN_TEST(test_modRad_negative);

    // Vector ops
    RUN_TEST(test_dotProduct_orthogonal);
    RUN_TEST(test_dotProduct_parallel);
    RUN_TEST(test_crossProduct_ij);
    RUN_TEST(test_crossProduct_anticommutative);
    RUN_TEST(test_angle2Vectors_same);
    RUN_TEST(test_angle2Vectors_perpendicular);
    RUN_TEST(test_angle2Vectors_opposite);
    RUN_TEST(test_norm_unit);
    RUN_TEST(test_norm_known);
    RUN_TEST(test_normalize);

    // Direction cosines
    RUN_TEST(test_toDirCos_origin);
    RUN_TEST(test_toDirCos_pole);
    RUN_TEST(test_toDirCos_unit_length);

    // Conversions
    RUN_TEST(test_toRad);
    RUN_TEST(test_toDeg);
    RUN_TEST(test_normalizeRads_positive);
    RUN_TEST(test_normalizeRads_negative);

    // Matrix ops
    RUN_TEST(test_identity);
    RUN_TEST(test_copy);
    RUN_TEST(test_transpose);
    RUN_TEST(test_transpose_double_is_original);
    RUN_TEST(test_determinant_identity);
    RUN_TEST(test_determinant_known);
    RUN_TEST(test_invert_identity);
    RUN_TEST(test_invert_product_is_identity);
    RUN_TEST(test_multiply_matrices_identity);
    RUN_TEST(test_multiply_matrix_vector_identity);
    RUN_TEST(test_multiply_matrix_vector_known);

    // Rotation
    RUN_TEST(test_rotation_X_90);
    RUN_TEST(test_rotation_Y_90);
    RUN_TEST(test_rotation_Z_90);
    RUN_TEST(test_rotation_determinant_is_one);
    RUN_TEST(test_multiple_rotation_composition);

    // Euler angles
    RUN_TEST(test_euler_RzRxRy_roundtrip);
    RUN_TEST(test_euler_RzRyRx_roundtrip);
    RUN_TEST(test_euler_RxRyRz_roundtrip);
    RUN_TEST(test_euler_Rx0RyRx1_roundtrip);

    // Refraction (Meeus Saemundsson/Bennett)
    RUN_TEST(test_refraction_roundtrip_45);
    RUN_TEST(test_refraction_roundtrip_5);
    RUN_TEST(test_refraction_roundtrip_80);
    RUN_TEST(test_refraction_disabled);
    RUN_TEST(test_refraction_low_altitude_larger);
    RUN_TEST(test_refraction_zenith_zero);
    RUN_TEST(test_refraction_nautical_almanac_horizon);
    RUN_TEST(test_refraction_nautical_almanac_45);
    RUN_TEST(test_refraction_pressure_temperature_scaling);

    // SVD
    RUN_TEST(test_svd_identity);
    RUN_TEST(test_svd_rotation);

    return UNITY_END();
}
