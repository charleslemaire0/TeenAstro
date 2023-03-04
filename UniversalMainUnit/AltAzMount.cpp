#include "Global.h"

void AltAzMount::axesToSteps(Axes *aP, Steps *sP)
{
  sP->steps1 = (long)((aP->axis1) * geoA1.stepsPerDegree);
  sP->steps2 = (long)((aP->axis2) * geoA2.stepsPerDegree);
}

void AltAzMount::stepsToAxes(Steps *sP, Axes *aP)
{
  aP->axis1 = ((double)(sP->steps1) / geoA1.stepsPerDegree);
  aP->axis2 = ((double)(sP->steps2) / geoA2.stepsPerDegree);
}

void AltAzMount::axesToHor(Axes *aP, HorCoords *hP)
{
	if (*localSite.latitude() >= 0)
  	hP->az  = AzRange(aP->axis1 + 180);
  else
  	hP->az  = AzRange(aP->axis1);

  hP->alt = aP->axis2;
}

/*
 * horToAxes: computes axes from horizontal coordinates
 * return false if we cannot reach the given position on the given side
 */
bool AltAzMount::horToAxes(HorCoords *hP, Axes *aP)
{
	if (*localSite.latitude() >= 0)
  	aP->axis1 = hP->az - 180;
  else
  	aP->axis1 = hP->az;

  aP->axis2 = hP->alt;

  if (!withinLimits(aP->axis1, aP->axis2))
    return false;

  return true;
}

byte AltAzMount::goToEqu(EqCoords *eP)
{
	HorCoords hor;

	EquToHor(eP->ha, eP->dec, false, &hor.az, &hor.alt, localSite.cosLat(), localSite.sinLat());
	return goToHor(&hor);
}

byte AltAzMount::goToHor(HorCoords *hP)
{
	Axes axes;
	Steps steps;

  // check destination altitude
  if (hP->alt < limits.minAlt)
    return ERRGOTO_BELOWHORIZON;
  if (hP->alt > limits.maxAlt)
    return ERRGOTO_ABOVEOVERHEAD;

  if (!horToAxes(hP, &axes))
  {
    return ERRGOTO_LIMITS;                    //fail, outside limit
  }
  axesToSteps(&axes, &steps);

  return goTo(&steps);
}

bool AltAzMount::getEqu(double *haP, double *decP, const double *cosLatP, const double *sinLatP, bool returnHA)
{
	HorCoords horCoords;

	getHorApp(&horCoords);
	HorTopoToEqu(horCoords.az, horCoords.alt, haP, decP, cosLatP, sinLatP);

  if (!returnHA)  // return RA instead of HA
  {
    *haP = degRange(rtk.LST() * 15.0 - *haP);
  }

	return true;
}

bool AltAzMount::getHorApp(HorCoords *hP)
{
	Steps steps;
	Axes axes;

	steps.steps1 = motorA1.getCurrentPos();
	steps.steps2 = motorA2.getCurrentPos();

	stepsToAxes(&steps, &axes);
	axesToHor(&axes, hP);

  return 0;
}

byte AltAzMount::goTo(Steps *sP)
{
  if (getEvent(EV_ABORT))
    return ERRGOTO____;
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  msg[0] = CTL_MSG_GOTO; 
  msg[1] = sP->steps1;
  msg[2] = sP->steps2;
  xQueueSend( controlQueue, &msg, 0);
  waitSlewing();
  DecayModeGoto();
  return ERRGOTO_NONE;
}

bool AltAzMount::syncEqu(double HA, double Dec, UNUSED(PierSide Side), const double *cosLatP, const double *sinLatP)
{
	HorCoords horCoords;

	EquToHor(HA, Dec, false, &horCoords.az, &horCoords.alt, cosLatP, sinLatP);
	return syncAzAlt(horCoords.az, horCoords.alt, PIER_NOTVALID);
}


bool AltAzMount::syncAzAlt(double Azm, double Alt, UNUSED(PierSide Side))
{
	HorCoords horCoords;
	Axes axes;
	Steps steps;

	horCoords.az = Azm;
	horCoords.alt = Alt;
	horToAxes(&horCoords, &axes);
	axesToSteps(&axes, &steps);
	motorA1.setCurrentPos(steps.steps1); 
	motorA2.setCurrentPos(steps.steps2); 
	return true;
}

bool AltAzMount::withinLimits(long axis1, long axis2)
{
  if (! ((geoA1.withinLimits(axis1) && geoA2.withinLimits(axis2))))
    return false;
  return true;
}

/*
 * getTrackingSpeeds
 * called periodically by Control task
 * computes axes speeds (in steps) for both axes
 * return true if the speed has changed
 */
