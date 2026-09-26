/*
 * Title       TeenAstro mount head geometry model
 * by          Charles Lemaire
 *
 * Copyright (C) Charles Lemaire
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Revision History, see GitHub
 *
 * Description:
 *
 * Rigid-body ("head") geometry of a two axis mount: the three degrees of
 * freedom that a single 3x3 rotation matrix cannot express.
 *
 * A rigid two axis mount has six geometric degrees of freedom:
 *
 *   axis1 direction in space          2   ) absorbed by CoordConv's T, because a
 *   axis1 index                       1   ) rotation of T about axis1 is the index
 *   axis2 vs axis1 non-perpendicular  1   ) this header
 *   axis2 index                       1   ) this header
 *   optical axis vs axis2 (cone)      1   ) this header
 *
 * T therefore stays exactly what it always was (three degrees of freedom) and
 * this header adds the missing three as scalars. All three zero reproduces the
 * previous behaviour identically, term by term, so an old stored T keeps
 * pointing the same way.
 *
 * Everything is an exact rotation: there is no small angle expansion, no
 * tan()/sec() factor and therefore no breakdown near the pole.
 *
 * Header only on purpose, so that adding it costs no build file changes in the
 * firmware, the emulator single translation unit builds or the native tests.
 */
#pragma once

#ifndef __TeenAstroHeadModel_hpp__
#define __TeenAstroHeadModel_hpp__

#include <math.h>

#include "TeenAstroLA3.hpp"

/// Number of rotations in the instrument frame chain with the head model applied.
#define HEADMODEL_CHAIN_LEN 5

/**
 * Head geometry, all angles in radians, all zero meaning a geometrically
 * perfect mount head.
 *
 * Sign conventions, all as seen in the instrument frame where axis1 is z,
 * axis2 is y and the optical axis is x:
 *   cone  rotation of the optical axis about z, i.e. out of the plane
 *         perpendicular to axis2 (collimation / cone error)
 *   perp  rotation of axis2 about x, i.e. axis2 tilted out of perpendicular
 *         to axis1
 *   idx2  added to axis2 before the axis2 rotation is applied (axis2 index)
 *
 * The axis1 index is deliberately absent: it is not independent of T.
 */
struct HeadModel
{
  double cone;
  double perp;
  double idx2;

  HeadModel() : cone(0.0), perp(0.0), idx2(0.0) {}
  HeadModel(double cone_, double perp_, double idx2_) : cone(cone_), perp(perp_), idx2(idx2_) {}

  /// True when the head is geometrically perfect, i.e. the legacy T only model.
  bool isZero() const { return cone == 0.0 && perp == 0.0 && idx2 == 0.0; }

  /// Largest single term, radians. Handy for reporting and for tolerance scaling.
  double magnitude() const
  {
    double a = fabs(cone), b = fabs(perp), c = fabs(idx2);
    double m = a > b ? a : b;
    return m > c ? m : c;
  }
};

/// Head model helpers. Static only; no state.
class HeadGeom : public LA3
{
public:
  /**
   * Instrument frame rotation chain including the head model.
   *
   * Fills \p out with
   *   Rx(axis3) Rz(-cone) Ry(axis2 + idx2) Rx(-perp) Rz(axis1Direct)
   * which for a zero head collapses to the historical three rotation chain
   *   Rx(axis3) Ry(axis2) Rz(axis1Direct)
   * because Rz(0) and Rx(0) are the identity. \p axis1Direct is the raw Rz
   * Euler angle stored by Coord_IN, i.e. -axis1.
   */
  static void chain(LA3::SingleRotation (&out)[HEADMODEL_CHAIN_LEN],
                    double axis3, double axis2, double axis1Direct,
                    const HeadModel &head)
  {
    out[0].axis = LA3::ROTAXISX; out[0].angle = axis3;
    out[1].axis = LA3::ROTAXISZ; out[1].angle = -head.cone;
    out[2].axis = LA3::ROTAXISY; out[2].angle = axis2 + head.idx2;
    out[3].axis = LA3::ROTAXISX; out[3].angle = -head.perp;
    out[4].axis = LA3::ROTAXISZ; out[4].angle = axis1Direct;
  }

