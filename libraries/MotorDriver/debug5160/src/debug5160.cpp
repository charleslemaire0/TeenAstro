/**
 * Author Teemu Mäntykallio
 * Initializes the library and runs the stepper
 * motor in alternating directions.
 */

#include "TMCStepper.h"
#include "MotionControl.h"
#include "StepDir.h"
#include "Mc5160.h"
#include "MotorDriver.h"

#define MAX_CMD_SIZE 50
#define MAX_ARG_SIZE 20

#ifdef __ESP32__
#define Axis1CSPin      5
#define Axis1DirPin     26
#define Axis1StepPin    25
#define Axis1EnablePin  27
#define ISR(f) void IRAM_ATTR f(void) 
#define PORT Serial2
#endif

#ifdef __arm__
#define Axis1CSPin      9
#define Axis1DirPin     2
#define Axis1StepPin    22
#define Axis1EnablePin  3
#define ISR(f)  void f(void)
#define PORT Serial
#endif


MotorDriver motorA1;
SemaphoreHandle_t hwMutex;       // to prevent concurrent hardware accesses 

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
}


void init(char *arg1, char *arg2)
{
 	pinMode(Axis1CSPin, OUTPUT);
 	pinMode(Axis1EnablePin, OUTPUT);
  SPI.begin();

  // Generic initialization (works for both types)
 	digitalWrite(Axis1EnablePin, 1);
  motorA1.init(Axis1CSPin, hwMutex);
 	digitalWrite(Axis1EnablePin, 0);

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
	    motorA1.initMc5160();  		
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
  {"get",    &get,           "Get parameter"},
  {"set",    &set,           "Set parameter"},
};

#define NUM_COMMANDS (sizeof(Commands) / sizeof (CMD_STRUCT))


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

  for (int i=0;i<NUM_COMMANDS;i++)
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

}

void loop() 
{
}