// Telescope coordinate conversion
// (C) 2016 Markus L. Noga
// (C) 2019 Charles Lemaire

// Inspired by:
// Toshimi Taki, "A New Concept in Computer-Aided Telescopes", Sky & Telescope Feb 1989, pp. 194-196
// Wikipedia, siderial time article
// Wikipedia, local hour angle article
// Cosine direction vectors


#include "TeenAstroCoordConv.hpp"

#include <Arduino.h>

using namespace std;

// CoordConv methods
//

// get the transformation to be stored into EEPROM
void CoordConv::getT(float& m11, float& m12, float& m13, float& m21, float& m22, float& m23, float& m31, float& m32, float& m33)
{
  m11 = T[0][0];
  m12 = T[0][1];
  m13 = T[0][2];
  m21 = T[1][0];
  m22 = T[1][1];
  m23 = T[1][2];
  m31 = T[2][0];
  m32 = T[2][1];
  m33 = T[2][2];
}

// set the transformation from EEPROM
void CoordConv::setT(float m11, float m12, float m13, float m21, float m22, float m23, float m31, float m32, float m33)
{
  T[0][0] = m11;
  T[0][1] = m12;
  T[0][2] = m13;
  T[1][0] = m21;
  T[1][1] = m22;
  T[1][2] = m23;
  T[2][0] = m31;
  T[2][1] = m32;
  T[2][2] = m33;
  refs = 0;
  isready = true;
}

void CoordConv::setTinvFromT() {
  invert(Tinv, T);
}

double CoordConv::polErrorDeg(double latRad, PolarErrSel sel) const {
  if (!isready)
    return 0.0;
  double x_id[3] = { cos(latRad), 0.0, sin(latRad) };
  double x[3] = { Tinv[0][2], Tinv[1][2], Tinv[2][2] };
  double nrm = norm(x);
  if (nrm < 1e-15)
    return 0.0;
  for (int i = 0; i < 3; i++)
    x[i] /= nrm;
  switch (sel) {
  case PE_EQ_AZ:
    if (x[0] == 0.0)
      return (x[1] > 0.0 ? 90.0 : -90.0);
    return atan(x[1] / x[0]) * 180.0 / M_PI;
  case PE_EQ_ALT:
    if (x[0] == 0.0)
      return (x[2] > 0.0 ? 90.0 - latRad * 180.0 / M_PI : -90.0 - latRad * 180.0 / M_PI);
    return (atan(x[2] / x[0]) - latRad) * 180.0 / M_PI;
  case PE_POL_W: {
    double c = x[0] * x_id[0] + x[1] * x_id[1] + x[2] * x_id[2];
    if (c > 1.0) c = 1.0;
    if (c < -1.0) c = -1.0;
    return acos(c) * 180.0 / M_PI;
  }
  default:
    return 0.0;
  }
}

// add reference star (all values in radians). adding more than three has no effect
void CoordConv::addReference(double angle1, double angle2, double axis1, double axis2) {
  // Note: this only feeds the two star Taki pass. Stars for the rigid fit are
  // recorded separately via addStar(), so that a caller collecting more than
  // two stars does not restart this pass and rebuild T from an arbitrary pair.
#ifdef DEBUG_COUT
  //cout << "adding ref star: angle1 " << angle1 << "r angle2 " << angle2 << "r axis1 " << axis1 << "r axis2 " << axis2 << "r" << endl;
#endif

  toDirCos(dcHDRef[refs], angle1, angle2);
  printV("dcHD", dcHDRef[refs]);
  toDirCos(dcAARef[refs], axis1, axis2);
  printV("dcAA", dcAARef[refs]);
  ax1[refs] = axis1;
  ax2[refs] = axis2;
  refs++;
  if (storedAxes < 2)
    storedAxes = refs;
  if (refs == 2)
  {
    //check angle between the input observation
    anglediff = angle2Vectors(dcHDRef[0], dcHDRef[1]) - angle2Vectors(dcAARef[0], dcAARef[1]);
    calculateThirdReference();
  }
  else
  {
    isready = false;
  }
}

