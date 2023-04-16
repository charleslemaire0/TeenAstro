#include "Global.h"


void EqMount::axesToSteps(Axes *aP, Steps *sP)
{
  sP->steps1 = (long)((aP->axis1) * geoA1.stepsPerDegree);
  sP->steps2 = (long)((aP->axis2) * geoA2.stepsPerDegree);
}

void EqMount::stepsToAxes(Steps *sP, Axes *aP)
{
  aP->axis1 = ((double)(sP->steps1) / geoA1.stepsPerDegree);
  aP->axis2 = ((double)(sP->steps2) / geoA2.stepsPerDegree);
}

/*
 * Our definition for the flipped condition: if axis2 position is negative (for N hemisphere)
 */
bool EqMount::isFlipped()
{
  long pos = motorA2.getCurrentPos();
 	return ((pos < 0) == (*localSite.latitude() >= 0));	
}

PierSide EqMount::GetPierSide()
{
 	return (isFlipped()) ? PIER_WEST : PIER_EAST;
}

PierSide EqMount::otherSide(PierSide ps)
{
  return (ps == PIER_EAST ? PIER_WEST : PIER_EAST);
}

byte EqMount::goToEqu(EqCoords *eP)
{
	Axes axes;
	Steps steps;
	PierSide ps;
  HorCoords hor;

  // check destination altitude
  EquToHor(eP->ha, eP->dec, false, &hor.az, &hor.alt, localSite.cosLat(), localSite.sinLat());
  if (hor.alt < limits.minAlt)
    return ERRGOTO_BELOWHORIZON;
  if (hor.alt > limits.maxAlt)
    return ERRGOTO_ABOVEOVERHEAD;

	ps = GetPierSide();
  if (!eqToAxes(eP, &axes, ps))                 // try current side
  {
    if (!eqToAxes(eP, &axes, otherSide(ps)))    // try other side
      return ERRGOTO_LIMITS;                    //fail, outside limit
  }
  axesToSteps(&axes, &steps);

  return goTo(&steps);
}

byte EqMount::goToHor(HorCoords *hP)
{
	EqCoords eq;
	
	HorTopoToEqu(hP->az, hP->alt, &eq.ha, &eq.dec, localSite.cosLat(), localSite.sinLat());
	eq.ha = haRange(eq.ha);	// in case ha is +180
	return goToEqu(&eq);
}


/*
 * Find on which pier side the target will be 
 *
 */
bool EqMount::getTargetPierSide(EqCoords *eP, PierSide *psOutP)
{
  Axes axes;
  PierSide ps = GetPierSide();

  if (!eqToAxes(eP, &axes, ps))                 // try current side
  {
    ps = otherSide(ps);
    if (!eqToAxes(eP, &axes, ps))                 // try other side
      return false;                             //fail, outside limit
  }
  *psOutP = ps;
  return true;
}


/*
 * eqToAxes: computes axes from equatorial coordinates
 * return false if we cannot reach the given position on the given side
 */
bool EqMount::eqToAxes(EqCoords *eP, Axes *aP, PierSide ps)
{  
  int hemisphere = (*localSite.latitude() >= 0) ? 1 : -1;                     // 1 for Northern Hemisphere
  if (type == MOUNT_TYPE_GEM)
  {
    int flipSign = ((ps==PIER_EAST)==(*localSite.latitude() >= 0)) ? 1 : -1;    // -1 if mount is flipped

    aP->axis1 = hemisphere * eP->ha  - flipSign * 90;
    aP->axis2 = flipSign * (90 - hemisphere * eP->dec);  

    if (!withinLimits(aP->axis1, aP->axis2))
      return false;

    if (!checkMeridian(aP, CHECKMODE_GOTO, ps))
      return false;

    if (!checkPole(aP->axis1, CHECKMODE_GOTO))
      return false;

    return true;    
  }
  else // eq fork
  {
    aP->axis1 = hemisphere * eP->ha;
    aP->axis2 = 90 - (hemisphere * eP->dec);  

    if (!withinLimits(aP->axis1, aP->axis2))
      return false;

    if (!checkPole(aP->axis1, CHECKMODE_GOTO))
      return false;

    return true;    
  }
}