void AltAzMount::getTrackingSpeeds(Speeds *sP)
{
  HorCoords hor1, hor2;
  Axes axes1, axes2;
  EqCoords equ, equ1, equ2;
  Steps s1, s2, delta;

  // read current position 
  getEqu(&equ.ha, &equ.dec, localSite.cosLat(), localSite.sinLat(), true);

  // compute horizontal coordinates of points one arc-minute behind and ahead
  equ1.ha = equ.ha - trackingSpeed * (15.0 / 60.0);
  equ1.dec = equ.dec;
  EquToHor(equ1.ha, equ1.dec, false, &hor1.az, &hor1.alt, localSite.cosLat(), localSite.sinLat());
  // convert to steps
  horToAxes(&hor1, &axes1);
  axesToSteps(&axes1, &s1);

  equ2.ha = equ.ha + trackingSpeed * (15.0 / 60.0);
  equ2.dec = equ.dec;
  EquToHor(equ2.ha, equ2.dec, false, &hor2.az, &hor2.alt, localSite.cosLat(), localSite.sinLat());
  // convert to steps
  horToAxes(&hor2, &axes2);
  axesToSteps(&axes2, &s2);

  // compute difference
  delta.steps1 = s2.steps1 - s1.steps1;
  delta.steps2 = s2.steps2 - s1.steps2;

  // normalize to steps per clock second
  sP->speed1 = ((double) delta.steps1) / (SIDEREAL_SECOND * 60.0 * 2);
  sP->speed2 = ((double) delta.steps2) / (SIDEREAL_SECOND * 60.0 * 2);
}




void AltAzMount::MoveAxis1(const byte dir)
{
  MoveAxis1AtRate(guideRates[activeGuideRate], dir);
}

void AltAzMount::MoveAxis1AtRate(double speed, const byte dir)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];

  msg[0] = CTL_MSG_MOVE_AXIS1; 
  msg[1] = (dir == 'w' ? geoA1.westDef : geoA1.eastDef);
  memcpy(&msg[2], &speed, sizeof(double));
  xQueueSend(controlQueue, &msg, 0);
}

void AltAzMount::StopAxis1()
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
 
  msg[0] = CTL_MSG_STOP_AXIS1; 
  xQueueSend(controlQueue, &msg, 0);
}

void AltAzMount::MoveAxis2(const byte dir)
{
  MoveAxis2AtRate(guideRates[activeGuideRate], dir);
}

/*
 * MoveAxis2 for AltAz moves towards or away from zenith, which is at 90º of home position
 */
void AltAzMount::MoveAxis2AtRate(double speed, const byte dir)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  long target;

  target = poleDir(dir);

  msg[0] = CTL_MSG_MOVE_AXIS2; 
  msg[1] = target;
  memcpy(&msg[2], &speed, sizeof(double));
  xQueueSend(controlQueue, &msg, 0);
}

void AltAzMount::StopAxis2()
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
 
  msg[0] = CTL_MSG_STOP_AXIS2; 
  xQueueSend(controlQueue, &msg, 0);
}

void AltAzMount::startTracking(void)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  msg[0] = CTL_MSG_START_TRACKING; 
  xQueueSend( controlQueue, &msg, 0);
}

void AltAzMount::stopTracking(void)
{
  unsigned msg[CTL_MAX_MESSAGE_SIZE];
  msg[0] = CTL_MSG_STOP_TRACKING; 
  xQueueSend( controlQueue, &msg, 0);
}

/*
 * setTrackingSpeed
 * speed is expressed as a multiple of sidereal speed 
 */
void AltAzMount::setTrackingSpeed(double speed)
{
  trackingSpeed = speed;
}

void AltAzMount::startGuiding(char dir, int milliseconds)
{

}


void AltAzMount::initHome(Steps *sP)
{
  sP->steps1 = 0;
  sP->steps2 = 0;
}

/*
 * For AltAz mounts, 
 * "pole" is really the zenith at +90º, antipole at -90º 
 */
long AltAzMount::poleDir(char pole)
{
  return ((*localSite.latitude() >= 0) == (pole=='n')) ? geoA2.quarterRot : -geoA2.quarterRot;
}


// These functions are never called. They keep the linker happy
byte AltAzMount::Flip(void)
{
	return 0;
}
PierSide AltAzMount::GetPierSide()
{
	return PIER_NOTVALID;
}
bool AltAzMount::getTargetPierSide(EqCoords *eP, PierSide *psOutP)
{
  return false;
}

int AltAzMount::decDirection(void)
{
  return 0;
}