void CoordConv::minimizeAxis1(double Offset)
{
  double axis3, axis2, direct_axis1;
  LA3::getEulerRxRyRz(Tinv, axis3, axis2, direct_axis1);
#ifdef DEBUG_COUT
  char txt[512];
  sprintf(txt, "%s \t [%f, %f, %f]", "rotations", axis3 * 180. / PI, axis2 * 180. / PI, direct_axis1 * 180. / PI);
  Serial.println(txt);
#endif


  ax1[0] -= direct_axis1 - Offset ;
  ax1[1] -= direct_axis1 - Offset ;

  toDirCos(dcAARef[0], ax1[0], ax2[0]);
  toDirCos(dcAARef[1], ax1[1], ax2[1]);
  anglediff = angle2Vectors(dcHDRef[0], dcHDRef[1]) - angle2Vectors(dcAARef[0], dcAARef[1]);
  refs = 2;
  calculateThirdReference();
}

void CoordConv::minimizeAxis2()
{
  double delta = 0;
  #ifdef DEBUG_COUT
  char txt[512];
  Serial.println("minimizeSync");
  #endif
  for (int k = 0; k < 5; k++)
  {
    double denom = 0;
    ax2[0] += 0.8*delta;
    ax2[1] += 0.8*delta;

    toDirCos(dcAARef[0], ax1[0], ax2[0]);
    toDirCos(dcAARef[1], ax1[1], ax2[1]);
    anglediff = angle2Vectors(dcAARef[0], dcAARef[1]) - angle2Vectors(dcHDRef[0], dcHDRef[1]);

    denom += -dcAARef[0][0] * sin(ax2[1]) * cos(-ax1[1]);
    denom += -dcAARef[1][0] * sin(ax2[0]) * cos(-ax1[0]);
    denom += -dcAARef[0][1] * sin(ax2[1]) * sin(-ax1[1]);
    denom += -dcAARef[1][1] * sin(ax2[0]) * sin(-ax1[0]);
    denom += +dcAARef[0][2] * cos(ax2[1]);
    denom += +dcAARef[1][2] * cos(ax2[0]);

    delta = anglediff / denom;
#ifdef DEBUG_COUT
    sprintf(txt, "interartion %d: \t anglediff %f arcmin,delta = %f arcmin]", k, anglediff * 180. / PI * 60, delta * 180. / PI * 60);
    Serial.println(txt);
#endif
 
  }
  refs = 2;
  calculateThirdReference();
}

void CoordConv::setPoleError(double latRad, double dAzRad, double dAltRad, double indexRad)
{
  // Invert polErrorDeg. Azimuth is atan(x[1]/x[0]) and altitude is
  // atan(x[2]/x[0]) - lat, so a toDirCos of the offset pole is not the vector
  // those two angles describe once the azimuth error is nonzero.
  const double a = latRad + dAltRad;
  const double tAz = tan(dAzRad);
  const double tAlt = tan(a);
  double p[3];
  if (fabs(cos(a)) < 1e-6)
  {
    p[0] = 0.0;
    p[1] = 0.0;
    p[2] = sin(a) >= 0.0 ? 1.0 : -1.0;
  }
  else
  {
    double x0 = 1.0 / sqrt(1.0 + tAz * tAz + tAlt * tAlt);
    if (cos(a) < 0.0)
      x0 = -x0;
    p[0] = x0;
    p[1] = x0 * tAz;
    p[2] = x0 * tAlt;
  }

  double ref[3] = { 0.0, 0.0, 1.0 };
  if (fabs(p[2]) > 0.9)
  {
    ref[0] = 1.0;
    ref[1] = 0.0;
    ref[2] = 0.0;
  }
  double u[3], v[3];
  crossProduct(u, ref, p);
  normalize(u, u);
  crossProduct(v, p, u);
  normalize(v, v);

  const double c = cos(indexRad);
  const double s = sin(indexRad);
  double ur[3], vr[3];
  for (int i = 0; i < 3; i++)
  {
    ur[i] = c * u[i] + s * v[i];
    vr[i] = -s * u[i] + c * v[i];
  }
  for (int i = 0; i < 3; i++)
  {
    Tinv[i][0] = ur[i];
    Tinv[i][1] = vr[i];
    Tinv[i][2] = p[i];
  }
  transpose(T, Tinv);
  isready = true;
  refs = 0;
}

