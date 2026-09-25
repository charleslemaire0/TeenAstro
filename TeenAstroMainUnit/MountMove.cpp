/*
 * Title       TeenAstro
 * by          Howard Dutton, Charles Lemaire, Markus Noga, Francois Desvalee
 *
 * Copyright (C) 2012 to 2016 On-Step by Howard Dutton
 * Copyright (C) 2016 to 2024 TeenAstro by Charles Lemaire, Markus Noga, Francois Desvalee
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
 * Description: Mount manual axis move (moveAxis1/2, moveAxisAtRate1/2, stopAxis1/2). See Mount.h.
 */
#include "MainUnit.h"

namespace {

// ----- Stop one guide axis (brake) -----
static void stopAxis(Mount& m, GuideAxis* guideA, StatusAxis*)
{
  if (!m.motorsEncoders.enableMotor)
    return;
  if (!guideA->isMoving())
    return;
  guideA->brake();
}

// ----- Move one axis in a direction (FW/BW) with a guiding mode -----
static void moveAxis(Mount& m, GuideAxis* guideA, StatusAxis*, bool BW, Guiding Mode)
{
  if (!m.motorsEncoders.enableMotor)
    return;
  bool canMove = m.parkHome.parkStatus == PRK_UNPARKED;
  canMove &= (Mode == GuidingRecenter || m.errors.lastError == ERRT_NONE);
  canMove &= !m.isMovingTo();
  canMove &= (m.guiding.GuidingState == GuidingOFF || m.guiding.GuidingState == Mode);

  if (canMove)
  {
    if (guideA->absRate == 0 && guideA->isBusy())
    {
      cli();
      guideA->brake();
      sei();
      return;
    }
    bool samedirection = (BW == guideA->isDirBW());
    if (guideA->isBusy() && !samedirection && guideA->absRate > 2)
      stopAxis(m, guideA, nullptr);
    else
    {
      if (Mode == GuidingST4)
        m.enableST4GuideRate();
      else if (Mode == GuidingRecenter)
        m.enableRecenterGuideRate();
      else
        m.enableGuideRate(m.guiding.activeGuideRate);
      m.guiding.GuidingState = Mode;
      BW ? guideA->moveBW() : guideA->moveFW();
      m.parkHome.atHome = false;
      guideA->duration = 0UL;
    }
  }
}

// ----- Move one axis at a given rate (deg/s or similar) -----
static void moveAxisAtRate(Mount& m, GuideAxis* guideA, StatusAxis*, double newrate)
{
  if (!m.motorsEncoders.enableMotor)
    return;
  // ASCOM MoveAxis rate 0: stop immediately. Clear guide busy and GuidingState
  // here (do not wait for TIMER1) so AtRate ends even if the high-rate path left
  // residual tmp_guideRate / goto-decay state, while the other axis may still track.
  if (newrate == 0)
  {
    stopAxis(m, guideA, nullptr);
    guideA->setIdle();
    guideA->moveAxisActive = false;
    if (!m.guiding.guideA1.isBusy() && !m.guiding.guideA2.isBusy()
        && !m.guiding.guideA1.moveAxisActive && !m.guiding.guideA2.moveAxisActive)
      m.guiding.GuidingState = Guiding::GuidingOFF;
    return;
  }
  bool canMove = m.parkHome.parkStatus == PRK_UNPARKED;
  canMove &= m.errors.lastError == ERRT_NONE;
  canMove &= !m.isMovingTo();
  canMove &= (m.guiding.GuidingState == GuidingOFF || m.guiding.GuidingState == GuidingAtRate);

  if (canMove)
  {
    // OnStep-style: keep sideralTracking as-is. Timer adds guide+tracking per axis,
    // so the other axis (and both axes on AltAz / dual-rate tracking) keep tracking.
    // Do not clear tracking globally — ASCOM MoveAxis only replaces motion on the
    // commanded axis; OnStep similarly keeps trackingTimerRate on unmoved axes.
    bool samedirection = ((newrate > 0) == (guideA->getRate() >= 0));
    if (guideA->isBusy() && !samedirection && guideA->absRate > 2)
      stopAxis(m, guideA, nullptr);
    else
    {
      guideA->enableAtRate(abs(newrate));
      guideA->moveAxisActive = true;
      m.guiding.GuidingState = Guiding::GuidingAtRate;
      newrate > 0 ? guideA->moveFW() : guideA->moveBW();
      m.parkHome.atHome = false;
      guideA->duration = 0UL;
    }
  }
}

}  // namespace

// -----------------------------------------------------------------------------
// Public API: axis 1 (RA/HA) and axis 2 (Dec/Alt)
// -----------------------------------------------------------------------------

void Mount::moveAxis1(bool BW, Guiding Mode)
{
  moveAxis(*this, &guiding.guideA1, &axes.staA1, BW, Mode);
}

void Mount::moveAxisAtRate1(double newrate)
{
  moveAxisAtRate(*this, &guiding.guideA1, &axes.staA1, newrate);
}

void Mount::stopAxis1()
{
  stopAxis(*this, &guiding.guideA1, &axes.staA1);
}

void Mount::moveAxis2(bool BW, Guiding Mode)
{
  moveAxis(*this, &guiding.guideA2, &axes.staA2, BW, Mode);
}

void Mount::moveAxisAtRate2(double newrate)
{
  moveAxisAtRate(*this, &guiding.guideA2, &axes.staA2, newrate);
}

void Mount::stopAxis2()
{
  stopAxis(*this, &guiding.guideA2, &axes.staA2);
}
