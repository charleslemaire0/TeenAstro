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
  Axes sky;

  int hemisphere = (*localSite.latitude() >= 0) ? 1 : -1;                     // 1 for Northern Hemisphere
  if (type == MOUNT_TYPE_GEM)
  {
    int flipSign = ((ps==PIER_EAST)==(*localSite.latitude() >= 0)) ? 1 : -1;    // -1 if mount is flipped

    sky.axis1 = hemisphere * eP->ha  - flipSign * 90;
    sky.axis2 = flipSign * (90 - hemisphere * eP->dec);  

    // Get instrument coordinates from sky coordinates using alignment matrix
    pm.instr(&sky, aP);

    if (!withinLimits(aP->axis1 * geoA1.stepsPerDegree, aP->axis2 * geoA2.stepsPerDegree))
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

    if (!withinLimits(aP->axis1 * geoA1.stepsPerDegree, aP->axis2 * geoA2.stepsPerDegree))
      return false;

    if (!checkPole(aP->axis1, CHECKMODE_GOTO))
      return false;

    return true;    
  }
}

void EqMount::axesToEqu(Axes *aP, EqCoords *eP)
{
  Axes instr;
  int hemisphere = (*localSite.latitude() >= 0) ? 1 : -1;
  int flipSign = (aP->axis2 < 0) ? -1 : 1;

  pm.sky(aP, &instr);

  if (type == MOUNT_TYPE_GEM)
  {
    eP->ha  = hemisphere * (instr.axis1 + flipSign * 90);
    eP->dec = hemisphere * (90 - (flipSign * instr.axis2));    
  }
  else // eq fork
  {
    eP->ha  = hemisphere * instr.axis1;
    eP->dec = hemisphere * (90 - instr.axis2);    
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
	Steps newSteps;

	eqCoords.ha = HA;
	eqCoords.dec = Dec;
	eqToAxes(&eqCoords, &axes, Side);
	axesToSteps(&axes, &newSteps);

  sync(&newSteps);
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
  Steps steps;
  Axes axes;
  EqCoords eqCoords;
  PierSide ps = GetPierSide();

  steps.steps1 = motorA1.getCurrentPos();
  steps.steps2 = motorA2.getCurrentPos();

  stepsToAxes(&steps, &axes);
  axesToEqu(&axes, &eqCoords);
  if (eqToAxes(&eqCoords, &axes, otherSide(ps)))
  {
    axesToSteps(&axes, &steps);
    return goTo(&steps);
  }
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
  trackingSpeeds.speed1 = speed * axis1Direction('w');	// multiple of sidereal
  trackingSpeeds.speed2 = 0;
  setEvents(EV_SPEED_CHANGE);
}



/*
 * getTrackingSpeeds
 * called periodically by Control task
 * computes axes speeds (in steps) for both axes
 */
void EqMount::getTrackingSpeeds(Speeds *sP)
{
  int dir;
  double deltaV;

  sP->speed1 = trackingSpeeds.speed1;
  sP->speed2 = trackingSpeeds.speed2;

  if (getEvent(EV_GUIDING_AXIS1 | EV_WEST))
  {
    deltaV = guideRates[0] * axis1Direction('w');
    sP->speed1 += deltaV;
  }
  if (getEvent(EV_GUIDING_AXIS1 | EV_EAST))
  {
    deltaV = guideRates[0] * axis1Direction('e');
    sP->speed1 += deltaV;
  }

  if (getEvent(EV_GUIDING_AXIS2 | EV_NORTH))
  {
    dir = axis2Direction('n');    // direction is zero for the pole, ie home position. Convert to +1 or -1 depending on our current position
    if (dir == 0)
    {
      dir = (motorA2.getCurrentPos() < 0 ? 1:-1);
    }
    deltaV = guideRates[0] * dir;
    sP->speed2 += deltaV;
  }
  if (getEvent(EV_GUIDING_AXIS2 | EV_SOUTH))
  {
    dir = axis2Direction('s');
    if (dir == 0)
    {
      dir = (motorA2.getCurrentPos()  < 0 ? 1:-1);
    }
    deltaV = guideRates[0] * dir;
    sP->speed2 += deltaV;
  }

  if (getEvent(EV_SPIRAL))
  {
    Speeds v;
    getSpiralSpeeds(&v);
    sP->speed1 += v.speed1;
    sP->speed2 += v.speed2;
  }

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
 * return the axis1 tracking direction
 * 
 *   W in N hemisphere: +1 
 *   W in S hemisphere: -1
 *   E in N hemisphere: -1 
 *   E in S hemisphere: +1
 */
int EqMount::axis1Direction(char dir)
{
  return ((dir == 'w') == (*localSite.latitude() >= 0)) ? 1 : -1;
}


/*
 * Pole / Antipole direction
 * return the axis2 direction of the pole / antipole 
 * 
 * pole: always 0
 * antipole:
 *   in N hemisphere: +1 if PierSide=E (not flipped), -1 if PierSide=W (flipped)
 *   in S hemisphere: reverse
 */
int EqMount::axis2Direction(char pole)
{ 
  if ((*localSite.latitude() >= 0) == (pole == 'n'))	// pole?
  {
 		return 0;
  }	

 	return (isFlipped() == (*localSite.latitude() >= 0)) ? -1 : 1;
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
