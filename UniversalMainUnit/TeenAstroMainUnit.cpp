#define TEENASTRO_MAIN_UNIT
/*
 * TeenAstroMainUnit
 *
 */
#include "Global.h"


ISR (siderealTimer)
{
  rtk.m_lst++;
}
// Define interrupt handlers
ISR (isrStepDir1)
{
  StepDirInterruptHandler((StepDir *) motorA1.mcP);
}

ISR (isrStepDir2)
{
  StepDirInterruptHandler((StepDir *) motorA2.mcP);
}
const unsigned long siderealTickCount = (unsigned long ) (SIDEREAL_SECOND * 100000);

#ifdef __ESP32__
// We run at 80MHz 
hw_timer_t *timer1P;

void beginTimer(void)
{
  timer1P = timerBegin(0, 8, true);   // set divider to 8: count increments every µS / 10
  timerAttachInterrupt(timer1P, &siderealTimer, true);
  timerAlarmWrite(timer1P, siderealTickCount, true);      // interrupt every 10mS (sidereal)  
  timerAlarmEnable(timer1P);
}

#endif

#ifdef __arm__
IntervalTimer  itimer1;
void beginTimer(void)
{
  // interrupt every 10mS (sidereal) or 10000 µS (sidereal)
  itimer1.begin(siderealTimer, siderealTickCount/10);  // argument is in µS   
  itimer1.priority(32);
}
#endif


void initTransformation(bool reset)
{
  float t11 = 0, t12 = 0, t13 = 0, t21 = 0, t22 = 0, t23 = 0, t31 = 0, t32 = 0, t33 = 0;
  hasStarAlignment = false;
  alignment.clean();
  byte TvalidFromEEPROM = XEEPROM.read(EE_Tvalid);

  if (TvalidFromEEPROM ==1 && reset)
  {
    XEEPROM.write(EE_Tvalid, 0);
  }
  if (TvalidFromEEPROM == 1 && !reset)
  {
    t11 = XEEPROM.readFloat(EE_T11);
    t12 = XEEPROM.readFloat(EE_T12);
    t13 = XEEPROM.readFloat(EE_T13);
    t21 = XEEPROM.readFloat(EE_T21);
    t22 = XEEPROM.readFloat(EE_T22);
    t23 = XEEPROM.readFloat(EE_T23);
    t31 = XEEPROM.readFloat(EE_T31);
    t32 = XEEPROM.readFloat(EE_T32);
    t33 = XEEPROM.readFloat(EE_T33);
    alignment.setT(t11, t12, t13, t21, t22, t23, t31, t32, t33);
    alignment.setTinvFromT();
    hasStarAlignment = true;
  }
  else
  {
      alignment.addReferenceDeg(90, 45, 90, 45);
      alignment.addReferenceDeg(270, 45, 270, 45);
      alignment.calculateThirdReference();
  }
}


void initMount()
{
  byte val = XEEPROM.read(EE_mountType);
  long lval = 0;

  switch(val)
  {
    case 1:
      mount.init(MOUNT_TYPE_GEM);
    break;
    case 2:
      mount.init(MOUNT_TYPE_FORK);
    break;
    case 3:
      mount.init(MOUNT_TYPE_ALTAZM);
    break;
    case 4:
      mount.init(MOUNT_TYPE_FORK_ALT);
    break;
    default:
      mount.init(MOUNT_TYPE_GEM);
  }
  mount.DegreesForAcceleration = 0.1*EEPROM.read(EE_degAcc);
  if (mount.DegreesForAcceleration == 0 || mount.DegreesForAcceleration > 25)
  {
    mount.DegreesForAcceleration = 3.0;
    XEEPROM.write(EE_degAcc, (uint8_t)(mount.DegreesForAcceleration * 10));
  }
  // get the min. and max altitude
  limits.minAlt = XEEPROM.read(EE_minAlt) - 128;
  limits.maxAlt = XEEPROM.read(EE_maxAlt);
  limits.minutesPastMeridianGOTOE = round(((EEPROM.read(EE_dpmE) - 128)*60.0) / 15.0);
  if (abs(limits.minutesPastMeridianGOTOE) > 180)
    limits.minutesPastMeridianGOTOE = 60;
  limits.minutesPastMeridianGOTOW = round(((EEPROM.read(EE_dpmW) - 128)*60.0) / 15.0);
  if (abs(limits.minutesPastMeridianGOTOW) > 180)
    limits.minutesPastMeridianGOTOW = 60;
  limits.underPoleLimitGOTO = (double)EEPROM.read(EE_dup) / 10;
  if (limits.underPoleLimitGOTO < 9 || limits.underPoleLimitGOTO>12)
    limits.underPoleLimitGOTO = 12;
  
  // initialize some fixed-point values
  guideA1.amount = 0;
  guideA2.amount = 0;

  // Tracking and rate control
  val = XEEPROM.read(EE_TC_Axis);
  tc = val < 0 || val >  2 ? TC_NONE : static_cast<TrackingCompensation>(val);
  lval = XEEPROM.read(EE_RA_Drift);
  storedTrackingRateRA  = lval < -50000 || lval > 50000? 0 :lval;
  lval = XEEPROM.read(EE_DEC_Drift);
  storedTrackingRateDEC = lval < -50000 || lval > 50000 ? 0 : lval;
}



