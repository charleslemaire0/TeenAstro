//#pragma once
static void MoveAxis(GuideAxis* guideA, StatusAxis* staA, const bool BW, const Guiding Mode)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= (Mode == GuidingRecenter || lastError == ERRT_NONE);
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == Mode);
  if (canMove)
  {
    // block user from changing direction at high rates, just stop the guide instead
    // estimate new guideTimerBaseRate
    if (guideA->absRate == 0 && guideA->isBusy())
    {
      cli();
      guideA->brake();
      sei();
      return;
    }

    bool samedirection = (BW == guideA->isDirBW());
    if (guideA->isBusy() && !samedirection && guideA->absRate > 2)
    {
      StopAxis(guideA, staA);
    }
    else
    {
      if (Mode == GuidingST4)
      {
        enableST4GuideRate();
      }
      else if (Mode == GuidingRecenter)
      {
        enableRecenterGuideRate();
      }
      else
      {
        enableGuideRate(activeGuideRate);
      }

      GuidingState = Mode;
      BW ? guideA->moveBW() : guideA->moveFW();
      atHome = false;
      guideA->duration = -1;
    }
  }
}

static void MoveAxisAtRate(GuideAxis* guideA, StatusAxis* staA, const double newrate)
{
  bool canMove = parkStatus == PRK_UNPARKED;
  canMove &= lastError == ERRT_NONE;
  canMove &= !movingTo;
  canMove &= (GuidingState == GuidingOFF || GuidingState == GuidingAtRate ) ;
  if (canMove)
  {
    if (newrate == 0)
    {
      StopAxis(guideA, staA);
      return;
    }
    if (GuidingState != GuidingAtRate)
    {
      lastSideralTracking = sideralTracking;
      sideralTracking = false;
    }
    bool samedirection = ((newrate > 0) == (guideA->getRate() >= 0));
    if (guideA->isBusy() && !samedirection && guideA->absRate > 2)
    {
      StopAxis(guideA, staA);
    }
    else
    {
      guideA->enableAtRate(abs(newrate));
      GuidingState = Guiding::GuidingAtRate;
      newrate > 0 ? guideA->moveFW() : guideA->moveBW();
      atHome = false;
      guideA->duration = -1;
    }
  }
}

static void StopAxis(GuideAxis* guideA, StatusAxis* staA)
{
  if (!guideA->isMoving())
    return;
  guideA->brake();
}


void MoveAxis1(const bool BW, const Guiding Mode)
{
  MoveAxis(&guideA1, &staA1, BW, Mode);
}

void MoveAxisAtRate1(const double newrate)
{
  MoveAxisAtRate(&guideA1, &staA1, newrate);
}

void StopAxis1()
{
  StopAxis(&guideA1, &staA1);
}

void MoveAxis2(const bool BW, const Guiding Mode)
{
  MoveAxis(&guideA2, &staA2, BW, Mode);
}

void MoveAxisAtRate2(const double newrate)
{
  MoveAxisAtRate(&guideA2, &staA2, newrate);
}

void StopAxis2()
{
  StopAxis(&guideA2, &staA2);
}



void CheckEndOfMoveAxisAtRate()
{
  if (lastGuidingState == GuidingAtRate && GuidingState == GuidingOFF)
  {
    if (lastSideralTracking)
    {
      lastSetTrakingEnable = millis();
      sideralTracking = true;
      computeTrackingRate(true);
    }
    resetGuideRate();
  }
  lastGuidingState = GuidingState;
}

void CheckSpiral()
{
  // the spiral is a virtual moving object on the sky at a certain RA DEC rate
  static bool startPointDefined = false;
  static CoordConv helper;
  static unsigned long clk_ini, clk_last, clk_now;

  if (!doSpiral)
  {
    //reset startPointDefined
    if (startPointDefined )
    {
      startPointDefined = false;
    }
    return;
  }

  if (lastError != ERRT_NONE)
  {
    StopAxis1();
    StopAxis2();
    doSpiral = false;
    return;
  }

  if (!startPointDefined)
  {
    //compute local sky coordinate system at the start position
    //we define the local coordinate system in or that the telescop is now at HA_L = 0, and DEC_L =0
    double HA_ref, Dec_ref;
    getEqu(&HA_ref, &Dec_ref, localSite.cosLat(), localSite.sinLat(), true);
    helper.addReferenceDeg(HA_ref, Dec_ref, 0, 0);
    double shift = Dec_ref > 0 ? -45 : 45;
    helper.addReferenceDeg(HA_ref, Dec_ref + shift, 0, shift);
    helper.calculateThirdReference();
    //init time
    clk_ini = millis();
    clk_last = clk_ini;
    startPointDefined = true;
    return;
  }

  clk_now = millis();
  unsigned long t = clk_now - clk_ini;
  unsigned long dt = clk_now - clk_last;

  //if the Spiral runs more than 5 minutes stop it
  if (t > 300000 )
  {
    StopAxis1();
    StopAxis2();
    doSpiral = false;
    return;
  }

  // update Spiral only each 200ms
  if (dt < 200)
  {
    return;
  }

  double HA_prev, HA_next, Dec_prev, Dec_next;
  double SpiraleRateA1, SpiraleRateA2;

  double t_prev = 0.001 * (t - dt);
  double t_next = 0.001 * (t + dt);

  //compute local position along the spiral before
  double hl_prev = 0.4 * SpiralFOV * sqrt(t_prev) * cos(sqrt(t_prev));
  double dl_prev = 0.4 * SpiralFOV * sqrt(t_prev) * sin(sqrt(t_prev));
  //compute local position along the spiral after
  double hl_next = 0.4 * SpiralFOV * sqrt(t_next) * cos(sqrt(t_next));
  double dl_next = 0.4 * SpiralFOV * sqrt(t_next) * sin(sqrt(t_next));
  //now get these position in the sky

  helper.toReferenceDeg(HA_prev, Dec_prev, hl_prev, dl_prev);
  helper.toReferenceDeg(HA_next, Dec_next, hl_next, dl_next);

  PierSide side_tmp = GetPierSide();

  RateFromMovingTarget(HA_prev, Dec_prev, HA_next, Dec_next,
    0.002*dt, side_tmp, doesRefraction.forGoto,
    SpiraleRateA1, SpiraleRateA2);

  if (abs(SpiraleRateA1) > guideRates[4] || abs(SpiraleRateA2) > guideRates[4])
  {
    StopAxis1();
    StopAxis2();
    doSpiral = false;
    return;
  }

  MoveAxisAtRate1(SpiraleRateA1);
  MoveAxisAtRate2(SpiraleRateA2);
  clk_last = clk_now;
}


