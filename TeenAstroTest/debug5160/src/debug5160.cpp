/**
 * Author François Desvallées
 * 
 * Used code from Teemu Mäntykallio
 * Initializes the library and runs motor commands
 */
#ifdef __ESP32__
#define Axis1CSPin      5
#define Axis1DirPin     26
#define Axis1StepPin    25
#define Axis1EnablePin  0

#define DebugPin0       32
#define DebugPin1       33

// For debugging axis2
//#define Axis1CSPin      22
//#define Axis1DirPin     32
//#define Axis1StepPin    21
//#define Axis1EnablePin  17

#define ISR(f) void IRAM_ATTR f(void) 
#define PORT Serial2
#endif

#ifdef __arm__
#define Axis1CSPin      9
#define Axis1DirPin     2
#define Axis1StepPin    22
#define Axis1EnablePin  3

#define Axis2CSPin      10
#define Axis2DirPin     4
#define Axis2StepPin    20 
#define Axis2EnablePin  5

#define ISR(f)  void f(void)
#define PORT Serial
#include "arduino_freertos.h"
#include "avr/pgmspace.h"
#include "queue.h"
#include "event_groups.h"
#include "semphr.h"
#include "timers.h"
#endif

#include "TMCStepper.h"
#include "MotionControl.h"
#include "StepDir.h"
#include "Mc5160.h"
#include "MotorDriver.h"
#include <SoftwareSerial.h>

#define MAX_CMD_SIZE 50
#define MAX_ARG_SIZE 20

MotorDriver motorA1;
SemaphoreHandle_t hwMutex;       // to prevent concurrent hardware accesses 
#ifdef __ESP32__
EspSoftwareSerial::UART debugOut;
#endif

#ifdef DBG_STEPDIR
void dumpLog(char *arg1, char *arg2)
{
  char buf[50];
  LOG_ENTRY *lP = ((StepDir *) motorA1.mcP)->logTable;
  for (int i=0;i<3000;i++)
  {
    sprintf (buf, "%06d, %02d, %06d, %06.02f\n", lP->t, lP->state, lP->delta, lP->speed);
    PORT.printf(buf);
    lP++;
  }
}
#endif



ISR (isrStepDir1)
{
  StepDirInterruptHandler((StepDir *) motorA1.mcP);
}

void get(char *arg1, char *arg2)
{
  int val;
  if (!strcmp(arg1, "pos"))
  {
    int pos = motorA1.mcP->getCurrentPos();
    PORT.printf("Position: %d\n", pos);
  }
  if (!strcmp(arg1, "speed"))
  {
    int speed = motorA1.mcP->getSpeed();
    PORT.printf("Speed: %d\n", speed);
  }
  if (!strcmp(arg1, "target"))
  {
    int target = motorA1.mcP->getTargetPos();
    PORT.printf("Target: %d\n", target);
  }
  if (!strcmp(arg1, "ioin"))
  {
    int val = motorA1.drvP->IOIN();
    PORT.printf("ioin: %08X\n", val);
  }
  if (!strcmp(arg1, "gconf"))
  {
    int val = motorA1.drvP->GCONF();
    PORT.printf("gconf: %08X\n", val);
  }
  if (!strcmp(arg1, "gstat"))
  {
    int val = motorA1.drvP->GSTAT();
    PORT.printf("gstat: %08X\n", val);
  }
  if (!strcmp(arg1, "current"))
  {
    int val = motorA1.drvP->rms_current();
    PORT.printf("current: %d\n", val);
  }
  if (!strcmp(arg1, "sw_mode"))
  {
    int val = motorA1.drvP->SW_MODE();
    PORT.printf("sw_mode: %08x\n", val);
  }
  if (!strcmp(arg1, "ramp_stat"))
  {
    int val = motorA1.drvP->RAMP_STAT();
    PORT.printf("ramp_stat: %08x\n", val);
  }
  if (!strcmp(arg1, "chopconf"))
  {
    int val = motorA1.drvP->CHOPCONF();
    PORT.printf("chopconf: %08x\n", val);
  }
  if (!strcmp(arg1, "drv_status"))
  {
    int val = motorA1.drvP->DRV_STATUS();
    PORT.printf("drv_status: %08x\n", val);
  }
  if (!strcmp(arg1, "pwm_auto"))
  {
    int val = motorA1.drvP->PWM_AUTO();
    PORT.printf("pwm_auto: %08x\n", val);
  }
}