void initMotors(bool deleteAlignment)
{
  readEEPROMmotor();

  pinMode(Axis1CSPin, OUTPUT);
  pinMode(Axis2CSPin, OUTPUT);
  SPI.begin();

  // Generic initialization (works for both types)
  motorA1.init(Axis1CSPin, hwMutex);
  motorA2.init(Axis2CSPin, hwMutex);

#if 0
  // find out if we have StepDir or SPI-only and initialize either type (see SD_MODE in TMC5160 data sheet)
  if (motorA1.drvP->sd_mode())
    motorA1.initStepDir(Axis1DirPin, Axis1StepPin, isrStepDir1, 2);
  else
    motorA1.initMc5160();

  if (motorA1.drvP->sd_mode())
    motorA2.initStepDir(Axis2DirPin, Axis2StepPin, isrStepDir2, 3);
  else
    motorA2.initMc5160();
#else 
  motorA1.initStepDir(Axis1DirPin, Axis1StepPin, isrStepDir1, 2);
  motorA2.initStepDir(Axis2DirPin, Axis2StepPin, isrStepDir2, 3);
#endif

  // cannot call updateRatios before motors are initialized!
  updateRatios(deleteAlignment,false);
}

void readEEPROMmotor()
{
  mount.backlashA1.inSeconds = XEEPROM.readInt(EE_backlashAxis1);
  mount.backlashA1.movedSteps = 0;
  motorA1.gear = XEEPROM.readInt(EE_motorA1gear);
  motorA1.stepRot = XEEPROM.readInt(EE_motorA1stepRot);

  motorA1.micro = XEEPROM.read(EE_motorA1micro);
  if (motorA1.micro > 8 || motorA1.micro < 1)
  {
    motorA1.micro = 4;
    XEEPROM.write(EE_motorA1micro, 4u);
  }

  motorA1.reverse = XEEPROM.read(EE_motorA1reverse);

  motorA1.lowCurr = (unsigned int)XEEPROM.read(EE_motorA1lowCurr) * 100;
  if (motorA1.lowCurr > 2800u || motorA1.lowCurr < 200u)
  {
    motorA1.lowCurr = 1000u;
    XEEPROM.write(EE_motorA1lowCurr, 10u);
  }

  motorA1.highCurr = (unsigned int)XEEPROM.read(EE_motorA1highCurr) * 100;
  if (motorA1.highCurr > 2800u || motorA1.highCurr < 200u)
  {
    motorA1.highCurr = 1000u;
    XEEPROM.write(EE_motorA1highCurr, 10u);
  }

  motorA1.silent = XEEPROM.read(EE_motorA1silent);
  mount.backlashA2.inSeconds = XEEPROM.readInt(EE_backlashAxis2);
  mount.backlashA2.movedSteps = 0;
  motorA2.gear = XEEPROM.readInt(EE_motorA2gear);
  motorA2.stepRot = XEEPROM.readInt(EE_motorA2stepRot);
  motorA2.micro = XEEPROM.read(EE_motorA2micro);

  // do not allow microsteps higher than 256 (2^8)
  if (motorA2.micro > 8 || motorA2.micro < 1)
  {
    motorA2.micro = 4;
    XEEPROM.write(EE_motorA2micro, 4);
  }
  motorA2.reverse = XEEPROM.read(EE_motorA2reverse);

  motorA2.lowCurr = (unsigned int)XEEPROM.read(EE_motorA2lowCurr) * 100;
  if (motorA2.lowCurr > 2800u || motorA2.lowCurr < 200u)
  {
    motorA2.lowCurr = 1000u;
    XEEPROM.write(EE_motorA2lowCurr, 10u);
  }

  motorA2.highCurr = (unsigned int)XEEPROM.read(EE_motorA2highCurr) * 100;
  if (motorA2.highCurr > 2800u || motorA2.highCurr < 200u)
  {
    motorA2.highCurr = 1000u;
    XEEPROM.write(EE_motorA2highCurr, 10u);
  }

  motorA2.silent = XEEPROM.read(EE_motorA2silent);
}

