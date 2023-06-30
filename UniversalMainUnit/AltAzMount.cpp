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
  HorCoords instr;

	if (*localSite.latitude() >= 0)
  	instr.az  = AzRange(aP->axis1 + 180);
  else
  	instr.az  = AzRange(aP->axis1);

  instr.alt = aP->axis2;
//  alignment.toReferenceDeg(hP->az, hP->alt, instr.az, instr.alt);
  hP->az = instr.az;
  hP->alt = instr.alt;
}



/*
 * horToAxes: computes axes from horizontal coordinates
 * return false if we cannot reach the given position (TODO: check zenith limit)
 */
bool AltAzMount::horToAxes(HorCoords *hP, Axes *aP)
{
  HorCoords ref;

  // Get instrument coordinates from ref. coordinates using alignment matrix
//  alignment.toInstrumentDeg(ref.az, ref.alt, hP->az, hP->alt);
  ref.az = hP->az;
  ref.alt = hP->alt;

  long currentAxis1 = motorA1.getCurrentPos() / geoA1.stepsPerDegree;

	aP->axis2 = ref.alt;
  if (*localSite.latitude() >= 0)
    aP->axis1 = ref.az - 180;
  else
    aP->axis1 = ref.az;

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
  Steps newSteps;

	horCoords.az = Azm;
	horCoords.alt = Alt;
	horToAxes(&horCoords, &axes);
	axesToSteps(&axes, &newSteps);
  
  motorA1.syncPos(newSteps.steps1);
  motorA2.syncPos(newSteps.steps2);
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
  double deltaV;

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
  {
    deltaV = guideRates[0] * axis1Direction('w');
    equ2.ha = haRange(equ2.ha - deltaV);
  }
  if (getEvent(EV_GUIDING_AXIS1 | EV_EAST))
  {
    deltaV = guideRates[0] * axis1Direction('e');
    equ2.ha = haRange(equ2.ha + deltaV);
  }
  if (getEvent(EV_GUIDING_AXIS2 | EV_NORTH))
  {
    deltaV = guideRates[0] * axis2Direction('n');
    equ2.dec = fmin (90, equ2.dec + deltaV);
  }
  if (getEvent(EV_GUIDING_AXIS2 | EV_SOUTH))
  {
    deltaV = guideRates[0] * axis2Direction('s');
    equ2.dec = fmax (0, equ2.dec - deltaV);
  }

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

  // if needed, add spiral speeds directly to the computed speeds (or should we add it to the Ha/Dec?)
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
 * same as EqMount 
 */
int AltAzMount::axis1Direction(char dir)
{
  return ((dir == 'w') == (*localSite.latitude() >= 0)) ? 1 : -1;
}


/*
 * For AltAz mounts, 
 * "pole" is really the zenith at +90º, antipole at -90º 
 */
int AltAzMount::axis2Direction(char pole)
{
  return ((*localSite.latitude() >= 0) == (pole=='n')) ? 1 : -1;
}

/*
 * After any move, keep track of the current eq coordinates for computing tracking rates
 */ 
void AltAzMount::updateRaDec(void)
{
  getEqu(&currentRA, &currentDec, localSite.cosLat(), localSite.sinLat(), false);
}

void AltAzMount::initTransformation(bool reset)
{
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  mount.mP->hasStarAlignment(false);
  mount.mP->alignment.clean();
  byte TvalidFromEEPROM = XEEPROM.read(getMountAddress(EE_Tvalid));

  if (TvalidFromEEPROM == 1 && reset)
  {
    XEEPROM.write(getMountAddress(EE_Tvalid), 0);
  }
  if (TvalidFromEEPROM == 1 && !reset)
  {
    t11 = XEEPROM.readFloat(getMountAddress(EE_T11));
    t12 = XEEPROM.readFloat(getMountAddress(EE_T12));
    t13 = XEEPROM.readFloat(getMountAddress(EE_T13));
    t21 = XEEPROM.readFloat(getMountAddress(EE_T21));
    t22 = XEEPROM.readFloat(getMountAddress(EE_T22));
    t23 = XEEPROM.readFloat(getMountAddress(EE_T23));
    t31 = XEEPROM.readFloat(getMountAddress(EE_T31));
    t32 = XEEPROM.readFloat(getMountAddress(EE_T32));
    t33 = XEEPROM.readFloat(getMountAddress(EE_T33));
    mount.mP->alignment.setT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    mount.mP->alignment.setTinvFromT();
    mount.mP->hasStarAlignment(true);
  }
  else
  {
    alignment.addReferenceDeg(90, 0, 90, 0);
    alignment.addReferenceDeg(180, 0, 180, 0);
    alignment.calculateThirdReference();
  }
}


// These functions are never called. They keep the linker happy
byte AltAzMount::Flip(void)
{
	return 0;
}

bool AltAzMount::getTargetPierSide(UNUSED(EqCoords *eP), UNUSED(PierSide *psOutP))
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

bool AltAzMount::checkMeridian(UNUSED(Axes *aP), UNUSED(CheckMode mode), UNUSED(PierSide ps))
{
  return true;
}

bool AltAzMount::checkPole(UNUSED(double axis1), UNUSED(CheckMode mode))
{
  return true;
}