void set(char *arg1, char *arg2)
{
  int val;
  if (!strcmp(arg1, "pos"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
      motorA1.mcP->setCurrentPos(val);
      PORT.printf("set pos to %d\n", val);
    }
  }
  if (!strcmp(arg1, "target"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
      motorA1.mcP->setTargetPos(val);
      PORT.printf("set target to %d\n", val);
    }
  }
  if (!strcmp(arg1, "vmax"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
      motorA1.mcP->setVmax(val);
      PORT.printf("set vmax to %d\n", val);
    }
  }
  // only for MotionControl
  if (!strcmp(arg1, "v1"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
      motorA1.drvP->v1(val);
      PORT.printf("set v1 to %d\n", val);
    }
  }
  // only for MotionControl
  if (!strcmp(arg1, "a1"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
      motorA1.drvP->d1(val);
      motorA1.drvP->a1(val);
      PORT.printf("set a1/d1 to %d\n", val);
    }
  }
  if (!strcmp(arg1, "amax"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
      motorA1.mcP->setAmax(val);
      PORT.printf("set amax to %d\n", val);
    }
  }
  if (!strcmp(arg1, "enable"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
      digitalWrite(Axis1EnablePin, (val!=0));
      PORT.printf("set EN_PIN to %d\n", (val!=0));
    }
  }
  if (!strcmp(arg1, "current"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
    	motorA1.drvP->rms_current(val);
      PORT.printf("set current to %d\n", val);
    }
  }
  if (!strcmp(arg1, "micro"))
  {
    if (sscanf( arg2, "%d", &val ) == 1)
    {
    	motorA1.drvP->microsteps(val);
      PORT.printf("set microsteps to %d\n", val);
    }
  }
  if (!strcmp(arg1, "gstat"))
  {
    if (sscanf( arg2, "%x", &val ) == 1)
    {
    	motorA1.drvP->GSTAT(val);
      PORT.printf("set gstat to %x\n", val);
    }
  }
  if (!strcmp(arg1, "gconf"))
  {
    if (sscanf( arg2, "%x", &val ) == 1)
    {
    	motorA1.drvP->GCONF(val);
      PORT.printf("set gconf to %x\n", val);
    }
  }
}

void custom(char *arg1, char *arg2)
{
  int current = 800;
  int micro = 16;
  long amax=24000;
  long vmax=37000;
  motorA1.drvP->rms_current(current);
  motorA1.drvP->microsteps(micro);
  motorA1.mcP->setAmax(amax);
  motorA1.mcP->setVmax(vmax);
  PORT.printf("Initialized custom parameters: current=%d micro=%d amax=%ld vmax=%ld\n", current, micro, amax, vmax);
}

void reset(char *arg1, char *arg2)
{
  motorA1.drvP->reset();
  PORT.printf("Driver reset\n");
}


void stop(char *arg1, char *arg2)
{
  motorA1.abort();
  PORT.printf("Stopping Motor\n");
}

void init(char *arg1, char *arg2)
{
 	pinMode(Axis1CSPin, OUTPUT);
  pinMode(Axis1StepPin, OUTPUT);
  pinMode(Axis1DirPin, OUTPUT);
#ifdef __ESP32__  
  pinMode(DebugPin0, OUTPUT);
  debugOut.begin(57600, SWSERIAL_8N1, DebugPin1, DebugPin0, false);
#endif
//  pinMode(Axis2CSPin, OUTPUT);
//  pinMode(Axis2EnablePin, OUTPUT);
//  pinMode(Axis2StepPin, OUTPUT);
//  pinMode(Axis2DirPin, OUTPUT);

  digitalWrite(Axis1EnablePin, LOW); 

  SPI.begin();
#ifdef __arm__
//  pinMode(SPI_MOSI, OUTPUT);
//  pinMode(MISO, INPUT_PULLUP);
#endif  


  hwMutex = xSemaphoreCreateMutex();  // hardware accesses (ie SPI etc.)

  // Generic initialization (works for both types)
  motorA1.init(Axis1CSPin, Axis1EnablePin, hwMutex);
//  motorA1.drvP->setSPISpeed(4000000);

  unsigned version = motorA1.drvP->version();
  PORT.printf("TMC version %03x\n", version);

  if (version == 0)	// no chip - initialize a step/dir for debug
  {
    motorA1.initStepDir(Axis1DirPin, Axis1StepPin, isrStepDir1, 2);
  	PORT.printf("Initialized StepDir\n");
  }
  else
  {
  	bool sd_mode = motorA1.drvP->sd_mode();
  	if (sd_mode == true)
  	{
	    motorA1.initStepDir(Axis1DirPin, Axis1StepPin, isrStepDir1, 2);
  		PORT.printf("Initialized StepDir\n");
  	}
	  else
	  {
	    motorA1.initMc5160(hwMutex, 16000);  		
  		PORT.printf("Initialized Mc5160\n");
	  }
  }
}