bool CoordConv::alignTwoStarKnownGeom(double coneRad, double perpRad)
{
  if (storedAxes < 2)
    return false;

  // Take the known cone and perpendicularity out of the two measured axes, then
  // let the usual two-star build estimate the pole. The head keeps both, so
  // pointing puts them back on the real encoders.
  HeadModel known(coneRad, perpRad, 0.0);
  HeadModel zero;
  double corr1[2], corr2[2];
  for (int i = 0; i < 2; i++)
  {
    double dc[3];
    HeadGeom::forward(dc, ax1[i], ax2[i], known);
    if (!HeadGeom::inverse(dc, zero, ax2[i], corr1[i], corr2[i]))
      return false;
  }
  for (int i = 0; i < 2; i++)
  {
    ax1[i] = corr1[i];
    ax2[i] = corr2[i];
    toDirCos(dcAARef[i], ax1[i], ax2[i]);
  }
  head = known;
  refs = 2;
  return calculateThirdReference();
}

// Calculate third reference star from two provided ones. Returns false if more or less than two provided 
bool CoordConv::calculateThirdReference() {
  if (refs != 2)
  {
    isready = false;
    return false;
  }

#ifdef DEBUG_COUT
  //cout << "adding artificial 3rd ref star" << endl;
#endif
  crossProduct(dcHDRef[2], dcHDRef[0], dcHDRef[1]);
  normalize(dcHDRef[2], dcHDRef[2]);
  printV("dcHD", dcHDRef[2]);
  crossProduct(dcAARef[2], dcAARef[0], dcAARef[1]);
  normalize(dcAARef[2], dcAARef[2]);
  printV("dcAA", dcAARef[2]);

  buildTransformations();
  isready = true;
  refs = 0;
  return true;
}

// Build coordinate system transformation matrix
void CoordConv::buildTransformations() {
  double dcAARefT[3][3], dcHDRefT[3][3], inv[3][3], test[3][3];

  printV("dcAA ", dcAARef);
  transpose(dcAARefT, dcAARef);
  printV("dcAAt", dcAARefT);

  printV("dcHD ", dcHDRef);
  transpose(dcHDRefT, dcHDRef);
  printV("dcHDt", dcHDRefT);
  invert(inv, dcHDRefT);
  printV("inv", inv);
  multiply(test, dcHDRefT, inv);
  printV("test", test);
  multiply(T, dcAARefT, inv);
  printV("T", T);
  invert(Tinv, T);
  printV("Tinv", Tinv);
  multiply(test, T, Tinv);
  printV("test", test);

  // now compute the optimal rotation matrix:
  getsvd(Tinv, u, v);
  printV("u", u);
  printV("v", v);

  double du = determinant(u);
  double dv = determinant(v);

  double tmp[3][3];
  double vt[3][3];
  double diag[3][3];
  transpose(vt, v);
  getIdentityMatrix(diag);

  diag[2][2] = du * dv;
  printV("diag", diag);
  multiply(tmp, diag, vt);
  multiply(Tinv, u, tmp);
  printV("Tinv", Tinv);
  transpose(T, Tinv);
  multiply(test, T, Tinv);
  printV("test", test);
}

// -----------------------------------------------------------------------------
// Rigid six degree of freedom model
//
// The mount is modelled as an exact rotation chain, three degrees of freedom in
// T plus three in the head. Both halves are fitted together by damped
// Gauss-Newton on the angular residual of the retained stars.
//
// The model is linear in the head terms to first order and the Jacobian with
// respect to a rotation of T is a cross product, so convergence from the two
// star Taki seed takes two or three iterations.
// -----------------------------------------------------------------------------

