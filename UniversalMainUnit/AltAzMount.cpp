#include "Global.h"

void AltAzMount::axesToSteps(Axes *aP, Steps *sP)
{
  sP->steps1 = (long)((aP->axis1) * geoA1.stepsPerDegree + axisOffset.steps1);
  sP->steps2 = (long)((aP->axis2) * geoA2.stepsPerDegree + axisOffset.steps2);
}

void AltAzMount::stepsToAxes(Steps *sP, Axes *aP)
{
  aP->axis1 = ((double)(sP->steps1 - axisOffset.steps1) / geoA1.stepsPerDegree);
  aP->axis2 = ((double)(sP->steps2 - axisOffset.steps2) / geoA2.stepsPerDegree);
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
 * return false if we cannot reach the given position (TODO: check zenith limit)
 */
bool AltAzMount::horToAxes(HorCoords *hP, Axes *aP)
{
  long currentAxis1 = motorA1.getCurrentPos() / geoA1.stepsPerDegree;

	aP->axis2 = hP->alt;
  if (*localSite.latitude() >= 0)
    aP->axis1 = hP->az - 180;
  else
    aP->axis1 = hP->az;

  // easiest case
  if (fabs(aP->axis1 - currentAxis1) <= 180)
    return true;

  // target is more than 180º away - find the shortest movement within axis1 limits
  if (aP->axis1 < currentAxis1)
  {
    if (withinLimits((aP->axis1+360)*geoA1.stepsPerDegree, (aP->axis2)*geoA2.stepsPerDegree))
    {
      aP->axis1 = aP->axis1+360;
    }
  }
  else
  {
    if (withinLimits((aP->axis1-360)*geoA1.stepsPerDegree, (aP->axis2)*geoA2.stepsPerDegree))
    {
      aP->axis1 = aP->axis1-360;
    }
  }
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
  Steps oldSteps, newSteps;

  oldSteps.steps1 = motorA1.getCurrentPos();
  oldSteps.steps2 = motorA2.getCurrentPos();  

	horCoords.az = Azm;
	horCoords.alt = Alt;
	horToAxes(&horCoords, &axes);
	axesToSteps(&axes, &newSteps);
  
  // keep track of the new offset for future goto
  axisOffset.steps1 += (oldSteps.steps1 - newSteps.steps1);
  axisOffset.steps2 += (oldSteps.steps2 - newSteps.steps2);

	return true;
}


/*
 * getTrackingSpeeds
 * called periodically by Control task
 * computes axes speeds (in steps) for both axes
 */
void AltAzMount::getTrackingSpeeds(Speeds *sP)
{
  HorCoords hor1, hor2;
  Axes axes1, axes2;
  EqCoords equ, equ1, equ2;
  Steps s1, s2, delta;

  // get current position 
  equ.ha = haRange(rtk.LST() * 15.0 - currentRA);
  equ.dec = currentDec;

  // compute horizontal coordinates of points one arc-minute behind and ahead
  equ1.ha = equ.ha - trackingSpeed * (15.0 / 60.0);
  equ1.dec = equ.dec;
  EquToHor(equ1.ha, equ1.dec, false, &hor1.az, &hor1.alt, localSite.cosLat(), localSite.sinLat());
  // convert to steps
  horToAxes(&hor1, &axes1);
  axesToSteps(&axes1, &s1);

  equ2.ha = equ.ha + trackingSpeed * (15.0 / 60.0);
  equ2.dec = equ.dec;

  // if guiding, adjust targets by the angle traversed at the guide rate in one minute
  if (getEvent(EV_GUIDING_AXIS1 | EV_WEST))
    equ2.ha += guideRates[0];
  if (getEvent(EV_GUIDING_AXIS1 | EV_EAST))
    equ2.ha -= guideRates[0];
  if (getEvent(EV_GUIDING_AXIS2 | EV_NORTH))
    equ2.dec += guideRates[0];
  if (getEvent(EV_GUIDING_AXIS2 | EV_SOUTH))
    equ2.dec -= guideRates[0];

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

  // if needed, add spiral speeds directly to the computed speeds
  if (getEvent(EV_SPIRAL))
  {
    Speeds v;
    getSpiralSpeeds(&v);
    sP->speed1 += v.speed1 * (geoA1.stepsPerSecond/SIDEREAL_SECOND);
    sP->speed2 += v.speed2 * (geoA2.stepsPerSecond/SIDEREAL_SECOND);
  }
}





/*
 * setTrackingSpeed
 * speed is expressed as a multiple of sidereal speed 
 */
void AltAzMount::setTrackingSpeed(double speed)
{
  trackingSpeed = speed;
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
long AltAzMount::axis2Target(char pole)
{
  return ((*localSite.latitude() >= 0) == (pole=='n')) ? geoA2.quarterRot : -geoA2.quarterRot;
}

/*
 * After any move, keep track of the current eq coordinates for computing tracking rates
 */ 
void AltAzMount::updateRaDec(void)
{
  getEqu(&currentRA, &currentDec, localSite.cosLat(), localSite.sinLat(), false);
}

// These functions are never called. They keep the linker happy
byte AltAzMount::Flip(void)
{
	return 0;
}

bool AltAzMount::getTargetPierSide(EqCoords *eP, PierSide *psOutP)
{
  return false;
}

int AltAzMount::decDirection(void)
{
  return 0;
}

PierSide AltAzMount::GetPierSide(void)
{
  return PIER_NOTVALID;
}

bool AltAzMount::checkMeridian(Axes *aP, CheckMode mode, PierSide ps)
{
  return true;
}

bool AltAzMount::checkPole(double axis1, CheckMode mode)
{
  return true;
}