typedef void (*Handler)(char *, char *);

typedef struct
{
    const char *verb;
    Handler handler;
    const char *description;
} CMD_STRUCT;

CMD_STRUCT Commands[] = 
{
  {"init",   &init,          "Initialize"},
  {"reset",  &reset,         "Reset"},
  {"custom", &custom,        "Custom"},
  {"get",    &get,           "Get parameter"},
  {"set",    &set,           "Set parameter"},
  {"stop",   &stop,          "Stop motor"},
#ifdef DBG_STEPDIR
  {"dump",   &dumpLog,       "Dump Log"},
#endif  
};

#define NUM_COMMANDS (sizeof(Commands) / sizeof (CMD_STRUCT))


#ifdef __ESP32__
void HAL_debug(uint8_t b)
{
  debugOut.write(b);
}
#endif

// if string is a known command with up to 2 arguments, return true
bool parseCmd(const char *str, CMD_STRUCT **cmdP, char *arg1, char *arg2)
{
  char cmd[MAX_CMD_SIZE]; 
  char *cP1, *cP2;
  *cmdP = nullptr;
  strcpy(arg1, "");
  strcpy(arg2, "");

  // is there a space in the string?
  cP1 = strchr(str, ' ');
  if (cP1 != nullptr)
  {
    *cP1++ = 0;             // terminate string
    strcpy(cmd, str); // copy first word 
    cP2 = strchr(cP1, ' ');  // another space?
    if (cP2 != nullptr)
    {
      *cP2++ = 0;
      strcpy(arg1, cP1);   // copy first argument 
      strcpy(arg2, cP2);    // copy second argument 
    }
    else
      strcpy(arg1, cP1);    // copy first and only argument 
  }
  else
  {
    strcpy(cmd, str);       // copy whole string
  }

  for (unsigned i=0;i<NUM_COMMANDS;i++)
  {
    if (!strcmp(cmd, Commands[i].verb))
    {
      *cmdP = &Commands[i];
      return true;
    }
  }
  // CR entered, command not recognized
  return false;
}

// add character to the string, return true after CR
bool parseStr(char c, char *str)
{
  if (c == (char) 10) // ignore LF
    return false;
  if (c == (char) 13)
  {
    PORT.println("");
    return true;
  }
  else
  {
    if (strlen(str)<MAX_CMD_SIZE)
    {
      PORT.print(c);
      strncat (str, &c, 1);
    }
    return false;
  }
}


void processCommands()
{
  static char reply[MAX_CMD_SIZE];
  static char command[MAX_CMD_SIZE];
  CMD_STRUCT *cmdP;
  char arg1[MAX_ARG_SIZE];
  char arg2[MAX_ARG_SIZE];

  if (PORT.available())
  {
    char c = PORT.read();
    if (parseStr(c, command))
    {
      if (parseCmd(command, &cmdP, arg1, arg2))
      {
        cmdP->handler(arg1, arg2);
      }
      strcpy(command, "");
    }
  }
}

void mainLoopTask(void *arg)
{
  while(1)
  { 
    processCommands();
    vTaskDelay(100);
  }
}


void setup() 
{
  PORT.begin(57600);
  delay(1000);
  PORT.println("Debug Monitor");

  xTaskCreate(
    mainLoopTask,    // Function that should be called
    "Main Loop",    // Name of the task (for debugging)
    2000,            // Stack size (bytes)
    NULL,           // Parameter to pass
    1,               // Task priority
    NULL             // Task handle
  );
#ifdef __arm__  
  // the ARM version of FreeRTOS needs to start the scheduler explicitly
  vTaskStartScheduler();
#endif    

}

void loop() 
{
}