#include "Global.h"


//----------------------------------------------------------------------------------
//   T - Tracking Commands
//  :T+#   Master sidereal clock faster by 0.1 Hertz (I use a fifth of the LX200 standard, stored in XEEPROM) Returns: Nothing
//  :T-#   Master sidereal clock slower by 0.1 Hertz (stored in XEEPROM) Returns: Nothing
//  :TS#   Track rate solar Returns: Nothing
//  :TL#   Track rate lunar Returns: Nothing
//  :TQ#   Track rate sidereal Returns: Nothing
//  :TT#   Track rate target Returns: Nothing
//  :TR#   Master sidereal clock reset (to calculated sidereal rate, stored in EEPROM) Returns: Nothing
//  :Te#   Tracking enable  (replies 0/1)
//  :Td#   Tracking disable (replies 0/1)

//  :T0#   Track compensation disable (replies 0/1)
//  :T1#   Track compensation only in RA (replies 0/1)
//  :T2#   Track compensation BOTH (replies 0/1)
void Command_T()
{

  switch (command[1])

  {
  case '+':
    siderealClockSpeed -= HzCf * (0.02);
    replyNothing();
    break;
  case '-':
    siderealClockSpeed += HzCf * (0.02);
    replyNothing();
    break;
  case 'S':
    // solar tracking rate 60Hz
    mount.mP->setTrackingSpeed(TrackingSolar, 0);
    siderealMode = SIDM_SUN;
     replyNothing();
    break;
  case 'L':
    // lunar tracking rate 57.9Hz
    mount.mP->setTrackingSpeed(TrackingLunar, 0);
    siderealMode = SIDM_MOON;
    replyNothing();
    break;
  case 'Q':
    // sidereal tracking rate
    mount.mP->setTrackingSpeed(TrackingStar, 0);
    siderealMode = SIDM_STAR;
    replyNothing();
    break;
  case 'R':
    // reset master sidereal clock interval
    siderealClockSpeed = mastersiderealClockSpeed;
    mount.mP->setTrackingSpeed(TrackingStar, 0);
    siderealMode = SIDM_STAR;
    replyNothing();
    break;
  case 'T':
    //set user-defined Target tracking rate
    mount.mP->setTrackingSpeed(1.0 - (double)storedTrackingRateRA / 10000.0, (double)storedTrackingRateDEC / 10000.0);
    siderealMode = SIDM_TARGET;
    replyNothing();
    break;
  case 'e':
    if (parkStatus() == PRK_UNPARKED)
    {
      startTracking();
      replyShortTrue();
    }
    else
      replyLongUnknown();
    break;
  case 'd':
    if (parkStatus() == PRK_UNPARKED)
    {
      stopTracking();
      replyShortTrue();
    }
    else
      replyLongUnknown();
    break;
  case '0':
    // turn compensation off
//    tc = TC_NONE;
//    computeTrackingRate(true);
    XEEPROM.write(getMountAddress(EE_TC_Axis), 0);
    replyShortTrue();
    break;
  case '1':
    // turn compensation RA only
    if (isAltAz())
    {
      replyShortFalse();
    }
    else
    {
//    tc = TC_RA;
//    computeTrackingRate(true);
      XEEPROM.write(getMountAddress(EE_TC_Axis), 0);
      replyShortTrue();
    }
    break;
  case '2':
    // turn compensation BOTH
        if (isAltAz())
    {
      replyShortFalse();
    }
    else
    {

//    tc = TC_BOTH;
//    computeTrackingRate(true);
    XEEPROM.write(getMountAddress(EE_TC_Axis), 2);
    replyShortTrue();
    }
    break;
  default:
    replyLongUnknown();
    break;
  }

  // Only burn the new rate if changing the sidereal interval
  if (command[1] == '+' || command[1] == '-' || command[1] == 'R')
  {
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), siderealClockSpeed);
    updateSidereal();
  }
}