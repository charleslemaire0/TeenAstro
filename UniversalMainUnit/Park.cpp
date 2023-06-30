// -----------------------------------------------------------------------------------
// functions related to PRK_PARKING the mount
#include <Global.h>


// Uses event group to avoid global variables and semaphores
ParkState parkStatus(void)
{
  unsigned long ev, status;

  // compute park state (00 to 11) from event bits
  ev = (xEventGroupGetBits(mountEvents));
  status = 0;

  if (ev & EV_PARK0)
    status |= 1;
  if (ev & EV_PARK1)
    status |= 2;

  return ((ParkState) status);
}

void parkStatus(ParkState status)
{
  switch (status)
  {
    case 0: resetEvents(EV_PARK0 | EV_PARK1); break;
    case 1: setEvents(EV_PARK0); resetEvents(EV_PARK1); break;
    case 2: resetEvents(EV_PARK0); setEvents(EV_PARK1); break;
    case 3: setEvents(EV_PARK0 | EV_PARK1);break;
  }
  XEEPROM.write(getMountAddress(EE_parkStatus), status);
}

// sets the park postion as the current position
bool setPark()
{
  if ((parkStatus() == PRK_UNPARKED) && !isSlewing())
  {
    lastsiderealTracking = isTracking();
    stopTracking();

    long    h = motorA1.getCurrentPos();
    long    d = motorA2.getCurrentPos();

    // store our position
    XEEPROM.writeLong(getMountAddress(EE_posAxis1), h);
    XEEPROM.writeLong(getMountAddress(EE_posAxis2), d);

    // and the align
    saveAlignModel();
    parkSaved = true;
    XEEPROM.write(getMountAddress(EE_parkSaved), parkSaved);
    if (lastsiderealTracking)
      startTracking();
    else
      stopTracking();

    return true;
  }

  return false;
}

void unsetPark()
{
  if (parkSaved)
  {
    parkSaved = false;
    XEEPROM.write(getMountAddress(EE_parkSaved), parkSaved);
  }
}

void saveAlignModel()
{
  // and store our corrections
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  XEEPROM.write(getMountAddress(EE_Tvalid), mount.mP->hasStarAlignment());
  if (mount.mP->hasStarAlignment())
  {
    mount.mP->alignment.getT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
  }
  XEEPROM.writeFloat(getMountAddress(EE_T11), t11);
  XEEPROM.writeFloat(getMountAddress(EE_T12), t12);
  XEEPROM.writeFloat(getMountAddress(EE_T13), t13);
  XEEPROM.writeFloat(getMountAddress(EE_T21), t21);
  XEEPROM.writeFloat(getMountAddress(EE_T22), t22);
  XEEPROM.writeFloat(getMountAddress(EE_T23), t23);
  XEEPROM.writeFloat(getMountAddress(EE_T31), t31);
  XEEPROM.writeFloat(getMountAddress(EE_T32), t32);
  XEEPROM.writeFloat(getMountAddress(EE_T33), t33);
  return;
}


// moves the telescope to the park position, stops tracking
byte park()
{
  // Gets park position and moves the mount there
  if (parkStatus() == PRK_PARKED)
  {
    return 0;
  }
  if (lastError() != ERRT_NONE)
  {
    return 4;
  }
  if (!isSlewing())
  {
    if (parkStatus() == PRK_UNPARKED)
    {
      if (parkSaved)
      {
        Steps steps;
        // get the position we're supposed to park at
        steps.steps1 = XEEPROM.readLong(getMountAddress(EE_posAxis1));
        steps.steps2 = XEEPROM.readLong(getMountAddress(EE_posAxis2));

        lastsiderealTracking = false;
        // record our status
        parkStatus(PRK_PARKING);

        goTo(&steps);
        return 0;
      }
      else
        return 1;   // no park position saved
    }
    else
      return 2;    // not parked
  }
  else
    return 3;     // already moving
}

// returns a parked telescope to operation, you must set date and time before calling this.  it also
bool syncAtPark()
{
  if (!parkSaved)
  {
    return false;
  }
  resetEvents(EV_AT_HOME);
  delay(10);
  // get corrections
  //GeoAlign.readCoe();

  // get our position
  long axis1, axis2;
  axis1 = XEEPROM.readLong(getMountAddress(EE_posAxis1));
  axis2 = XEEPROM.readLong(getMountAddress(EE_posAxis2));
 
  motorA1.setTargetPos(axis1);
  motorA2.setTargetPos(axis2);

  return true;
}

//initialisation at park
bool iniAtPark()
{
  parkSaved = XEEPROM.read(getMountAddress(EE_parkSaved));
  if (!parkSaved)
  {
    parkStatus(PRK_UNPARKED);
    return false;
  }
  byte parkStatusRead = XEEPROM.read(getMountAddress(EE_parkStatus));
  bool ok = false;
  switch (parkStatusRead)
  {
  case PRK_PARKED:
    if (syncAtPark())
    {
      parkStatus(PRK_PARKED);
      ok = true;
    }
    else
    {
      parkStatus(PRK_UNPARKED);
    }
    break;
  case PRK_UNPARKED:
    parkStatus(PRK_UNPARKED);
    return false;
    break;
  default:
    parkStatus(PRK_UNPARKED);
    break;
  }
  return ok;
}

// depends on the latitude, longitude, and timeZone; but those are stored and recalled automatically
void unpark()
{
  if (parkStatus() == PRK_UNPARKED)
    return;
  // update our status, we're not parked anymore
  parkStatus(PRK_UNPARKED);
}