void EqMount::axesToEqu(Axes *aP, EqCoords *eP)
{
  int hemisphere = (*localSite.latitude() >= 0) ? 1 : -1;
  int flipSign = (aP->axis2 < 0) ? -1 : 1;
  if (type == MOUNT_TYPE_GEM)
  {
    eP->ha  = hemisphere * (aP->axis1 + flipSign * 90);
    eP->dec = hemisphere * (90 - (flipSign * aP->axis2));    
  }
  else // eq fork
  {
    eP->ha  = hemisphere * aP->axis1;
    eP->dec = hemisphere * (90 - aP->axis2);    
  }
}

bool EqMount::getEqu(double *haP, double *decP, UNUSED(const double *cosLatP), UNUSED(const double *sinLatP), bool returnHA)
{
	Steps steps;
	Axes axes;
	EqCoords eqCoords;

	steps.steps1 = motorA1.getCurrentPos();
	steps.steps2 = motorA2.getCurrentPos();

	stepsToAxes(&steps, &axes);
	axesToEqu(&axes, &eqCoords);
	*haP = eqCoords.ha;
	*decP = eqCoords.dec;

  if (!returnHA)	// return RA instead of HA
  {
    *haP = degRange(rtk.LST() * 15.0 - *haP);
  }
  return true;
}

bool EqMount::getHorApp(HorCoords *hP)
{
	double ha, dec;

	getEqu(&ha, &dec, localSite.cosLat(), localSite.sinLat(), true);
	EquToHor(ha, dec, false, &hP->az, &hP->alt, localSite.cosLat(), localSite.sinLat());

  return true;
}



bool EqMount::syncEqu(double HA, double Dec, PierSide Side, UNUSED(const double *cosLat), UNUSED(const double *sinLat))
{
	EqCoords eqCoords;
	Axes axes;
	Steps steps;

	eqCoords.ha = HA;
	eqCoords.dec = Dec;
	eqToAxes(&eqCoords, &axes, Side);
	axesToSteps(&axes, &steps);
	motorA1.setCurrentPos(steps.steps1); 
	motorA2.setCurrentPos(steps.steps2); 
  return true;
}

bool EqMount::syncAzAlt(double Azm, double Alt, PierSide Side)
{
	EqCoords eq;
	
	HorTopoToEqu(Azm, Alt, &eq.ha, &eq.dec, localSite.cosLat(), localSite.sinLat());
	return syncEqu(eq.ha, eq.dec, Side, localSite.cosLat(), localSite.sinLat());
}

byte EqMount::Flip()
{
#if 0  
  long axis1Flip, axis2Flip;
  PierSide selectedSide = PIER_NOTVALID;
  PierSide preferedPierSide = (GetPierSide() == PIER_EAST) ? PIER_WEST : PIER_EAST;
  double Axis1 = motorA1.getCurrentPos() / (double)geoA1.stepsPerDegree;
  double Axis2 = motorA1.getCurrentPos() / (double)geoA2.stepsPerDegree;
  if (!predictTarget(Axis1, Axis2, preferedPierSide, axis1Flip, axis2Flip, selectedSide))
  {
    return ERRGOTO_SAMESIDE;
  }
  return goTo(axis1Flip, axis2Flip);
#endif  
  return ERRGOTO_NONE;
}

/*
 * checkPole
 * check if the hour angle is within the under pole limits 
 * min. allowable movement is += 9 decimal hours on either side of the pole
 * no limit: +- 12 decimal hours
 */
bool EqMount::checkPole(double axis1, CheckMode mode)
{
  double underPoleLimit;

  if (mode == CHECKMODE_GOTO)
    underPoleLimit = limits.underPoleLimitGOTO;              // for determining if a GOTO is valid, use exact value
  else 
    underPoleLimit = limits.underPoleLimitGOTO + 5.0 / 60;   // for triggering an error, add 5 degrees (1/12 decimal hour)

  return (axis1 > (-underPoleLimit * 15.0)) && (axis1 < (underPoleLimit * 15.0));
}



