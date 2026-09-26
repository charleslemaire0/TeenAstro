/*
 * test_rigid_model.cpp - Unit tests for the rigid six degree of freedom
 * pointing model (T plus the three head terms: cone, perp, axis2 index).
 *
 * Covers:
 *   1. A zero head reproduces the legacy T only conversions exactly.
 *   2. HeadGeom forward / inverse are exact inverses.
 *   3. Coord_HO -> Coord_IN -> Coord_HO round trips with a head model.
 *   4. Agreement with OnStep's GeoAlign for small errors (strict, sub arcsec).
 *   5. TeenAstro is closer to the truth than OnStep for large errors.
 *   6. CoordConv::fitRigidModel recovers injected head terms from synthetic stars.
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

static const LA3::RefrOpt NO_REFR = { false, 10.0, 1013.0 };

static const double ARCSEC = M_PI / (180.0 * 3600.0);
static const double ARCMIN = M_PI / (180.0 * 60.0);
static const double DEG    = M_PI / 180.0;

// =====================================================================
//  OnStep GeoAlign reference implementation (rigid subset)
//
//  Transcribed from OnStepX src/telescope/mount/coordinates/Align.ref.cpp,
//  keeping only the rigid terms: doCor (cone), pdCor (axis2 vs axis1) and the
//  axis index offsets. Flexure (tfCor, dfCor) and the harmonic terms are
//  deliberately omitted, and polar error is kept at zero so that the
//  comparison isolates the non-perpendicularity behaviour.
//
//  OnStep works directly in hour angle / declination and expresses each error
//  as a small angular offset, which is why 1/cos and tan factors appear. It is
//  a first order expansion of the exact rotation chain.
// =====================================================================
struct OnStepModel {
    double ax1Cor, ax2Cor, doCor, pdCor;
    OnStepModel() : ax1Cor(0), ax2Cor(0), doCor(0), pdCor(0) {}
};

/// OnStep observedPlaceToMount, rigid subset. (h, d) observed -> (ax1, ax2) mount.
static void onstepObservedPlaceToMount(const OnStepModel &m, double h, double d,
                                       double pierSideSign, double &ax1, double &ax2)
{
    double a1 = h;
    double a2 = d;
    if (fabs(d) < 89.98333333 * DEG) {
        for (int pass = 0; pass < 3; pass++) {
            const double sinA2 = sin(a2), cosA2 = cos(a2);
            const double DOh = m.doCor * (1.0 / cosA2) * pierSideSign;
            const double PDh = -m.pdCor * (sinA2 / cosA2) * pierSideSign;
            a1 = h + (PDh + DOh);
            a2 = d;
        }
    }
    ax1 = a1 - m.ax1Cor;
    ax2 = a2 - m.ax2Cor * -pierSideSign;
}

/// OnStep mountToObservedPlace, rigid subset. (ax1, ax2) mount -> (h, d) observed.
static void onstepMountToObservedPlace(const OnStepModel &m, double ax1In, double ax2In,
                                       double pierSideSign, double &h, double &d)
{
    double ax1 = ax1In + m.ax1Cor;
    double ax2 = ax2In + m.ax2Cor * -pierSideSign;
    if (fabs(ax2) < 89.98333333 * DEG) {
        const double sinAx2 = sin(ax2), cosAx2 = cos(ax2);
        const double DOh = m.doCor * (1.0 / cosAx2) * pierSideSign;
        const double PDh = -m.pdCor * (sinAx2 / cosAx2) * pierSideSign;
        ax1 = ax1 - (PDh + DOh);
    }
    h = ax1;
    d = ax2;
}

// =====================================================================
//  Sign mapping between the two parameterisations
//
//  Derived analytically and confirmed numerically by the agreement tests
//  below. For pier side east (OnStep p = +1):
//      cone = -doCor      perp = -pdCor      idx2 = -ax2Cor
//  The axis1 index has no head counterpart; it is absorbed by T.
// =====================================================================
static HeadModel headFromOnStep(const OnStepModel &m)
{
    return HeadModel(-m.doCor, -m.pdCor, -m.ax2Cor);
}

/// TeenAstro exact model: target (h, d) -> mount axes, with T == identity.
/// Returns false when the direction is unreachable for this head geometry.
static bool teenastroObservedPlaceToMount(const HeadModel &head, double h, double d,
                                          double &axis1, double &axis2)
{
    // In toDirCos terms the instrument frame uses ang1 = axis1Direct = -axis1.
    double dc[3];
    LA3::toDirCos(dc, -h, d);
    double a1d, a2;
    if (!HeadGeom::inverse(dc, head, d, a1d, a2))
        return false;
    axis1 = -a1d;
    axis2 = a2;
    return true;
}

/// TeenAstro exact model: mount axes -> sky direction (h, d), with T == identity.
static void teenastroMountToObservedPlace(const HeadModel &head, double axis1, double axis2,
                                          double &h, double &d)
{
    double dc[3];
    HeadGeom::forward(dc, -axis1, axis2, head);
    d = asin(dc[2] > 1.0 ? 1.0 : (dc[2] < -1.0 ? -1.0 : dc[2]));
    h = atan2(dc[1], dc[0]);
}

/// Feed one alignment star exactly the way Command_A does for a rigid session:
/// every star is recorded for the fit, only the first two also seed T through
/// the two star Taki pass.
static void feedStar(CoordConv &cc, double angle1, double angle2, double axis1Direct, double axis2)
{
    const bool seedTaki = cc.getStars() < 2;
    cc.addStar(angle1, angle2, axis1Direct, axis2);
    if (seedTaki)
        cc.addReference(angle1, angle2, axis1Direct, axis2);
}

/// Angular separation between two (h, d) sky positions, radians.
static double skySeparation(double h1, double d1, double h2, double d2)
{
    double a[3], b[3];
    LA3::toDirCos(a, -h1, d1);
    LA3::toDirCos(b, -h2, d2);
    return LA3::angle2Vectors(a, b);
}

// =====================================================================
//  1. A zero head must reproduce the legacy conversions exactly
// =====================================================================
void test_zero_head_matches_legacy_in_to_ho(void)
{
    // A non trivial T so the comparison is not accidentally passing on identity.
    CoordConv cc;
    feedStar(cc, 0.40, 0.50, 0.41, 0.49);
    feedStar(cc, 2.00, 0.80, 2.02, 0.78);
    TEST_ASSERT_TRUE(cc.isReady());

    const HeadModel zero;
    const double axes[5][2] = { { 0.0, 0.0 }, { 0.7, 0.3 }, { -1.2, -0.4 }, { 2.9, 1.1 }, { 0.2, -1.3 } };
    for (int k = 0; k < 5; k++) {
        Coord_IN in(0.0, axes[k][1], axes[k][0]);
        Coord_HO a = in.To_Coord_HO(cc.T, NO_REFR);
        Coord_HO b = in.To_Coord_HO(cc.T, NO_REFR, zero);
        TEST_ASSERT_DOUBLE_WITHIN(1e-15, a.Alt(), b.Alt());
        TEST_ASSERT_DOUBLE_WITHIN(1e-15, a.direct_Az_S(), b.direct_Az_S());
        TEST_ASSERT_DOUBLE_WITHIN(1e-15, a.FrH(), b.FrH());
    }
}

void test_zero_head_matches_legacy_ho_to_in(void)
{
    CoordConv cc;
    feedStar(cc, 0.40, 0.50, 0.41, 0.49);
    feedStar(cc, 2.00, 0.80, 2.02, 0.78);
    TEST_ASSERT_TRUE(cc.isReady());

    const HeadModel zero;
    const double sky[5][2] = { { 0.3, 0.2 }, { 1.0, 0.9 }, { -0.8, -0.5 }, { 2.5, 0.1 }, { 4.0, 1.2 } };
    for (int k = 0; k < 5; k++) {
        Coord_HO ho(0.0, sky[k][1], LA3::modRad(-sky[k][0] - M_PI), false);
        Coord_IN a = ho.To_Coord_IN(cc.Tinv);
        Coord_IN b = ho.To_Coord_IN(cc.Tinv, zero);
        TEST_ASSERT_DOUBLE_WITHIN(1e-15, a.Axis1(), b.Axis1());
        TEST_ASSERT_DOUBLE_WITHIN(1e-15, a.Axis2(), b.Axis2());
        TEST_ASSERT_DOUBLE_WITHIN(1e-15, a.Axis3(), b.Axis3());
    }
}

void test_new_coordconv_starts_with_zero_head(void)
{
    CoordConv cc;
    TEST_ASSERT_FALSE(cc.hasHead());
    TEST_ASSERT_EQUAL_UINT8(0, cc.getStars());
    TEST_ASSERT_TRUE(cc.head.isZero());
}

// =====================================================================
//  2. HeadGeom forward / inverse are exact inverses
// =====================================================================
void test_head_forward_inverse_roundtrip(void)
{
    const HeadModel head(0.7 * DEG, -0.4 * DEG, 0.25 * DEG);

    for (int i = -8; i <= 8; i++) {
        for (int j = -7; j <= 7; j++) {
            const double axis1d = i * 0.35;
            const double axis2  = j * 0.18;   // stays clear of the poles

            double dc[3];
            HeadGeom::forward(dc, axis1d, axis2, head);
            TEST_ASSERT_DOUBLE_WITHIN(1e-12, 1.0, LA3::norm(dc));

            double a1dBack, a2Back;
            TEST_ASSERT_TRUE(HeadGeom::inverse(dc, head, axis2, a1dBack, a2Back));
            TEST_ASSERT_DOUBLE_WITHIN(1e-10, axis2, a2Back);
            TEST_ASSERT_DOUBLE_WITHIN(1e-10, 0.0, HeadGeom::wrapPi(axis1d - a1dBack));
        }
    }
}

void test_head_forward_is_exact_near_pole(void)
{
    // OnStep switches its model off within one arcminute of the pole. The exact
    // rotation chain has no such hole, so forward/inverse must still round trip.
    const HeadModel head(0.5 * DEG, 0.3 * DEG, 0.1 * DEG);
    const double nearPole = 89.999 * DEG;

    double dc[3];
    HeadGeom::forward(dc, 0.9, nearPole, head);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 1.0, LA3::norm(dc));

    double a1d, a2;
    TEST_ASSERT_TRUE(HeadGeom::inverse(dc, head, nearPole, a1d, a2));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, nearPole, a2);
}

void test_head_zero_forward_equals_todircos(void)
{
    const HeadModel zero;
    double a[3], b[3];
    HeadGeom::forward(a, 0.7, 0.3, zero);
    LA3::toDirCos(b, 0.7, 0.3);
    for (int i = 0; i < 3; i++)
        TEST_ASSERT_DOUBLE_WITHIN(1e-15, b[i], a[i]);
}

// =====================================================================
//  3. Coord round trip through the head aware conversions
// =====================================================================
void test_coord_roundtrip_with_head(void)
{
    CoordConv cc;
    feedStar(cc, 0.40, 0.50, 0.41, 0.49);
    feedStar(cc, 2.00, 0.80, 2.02, 0.78);
    TEST_ASSERT_TRUE(cc.isReady());
    const HeadModel head(0.6 * DEG, -0.35 * DEG, 0.2 * DEG);

    const double sky[4][2] = { { 0.3, 0.2 }, { 1.0, 0.9 }, { -0.8, -0.5 }, { 2.5, 0.4 } };
    const HeadModel zero;
    for (int k = 0; k < 4; k++) {
        Coord_HO ho(0.0, sky[k][1], LA3::modRad(-sky[k][0] - M_PI), false);

        // The absolute round trip error here is set by the single precision
        // svd3 inside the legacy two star buildTransformations(): T and Tinv are
        // only mutual inverses to about 1e-6 rad. So compare the head model
        // against the legacy path rather than against an absolute tolerance,
        // which is what actually matters: the head must not add error.
        Coord_HO zeroBack = ho.To_Coord_IN(cc.Tinv, zero).To_Coord_HO(cc.T, NO_REFR, zero);
        Coord_HO headBack = ho.To_Coord_IN(cc.Tinv, head).To_Coord_HO(cc.T, NO_REFR, head);

        const double zeroErr = skySeparation(zeroBack.direct_Az_S(), zeroBack.Alt(), sky[k][0], sky[k][1]);
        const double headErr = skySeparation(headBack.direct_Az_S(), headBack.Alt(), sky[k][0], sky[k][1]);

        TEST_ASSERT_TRUE_MESSAGE(headErr < zeroErr + 1e-9,
                                 "head model round trip is worse than the legacy path");
        TEST_ASSERT_TRUE_MESSAGE(headErr < 1e-5, "head model round trip error unexpectedly large");
    }
}

void test_head_actually_changes_the_solution(void)
{
    // Guard against the head silently doing nothing: a 0.6 degree cone must move
    // the axis solution by an amount of that order.
    CoordConv cc;
    feedStar(cc, 0.40, 0.50, 0.40, 0.50);
    feedStar(cc, 2.00, 0.80, 2.00, 0.80);
    TEST_ASSERT_TRUE(cc.isReady());

    Coord_HO ho(0.0, 0.5, LA3::modRad(-0.7 - M_PI), false);
    Coord_IN plain = ho.To_Coord_IN(cc.Tinv);
    Coord_IN withHead = ho.To_Coord_IN(cc.Tinv, HeadModel(0.6 * DEG, 0.0, 0.0));
    const double moved = fabs(HeadGeom::wrapPi(plain.Axis1() - withHead.Axis1()));
    TEST_ASSERT_TRUE(moved > 0.2 * DEG);
}

// =====================================================================
//  4. Strict agreement with OnStep for small errors
//
//  Both models describe the same physics; OnStep linearises it. For error
//  terms of a few arcminutes the two must agree to well under an arcsecond.
// =====================================================================
void test_onstep_agreement_small_errors_forward(void)
{
    OnStepModel m;
    m.doCor = 3.0 * ARCMIN;
    m.pdCor = -2.0 * ARCMIN;
    m.ax2Cor = 1.5 * ARCMIN;
    const HeadModel head = headFromOnStep(m);

    // Declinations kept away from the pole, where OnStep's 1/cos blows up.
    const double decs[] = { -50 * DEG, -20 * DEG, 0.0, 20 * DEG, 50 * DEG, 70 * DEG };
    const double has[]  = { -60 * DEG, -20 * DEG, 10 * DEG, 45 * DEG, 80 * DEG };

    double worst = 0.0;
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 5; j++) {
            double oAx1, oAx2, tAx1, tAx2;
            onstepObservedPlaceToMount(m, has[j], decs[i], +1.0, oAx1, oAx2);
            TEST_ASSERT_TRUE(teenastroObservedPlaceToMount(head, has[j], decs[i], tAx1, tAx2));

            const double sep = skySeparation(oAx1, oAx2, tAx1, tAx2);
            if (sep > worst) worst = sep;
        }
    }
    // Strict: the two models must place the axes within one arcsecond.
    TEST_ASSERT_TRUE_MESSAGE(worst < 1.0 * ARCSEC,
                             "OnStep and TeenAstro disagree by more than 1 arcsec for small errors");
}

void test_onstep_agreement_small_errors_reverse(void)
{
    OnStepModel m;
    m.doCor = 2.0 * ARCMIN;
    m.pdCor = 2.5 * ARCMIN;
    m.ax2Cor = -1.0 * ARCMIN;
    const HeadModel head = headFromOnStep(m);

    const double a2s[] = { -50 * DEG, -20 * DEG, 0.0, 20 * DEG, 50 * DEG, 70 * DEG };
    const double a1s[] = { -60 * DEG, -20 * DEG, 10 * DEG, 45 * DEG, 80 * DEG };

    double worst = 0.0;
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 5; j++) {
            double oh, od, th, td;
            onstepMountToObservedPlace(m, a1s[j], a2s[i], +1.0, oh, od);
            teenastroMountToObservedPlace(head, a1s[j], a2s[i], th, td);

            const double sep = skySeparation(oh, od, th, td);
            if (sep > worst) worst = sep;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(worst < 1.0 * ARCSEC,
                             "OnStep and TeenAstro reverse transform disagree by more than 1 arcsec");
}

void test_onstep_agreement_pier_side_west(void)
{
    // The cone and non-perpendicularity terms flip sign with pier side in
    // OnStep. Check the mapping holds on the other side too.
    OnStepModel m;
    m.doCor = 2.5 * ARCMIN;
    m.pdCor = -1.5 * ARCMIN;
    m.ax2Cor = 0.0;
    // For p = -1 both terms change sign, and so does the head mapping.
    const HeadModel head(+m.doCor, +m.pdCor, 0.0);

    double worst = 0.0;
    const double decs[] = { -40 * DEG, 0.0, 30 * DEG, 60 * DEG };
    for (int i = 0; i < 4; i++) {
        double oAx1, oAx2, tAx1, tAx2;
        onstepObservedPlaceToMount(m, 0.6, decs[i], -1.0, oAx1, oAx2);
        TEST_ASSERT_TRUE(teenastroObservedPlaceToMount(head, 0.6, decs[i], tAx1, tAx2));
        const double sep = skySeparation(oAx1, oAx2, tAx1, tAx2);
        if (sep > worst) worst = sep;
    }
    TEST_ASSERT_TRUE_MESSAGE(worst < 1.0 * ARCSEC, "pier side west mapping disagrees");
}

// =====================================================================
//  5. For large errors TeenAstro must be closer to the truth
//
//  "Truth" is the exact rotation chain: we choose axis readings, compute where
//  the telescope really points, then ask each model to solve for those axes
//  from that sky position. The model whose solution returns closer to the
//  original axes is the more accurate one.
// =====================================================================
void test_teenastro_closer_to_truth_for_large_errors(void)
{
    // Multi degree cone and non-perpendicularity, where linearisation hurts.
    const double coneDeg = 3.0, perpDeg = 2.0;
    const HeadModel head(coneDeg * DEG, perpDeg * DEG, 0.0);
    OnStepModel m;
    m.doCor = -coneDeg * DEG;   // inverse of headFromOnStep
    m.pdCor = -perpDeg * DEG;
    m.ax2Cor = 0.0;

    const double axis1Truth[] = { -1.0, -0.3, 0.4, 1.1 };
    const double axis2Truth[] = { -40 * DEG, -10 * DEG, 25 * DEG, 55 * DEG };

    double sumOnStep = 0.0, sumTeenAstro = 0.0;
    int n = 0;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            const double a1 = axis1Truth[i], a2 = axis2Truth[j];

            // Where the telescope actually points with these axis readings.
            double h, d;
            teenastroMountToObservedPlace(head, a1, a2, h, d);

            // Each model solves for the axes needed to reach (h, d).
            double oAx1, oAx2, tAx1, tAx2;
            onstepObservedPlaceToMount(m, h, d, +1.0, oAx1, oAx2);
            TEST_ASSERT_TRUE(teenastroObservedPlaceToMount(head, h, d, tAx1, tAx2));

            // Feed each solution back through the true mount and measure where
            // it really ends up compared with the requested sky position.
            double oh, od, th, td;
            teenastroMountToObservedPlace(head, oAx1, oAx2, oh, od);
            teenastroMountToObservedPlace(head, tAx1, tAx2, th, td);

            sumOnStep += skySeparation(h, d, oh, od);
            sumTeenAstro += skySeparation(h, d, th, td);
            n++;
        }
    }
    const double meanOnStep = sumOnStep / n;
    const double meanTeenAstro = sumTeenAstro / n;

    // TeenAstro solves the exact model, so its pointing error must be
    // numerically zero, while OnStep retains a second order residual.
    TEST_ASSERT_TRUE_MESSAGE(meanTeenAstro < 0.05 * ARCSEC,
                             "TeenAstro should be exact for its own model");
    TEST_ASSERT_TRUE_MESSAGE(meanOnStep > 10.0 * meanTeenAstro + ARCSEC,
                             "OnStep linearisation should be measurably worse at multi degree errors");
}

// =====================================================================
//  6. Fitting: recover injected head terms from synthetic stars
// =====================================================================

/// Build a synthetic mount: sky (az, alt) -> instrument axes, given T and head.
static void synthMountAxes(const double (&Tm)[3][3], const HeadModel &head,
                           double az, double alt, double &axis1Direct, double &axis2)
{
    double dcSky[3], dcIn[3];
    LA3::toDirCos(dcSky, az, alt);
    LA3::multiply(dcIn, Tm, dcSky);      // CoordConv convention: T * sky = instrument
    LA3::normalize(dcIn, dcIn);
    // Invert the head to get the axis readings that point there.
    const double hint = asin(dcIn[2] > 1.0 ? 1.0 : (dcIn[2] < -1.0 ? -1.0 : dcIn[2]));
    HeadGeom::inverse(dcIn, head, hint, axis1Direct, axis2);
}

/// Head geometry of a plausible real mount: collimation and axis2
/// non-perpendicularity of a couple of arcminutes, axis2 index a few tens of
/// arcseconds. Magnitude matters for the tolerances below, because the part of
/// the cone / perp pair that no star set can separate scales with their size.
static const HeadModel HEAD_TRUTH(120.0 * ARCSEC, -70.0 * ARCSEC, 45.0 * ARCSEC);

/// A T that is misaligned enough to be non trivial, as a polar style tilt.
static void truthT(double (&Tm)[3][3])
{
    LA3::SingleRotation tilt[2] = { { LA3::ROTAXISY, 0.9 * DEG }, { LA3::ROTAXISX, -0.6 * DEG } };
    LA3::getMultipleRotationMatrix(Tm, tilt, 2);
}

/// Stars spread widely in both hour angle and declination.
static const double STARS_WIDE[6][2] = {
    { 0.30, 0.35 }, { 1.30, 0.80 }, { 2.40, 0.25 },
    { 3.50, 0.60 }, { 4.60, 0.95 }, { 5.40, 0.45 }
};

/// Feed a synthetic mount built from \p Tm and \p head, optionally adding a
/// deterministic error to the declared sky position to emulate the arcsecond
/// level quantisation of real catalogue and readout values.
static void feedSynthetic(CoordConv &cc, const double (&Tm)[3][3], const HeadModel &head,
                          const double stars[][2], int n, double noise = 0.0)
{
    for (int k = 0; k < n; k++) {
        double a1d, a2;
        synthMountAxes(Tm, head, stars[k][0], stars[k][1], a1d, a2);
        const double s1 = stars[k][0] + noise * ((k % 3) - 1);
        const double s2 = stars[k][1] + noise * (((k + 1) % 3) - 1);
        feedStar(cc, s1, s2, a1d, a2);
    }
}

/// Worst pointing error over a grid that deliberately avoids the star set.
static double worstPointingError(const CoordConv &cc, const double (&Tm)[3][3],
                                 const HeadModel &headTruth)
{
    double worst = 0.0;
    for (double a1 = 0.2; a1 < 6.0; a1 += 0.5) {
        for (double a2 = 0.15; a2 < 1.25; a2 += 0.2) {
            double a1d, a2m;
            synthMountAxes(Tm, headTruth, a1, a2, a1d, a2m);
            double dcIn[3], p[3], target[3];
            HeadGeom::forward(dcIn, a1d, a2m, cc.head);
            LA3::multiply(p, cc.Tinv, dcIn);
            LA3::normalize(p, p);
            LA3::toDirCos(target, a1, a2);
            const double e = LA3::angle2Vectors(p, target);
            if (e > worst)
                worst = e;
        }
    }
    return worst;
}

/// As synthMountAxes, but choosing which of the two mechanical configurations
/// reaches the target. \p flipped selects the beyond the pole branch, where
/// axis2 runs past 90 degrees; the mount reports exactly that, since
/// MountAxes::getInstrDeg divides the raw step count and Coord_IN stores the
/// angle verbatim.
static void synthMountAxesSide(const double (&Tm)[3][3], const HeadModel &head,
                               double az, double alt, bool flipped,
                               double &axis1Direct, double &axis2)
{
    double dcSky[3], dcIn[3];
    LA3::toDirCos(dcSky, az, alt);
    LA3::multiply(dcIn, Tm, dcSky);
    LA3::normalize(dcIn, dcIn);
    const double s = asin(dcIn[2] > 1.0 ? 1.0 : (dcIn[2] < -1.0 ? -1.0 : dcIn[2]));
    HeadGeom::inverse(dcIn, head, flipped ? (M_PI - s) : s, axis1Direct, axis2);
}

/// Build a session of \p n stars from STARS_WIDE, taking star \p flipIdx beyond
/// the pole (\p flipIdx < 0 keeps every star on the same side), and fit it.
static void fitSession(CoordConv &cc, const double (&Tm)[3][3], int n, int flipIdx)
{
    for (int k = 0; k < n; k++) {
        double a1d, a2;
        synthMountAxesSide(Tm, HEAD_TRUTH, STARS_WIDE[k][0], STARS_WIDE[k][1],
                           k == flipIdx, a1d, a2);
        feedStar(cc, STARS_WIDE[k][0], STARS_WIDE[k][1], a1d, a2);
    }
    char msg[64];
    sprintf(msg, "fit failed for n=%d flip=%d", n, flipIdx);
    TEST_ASSERT_TRUE_MESSAGE(cc.fitRigidModel(), msg);
}

void test_fit_drops_cone_without_a_meridian_flip(void)
{
    // Cone error and axis2 non-perpendicularity both displace axis1 with nearly
    // the same dependence on axis2, so within a single mechanical configuration
    // they stay ~99.9% redundant no matter how wide the sky coverage is: cone
    // keeps at most 0.00046 of its own information once perp is in the model.
    // The fit must keep only one of the pair, at every star count.
    double Ttruth[3][3];
    truthT(Ttruth);
    for (int n = COORDCONV_MIN_RIGID_STARS; n <= 6; n++) {
        CoordConv cc;
        fitSession(cc, Ttruth, n, -1);
        char msg[64];
        sprintf(msg, "n=%d mask=%x", n, (unsigned)cc.getRigidMask());
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, cc.getRigidMask() & COORDCONV_FIT_CONE, msg);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(COORDCONV_FIT_PERP,
                                        cc.getRigidMask() & COORDCONV_FIT_PERP, msg);
        float c, p, i2;
        cc.getHead(c, p, i2);
        TEST_ASSERT_EQUAL_DOUBLE(0.0, c);
        // The axis2 index stays separable and must still come out right, and
        // perp absorbing the observable combination has to keep pointing sane.
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(2.0 * ARCSEC, HEAD_TRUTH.idx2, i2, msg);
        TEST_ASSERT_TRUE_MESSAGE(worstPointingError(cc, Ttruth, HEAD_TRUTH) < 30.0 * ARCSEC, msg);
    }
}

void test_fit_recovers_cone_with_one_star_past_the_flip(void)
{
    // A cone error is fixed in the tube, so crossing to the beyond the pole
    // configuration reverses its effect on the sky, while the axis2
    // non-perpendicularity is a property of the head and does not follow it.
    // That asymmetry separates the pair, and one star is enough to exploit it.
    //
    // Sweeping every star count from the accepted minimum up, and every choice
    // of which star is the flipped one, the cone term always scores at least
    // 0.0278 - sixty times the single side figure - so all three terms are
    // solved and the model comes out exact. Getting this wrong is expensive:
    // declining cone when a flipped star is present leaves that star's reading
    // unexplainable and pushes pointing error past 60 arcsec, worse than never
    // having flipped at all.
    double Ttruth[3][3];
    truthT(Ttruth);
    for (int n = COORDCONV_MIN_RIGID_STARS; n <= 6; n++) {
        for (int flipIdx = 0; flipIdx < n; flipIdx++) {
            CoordConv cc;
            fitSession(cc, Ttruth, n, flipIdx);
            char msg[64];
            sprintf(msg, "n=%d flip=%d mask=%x", n, flipIdx, (unsigned)cc.getRigidMask());
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(
                COORDCONV_FIT_CONE | COORDCONV_FIT_PERP | COORDCONV_FIT_IDX2,
                cc.getRigidMask(), msg);
            float c, p, i2;
            cc.getHead(c, p, i2);
            TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(2.0 * ARCSEC, HEAD_TRUTH.cone, c, msg);
            TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(2.0 * ARCSEC, HEAD_TRUTH.perp, p, msg);
            TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(2.0 * ARCSEC, HEAD_TRUTH.idx2, i2, msg);
            // Fully determined geometry means the model is exact off the stars too.
            TEST_ASSERT_TRUE_MESSAGE(worstPointingError(cc, Ttruth, HEAD_TRUTH) < 1.0 * ARCSEC, msg);
        }
    }
}

void test_fit_recovers_axis2_index(void)
{
    // The axis2 index is the one head term that is always well separated from
    // the rest (its Jacobian column keeps over 95% of its own information even
    // for poor star distributions), so it must be recovered accurately.
    double Ttruth[3][3];
    truthT(Ttruth);
    const HeadModel headTruth = HEAD_TRUTH;

    CoordConv cc;
    feedSynthetic(cc, Ttruth, headTruth, STARS_WIDE, 6);
    TEST_ASSERT_TRUE(cc.isReady());
    TEST_ASSERT_EQUAL_UINT8(6, cc.getStars());

    const double rmsBefore = cc.residualRms();
    double rms = 0.0;
    int iters = 0;
    TEST_ASSERT_TRUE(cc.fitRigidModel(&rms, &iters));

    TEST_ASSERT_TRUE_MESSAGE(rms < rmsBefore, "fit did not improve on the two star model");
    TEST_ASSERT_TRUE_MESSAGE((cc.getRigidMask() & COORDCONV_FIT_IDX2) != 0,
                             "axis2 index should always be separable");
    TEST_ASSERT_DOUBLE_WITHIN(2.0 * ARCSEC, headTruth.idx2, cc.head.idx2);
}

void test_fit_selects_one_of_the_correlated_pair(void)
{
    // Cone error and axis2 non-perpendicularity both displace axis1 with a
    // similar dependence on axis2, so only their difference is observable: even
    // over a 35 degree declination span each retains under 1% of its own
    // information once the other is in. Exactly one of them must be selected,
    // otherwise the pair splits into two large opposing values.
    double Ttruth[3][3];
    truthT(Ttruth);
    const HeadModel headTruth = HEAD_TRUTH;

    CoordConv cc;
    feedSynthetic(cc, Ttruth, headTruth, STARS_WIDE, 6);
    TEST_ASSERT_TRUE(cc.fitRigidModel());

    const bool cone = (cc.getRigidMask() & COORDCONV_FIT_CONE) != 0;
    const bool perp = (cc.getRigidMask() & COORDCONV_FIT_PERP) != 0;
    TEST_ASSERT_TRUE_MESSAGE(cone != perp, "exactly one of cone / perp must be fitted");
    // The dropped one is held at zero, so the mask and the values agree.
    if (!cone) TEST_ASSERT_EQUAL_DOUBLE(0.0, cc.head.cone);
    if (!perp) TEST_ASSERT_EQUAL_DOUBLE(0.0, cc.head.perp);
}

void test_fit_drops_terms_for_single_declination(void)
{
    // All stars at one declination: neither cone nor perp is observable at all.
    // The fitter must decline to guess them rather than absorbing noise.
    double Ttruth[3][3];
    truthT(Ttruth);
    const HeadModel headTruth = HEAD_TRUTH;

    const double flat[6][2] = { { 0.30, 0.70 }, { 1.20, 0.70 }, { 2.10, 0.70 },
                                { 3.00, 0.70 }, { 3.90, 0.70 }, { 4.80, 0.70 } };
    CoordConv cc;
    feedSynthetic(cc, Ttruth, headTruth, flat, 6);
    TEST_ASSERT_TRUE(cc.fitRigidModel());

    TEST_ASSERT_EQUAL_UINT8(COORDCONV_FIT_IDX2, cc.getRigidMask());
    TEST_ASSERT_EQUAL_DOUBLE(0.0, cc.head.cone);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, cc.head.perp);
    TEST_ASSERT_DOUBLE_WITHIN(2.0 * ARCSEC, headTruth.idx2, cc.head.idx2);
}

void test_fit_does_not_invent_geometry_from_noise(void)
{
    // A geometrically perfect mount observed with 10 arcsec of input noise must
    // not acquire large head terms. This is the regression guard for the failure
    // mode that motivated parameter selection: an unconstrained six parameter
    // fit drove the correlated pair to hundreds of arcseconds here, which fits
    // the calibration stars but degrades pointing away from them.
    double Ttruth[3][3];
    truthT(Ttruth);
    const HeadModel zero;

    CoordConv cc;
    feedSynthetic(cc, Ttruth, zero, STARS_WIDE, 6, 10.0 * ARCSEC);
    TEST_ASSERT_TRUE(cc.fitRigidModel());

    TEST_ASSERT_TRUE_MESSAGE(fabs(cc.head.cone) < 60.0 * ARCSEC, "cone inflated by noise");
    TEST_ASSERT_TRUE_MESSAGE(fabs(cc.head.perp) < 60.0 * ARCSEC, "perp inflated by noise");
    TEST_ASSERT_TRUE_MESSAGE(fabs(cc.head.idx2) < 60.0 * ARCSEC, "idx2 inflated by noise");
    // And pointing away from the stars stays within the noise, not far outside it.
    TEST_ASSERT_TRUE(worstPointingError(cc, Ttruth, zero) < 60.0 * ARCSEC);
}

void test_fit_improves_pointing_off_the_star_set(void)
{
    // Selecting only the observable combination cannot reproduce a cone and a
    // perp error separately, but it must still be a large improvement over the
    // two star model, and remain valid away from the calibration stars.
    double Ttruth[3][3];
    truthT(Ttruth);
    const HeadModel headTruth = HEAD_TRUTH;

    CoordConv cc;
    feedSynthetic(cc, Ttruth, headTruth, STARS_WIDE, 6);

    const double before = worstPointingError(cc, Ttruth, headTruth);
    TEST_ASSERT_TRUE(cc.fitRigidModel());
    const double after = worstPointingError(cc, Ttruth, headTruth);

    TEST_ASSERT_TRUE_MESSAGE(after < 0.25 * before, "fit did not substantially improve pointing");
    TEST_ASSERT_TRUE_MESSAGE(after < 60.0 * ARCSEC, "residual pointing error too large");
}

void test_fit_rejects_too_few_stars(void)
{
    CoordConv cc;
    feedStar(cc, 0.40, 0.50, 0.41, 0.49);
    feedStar(cc, 2.00, 0.80, 2.02, 0.78);
    TEST_ASSERT_TRUE(cc.isReady());
    TEST_ASSERT_EQUAL_UINT8(2, cc.getStars());
    // Two stars cannot constrain six unknowns.
    TEST_ASSERT_FALSE(cc.fitRigidModel());
    TEST_ASSERT_FALSE(cc.hasHead());

    // Three is enough on paper but leaves no redundancy at all, so every
    // measurement error would land directly in the parameters.
    feedStar(cc, 4.00, 0.30, 4.01, 0.31);
    TEST_ASSERT_EQUAL_UINT8(3, cc.getStars());
    TEST_ASSERT_FALSE(cc.fitRigidModel());
    TEST_ASSERT_FALSE(cc.hasHead());

    // Four is accepted.
    feedStar(cc, 5.20, 0.90, 5.19, 0.91);
    TEST_ASSERT_EQUAL_UINT8(4, cc.getStars());
    TEST_ASSERT_TRUE(cc.fitRigidModel());
}

void test_fit_leaves_model_untouched_on_failure(void)
{
    CoordConv cc;
    feedStar(cc, 0.40, 0.50, 0.41, 0.49);
    feedStar(cc, 2.00, 0.80, 2.02, 0.78);
    double before[3][3];
    LA3::copy(before, cc.T);

    TEST_ASSERT_FALSE(cc.fitRigidModel());
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(1e-15, before[i][j], cc.T[i][j]);
}

void test_fit_keeps_t_orthonormal(void)
{
    double Ttruth[3][3];
    truthT(Ttruth);
    const HeadModel headTruth = HEAD_TRUTH;

    const double stars[5][2] = { { 0.3, 0.3 }, { 1.5, 0.8 }, { 2.7, 0.2 }, { 3.9, 0.7 }, { 5.1, 0.5 } };
    CoordConv cc;
    for (int k = 0; k < 5; k++) {
        double a1d, a2;
        synthMountAxes(Ttruth, headTruth, stars[k][0], stars[k][1], a1d, a2);
        feedStar(cc, stars[k][0], stars[k][1], a1d, a2);
    }
    TEST_ASSERT_TRUE(cc.fitRigidModel());

    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 1.0, LA3::determinant(cc.T));
    double prod[3][3];
    LA3::multiply(prod, cc.T, cc.Tinv);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, (i == j) ? 1.0 : 0.0, prod[i][j]);
}

void test_clean_clears_head_and_stars(void)
{
    CoordConv cc;
    feedStar(cc, 0.4, 0.5, 0.41, 0.49);
    feedStar(cc, 2.0, 0.8, 2.02, 0.78);
    cc.setHead(0.01, 0.02, 0.03);
    TEST_ASSERT_TRUE(cc.hasHead());
    cc.clean();
    TEST_ASSERT_FALSE(cc.hasHead());
    TEST_ASSERT_EQUAL_UINT8(0, cc.getStars());
}

void test_head_getter_setter_roundtrip(void)
{
    CoordConv cc;
    cc.setHead(0.001234, -0.002345, 0.003456);
    float c, p, i;
    cc.getHead(c, p, i);
    TEST_ASSERT_FLOAT_WITHIN(1e-7f, 0.001234f, c);
    TEST_ASSERT_FLOAT_WITHIN(1e-7f, -0.002345f, p);
    TEST_ASSERT_FLOAT_WITHIN(1e-7f, 0.003456f, i);
    cc.clearHead();
    TEST_ASSERT_FALSE(cc.hasHead());
}

// =====================================================================
void setUp()    {}
void tearDown() {}

int main(int, char **)
{
    UNITY_BEGIN();

    RUN_TEST(test_zero_head_matches_legacy_in_to_ho);
    RUN_TEST(test_zero_head_matches_legacy_ho_to_in);
    RUN_TEST(test_new_coordconv_starts_with_zero_head);

    RUN_TEST(test_head_forward_inverse_roundtrip);
    RUN_TEST(test_head_forward_is_exact_near_pole);
    RUN_TEST(test_head_zero_forward_equals_todircos);

    RUN_TEST(test_coord_roundtrip_with_head);
    RUN_TEST(test_head_actually_changes_the_solution);

    RUN_TEST(test_onstep_agreement_small_errors_forward);
    RUN_TEST(test_onstep_agreement_small_errors_reverse);
    RUN_TEST(test_onstep_agreement_pier_side_west);
    RUN_TEST(test_teenastro_closer_to_truth_for_large_errors);

    RUN_TEST(test_fit_drops_cone_without_a_meridian_flip);
    RUN_TEST(test_fit_recovers_cone_with_one_star_past_the_flip);
    RUN_TEST(test_fit_recovers_axis2_index);
    RUN_TEST(test_fit_selects_one_of_the_correlated_pair);
    RUN_TEST(test_fit_drops_terms_for_single_declination);
    RUN_TEST(test_fit_does_not_invent_geometry_from_noise);
    RUN_TEST(test_fit_improves_pointing_off_the_star_set);
    RUN_TEST(test_fit_rejects_too_few_stars);
    RUN_TEST(test_fit_leaves_model_untouched_on_failure);
    RUN_TEST(test_fit_keeps_t_orthonormal);
    RUN_TEST(test_clean_clears_head_and_stars);
    RUN_TEST(test_head_getter_setter_roundtrip);

    return UNITY_END();
}
