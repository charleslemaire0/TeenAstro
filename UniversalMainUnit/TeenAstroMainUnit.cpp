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

void initMotors(bool deleteAlignment)
{
  readEEPROMmotor();

  pinMode(Axis1CSPin, OUTPUT);
 	pinMode(Axis1StepPin, OUTPUT);
 	pinMode(Axis1DirPin, OUTPUT);

  pinMode(Axis2CSPin, OUTPUT);
 	pinMode(Axis2StepPin, OUTPUT);
 	pinMode(Axis2DirPin, OUTPUT);

  SPI.begin();

  // Generic initialization (works for both types)
  motorA1.init(Axis1CSPin, Axis1EnablePin, hwMutex);
  motorA2.init(Axis2CSPin, Axis2EnablePin, hwMutex);

  // find out if we have StepDir or SPI-only and initialize either type (see SD_MODE in TMC5160 data sheet)
  ulong version1 = motorA1.drvP->version();
  if (version1 == 0) // no TMC5160 found - init a stepdir for debug
  {
    strcpy(Axis1DriverName,"NONE");
    motorA1.initStepDir(Axis1DirPin, Axis1StepPin, isrStepDir1, 2);
  }
  else
  {
    if (motorA1.drvP->sd_mode())
    {
      strcpy(Axis1DriverName,"SD");
      motorA1.initStepDir(Axis1DirPin, Axis1StepPin, isrStepDir1, 2);
    }
    else
    {
      strcpy(Axis1DriverName,"SPI");
      motorA1.initMc5160(hwMutex, mount.clk5160);
    }
  }
  
  ulong version2 = motorA2.drvP->version();
  if (version2 == 0) // no TMC5160 found - init a stepdir for debug
  {
    strcpy(Axis2DriverName,"NONE");
    motorA2.initStepDir(Axis2DirPin, Axis2StepPin, isrStepDir2, 3);
  }
  else
  {
    if (motorA2.drvP->sd_mode())
    {
      strcpy(Axis2DriverName,"SD");
      motorA2.initStepDir(Axis2DirPin, Axis2StepPin, isrStepDir2, 3);
    }
    else
    {
      strcpy(Axis2DriverName,"SPI");
      motorA2.initMc5160(hwMutex, mount.clk5160);
    }
  }
  // cannot call updateRatios before motors are initialized!
  updateRatios(deleteAlignment,false);
}



void updateRatios(bool deleteAlignment, bool deleteHP)
{
  cli();
  geoA1.setstepsPerRot((double)motorA1.gear / 1000.0 * motorA1.stepRot * (int)pow(2, motorA1.micro));
  geoA2.setstepsPerRot((double)motorA2.gear / 1000.0 * motorA2.stepRot * (int)pow(2, motorA2.micro));
  mount.backlashA1.inSteps = (int)round(((double)mount.backlashA1.inSeconds * 3600.0) / (double)geoA1.stepsPerDegree);
  mount.backlashA2.inSteps = (int)round(((double)mount.backlashA2.inSeconds * 3600.0) / (double)geoA2.stepsPerDegree);
  sei();

  mount.mP->initTransformation(deleteAlignment);
  if (deleteHP)
  {
    unsetPark();
    unsetHome();
  }
  initHome();
  motorA1.setCurrentPos(geoA1.homeDef);
  motorA2.setCurrentPos(geoA2.homeDef);

  initLimits();
//  updateSidereal();
  initMaxSpeed();
}

void updateSidereal()
{
#if 0 
  // 16MHZ clocks for steps per second of sidereal tracking
  cli();
  SiderealRate = siderealClockSpeed / geoA1.stepsPerSecond;
  TakeupRate = SiderealRate / 8L;
  sei();
  staA1.timerRate = SiderealRate;
  staA2.timerRate = SiderealRate;
  SetTrackingRate(default_tracking_rate,0);

  // backlash takeup rates
  backlashA1.timerRate = staA1.timerRate / BacklashTakeupRate;
  backlashA2.timerRate = staA2.timerRate / BacklashTakeupRate;

  // initialize the timers that handle the sidereal clock, RA, and Dec
  setsiderealClockSpeed(siderealClockSpeed);
#endif
}



void setup()
{
  pinMode(LEDPin, OUTPUT);
  for (int k = 0; k < 20; k++)
  {
    digitalWrite(LEDPin, HIGH);
    delay(10);
    digitalWrite(LEDPin, LOW);
    delay(50);
  }
  
  HAL_preInit();
  HAL_EEPROM_begin();
  EEPROM_AutoInit();
  reboot_unit = false;

  // FreeRTOS structures
  // Mutexes to prevent concurrent accesses 
  // A task must take the mutex before hw access and release it afterwards
  hwMutex = xSemaphoreCreateMutex();  // hardware accesses (ie SPI etc.)
  swMutex = xSemaphoreCreateMutex();  // global variables
  // queue for receiving messages from other tasks
  controlQueue = xQueueCreate( CTL_QUEUE_SIZE, CTL_MAX_MESSAGE_SIZE * sizeof(unsigned));
  // Event group for all mount events
  mountEvents = xEventGroupCreate();

  // get the site information from EEPROM
  currentSite = XEEPROM.read(EE_currentSite);
  localSite.ReadCurrentSiteDefinition(currentSite);
  doesRefraction.readFromEEPROM();
  highPrecision = true;
  initMount();
  initMotors(false);

  #ifdef HASST4
  setupST4();
  #endif
  initGuiding();

  // init time and date
  setSyncProvider(rtk.getTime);
  setSyncInterval(1);
  setTime(rtk.getTime());


  // automatic mode switching before/after slews, initialize micro-step mode
  DecayModeGoto();

  siderealClockSpeed = XEEPROM.readLong(getMountAddress(EE_siderealClockSpeed));
  updateSidereal();
  HAL_beginTimer(siderealTimer, siderealTickCount);

  mount.mP->updateRaDec();
  mount.mP->setTrackingSpeed(TrackingStar);

  rtk.resetLongitude(*localSite.longitude());

  // get the Park status
  if (!iniAtPark())
  {
      syncAtHome();
  }

  // get the pulse-guide rate
  int val = XEEPROM.read(getMountAddress(EE_Rate0));
  guideRates[0] = val > 0 ? (float)val / 100 : DefaultR0;
  val = XEEPROM.read(getMountAddress(EE_Rate1));
  guideRates[1] = val > 0 ? (float)val : DefaultR1;
  val = XEEPROM.read(getMountAddress(EE_Rate2));
  guideRates[2] = val > 0 ? (float)val : DefaultR2;
  val = XEEPROM.read(getMountAddress(EE_Rate3));
  guideRates[3] = val > 0 ? (float)val : DefaultR3;
  enableGuideRate(XEEPROM.read(getMountAddress(EE_DefaultRate)));
  delay(10);

  // prep timers
  rtk.updateTimers();


  HAL_initSerial();


  // Monitor - safety check and heartbeat
  xTaskCreate(
    monitorTask,    // Function that should be called
    "Monitor",      // Name of the task (for debugging)
    4096,           // Stack size (bytes)
    NULL,           // Parameter to pass
    MON_TASK_PRTY,  // Task priority
    NULL            // Task handle
  );
  // Main control task
  xTaskCreate(
    controlTask, 
    "Control",   
    4096,        
    NULL,     
    CTRL_TASK_PRTY,        
    NULL      
  );

  // Communication task
  xTaskCreate(
    processCommandsTask,
    "Command",   
    4096,
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


