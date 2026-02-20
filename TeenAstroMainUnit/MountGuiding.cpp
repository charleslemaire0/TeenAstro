/**
 * MountGuiding: state and guiding behaviour (pulse, ST4, recenter, at-rate).
 * See MountGuiding.h.
 */
#include "MountGuiding.h"
#include "TelTimer.hpp"  // rtk
#include "MainUnit.h"

MountGuiding::MountGuiding(Mount& mount) : mount_(mount) {}

bool MountGuiding::hasGuiding() const { return GuidingState != Guiding::GuidingOFF; }
void MountGuiding::setGuidingState(Guiding g) { GuidingState = g; }
bool MountGuiding::isGuidingStar() const { return GuidingState == GuidingPulse || GuidingState == GuidingST4; }

void MountGuiding::stopGuiding()
{
  guideA1.duration = 0UL;
  guideA2.duration = 0UL;
  cli();
  guideA1.brake();
  guideA2.brake();
  sei();
}

void MountGuiding::applyGuidingA1()
{
  cli();
  mount_.axes.staA1.target += guideA1.getAmount();
  sei();
}

void MountGuiding::applyGuidingA2()
{
  cli();
  mount_.axes.staA2.target += guideA2.getAmount();
  sei();
}

bool MountGuiding::stopIfMountError()
{
  bool error = mount_.errors.lastError != ERRT_NONE;
  if (error)
    stopGuiding();
  return error;
}

void MountGuiding::performPulseGuiding()
{
  if (stopIfMountError())
    return;
  if (guideA2.duration == 0 && guideA1.duration == 0)
  {
    cli();
    guideA1.brake();
    guideA2.brake();
    guideA1.speedMultiplier = 1.0;
    guideA2.speedMultiplier = 1.0;
    sei();
    return;
  }
  if (guideA1.isMoving())
  {
    if (guideA1.duration > 0)
    {
      if (!mount_.axes.staA1.backlash_correcting)
      {
        applyGuidingA1();
        unsigned long elapsedtime = micros() - guideA1.durationLast;
        if (elapsedtime > guideA1.duration)
          guideA1.duration = 0;
        else
          guideA1.duration -= elapsedtime;
        guideA1.durationLast = micros();
      }
      else
        guideA1.durationLast = micros();
    }
    else
    {
      cli();
      guideA1.brake();
      guideA1.speedMultiplier = 1.0;
      sei();
    }
  }
  else
  {
    guideA1.duration = 0UL;
    guideA1.speedMultiplier = 1.0;
  }
  if (guideA2.isMoving())
  {
    if (guideA2.duration > 0)
    {
      if (!mount_.axes.staA2.backlash_correcting)
      {
        applyGuidingA2();
        unsigned long elapsedtime = micros() - guideA2.durationLast;
        if (elapsedtime > guideA2.duration)
          guideA2.duration = 0;
        else
          guideA2.duration -= elapsedtime;
        guideA2.durationLast = micros();
      }
      else
        guideA2.durationLast = micros();
    }
    else
    {
      cli();
      guideA2.brake();
      guideA2.speedMultiplier = 1.0;
      sei();
    }
  }
  else
  {
    guideA2.duration = 0UL;
    guideA2.speedMultiplier = 1.0;
  }
}

void MountGuiding::performST4Guiding()
{
  if (stopIfMountError())
    return;
  if (guideA1.isMoving() && !mount_.axes.staA1.backlash_correcting)
    applyGuidingA1();
  if (guideA2.isMoving() && !mount_.axes.staA2.backlash_correcting)
    applyGuidingA2();
}

void MountGuiding::performGuidingRecenter()
{
  if (guideA1.isMoving() && !mount_.axes.staA1.backlash_correcting)
    applyGuidingA1();
  if (guideA2.isMoving() && !mount_.axes.staA2.backlash_correcting)
    applyGuidingA2();
}

void MountGuiding::performGuidingAtRate()
{
  if (stopIfMountError())
    return;
  if (guideA1.isMoving() && !mount_.axes.staA1.backlash_correcting)
  {
    cli();
    mount_.axes.staA1.target += guideA1.getAmount();
    sei();
  }
  if (guideA2.isMoving() && !mount_.axes.staA2.backlash_correcting)
  {
    cli();
    mount_.axes.staA2.target += guideA2.getAmount();
    sei();
  }
}

void MountGuiding::guide()
{
  if (GuidingState == GuidingOFF)
    return;
  if (rtk.updateguideSiderealTimer())
  {
    if (GuidingState == GuidingPulse)
      performPulseGuiding();
    else if (GuidingState == GuidingST4)
      performST4Guiding();
    else if (GuidingState == GuidingRecenter)
      performGuidingRecenter();
    else if (GuidingState == GuidingAtRate)
      performGuidingAtRate();
  }
}