void writeDefaultEEPROMmotor()
{
  // init (clear) the backlash amounts
  XEEPROM.writeInt(EE_backlashAxis1, 0);
  XEEPROM.writeInt(EE_motorA1gear, 1800);
  XEEPROM.writeInt(EE_motorA1stepRot, 200);
  XEEPROM.write(EE_motorA1micro, 4);
  XEEPROM.write(EE_motorA1reverse, 0);
  XEEPROM.write(EE_motorA1highCurr, 10);
  XEEPROM.write(EE_motorA1lowCurr, 10);
  XEEPROM.write(EE_motorA1silent, 0);

  XEEPROM.writeInt(EE_backlashAxis2, 0);
  XEEPROM.writeInt(EE_motorA2gear, 1800);
  XEEPROM.writeInt(EE_motorA2stepRot, 200);
  XEEPROM.write(EE_motorA2micro, 4);
  XEEPROM.write(EE_motorA2reverse, 0);
  XEEPROM.write(EE_motorA2highCurr, 10);
  XEEPROM.write(EE_motorA2lowCurr, 10);
  XEEPROM.write(EE_motorA2silent, 0);
}

void updateRatios(bool deleteAlignment, bool deleteHP)
{
  cli();
  geoA1.setstepsPerRot((long)motorA1.gear * motorA1.stepRot * (int)pow(2, motorA1.micro));
  geoA2.setstepsPerRot((long)motorA2.gear * motorA2.stepRot * (int)pow(2, motorA2.micro));
  mount.backlashA1.inSteps = (int)round(((double)mount.backlashA1.inSeconds * 3600.0) / (double)geoA1.stepsPerDegree);
  mount.backlashA2.inSteps = (int)round(((double)mount.backlashA2.inSeconds * 3600.0) / (double)geoA2.stepsPerDegree);
  sei();

  initTransformation(deleteAlignment);
  if (deleteHP)
  {
    unsetPark();
    unsetHome();
  }
  initHome();
  initLimit();
//  updateSidereal();
  initMaxSpeed();
}

void updateSidereal()
{
#if 0 
  // 16MHZ clocks for steps per second of sidereal tracking
  cli();
  SiderealRate = siderealClockRate / geoA1.stepsPerSecond;
  TakeupRate = SiderealRate / 8L;
  sei();
  staA1.timerRate = SiderealRate;
  staA2.timerRate = SiderealRate;
  SetTrackingRate(default_tracking_rate,0);

  // backlash takeup rates
  backlashA1.timerRate = staA1.timerRate / BacklashTakeupRate;
  backlashA2.timerRate = staA2.timerRate / BacklashTakeupRate;

  // initialize the timers that handle the sidereal clock, RA, and Dec
  setSiderealClockRate(siderealClockRate);
#endif
}


void EEPROM_AutoInit(void)
{

#ifdef __ESP32__
#define EEPROM_SIZE 512  
  EEPROM.begin(EEPROM_SIZE);
#endif  
#ifdef __arm__
  EEPROM.begin();
#endif  

  // EEPROM automatic initialization
  long thisAutoInitKey = XEEPROM.readLong(EE_autoInitKey);
  if (thisAutoInitKey != initKey)
  {
    for (int i = 0; i < XEEPROM.length(); i++)
    {
      XEEPROM.write(i, 0);
    }
    // init the site information, lat/long/tz/name
    localSite.initdefault();
    XEEPROM.write(EE_mountType, MOUNT_TYPE_GEM);
    // init the min and max altitude
    limits.minAlt = -10;
    limits.maxAlt = 91;
    XEEPROM.write(EE_minAlt, limits.minAlt + 128);
    XEEPROM.write(EE_maxAlt, limits.maxAlt);
    XEEPROM.write(EE_dpmE, 0);
    XEEPROM.write(EE_dpmW, 0);
    XEEPROM.write(EE_dup, (12 - 9) * 15);
    XEEPROM.writeInt(EE_minAxis1, 3600);
    XEEPROM.writeInt(EE_maxAxis1, 3600);
    XEEPROM.writeInt(EE_minAxis2, 3600);
    XEEPROM.writeInt(EE_maxAxis2, 3600);
    writeDefaultEEPROMmotor();

    // init the Park status
    XEEPROM.write(EE_parkSaved, false);
    XEEPROM.write(EE_homeSaved, false);
    XEEPROM.write(EE_parkStatus, PRK_UNPARKED);

    // init the  rate
    XEEPROM.write(EE_Rate0, 100);
    XEEPROM.write(EE_Rate1, 4);
    XEEPROM.write(EE_Rate2, 16);
    XEEPROM.write(EE_Rate3, 64);

#if 0
!! fix this!!
    // init the default maxSpeed
    if (mount.maxSpeed < 2L * 16L) mount.maxSpeed = 2L * 16L;
    if (mount.maxSpeed > motorA1.siderealSpeed(MAX_TEENASTRO_SPEED)) 
      mount.maxSpeed = motorA1.siderealSpeed(MAX_TEENASTRO_SPEED);
    XEEPROM.writeInt(EE_maxRate, (int)(mount.maxSpeed / 16L));
#endif

    // init degree for acceleration
    XEEPROM.write(EE_degAcc, (uint8_t)(mount.DegreesForAcceleration * 10));

    // init the sidereal tracking rate, use this once - then issue the T+ and T- commands to fine tune
    // 1/16uS resolution timer, ticks per sidereal second
    XEEPROM.writeLong(EE_siderealClockRate, siderealClockRate);

    // the transformation is not valid
    XEEPROM.write(EE_Tvalid, 0);
    // reset flag for Tracking Correction
    XEEPROM.write(EE_TC_Axis, 0);

    XEEPROM.writeLong(EE_RA_Drift, 0);
    XEEPROM.writeLong(EE_DEC_Drift, 0);

    // reset flag for Apparent Pole
    doesRefraction.resetEEPROM();
    // finally, stop the init from happening again
    XEEPROM.writeLong(EE_autoInitKey, initKey);
  }  
}