  /**
   * Pointing direction of the instrument for given axis readings.
   *
   * Returns the same direction cosine vector that LA3::toDirCos(axis1Direct,
   * axis2) returns when \p head is zero, i.e. the first row of the instrument
   * frame matrix. Exact for any angle; no small angle assumption.
   */
  static void forward(double (&dc)[3], double axis1Direct, double axis2, const HeadModel &head)
  {
    if (head.isZero())
    {
      LA3::toDirCos(dc, axis1Direct, axis2);
      return;
    }
    double w[3];
    intermediate(w, axis2 + head.idx2, head);
    // dc = Rz(-axis1Direct) * w, which only turns the xy part.
    const double ca = cos(-axis1Direct), sa = sin(-axis1Direct);
    dc[0] = w[0] * ca - w[1] * sa;
    dc[1] = w[0] * sa + w[1] * ca;
    dc[2] = w[2];
  }

  /**
   * Axis readings that point the instrument along \p dc. Exact inverse of
   * forward().
   *
   * \p axis2Hint selects between the two mathematically valid solutions, which
   * are the two mount configurations (normal and beyond the pole). The solution
   * whose axis2 is nearest the hint is returned, so passing the axis2 that the
   * legacy zero head path would have produced keeps pier side and under pole
   * behaviour unchanged.
   *
   * Returns false when \p dc cannot be reached with this head geometry, which
   * can only happen within |cone| + |perp| of the pole.
   */
  static bool inverse(const double (&dc)[3], const HeadModel &head, double axis2Hint,
                      double &axis1Direct, double &axis2)
  {
    const double cc = cos(head.cone), sc = sin(head.cone);
    const double cn = cos(head.perp), sn = sin(head.perp);
    const double denom = cc * cn;
    if (fabs(denom) < 1e-12)
      return false;

    // dc[2] is invariant under the axis1 rotation, so psi follows directly:
    //   dc[2] = sin(cone) sin(perp) + cos(cone) cos(perp) sin(psi)
    const double s = (dc[2] - sc * sn) / denom;
    if (s < -1.0 - 1e-9 || s > 1.0 + 1e-9)
      return false;
    const double psiA = asin(clampUnit(s));
    const double psiB = M_PI - psiA;

    // Pick the branch nearest the hint, hint being an axis2 not a psi.
    const double hintPsi = axis2Hint + head.idx2;
    const double psi = (fabs(wrapPi(psiB - hintPsi)) < fabs(wrapPi(psiA - hintPsi))) ? psiB : psiA;

    double w[3];
    intermediate(w, psi, head);
    // dc = Rz(-axis1Direct) * w turns the xy part by -axis1Direct.
    axis1Direct = wrapPi(atan2(w[1], w[0]) - atan2(dc[1], dc[0]));
    axis2 = psi - head.idx2;
    return true;
  }

  /**
   * Field rotation about the optical axis for a target instrument frame.
   *
   * \p target is the instrument frame matrix being solved for and \p axis1Direct
   * / \p axis2 the solution from inverse(). Returns the axis3 that makes the
   * head model chain reproduce \p target.
   */
  static double fieldRotation(const double (&target)[3][3], double axis1Direct, double axis2,
                              const HeadModel &head)
  {
    // B = Rz(-cone) Ry(axis2 + idx2) Rx(-perp) Rz(axis1Direct); target = Rx(axis3) B
    LA3::SingleRotation rots[4];
    rots[0].axis = LA3::ROTAXISZ; rots[0].angle = -head.cone;
    rots[1].axis = LA3::ROTAXISY; rots[1].angle = axis2 + head.idx2;
    rots[2].axis = LA3::ROTAXISX; rots[2].angle = -head.perp;
    rots[3].axis = LA3::ROTAXISZ; rots[3].angle = axis1Direct;
    double B[3][3], Bt[3][3], R[3][3];
    LA3::getMultipleRotationMatrix(B, rots, 4);
    LA3::transpose(Bt, B);
    LA3::multiply(R, target, Bt);
    return atan2(R[2][1], R[1][1]);
  }

  /// Wrap an angle to (-pi, pi].
  static double wrapPi(double a)
  {
    while (a > M_PI) a -= 2.0 * M_PI;
    while (a <= -M_PI) a += 2.0 * M_PI;
    return a;
  }

private:
  static double clampUnit(double v) { return v > 1.0 ? 1.0 : (v < -1.0 ? -1.0 : v); }

  /// w = Rx(perp) Ry(-psi) Rz(cone) xhat, the direction before the axis1 rotation.
  static void intermediate(double (&w)[3], double psi, const HeadModel &head)
  {
    const double cc = cos(head.cone), sc = sin(head.cone);
    const double cn = cos(head.perp), sn = sin(head.perp);
    const double cp = cos(psi), sp = sin(psi);
    w[0] = cc * cp;
    w[1] = sc * cn - cc * sp * sn;
    w[2] = sc * sn + cc * sp * cn;
  }
};

#endif // __TeenAstroHeadModel_hpp__