namespace {

const int RIGID_NPAR = 6;

/// Rodrigues rotation for an axis-angle vector, exact so T stays orthonormal.
void rotationFromVector(double (&R)[3][3], const double (&d)[3])
{
  const double th2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
  if (th2 < 1e-30)
  {
    LA3::getIdentityMatrix(R);
    return;
  }
  const double th = sqrt(th2);
  const double x = d[0] / th, y = d[1] / th, z = d[2] / th;
  const double c = cos(th), s = sin(th), C = 1.0 - c;
  R[0][0] = c + x * x * C;     R[0][1] = x * y * C - z * s; R[0][2] = x * z * C + y * s;
  R[1][0] = y * x * C + z * s; R[1][1] = c + y * y * C;     R[1][2] = y * z * C - x * s;
  R[2][0] = z * x * C - y * s; R[2][1] = z * y * C + x * s; R[2][2] = c + z * z * C;
}

/// Force a near-rotation matrix to be exactly orthonormal, in double precision.
///
/// buildTransformations() runs its SVD through svd3, which is single precision,
/// so the seed T is only accurate to about 1e-7 and is not quite a rotation.
/// Gram-Schmidt on the rows cleans that up before the fit refines it, otherwise
/// the float error propagates into the fitted model and caps its accuracy.
void orthonormalize(double (&M)[3][3])
{
  double r0[3] = { M[0][0], M[0][1], M[0][2] };
  double r1[3] = { M[1][0], M[1][1], M[1][2] };
  double r2[3];
  LA3::normalize(r0, r0);
  const double d = LA3::dotProduct(r1, r0);
  r1[0] -= d * r0[0];
  r1[1] -= d * r0[1];
  r1[2] -= d * r0[2];
  LA3::normalize(r1, r1);
  LA3::crossProduct(r2, r0, r1);
  LA3::normalize(r2, r2);
  M[0][0] = r0[0]; M[0][1] = r0[1]; M[0][2] = r0[2];
  M[1][0] = r1[0]; M[1][1] = r1[1]; M[1][2] = r1[2];
  M[2][0] = r2[0]; M[2][1] = r2[1]; M[2][2] = r2[2];
}

/// Orthonormal tangent basis at the unit vector \p p.
void tangentBasis(const double (&p)[3], double (&e1)[3], double (&e2)[3])
{
  double a[3];
  if (fabs(p[2]) < 0.9) { a[0] = 0; a[1] = 0; a[2] = 1; }
  else                  { a[0] = 1; a[1] = 0; a[2] = 0; }
  LA3::crossProduct(e1, a, p);
  LA3::normalize(e1, e1);
  LA3::crossProduct(e2, p, e1);
  LA3::normalize(e2, e2);
}

/// Solve the symmetric positive definite system A x = b in place by Cholesky.
/// Returns false when A is not positive definite, i.e. the star geometry does
/// not constrain the model.
bool choleskySolve(double A[RIGID_NPAR][RIGID_NPAR], double b[RIGID_NPAR], double x[RIGID_NPAR])
{
  double L[RIGID_NPAR][RIGID_NPAR];
  for (int i = 0; i < RIGID_NPAR; i++)
    for (int j = 0; j < RIGID_NPAR; j++)
      L[i][j] = 0.0;

  for (int i = 0; i < RIGID_NPAR; i++)
  {
    for (int j = 0; j <= i; j++)
    {
      double s = A[i][j];
      for (int k = 0; k < j; k++)
        s -= L[i][k] * L[j][k];
      if (i == j)
      {
        if (s <= 1e-18)
          return false;
        L[i][i] = sqrt(s);
      }
      else
      {
        L[i][j] = s / L[j][j];
      }
    }
  }
  // forward then back substitution
  double y[RIGID_NPAR];
  for (int i = 0; i < RIGID_NPAR; i++)
  {
    double s = b[i];
    for (int k = 0; k < i; k++)
      s -= L[i][k] * y[k];
    y[i] = s / L[i][i];
  }
  for (int i = RIGID_NPAR - 1; i >= 0; i--)
  {
    double s = y[i];
    for (int k = i + 1; k < RIGID_NPAR; k++)
      s -= L[k][i] * x[k];
    x[i] = s / L[i][i];
  }
  return true;
}

/// Solve A x = b over the \p keep subset only, leaving the rest of x at zero.
bool solveSubset(const double A[RIGID_NPAR][RIGID_NPAR], const double b[RIGID_NPAR],
                 const bool keep[RIGID_NPAR], double x[RIGID_NPAR])
{
  int idx[RIGID_NPAR];
  int n = 0;
  for (int i = 0; i < RIGID_NPAR; i++)
  {
    x[i] = 0.0;
    if (keep[i])
      idx[n++] = i;
  }
  if (n == 0)
    return false;

  // Gather the sub-system, solve at full size by padding the unused rows and
  // columns with an identity block and a zero right hand side.
  double As[RIGID_NPAR][RIGID_NPAR], bs[RIGID_NPAR], xs[RIGID_NPAR];
  for (int i = 0; i < RIGID_NPAR; i++)
  {
    bs[i] = (i < n) ? b[idx[i]] : 0.0;
    for (int j = 0; j < RIGID_NPAR; j++)
      As[i][j] = (i < n && j < n) ? A[idx[i]][idx[j]] : (i == j ? 1.0 : 0.0);
  }
  if (!choleskySolve(As, bs, xs))
    return false;
  for (int i = 0; i < n; i++)
    x[idx[i]] = xs[i];
  return true;
}

/// Squared norm of Jacobian column \p c after removing everything the \p keep
/// columns already explain, i.e. the Schur complement of N at c. Derived from
/// the normal matrix alone, so no Jacobian rows need to be retained.
double independentInfo(const double N[RIGID_NPAR][RIGID_NPAR], const bool keep[RIGID_NPAR], int c)
{
  double rhs[RIGID_NPAR], sol[RIGID_NPAR];
  for (int i = 0; i < RIGID_NPAR; i++)
    rhs[i] = N[i][c];
  if (!solveSubset(N, rhs, keep, sol))
    return N[c][c];
  double explained = 0.0;
  for (int i = 0; i < RIGID_NPAR; i++)
    if (keep[i])
      explained += N[c][i] * sol[i];
  const double schur = N[c][c] - explained;
  return schur > 0.0 ? schur : 0.0;
}

/// Choose which parameters the star distribution actually supports.
///
/// The three rotation terms are always fitted: two stars already determine
/// them, and they are what the two star pass produced. The head terms are added
/// one at a time, greedily, and only while a candidate still carries enough
/// information that is independent of the terms already chosen.
///
/// This matters because the head terms are not mutually independent in general.
/// Cone error and axis2 non-perpendicularity both displace axis1 with a similar
/// dependence on axis2, so over a narrow declination span only their difference
/// is observable. Fitting both anyway splits the pair into two large opposing
/// values that reproduce the calibration stars but degrade pointing away from
/// them, and look alarming when displayed. Leaving the unsupported term at zero
/// costs nothing on these stars and keeps the model honest.
///
/// The pair does become separable once a star is taken beyond the pole, because
/// a cone error is fixed in the tube and reverses across the flip while the
/// head's non-perpendicularity does not. That is not sufficient to publish a
/// cone value: \p eligible (COORDCONV_FIT_* bits) withholds cone unless each
/// pier side has at least COORDCONV_MIN_CONE_PER_SIDE stars, and withholds perp
/// below COORDCONV_MIN_PERP_STARS. The rank test still runs on whatever remains,
/// so a session that never leaves one pier side does not get a cone.
void selectParams(const double N[RIGID_NPAR][RIGID_NPAR], unsigned char nstars,
                  unsigned char eligible, bool (&active)[RIGID_NPAR])
{
  // A term must retain at least this fraction of its sensitivity after the
  // already selected terms are projected out; equivalently 1 - R^2 against
  // them. The threshold is placed inside a measured gap rather than picked for
  // roundness: sweeping 4 to 6 stars over every choice of which one is taken
  // beyond the pole, the cone term scores at most 0.00046 when every star is on
  // the same side and at least 0.0278 when one is not, so the two populations
  // are 60 times apart and this sits near the middle of that gap.
  //
  // Erring low is deliberate. Declining a term the stars cannot separate is
  // cheap, because the remaining terms absorb the combination that is actually
  // observable. Declining one they can separate is not: the flipped star's
  // reading then cannot be reproduced at all, and pointing degrades further
  // than it would have with no flipped star in the set.
  //
  // The score is a property of the Jacobian, which depends on the axis readings
  // and T but not on the recorded sky positions, so it does not drift with
  // measurement noise the way a residual based criterion would.
  const double minIndependent = 0.004;
  // And it must move the prediction at all: guards a term the geometry makes
  // completely inert, such as the axis2 index for stars all at one declination.
  const double minSensitivity = 1e-6 * (double)nstars;

  for (int i = 0; i < 3; i++)
    active[i] = true;
  for (int i = 3; i < RIGID_NPAR; i++)
    active[i] = false;

  for (;;)
  {
    int best = -1;
    double bestRatio = 0.0;
    for (int c = 3; c < RIGID_NPAR; c++)
    {
      const unsigned char bit = (unsigned char)(1u << (c - 3));
      if ((eligible & bit) == 0 || active[c] || N[c][c] <= minSensitivity)
        continue;
      const double ratio = independentInfo(N, active, c) / N[c][c];
      if (ratio > bestRatio)
      {
        bestRatio = ratio;
        best = c;
      }
    }
    if (best < 0 || bestRatio < minIndependent)
      break;
    active[best] = true;
  }
}

} // namespace

