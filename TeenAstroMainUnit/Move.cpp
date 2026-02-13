/** Axis motion: MoveAxis1/2, MoveAxisAtRate, StopAxis, CheckSpiral, StartSideralTracking. */
#include "Global.h"

static void StopAxis(GuideAxis* guideA, StatusAxis* staA);

static void MoveAxis(GuideAxis* guideA, StatusAxis* staA, const bool BW, const Guiding Mode)
{
  if (!mount.enableMotor) return;
  bool canMove = mount.parkStatus == PRK_UNPARKED;
  canMove &= (Mode == GuidingRecenter || mount.lastError == ERRT_NONE);
  canMove &= !mount.movingTo;
  canMove &= (mount.GuidingState == GuidingOFF || mount.GuidingState == Mode);
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
        enableGuideRate(mount.activeGuideRate);
      }

      mount.GuidingState = Mode;
      BW ? guideA->moveBW() : guideA->moveFW();
      mount.atHome = false;
      guideA->duration = 0UL;
    }
  }
}

static void MoveAxisAtRate(GuideAxis* guideA, StatusAxis* staA, const double newrate)
{
  if (!mount.enableMotor) return;
  bool canMove = mount.parkStatus == PRK_UNPARKED;
  canMove &= mount.lastError == ERRT_NONE;
  canMove &= !mount.movingTo;
  canMove &= (mount.GuidingState == GuidingOFF || mount.GuidingState == GuidingAtRate ) ;
  if (canMove)
  {
    if (newrate == 0)
    {
      StopAxis(guideA, staA);
      return;
    }
    if (mount.GuidingState != GuidingAtRate)
    {
      mount.lastSideralTracking = mount.sideralTracking;
      mount.sideralTracking = false;
    }
    bool samedirection = ((newrate > 0) == (guideA->getRate() >= 0));
    if (guideA->isBusy() && !samedirection && guideA->absRate > 2)
    {
      StopAxis(guideA, staA);
    }
    else
    {
      guideA->enableAtRate(abs(newrate));
      mount.GuidingState = Guiding::GuidingAtRate;
      newrate > 0 ? guideA->moveFW() : guideA->moveBW();
      mount.atHome = false;
      guideA->duration = 0UL;
    }
  }
}

static void StopAxis(GuideAxis* guideA, StatusAxis* staA)
{
  if (!mount.enableMotor) return;
  if (!guideA->isMoving())
    return;
  guideA->brake();
}


void MoveAxis1(const bool BW, const Guiding Mode)
{
  MoveAxis(&mount.guideA1, &mount.staA1, BW, Mode);
}

void MoveAxisAtRate1(const double newrate)
{
  MoveAxisAtRate(&mount.guideA1, &mount.staA1, newrate);
}

void StopAxis1()
{
  StopAxis(&mount.guideA1, &mount.staA1);
}

void MoveAxis2(const bool BW, const Guiding Mode)
{
  MoveAxis(&mount.guideA2, &mount.staA2, BW, Mode);
}

void MoveAxisAtRate2(const double newrate)
{
  MoveAxisAtRate(&mount.guideA2, &mount.staA2, newrate);
}

void StopAxis2()
{
  StopAxis(&mount.guideA2, &mount.staA2);
}



void CheckEndOfMoveAxisAtRate()
{
  if (!mount.enableMotor) return;
  if (mount.lastGuidingState == GuidingAtRate && mount.GuidingState == GuidingOFF)
  {
    if (mount.lastSideralTracking)
    {
      StartSideralTracking();
    }
    resetGuideRate();
  }
  mount.lastGuidingState = mount.GuidingState;
}

void CheckSpiral()
{
  // the spiral is a virtual moving object on the sky at a certain RA DEC rate
  static bool startPointDefined = false;
  static CoordConv helper;
  static unsigned long clk_ini, clk_last, clk_now;

  if (!mount.doSpiral)
  {
    //reset startPointDefined
    if (startPointDefined )
    {
      startPointDefined = false;
    }
    return;
  }

  if (mount.lastError != ERRT_NONE)
  {
    StopAxis1();
    StopAxis2();
    mount.doSpiral = false;
    return;
  }

  if (!startPointDefined)
  {
    //compute local sky coordinate system at the start position local sky coordinate are here instrement coordinates
    //we define the local coordinate system in or that the telescop is now at HA_L = 0, and DEC_L =0
   
    Coord_EQ EQ_R = getEqu(*localSite.latitude() * DEG_TO_RAD);
    helper.addReference(EQ_R.Ha(), EQ_R.Dec(), 0, 0);
    double shift = EQ_R.Dec() > 0 ? -M_PI_4 : M_PI_4;
    helper.addReference(EQ_R.Ha(), EQ_R.Dec() + shift, 0, shift);
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
    mount.doSpiral = false;
    return;
  }

  // update Spiral only each 200ms
  if (dt < 200)
  {
    return;
  }

  double SpiraleRateA1, SpiraleRateA2;

  double t_prev = 0.001 * (t - dt);
  double t_next = 0.001 * (t + dt);

  //compute local position along the spiral before
  double hl_prev = 0.4 * mount.SpiralFOV * sqrt(t_prev) * cos(sqrt(t_prev));
  double dl_prev = 0.4 * mount.SpiralFOV * sqrt(t_prev) * sin(sqrt(t_prev));
  //compute local position along the spiral after
  double hl_next = 0.4 * mount.SpiralFOV * sqrt(t_next) * cos(sqrt(t_next));
  double dl_next = 0.4 * mount.SpiralFOV * sqrt(t_next) * sin(sqrt(t_next));
  //now get these position in the sky
  
  Coord_EQ EQ_prev = Coord_LO(0, dl_prev * DEG_TO_RAD, hl_prev * DEG_TO_RAD).To_Coord_EQ(helper.T);
  Coord_EQ EQ_next = Coord_LO(0, dl_next * DEG_TO_RAD, hl_next * DEG_TO_RAD).To_Coord_EQ(helper.T);

  PoleSide side_tmp = GetPoleSide();

  RateFromMovingTarget(EQ_prev, EQ_next,
    0.002*dt, side_tmp, environment.doesRefraction.forGoto,
    SpiraleRateA1, SpiraleRateA2);

  if (abs(SpiraleRateA1) > mount.guideRates[4] || abs(SpiraleRateA2) > mount.guideRates[4])
  {
    StopAxis1();
    StopAxis2();
    mount.doSpiral = false;
    return;
  }

  MoveAxisAtRate1(SpiraleRateA1);
  MoveAxisAtRate2(SpiraleRateA2);
  clk_last = clk_now;
}

void StartSideralTracking()
{
  if (!mount.sideralTracking)
  {
    mount.sideralTracking = true;
    computeTrackingRate(true);
    mount.lastSetTrakingEnable = millis();
    mount.atHome = false;
  }
}