bool EqMount::checkMeridian(Axes *aP, CheckMode mode, PierSide ps)
{
	EqCoords eqCoords;
	axesToEqu(aP, &eqCoords);
  // limits are stored in minutes of RA. Divide by 4 to get degrees
  double DegreesPastMeridianW = ((mode == CHECKMODE_GOTO) ? limits.minutesPastMeridianGOTOW : limits.minutesPastMeridianGOTOW + 5)/4;
  double DegreesPastMeridianE = ((mode == CHECKMODE_GOTO) ? limits.minutesPastMeridianGOTOE : limits.minutesPastMeridianGOTOE + 5)/4;
  switch (ps)
  {
    case PIER_WEST:
      if (eqCoords.ha >  DegreesPastMeridianW) 
        return false;
      break;
    case PIER_EAST:
      if (eqCoords.ha < -DegreesPastMeridianE)
        return false;
      break;
    default:
        return false;
      break;
  }
  return true;
}

/*
 * setTrackingSpeed
 * speed is expressed as a multiple of sidereal speed 
 */
void EqMount::setTrackingSpeed(double speed)
{
  trackingSpeed = speed;					// remember to reset it after guiding etc.
  trackingSpeeds.speed1 = speed;	// multiple of sidereal
  trackingSpeeds.speed2 = 0;
}



/*
 * getTrackingSpeeds
 * called periodically by Control task
 * computes axes speeds (in steps) for both axes
 */
void EqMount::getTrackingSpeeds(Speeds *sP)
{
  sP->speed1 = trackingSpeeds.speed1;
  sP->speed2 = trackingSpeeds.speed2;

  if (getEvent(EV_GUIDING_AXIS1 | EV_WEST))
    sP->speed1 += guideRates[0];
  if (getEvent(EV_GUIDING_AXIS1 | EV_EAST))
    sP->speed1 -= guideRates[0];

  if (getEvent(EV_GUIDING_AXIS2 | EV_NORTH))
    sP->speed2 += guideRates[0];
  if (getEvent(EV_GUIDING_AXIS2 | EV_SOUTH))
    sP->speed2 -= guideRates[0];

 sP->speed1 *= (geoA1.stepsPerSecond/SIDEREAL_SECOND);
 sP->speed2 *= (geoA2.stepsPerSecond/SIDEREAL_SECOND);
}

void EqMount::initHome(Steps *sP)
{
  if (type == MOUNT_TYPE_GEM)
  {
    sP->steps1 = 0;
    sP->steps2 = 0;
  }
  else // eq fork
  {
    sP->steps1 = geoA1.halfRot;
    sP->steps2 = 0;
  }
}



/*
 * Pole / Antipole direction
 * return the axis2 position in steps of the pole / antipole directions
 * 
 * pole: always 0
 * antipole:
 *   in N hemisphere: +∞ if PierSide=E (not flipped), -∞ if PierSide=W (flipped)
 *   in S hemisphere: reverse
 */
long EqMount::poleDir(char pole)
{
  
  if ((*localSite.latitude() >= 0) == (pole == 'n'))	// pole?
  {
 		return 0;
  }	

	long pos = motorA2.getCurrentPos();
 	return ((pos >= 0) == (*localSite.latitude() >= 0)) ? geoA2.stepsPerRot : -geoA2.stepsPerRot;
}


/*
 * decDirection
 *  1 if declination is increasing
 * -1 if decreasing
 *  0 if not changing
 */
int EqMount::decDirection(void)
{
  long pos = motorA2.getCurrentPos(); 
  double sp = motorA2.getSpeed();
  if (sp == 0.0)
    return 0;

  if (pos < 0)
    return (sp < 0.0 ? -1:1);
  else 
    return (sp < 0.0 ? 1:-1);
}

// not used at this time for EQ mounts
void EqMount::updateRaDec(void)
{

}