time_t getFreeRTOSTimer(void)
{
  // until we get a real-time clock, return phony time around Jan 1 2020
  return ((50 * 365.25 * 24 * 3600) + ((double) xTaskGetTickCount() / 1000.0));
}



void setup()
{
  Serial.begin(BAUD);
  S_SHC.attach_Stream((Stream *)&Serial, COMMAND_SERIAL1);

  pinMode(LED_BUILTIN, OUTPUT);
  for (int k = 0; k < 20; k++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10);
    digitalWrite(LED_BUILTIN, LOW);
    delay(50);
  }
  EEPROM_AutoInit();

  // get the site information from EEPROM
  localSite.ReadCurrentSiteDefinition();
  doesRefraction.readFromEEPROM();
  highPrecision = true;
  initMount();
  initMotors(false);

  // init the date and time January 1, 2013. 0 hours LMT
  setSyncProvider(getFreeRTOSTimer);

#if 0
  setSyncInterval(1);
  setTime(rtk.getTime());
#endif

  // FreeRTOS structures
  // Mutexes to prevent concurrent accesses 
  // A task must take the mutex before hw access and release it afterwards
  hwMutex = xSemaphoreCreateMutex();  // hardware accesses (ie SPI etc.)
  swMutex = xSemaphoreCreateMutex();  // global variables
  // queue for receiving messages from other tasks
  controlQueue = xQueueCreate( CTL_QUEUE_SIZE, CTL_MAX_MESSAGE_SIZE * sizeof(unsigned));
  // Event group for all mount events
  mountEvents = xEventGroupCreate();

  // automatic mode switching before/after slews, initialize micro-step mode
//  DecayModeTracking();

  siderealClockRate = XEEPROM.readLong(EE_siderealClockRate);
  updateSidereal();
  beginTimer();
  mount.mP->setTrackingSpeed(TrackingStar);

#ifndef __arm__
  Serial2.begin(BAUD);
  S_USB.attach_Stream((Stream *)&Serial2, COMMAND_SERIAL);
#endif
  

  rtk.resetLongitude(*localSite.longitude());

  // get the Park status
  if (!iniAtPark())
  {
      syncAtHome();
  }

  // get the pulse-guide rate
  int val = EEPROM.read(EE_Rate0);
  guideRates[0] = val > 0 ? (float)val / 100 : DefaultR0;
  val = EEPROM.read(EE_Rate1);
  guideRates[1] = val > 0 ? (float)val : DefaultR1;
  val = EEPROM.read(EE_Rate2);
  guideRates[2] = val > 0 ? (float)val : DefaultR2;
  val = EEPROM.read(EE_Rate3);
  guideRates[3] = val > 0 ? (float)val : DefaultR3;
  enableGuideRate(EEPROM.read(EE_DefaultRate), true);
  delay(10);

  // prep timers
  rtk.updateTimers();


  // Monitor - safety check and heartbeat
  xTaskCreate(
    monitorTask,    // Function that should be called
    "Monitor",      // Name of the task (for debugging)
    2000,           // Stack size (bytes)
    NULL,           // Parameter to pass
    MON_TASK_PRTY,  // Task priority
    NULL            // Task handle
  );
  // Main control task
  xTaskCreate(
    controlTask, 
    "Control",   
    2000,        
    NULL,     
    CTRL_TASK_PRTY,        
    NULL      
  );

  // Communication task
  xTaskCreate(
    processCommandsTask,
    "Command",   
    2000,
    NULL,
    CMD_TASK_PRTY, 
    NULL
  );
#ifdef __arm__  
  // the ARM version of FreeRTOS needs to start the scheduler explicitly
  vTaskStartScheduler();
#endif    
}

// not used in FreeRTOS
void loop()
{
}


