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
 *   7. A 3+3 session whose axes come from OnStep X recovers that model's head,
 *      including when the model is refit after each star from the third.
 *   8. Each star after the second pulls the next goto closer when the pole is far off.
 *   9. Close-out / pier-side application regressions (sync, hasHead fallback,
 *      and goto across pier with CH must use HeadGeom beyond-pole inverse).
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

static const double STARS9[9][2] = {
    { 0.30, 0.35 }, { 1.30, 0.80 }, { 2.40, 0.25 },
    { 3.50, 0.60 }, { 4.60, 0.95 }, { 5.40, 0.45 },
    { 0.90, 0.15 }, { 2.90, 1.05 }, { 4.10, 0.30 }
};

/// Deterministic uniform noise in [-0.5, 0.5], so the statistics below are
/// reproducible across runs and platforms.
static unsigned long g_seed = 12345;
static double urand()
{
    g_seed = g_seed * 1103515245UL + 12345UL;
    return ((double)((g_seed >> 16) & 0x7FFF) / 32767.0) - 0.5;
}

/// Mean worst-case pointing error over \p trials sessions of \p n stars, with a
/// 15 arcsec peak centring error on each declared sky position. The first
/// \p nFlip stars are taken beyond the pole. Also reports how often all three
/// head terms were selected.
static double meanWorstUnderNoise(int n, int nFlip, int trials, int *allThree)
{
    double Tt[3][3];
    truthT(Tt);
    const double peak = 15.0 * ARCSEC;
    double sum = 0.0;
    int fits = 0;
    *allThree = 0;
    g_seed = 12345;
    for (int t = 0; t < trials; t++) {
        CoordConv cc;
        for (int k = 0; k < n; k++) {
            double a1d, a2;
            synthMountAxesSide(Tt, HEAD_TRUTH, STARS9[k][0], STARS9[k][1],
                               k < nFlip, a1d, a2);
            const double s2 = STARS9[k][1] + 2.0 * peak * urand();
            const double s1 = STARS9[k][0] + 2.0 * peak * urand() / cos(STARS9[k][1]);
            feedStar(cc, s1, s2, a1d, a2);
        }
        if (!cc.fitRigidModel())
            continue;
        fits++;
        if (cc.getRigidMask() == (COORDCONV_FIT_CONE | COORDCONV_FIT_PERP | COORDCONV_FIT_IDX2))
            (*allThree)++;
        sum += worstPointingError(cc, Tt, HEAD_TRUTH);
    }
    return fits ? sum / fits : 1.0;
}

void test_flip_pays_off_from_five_stars(void)
{
    // Cone is published only for a 3+3 pier split. Under a 15 arcsec centring
    // error that split must select all three terms, and must not point worse
    // than the same six stars kept on one side.
    const int TRIALS = 60;
    int all3 = 0;

    // Six stars split 3 and 3 selects cone on every trial. Under a 15 arcsec
    // centring error the pointing gain over a one-side session is small
    // (about 21 arcsec against 23), so the check is that the split is not worse.
    const double oneSide6 = meanWorstUnderNoise(6, 0, TRIALS, &all3);
    TEST_ASSERT_EQUAL_INT(0, all3);
    const double flipped6 = meanWorstUnderNoise(6, 3, TRIALS, &all3);
    TEST_ASSERT_EQUAL_INT(TRIALS, all3);
    TEST_ASSERT_TRUE_MESSAGE(flipped6 <= oneSide6,
                             "3+3 must not point worse than six stars on one side");

    // Six stars with only one past the pole is not 3 per side, so no cone.
    meanWorstUnderNoise(6, 1, TRIALS, &all3);
    TEST_ASSERT_EQUAL_INT(0, all3);

    // Four stars, even with one past the pole, must not publish a cone.
    meanWorstUnderNoise(4, 1, TRIALS, &all3);
    TEST_ASSERT_EQUAL_INT(0, all3);
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

/// Fit \p n stars from STARS_WIDE, taking the first \p nFlip beyond the pole.
static void fitSplit(CoordConv &cc, const double (&Tm)[3][3], int n, int nFlip)
{
    for (int k = 0; k < n; k++) {
        double a1d, a2;
        synthMountAxesSide(Tm, HEAD_TRUTH, STARS_WIDE[k][0], STARS_WIDE[k][1],
                           k < nFlip, a1d, a2);
        feedStar(cc, STARS_WIDE[k][0], STARS_WIDE[k][1], a1d, a2);
    }
    char msg[64];
    sprintf(msg, "fit failed for n=%d nFlip=%d", n, nFlip);
    TEST_ASSERT_TRUE_MESSAGE(cc.fitRigidModel(), msg);
}

void test_fit_withholds_cone_below_three_per_side(void)
{
    // Cone needs three stars on each pier side. Fewer than that, including a
    // six star session with only one or two past the pole, must not publish it.
    // Perp still needs only four stars in total.
    double Ttruth[3][3];
    truthT(Ttruth);
    const int splits[][2] = { { 4, 1 }, { 5, 1 }, { 6, 1 }, { 6, 2 } };
    for (int s = 0; s < 4; s++) {
        CoordConv cc;
        fitSplit(cc, Ttruth, splits[s][0], splits[s][1]);
        char msg[64];
        sprintf(msg, "n=%d nFlip=%d mask=%x", splits[s][0], splits[s][1],
                (unsigned)cc.getRigidMask());
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, cc.getRigidMask() & COORDCONV_FIT_CONE, msg);
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(COORDCONV_FIT_PERP,
                                        cc.getRigidMask() & COORDCONV_FIT_PERP, msg);
        float c, p, i2;
        cc.getHead(c, p, i2);
        TEST_ASSERT_EQUAL_DOUBLE(0.0, c);
        (void)p;
        (void)i2;
    }
}