void CoordConv::addStar(double angle1, double angle2, double axis1Direct, double axis2)
{
  if (nstars >= COORDCONV_MAX_STARS)
    return;
  starSky[nstars][0] = angle1;
  starSky[nstars][1] = angle2;
  starAxis[nstars][0] = axis1Direct;
  starAxis[nstars][1] = axis2;
  nstars++;
}

void CoordConv::predictSky(double (&p)[3], unsigned char i, const HeadModel &h) const
{
  double dcIn[3];
  HeadGeom::forward(dcIn, starAxis[i][0], starAxis[i][1], h);
  // CoordConv defines T * dcSky = dcInstrument, so the sky direction is Tinv * dcInstrument.
  LA3::multiply(p, Tinv, dcIn);
  LA3::normalize(p, p);
}

double CoordConv::residualRms() const
{
  if (nstars == 0)
    return 0.0;
  double sum = 0.0;
  for (unsigned char i = 0; i < nstars; i++)
  {
    double p[3], t[3];
    predictSky(p, i, head);
    LA3::toDirCos(t, starSky[i][0], starSky[i][1]);
    const double a = LA3::angle2Vectors(p, t);
    sum += a * a;
  }
  return sqrt(sum / nstars);
}

double CoordConv::accumulateNormals(const double (&Tinv_w)[3][3], const HeadModel &head_w,
                                    const double (&targets)[COORDCONV_MAX_STARS][3],
                                    double (&N)[RIGID_NPAR][RIGID_NPAR],
                                    double (&g)[RIGID_NPAR]) const
{
  // Numeric step for the head Jacobian columns. The model is smooth and the
  // terms are radians, so this is comfortably inside double precision.
  const double hStep = 1e-7;

  for (int a = 0; a < RIGID_NPAR; a++)
  {
    g[a] = 0.0;
    for (int b = 0; b < RIGID_NPAR; b++)
      N[a][b] = 0.0;
  }

  double sumSq = 0.0;
  for (unsigned char i = 0; i < nstars; i++)
  {
    double dcIn[3], p[3];
    HeadGeom::forward(dcIn, starAxis[i][0], starAxis[i][1], head_w);
    LA3::multiply(p, Tinv_w, dcIn);
    LA3::normalize(p, p);

    double e1[3], e2[3];
    tangentBasis(p, e1, e2);

    const double r[3] = { targets[i][0] - p[0], targets[i][1] - p[1], targets[i][2] - p[2] };
    const double r1 = LA3::dotProduct(r, e1);
    const double r2 = LA3::dotProduct(r, e2);
    sumSq += r1 * r1 + r2 * r2;

    double J1[RIGID_NPAR], J2[RIGID_NPAR];

    // Columns 0..2: rotating the sky side of Tinv. d(R(d) p)/d(d_k) = u_k x p.
    for (int k = 0; k < 3; k++)
    {
      double uk[3] = { 0, 0, 0 };
      uk[k] = 1.0;
      double dp[3];
      LA3::crossProduct(dp, uk, p);
      J1[k] = LA3::dotProduct(dp, e1);
      J2[k] = LA3::dotProduct(dp, e2);
    }

    // Columns 3..5: the head terms, by central difference.
    for (int k = 0; k < 3; k++)
    {
      HeadModel hp = head_w, hm = head_w;
      double *pp = (k == 0) ? &hp.cone : (k == 1) ? &hp.perp : &hp.idx2;
      double *pm = (k == 0) ? &hm.cone : (k == 1) ? &hm.perp : &hm.idx2;
      *pp += hStep;
      *pm -= hStep;

      double dcP[3], dcM[3], vP[3], vM[3];
      HeadGeom::forward(dcP, starAxis[i][0], starAxis[i][1], hp);
      HeadGeom::forward(dcM, starAxis[i][0], starAxis[i][1], hm);
      LA3::multiply(vP, Tinv_w, dcP);
      LA3::multiply(vM, Tinv_w, dcM);
      const double dp[3] = { (vP[0] - vM[0]) / (2 * hStep),
                             (vP[1] - vM[1]) / (2 * hStep),
                             (vP[2] - vM[2]) / (2 * hStep) };
      J1[3 + k] = LA3::dotProduct(dp, e1);
      J2[3 + k] = LA3::dotProduct(dp, e2);
    }

    for (int a = 0; a < RIGID_NPAR; a++)
    {
      g[a] += J1[a] * r1 + J2[a] * r2;
      for (int b = 0; b < RIGID_NPAR; b++)
        N[a][b] += J1[a] * J1[b] + J2[a] * J2[b];
    }
  }
  return sumSq;
}