void test_fit_recovers_cone_with_three_stars_per_side(void)
{
    // Three stars on each pier side is the smallest session allowed to estimate
    // cone. All three head terms then come out, and the model is exact off the
    // star set.
    double Ttruth[3][3];
    truthT(Ttruth);
    CoordConv cc;
    fitSplit(cc, Ttruth, 6, COORDCONV_MIN_CONE_PER_SIDE);
    TEST_ASSERT_EQUAL_UINT8(COORDCONV_FIT_CONE | COORDCONV_FIT_PERP | COORDCONV_FIT_IDX2,
                            cc.getRigidMask());
    float c, p, i2;
    cc.getHead(c, p, i2);
    TEST_ASSERT_DOUBLE_WITHIN(2.0 * ARCSEC, HEAD_TRUTH.cone, c);
    TEST_ASSERT_DOUBLE_WITHIN(2.0 * ARCSEC, HEAD_TRUTH.perp, p);
    TEST_ASSERT_DOUBLE_WITHIN(2.0 * ARCSEC, HEAD_TRUTH.idx2, i2);
    TEST_ASSERT_TRUE(worstPointingError(cc, Ttruth, HEAD_TRUTH) < 1.0 * ARCSEC);
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

/// Pointing error at one sky position under the model held by \p cc.
static double pointingAt(const CoordConv &cc, const double (&Tm)[3][3], const HeadModel &head,
                         double az, double alt)
{
    double a1d, a2m;
    synthMountAxes(Tm, head, az, alt, a1d, a2m);
    double dcIn[3], p[3], target[3];
    HeadGeom::forward(dcIn, a1d, a2m, cc.head);
    LA3::multiply(p, cc.Tinv, dcIn);
    LA3::normalize(p, p);
    LA3::toDirCos(target, az, alt);
    return LA3::angle2Vectors(p, target);
}

static double pointingAtSide(const CoordConv &cc, const double (&Tm)[3][3], const HeadModel &head,
                             double az, double alt, bool flipped)
{
    double a1d, a2m;
    synthMountAxesSide(Tm, head, az, alt, flipped, a1d, a2m);
    double dcIn[3], p[3], target[3];
    HeadGeom::forward(dcIn, a1d, a2m, cc.head);
    LA3::multiply(p, cc.Tinv, dcIn);
    LA3::normalize(p, p);
    LA3::toDirCos(target, az, alt);
    return LA3::angle2Vectors(p, target);
}

void test_each_added_star_improves_the_next_goto(void)
{
    // Several degrees of polar error and a half-degree cone. Two stars reproduce
    // themselves and leave the next goto a long way off, which is the painful
    // 3+3 session on a mount that has not been aligned yet.
    double Tm[3][3];
    LA3::SingleRotation tilt[2] = { { LA3::ROTAXISY, 4.0 * DEG }, { LA3::ROTAXISX, -3.0 * DEG } };
    LA3::getMultipleRotationMatrix(Tm, tilt, 2);
    const HeadModel head(0.5 * DEG, -0.3 * DEG, 0.2 * DEG);
    const double stars[5][2] = {
        { 0.40, 0.40 }, { 1.60, 0.90 }, { 2.50, 0.30 }, { 3.60, 0.70 }, { 4.80, 1.00 }
    };

    CoordConv cc;
    for (int k = 0; k < 2; k++) {
        double a1d, a2;
        synthMountAxes(Tm, head, stars[k][0], stars[k][1], a1d, a2);
        feedStar(cc, stars[k][0], stars[k][1], a1d, a2);
    }
    TEST_ASSERT_FALSE(cc.fitProgressive());

    char msg[96];
    const double goto4before = pointingAt(cc, Tm, head, stars[3][0], stars[3][1]);
    const double twoStar5 = pointingAt(cc, Tm, head, stars[4][0], stars[4][1]);
    const double flipBefore = pointingAtSide(cc, Tm, head, 0.80, 0.55, true);
    double a1d, a2;
    synthMountAxes(Tm, head, stars[2][0], stars[2][1], a1d, a2);
    feedStar(cc, stars[2][0], stars[2][1], a1d, a2);
    TEST_ASSERT_TRUE(cc.fitProgressive());
    TEST_ASSERT_TRUE((cc.getRigidMask() & COORDCONV_FIT_CONE) == 0);
    const double goto4after = pointingAt(cc, Tm, head, stars[3][0], stars[3][1]);
    const double flipAfter = pointingAtSide(cc, Tm, head, 0.80, 0.55, true);
    sprintf(msg, "flip before %.4f deg after %.4f deg",
            flipBefore * 180.0 / M_PI, flipAfter * 180.0 / M_PI);
    // Publishing the one-sided head term must not throw the first star on the
    // other pier further out than the two-star model already did.
    TEST_ASSERT_TRUE_MESSAGE(flipAfter < flipBefore + 0.05 * DEG, msg);
    sprintf(msg, "goto4 before %.4f deg after %.4f deg",
            goto4before * 180.0 / M_PI, goto4after * 180.0 / M_PI);
    TEST_ASSERT_TRUE_MESSAGE(goto4after < 0.5 * goto4before, msg);

    synthMountAxes(Tm, head, stars[3][0], stars[3][1], a1d, a2);
    feedStar(cc, stars[3][0], stars[3][1], a1d, a2);
    TEST_ASSERT_TRUE(cc.fitProgressive());
    const double goto5after = pointingAt(cc, Tm, head, stars[4][0], stars[4][1]);
    sprintf(msg, "goto5 two-star %.4f deg after fourth %.4f deg",
            twoStar5 * 180.0 / M_PI, goto5after * 180.0 / M_PI);
    TEST_ASSERT_TRUE_MESSAGE(goto5after < 0.5 * twoStar5, msg);
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

/// OnStep keeps both pier sides in hour angle / declination and flips the
/// cone and perpendicularity with a sign. TeenAstro records the other pier as
/// the raw motor angles: axis2 reflected through the pole (pi - ax2) and
/// axis1 advanced half a turn. That is what getInstr() stores in a 3+3 session.
static void onstepAxesToMount(double ax1, double ax2, double pier,
                              double &axis1Direct, double &axis2Out)
{
    if (pier > 0.0)
    {
        axis1Direct = -ax1;
        axis2Out = ax2;
        return;
    }
    axis2Out = M_PI - ax2;
    axis1Direct = remainder(-(ax1 + M_PI), 2.0 * M_PI);
}

void test_onstep_3plus3_dataset_recovers_head(void)
{
    // Published OnStep X rigid terms, a few arcminutes, the size of a real mount.
    // The axis1 index is left at zero: OnStep subtracts it from hour angle and
    // TeenAstro folds that into T, so it is not one of the three head terms.
    OnStepModel m;
    m.doCor = 3.0 * ARCMIN;
    m.pdCor = -1.5 * ARCMIN;
    m.ax2Cor = 0.7 * ARCMIN;
    m.ax1Cor = 0.0;
    const HeadModel expect = headFromOnStep(m);

    // Three stars east (pier +1), then three west (pier -1). Spread in hour
    // angle and declination, kept off the pole where OnStep's 1/cos diverges.
    struct Star { double ha; double dec; double pier; const char *name; };
    const Star stars[6] = {
        { -40.0 * DEG,  15.0 * DEG, +1.0, "east Alioth" },
        {  25.0 * DEG,  48.0 * DEG, +1.0, "east Vega" },
        {  70.0 * DEG, -18.0 * DEG, +1.0, "east Rigel" },
        { -35.0 * DEG,  32.0 * DEG, -1.0, "west Arcturus" },
        {  12.0 * DEG,  62.0 * DEG, -1.0, "west Deneb" },
        {  55.0 * DEG,   8.0 * DEG, -1.0, "west Altair" },
    };

    CoordConv cc;
    for (int k = 0; k < 6; k++)
    {
        double oAx1, oAx2, a1d, a2;
        onstepObservedPlaceToMount(m, stars[k].ha, stars[k].dec, stars[k].pier, oAx1, oAx2);
        onstepAxesToMount(oAx1, oAx2, stars[k].pier, a1d, a2);

        // The folded west reading, under the single east head, must still
        // point at the catalogue star. If this fails, the pier conversion is
        // wrong and the fit below would be meaningless.
        double dcMount[3], dcSky[3];
        HeadGeom::forward(dcMount, a1d, a2, expect);
        LA3::toDirCos(dcSky, -stars[k].ha, stars[k].dec);
        char msg[80];
        sprintf(msg, "%s does not point at its catalogue position", stars[k].name);
        TEST_ASSERT_TRUE_MESSAGE(LA3::angle2Vectors(dcMount, dcSky) < 2.0 * ARCSEC, msg);

        // Sky angles use the same convention as toDirCos(-ha, dec).
        feedStar(cc, -stars[k].ha, stars[k].dec, a1d, a2);
    }

    int nIn = 0, nOut = 0;
    cc.pierSideCounts(nIn, nOut);
    TEST_ASSERT_EQUAL_INT(3, nIn);
    TEST_ASSERT_EQUAL_INT(3, nOut);

    TEST_ASSERT_TRUE_MESSAGE(cc.fitRigidModel(), "3+3 OnStep session failed to fit");
    TEST_ASSERT_EQUAL_UINT8(COORDCONV_FIT_CONE | COORDCONV_FIT_PERP | COORDCONV_FIT_IDX2,
                            cc.getRigidMask());

    float cone, perp, idx2;
    cc.getHead(cone, perp, idx2);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(5.0 * ARCSEC, expect.cone, cone, "cone");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(5.0 * ARCSEC, expect.perp, perp, "perp");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(5.0 * ARCSEC, expect.idx2, idx2, "axis2 index");
    TEST_ASSERT_TRUE_MESSAGE(cc.getRigidRms() < 5.0 * ARCSEC, "residual above 5 arcsec");
}

void test_onstep_3plus3_progressive_matches_final_head(void)
{
    // Same OnStep X stars as the batch fit, but updated the way the mount does
    // it: refit after every star from the third, and close with the full fit
    // on the sixth. The head at the end has to be the OnStep head, not a
    // different compromise built up along the way.
    OnStepModel m;
    m.doCor = 3.0 * ARCMIN;
    m.pdCor = -1.5 * ARCMIN;
    m.ax2Cor = 0.7 * ARCMIN;
    m.ax1Cor = 0.0;
    const HeadModel expect = headFromOnStep(m);

    struct Star { double ha; double dec; double pier; const char *name; };
    const Star stars[6] = {
        { -40.0 * DEG,  15.0 * DEG, +1.0, "east Alioth" },
        {  25.0 * DEG,  48.0 * DEG, +1.0, "east Vega" },
        {  70.0 * DEG, -18.0 * DEG, +1.0, "east Rigel" },
        { -35.0 * DEG,  32.0 * DEG, -1.0, "west Arcturus" },
        {  12.0 * DEG,  62.0 * DEG, -1.0, "west Deneb" },
        {  55.0 * DEG,   8.0 * DEG, -1.0, "west Altair" },
    };

    CoordConv stepped, batch;
    for (int k = 0; k < 6; k++)
    {
        double oAx1, oAx2, a1d, a2;
        onstepObservedPlaceToMount(m, stars[k].ha, stars[k].dec, stars[k].pier, oAx1, oAx2);
        onstepAxesToMount(oAx1, oAx2, stars[k].pier, a1d, a2);

        double dcMount[3], dcSky[3];
        HeadGeom::forward(dcMount, a1d, a2, expect);
        LA3::toDirCos(dcSky, -stars[k].ha, stars[k].dec);
        char msg[80];
        sprintf(msg, "%s does not point at its catalogue position", stars[k].name);
        TEST_ASSERT_TRUE_MESSAGE(LA3::angle2Vectors(dcMount, dcSky) < 2.0 * ARCSEC, msg);

        feedStar(stepped, -stars[k].ha, stars[k].dec, a1d, a2);
        feedStar(batch, -stars[k].ha, stars[k].dec, a1d, a2);

        // Stars 3, 4 and 5. The sixth is the session close, not an intermediate refit.
        if (k >= 2 && k < 5)
        {
            sprintf(msg, "progressive fit failed after %s", stars[k].name);
            TEST_ASSERT_TRUE_MESSAGE(stepped.fitProgressive(), msg);
            sprintf(msg, "cone published before both piers had 3 stars (%s)", stars[k].name);
            TEST_ASSERT_TRUE_MESSAGE((stepped.getRigidMask() & COORDCONV_FIT_CONE) == 0, msg);
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(batch.fitRigidModel(), "batch OnStep fit failed");
    TEST_ASSERT_TRUE_MESSAGE(stepped.fitRigidModel(), "progressive OnStep session failed to close");

    float cone, perp, idx2;
    stepped.getHead(cone, perp, idx2);
    TEST_ASSERT_EQUAL_UINT8(COORDCONV_FIT_CONE | COORDCONV_FIT_PERP | COORDCONV_FIT_IDX2,
                            stepped.getRigidMask());
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(5.0 * ARCSEC, expect.cone, cone, "cone");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(5.0 * ARCSEC, expect.perp, perp, "perp");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(5.0 * ARCSEC, expect.idx2, idx2, "axis2 index");
    TEST_ASSERT_TRUE_MESSAGE(stepped.getRigidRms() < 5.0 * ARCSEC, "residual above 5 arcsec");

    float bCone, bPerp, bIdx2;
    batch.getHead(bCone, bPerp, bIdx2);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1.0 * ARCSEC, bCone, cone, "cone drifted from the batch fit");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1.0 * ARCSEC, bPerp, perp, "perp drifted from the batch fit");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1.0 * ARCSEC, bIdx2, idx2, "axis2 index drifted from the batch fit");
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
//  9. Rigid close-out / pier-side application regressions
//
//  These encode the mistakes that made a finished 3+3 session leave side A
//  far off: syncing the last star after a successful rigid fit, discarding
//  the multi-star T when no head term was published, and commanding the
//  other pier with the ideal angle2Step mirror instead of HeadGeom's
//  beyond-pole inverse once CH is in the model.
// =====================================================================

/// Sky miss of (az,alt) when the mount axes read (a1d,a2), under the fitted model.
static double skyMissFromAxes(const CoordConv &cc, double az, double alt,
                              double a1d, double a2)
{
    double p[3], t[3], dcIn[3];
    HeadGeom::forward(dcIn, a1d, a2, cc.head);
    LA3::multiply(p, cc.Tinv, dcIn);
    LA3::normalize(p, p);
    LA3::toDirCos(t, az, alt);
    return LA3::angle2Vectors(p, t);
}

/// Instrument axes the fitted model commands for a sky target.
static void modelAxes(const CoordConv &cc, double az, double alt, bool flipped,
                      double &a1d, double &a2)
{
    double dcSky[3], dcIn[3];
    LA3::toDirCos(dcSky, az, alt);
    LA3::multiply(dcIn, cc.T, dcSky);
    LA3::normalize(dcIn, dcIn);
    const double hint = asin(dcIn[2] > 1.0 ? 1.0 : (dcIn[2] < -1.0 ? -1.0 : dcIn[2]));
    HeadGeom::inverse(dcIn, cc.head, flipped ? (M_PI - hint) : hint, a1d, a2);
}

/// Ideal GEM pier flip on instrument angles — what MountAxes::angle2Step does
/// when PoleSide is OVER, after goToHor asked To_Coord_IN(..., beyondPole=false).
/// With a non-zero cone this is not the same as HeadGeom::inverse(beyondPole).
static void idealPierFlip(double a1dUnder, double a2Under, double &a1dOver, double &a2Over)
{
    a2Over = M_PI - a2Under;
    a1dOver = remainder(a1dUnder + M_PI, 2.0 * M_PI);
}

void test_post_fit_sync_throws_side_a_off(void)
{
    // A successful 3+3 fit points side A correctly. Syncing the encoder frame
    // afterward by the last-star residual (the old close-out) shifts every
    // commanded axis reading by that residual and throws side A off by about
    // the same amount. Keeping the frame (the fix) leaves side A intact.
    double Ttruth[3][3];
    truthT(Ttruth);
    CoordConv cc;
    const double lastSkyOffset = 30.0 * ARCMIN;
    for (int k = 0; k < 6; k++) {
        double a1d, a2;
        synthMountAxesSide(Ttruth, HEAD_TRUTH, STARS_WIDE[k][0], STARS_WIDE[k][1],
                           k < 3, a1d, a2);
        double az = STARS_WIDE[k][0];
        double alt = STARS_WIDE[k][1];
        if (k == 5)
            alt += lastSkyOffset;
        feedStar(cc, az, alt, a1d, a2);
    }
    TEST_ASSERT_TRUE(cc.fitRigidModel());

    double azL, altL, a1L, a2L;
    TEST_ASSERT_TRUE(cc.getStar(5, azL, altL, a1L, a2L));
    double a1m, a2m;
    modelAxes(cc, azL, altL, /*flipped=*/true, a1m, a2m);
    const double d1 = a1m - a1L;
    const double d2 = a2m - a2L;

    double az0, alt0, a1r0, a2r0;
    TEST_ASSERT_TRUE(cc.getStar(0, az0, alt0, a1r0, a2r0));
    double a1cmd, a2cmd;
    modelAxes(cc, az0, alt0, /*flipped=*/false, a1cmd, a2cmd);

    const double missKeep = skyMissFromAxes(cc, az0, alt0, a1cmd, a2cmd);
    const double missSync = skyMissFromAxes(cc, az0, alt0, a1cmd - d1, a2cmd - d2);

    TEST_ASSERT_TRUE_MESSAGE(missKeep < 5.0 * ARCMIN,
                             "keeping the encoder frame must leave side A close");
    TEST_ASSERT_TRUE_MESSAGE(missSync > 15.0 * ARCMIN,
                             "post-fit sync must throw side A off by tens of arcmin");
    TEST_ASSERT_TRUE_MESSAGE(missSync > 3.0 * missKeep,
                             "sync path must be several times worse than keep path");
    (void)a1r0;
    (void)a2r0;
}

void test_hasHead_fallback_discards_six_star_t(void)
{
    // A finished rigid fit leaves a multi-star T that matches all retained
    // stars. The old close-out gated on hasHead() and, when no CH/NP/ID was
    // published, ran the classic two-star minimisers — which rebuild T from
    // only the first pair and throw the other stars off. Keeping the fitted T
    // (the fix) is what this residual comparison defends.
    double Ttruth[3][3];
    truthT(Ttruth);
    CoordConv cc;
    fitSplit(cc, Ttruth, 6, COORDCONV_MIN_CONE_PER_SIDE);
    const double rmsKeep = cc.residualRms();
    TEST_ASSERT_TRUE_MESSAGE(rmsKeep < 1.0 * ARCSEC,
                             "six-star rigid T must fit the stars tightly");

    // Old close-out when !hasHead(): classic two-star refine on the seed pair.
    cc.minimizeAxis2();
    cc.minimizeAxis1(M_PI_2);
    const double rmsFallback = cc.residualRms();
    TEST_ASSERT_TRUE_MESSAGE(rmsFallback > 10.0 * rmsKeep + 30.0 * ARCSEC,
                             "two-star fallback must spoil the six-star residual");
}

void test_goto_other_pier_with_cone_needs_beyond_pole_inverse(void)
{
    // Field report: 3+3 progressive gotos tighten up, close-out reports a small
    // :GXAr#, then a goto to the first star (other pier) misses by many degrees.
    //
    // Progressive never publishes CH (needs 3+3). Final does. The old goToHor
    // path called To_Coord_IN(..., beyondPole=false) and mapped the other pier
    // with the ideal angle2Step mirror. Cone reverses across the flip, so that
    // mirror is not HeadGeom's beyond-pole solution — residual on the stored
    // raw stars stays small while the commanded OVER pose is wrong. Firmware
    // now uses predictTargetHO (beyondPole per pier); this test keeps the
    // failure mode locked so it cannot return.
    const HeadModel headTruth(1.0 * DEG, 0.25 * DEG, 0.10 * DEG);
    double Ttruth[3][3];
    truthT(Ttruth);

    CoordConv stepped;
    double missSideAProgressive = 0.0;
    for (int k = 0; k < 6; k++) {
        const bool flipped = k >= 3;
        double a1d, a2;
        synthMountAxesSide(Ttruth, headTruth, STARS_WIDE[k][0], STARS_WIDE[k][1],
                           flipped, a1d, a2);
        feedStar(stepped, STARS_WIDE[k][0], STARS_WIDE[k][1], a1d, a2);

        // Stars 3..5: progressive, as Command_A does mid-session. Cone stays off.
        if (k >= 2 && k < 5) {
            char msg[64];
            sprintf(msg, "progressive fit failed after star %d", k + 1);
            TEST_ASSERT_TRUE_MESSAGE(stepped.fitProgressive(), msg);
            TEST_ASSERT_TRUE_MESSAGE((stepped.getRigidMask() & COORDCONV_FIT_CONE) == 0,
                                     "cone published before both piers had 3 stars");
        }
        if (k == 4) {
            double az0, alt0, a1r, a2r;
            TEST_ASSERT_TRUE(stepped.getStar(0, az0, alt0, a1r, a2r));
            double a1cmd, a2cmd;
            modelAxes(stepped, az0, alt0, /*flipped=*/false, a1cmd, a2cmd);
            missSideAProgressive = skyMissFromAxes(stepped, az0, alt0, a1cmd, a2cmd);
            TEST_ASSERT_TRUE_MESSAGE(missSideAProgressive < 5.0 * ARCMIN,
                                     "progressive model must keep side A usable");
            (void)a1r;
            (void)a2r;
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(stepped.fitRigidModel(), "final 3+3 fit failed");
    TEST_ASSERT_TRUE_MESSAGE((stepped.getRigidMask() & COORDCONV_FIT_CONE) != 0,
                             "final 3+3 must publish CH");
    TEST_ASSERT_TRUE_MESSAGE(stepped.getRigidRms() < 1.0 * ARCMIN,
                             "final fit residual must stay tight on the raw stars");

    double az0, alt0, a1r0, a2r0;
    TEST_ASSERT_TRUE(stepped.getStar(0, az0, alt0, a1r0, a2r0));

    // Side A (under pole): HeadGeom under-pole inverse — what a correct preferred
    // pier UNDER goto must command. Final model must keep star 1 on-sky.
    double a1Under, a2Under;
    modelAxes(stepped, az0, alt0, /*flipped=*/false, a1Under, a2Under);
    const double missUnder = skyMissFromAxes(stepped, az0, alt0, a1Under, a2Under);
    TEST_ASSERT_TRUE_MESSAGE(missUnder < 1.0 * ARCMIN,
                             "final model with under-pole inverse must keep side A on star");

    // Same sky, other pier: correct beyond-pole HeadGeom inverse still points
    // at the star (mount on the flipped configuration).
    double a1OverOk, a2OverOk;
    modelAxes(stepped, az0, alt0, /*flipped=*/true, a1OverOk, a2OverOk);
    const double missOverOk = skyMissFromAxes(stepped, az0, alt0, a1OverOk, a2OverOk);
    TEST_ASSERT_TRUE_MESSAGE(missOverOk < 1.0 * ARCMIN,
                             "beyond-pole HeadGeom inverse must keep the star on sky");

    // Buggy goTo path after ending on the other pier: prefer OVER, build under
    // axes, then ideal-flip them. With CH this leaves the star by degrees while
    // :GXAr# (getRigidRms) stays small — the progressive-vs-final field pattern.
    double a1OverBug, a2OverBug;
    idealPierFlip(a1Under, a2Under, a1OverBug, a2OverBug);
    const double missOverBug = skyMissFromAxes(stepped, az0, alt0, a1OverBug, a2OverBug);
    TEST_ASSERT_TRUE_MESSAGE(missOverBug > 30.0 * ARCMIN,
                             "ideal pier mirror with CH must miss by tens of arcmin");
    TEST_ASSERT_TRUE_MESSAGE(missOverBug > 10.0 * missOverOk + 10.0 * ARCMIN,
                             "ideal pier mirror must be far worse than HeadGeom beyond-pole");
    TEST_ASSERT_TRUE_MESSAGE(missOverBug > 5.0 * missSideAProgressive + 10.0 * ARCMIN,
                             "final CH + ideal flip must be much worse than progressive side A");
    (void)a1r0;
    (void)a2r0;
}

void test_after_3plus3_goto_all_six_stars_match_raw_axes_alternating_pier(void)
{
    // After a finished 3+3 with CH, every alignment star must be recenterable by
    // commanding the stored raw instrument axes (getInstr frame). Visit them
    // alternating pier side — A1, B1, A2, B2, A3, B3 — the way a check after
    // close-out flips between sides. Each goto uses the predictTargetHO path:
    // To_Coord_IN(..., beyondPole) for that star's pier, no ideal angle2Step mirror.
    const HeadModel headTruth(1.0 * DEG, 0.25 * DEG, 0.10 * DEG);
    double Ttruth[3][3];
    truthT(Ttruth);

    CoordConv cc;
    bool flippedStar[6];
    for (int k = 0; k < 6; k++) {
        flippedStar[k] = k >= 3;
        double a1d, a2;
        synthMountAxesSide(Ttruth, headTruth, STARS_WIDE[k][0], STARS_WIDE[k][1],
                           flippedStar[k], a1d, a2);
        feedStar(cc, STARS_WIDE[k][0], STARS_WIDE[k][1], a1d, a2);
        if (k >= 2 && k < 5)
            TEST_ASSERT_TRUE(cc.fitProgressive());
    }
    TEST_ASSERT_TRUE_MESSAGE(cc.fitRigidModel(), "final 3+3 fit failed");
    TEST_ASSERT_TRUE_MESSAGE((cc.getRigidMask() & COORDCONV_FIT_CONE) != 0,
                             "final 3+3 must publish CH");
    const double rms = cc.getRigidRms();
    TEST_ASSERT_TRUE_MESSAGE(rms < 30.0 * ARCSEC,
                             "fit RMS must leave stars within the FOV");
    // Sky centering follows the fit residual. Raw axis agreement can be a bit
    // looser with a large CH (inverse vs stored synth) but must stay well inside
    // a typical eyepiece field — 1 arcmin is the ceiling here.
    const double axisTol = 1.0 * ARCMIN;
    const double skyTol = rms + 5.0 * ARCSEC;

    // Alternating pier: side A star i, then side B star i (indices 0,3,1,4,2,5).
    const int order[6] = { 0, 3, 1, 4, 2, 5 };
    for (int n = 0; n < 6; n++) {
        const int k = order[n];
        double az, alt, a1Stored, a2Stored;
        TEST_ASSERT_TRUE(cc.getStar(k, az, alt, a1Stored, a2Stored));
        TEST_ASSERT_EQUAL_MESSAGE(flippedStar[k], fabs(a2Stored) > M_PI_2 - 1e-3,
                                  "stored axis2 pier must match the synth side");

        // Same conversion goToHor/predictTargetHO uses for that pier.
        Coord_HO ho(0.0, alt, LA3::modRad(-az - M_PI), false);
        Coord_IN cmd = ho.To_Coord_IN(cc.Tinv, cc.head, flippedStar[k]);
        const double a1Cmd = cmd.Axis1_direct();
        const double a2Cmd = cmd.Axis2();

        char msg[96];
        sprintf(msg, "star %d (pier %c) axis1Direct vs stored raw",
                k + 1, flippedStar[k] ? 'B' : 'A');
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(axisTol, a1Stored, a1Cmd, msg);
        sprintf(msg, "star %d (pier %c) axis2 vs stored raw",
                k + 1, flippedStar[k] ? 'B' : 'A');
        TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(axisTol, a2Stored, a2Cmd, msg);

        // Commanded pose must put the catalogue star on axis (centered).
        const double miss = skyMissFromAxes(cc, az, alt, a1Cmd, a2Cmd);
        sprintf(msg, "star %d (pier %c) sky miss after goto",
                k + 1, flippedStar[k] ? 'B' : 'A');
        TEST_ASSERT_TRUE_MESSAGE(miss < skyTol, msg);

        // modelAxes helper must agree with To_Coord_IN(beyondPole) — one path.
        double a1m, a2m;
        modelAxes(cc, az, alt, flippedStar[k], a1m, a2m);
        TEST_ASSERT_DOUBLE_WITHIN(1e-12, a1Cmd, a1m);
        TEST_ASSERT_DOUBLE_WITHIN(1e-12, a2Cmd, a2m);
    }
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

    RUN_TEST(test_flip_pays_off_from_five_stars);
    RUN_TEST(test_fit_drops_cone_without_a_meridian_flip);
    RUN_TEST(test_fit_withholds_cone_below_three_per_side);
    RUN_TEST(test_fit_recovers_cone_with_three_stars_per_side);
    RUN_TEST(test_onstep_3plus3_dataset_recovers_head);
    RUN_TEST(test_onstep_3plus3_progressive_matches_final_head);
    RUN_TEST(test_fit_recovers_axis2_index);
    RUN_TEST(test_fit_selects_one_of_the_correlated_pair);
    RUN_TEST(test_fit_drops_terms_for_single_declination);
    RUN_TEST(test_fit_does_not_invent_geometry_from_noise);
    RUN_TEST(test_each_added_star_improves_the_next_goto);
    RUN_TEST(test_fit_improves_pointing_off_the_star_set);
    RUN_TEST(test_fit_rejects_too_few_stars);
    RUN_TEST(test_fit_leaves_model_untouched_on_failure);
    RUN_TEST(test_fit_keeps_t_orthonormal);
    RUN_TEST(test_clean_clears_head_and_stars);
    RUN_TEST(test_head_getter_setter_roundtrip);

    RUN_TEST(test_post_fit_sync_throws_side_a_off);
    RUN_TEST(test_hasHead_fallback_discards_six_star_t);
    RUN_TEST(test_goto_other_pier_with_cone_needs_beyond_pole_inverse);
    RUN_TEST(test_after_3plus3_goto_all_six_stars_match_raw_axes_alternating_pier);

    return UNITY_END();
}