bool CoordConv::fitRigidModel(double *rmsOut, int *iterOut)
{
  if (!isready || nstars < COORDCONV_MIN_RIGID_STARS)
    return false;

  // Work on copies so a failed fit leaves the existing model untouched.
  double Tinv_w[3][3];
  LA3::copy(Tinv_w, Tinv);
  orthonormalize(Tinv_w);
  HeadModel head_w = head;

  double targets[COORDCONV_MAX_STARS][3];
  for (unsigned char i = 0; i < nstars; i++)
    LA3::toDirCos(targets[i], starSky[i][0], starSky[i][1]);

  // Decide which head terms this star distribution can actually support, using
  // the Jacobian at the seed. Doing it once keeps the active set fixed for the
  // whole descent, so the iteration cannot oscillate between parameter sets.
  bool active[RIGID_NPAR];
  {
    double N0[RIGID_NPAR][RIGID_NPAR], g0[RIGID_NPAR];
    accumulateNormals(Tinv_w, head_w, targets, N0, g0);
    // Perp from four stars on either side. Cone only when each pier side has
    // at least three stars. Idx2 has no extra count gate; the rank test still
    // applies to all three.
    int nIn = 0, nOut = 0;
    pierSideCounts(nIn, nOut);
    unsigned char eligible = COORDCONV_FIT_IDX2;
    if (nstars >= COORDCONV_MIN_PERP_STARS)
      eligible = (unsigned char)(eligible | COORDCONV_FIT_PERP);
    if (nIn >= COORDCONV_MIN_CONE_PER_SIDE && nOut >= COORDCONV_MIN_CONE_PER_SIDE)
      eligible = (unsigned char)(eligible | COORDCONV_FIT_CONE);
    selectParams(N0, nstars, eligible, active);
  }

  double lambda = 1e-9;
  double bestRms = -1.0;
  int iter = 0;

  double Tinv_best[3][3];
  LA3::copy(Tinv_best, Tinv_w);
  HeadModel head_best = head_w;

  for (iter = 0; iter < 30; iter++)
  {
    double N[RIGID_NPAR][RIGID_NPAR];
    double g[RIGID_NPAR];
    const double sumSq = accumulateNormals(Tinv_w, head_w, targets, N, g);

    const double rms = sqrt(sumSq / nstars);
    if (bestRms < 0.0 || rms < bestRms)
    {
      bestRms = rms;
      LA3::copy(Tinv_best, Tinv_w);
      head_best = head_w;
    }

    // ---- Levenberg-Marquardt damping, insurance against a poor star spread ----
    for (int a = 0; a < RIGID_NPAR; a++)
      N[a][a] *= (1.0 + lambda);

    // Solve over the selected parameters only. Terms left out stay at zero
    // rather than being driven by whatever the residuals happen to look like.
    double step[RIGID_NPAR];
    if (!solveSubset(N, g, active, step))
      break;

    // ---- apply ----
    const double dvec[3] = { step[0], step[1], step[2] };
    double R[3][3], Tnew[3][3];
    rotationFromVector(R, dvec);
    LA3::multiply(Tnew, R, Tinv_w);
    LA3::copy(Tinv_w, Tnew);
    head_w.cone += step[3];
    head_w.perp += step[4];
    head_w.idx2 += step[5];

    const double mag = fabs(step[0]) + fabs(step[1]) + fabs(step[2]) +
                       fabs(step[3]) + fabs(step[4]) + fabs(step[5]);
    if (mag < 1e-12)
    {
      iter++;
      break;
    }
    lambda *= 0.5;
    if (lambda < 1e-12)
      lambda = 1e-12;
  }

  // Score the final state and keep whichever iterate was best.
  {
    double sumSq = 0.0;
    for (unsigned char i = 0; i < nstars; i++)
    {
      double dcIn[3], p[3];
      HeadGeom::forward(dcIn, starAxis[i][0], starAxis[i][1], head_w);
      LA3::multiply(p, Tinv_w, dcIn);
      LA3::normalize(p, p);
      const double a = LA3::angle2Vectors(p, targets[i]);
      sumSq += a * a;
    }
    const double rms = sqrt(sumSq / nstars);
    if (bestRms < 0.0 || rms <= bestRms)
    {
      bestRms = rms;
      LA3::copy(Tinv_best, Tinv_w);
      head_best = head_w;
    }
  }

  if (bestRms < 0.0)
    return false;

  // Unselected terms were never stepped, but clear them explicitly so the mask
  // and the stored values cannot disagree.
  if (!active[3]) head_best.cone = 0.0;
  if (!active[4]) head_best.perp = 0.0;
  if (!active[5]) head_best.idx2 = 0.0;

  LA3::copy(Tinv, Tinv_best);
  LA3::transpose(T, Tinv);
  head = head_best;
  rigidRms = bestRms;
  rigidMask = (unsigned char)((active[3] ? COORDCONV_FIT_CONE : 0) |
                              (active[4] ? COORDCONV_FIT_PERP : 0) |
                              (active[5] ? COORDCONV_FIT_IDX2 : 0));
  isready = true;

  if (rmsOut) *rmsOut = bestRms;
  if (iterOut) *iterOut = iter;
  return true;
}

