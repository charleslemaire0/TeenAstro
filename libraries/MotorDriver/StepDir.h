#pragma once

#define SD_MAX_MESSAGE_SIZE 10		// in words
#define SD_QUEUE_SIZE  (10 *	SD_MAX_MESSAGE_SIZE * sizeof(unsigned)) 

// Messages to Motor Task that require an async action
enum MT_MESSAGES {MSG_SET_CUR_POS, MSG_SET_TARGET_POS, MSG_SET_AMAX, MSG_SET_VMAX, MSG_SYNC_POS};

// Internal mode
enum PositionState {PS_IDLE, PS_ACCEL, PS_CRUISE, PS_DECEL, PS_DECEL_TARGET, PS_STOPPING};

// Events describing motor state
#define EV_MOT_GOTO    		(1 << 0)
#define EV_MOT_ABORT			(1 << 1)

#ifdef DBG_STEPDIR 
#define LOG_SIZE 4000
typedef struct
{
	unsigned t;
  uint8_t state;
	unsigned delta;
	double speed;
} LOG_ENTRY;
#endif


class StepDir : public MotionControl
{
public:
	void setCurrentPos(long);
  void syncPos(long);
	long getCurrentPos(void);
	long getTargetPos(void);
	double getSpeed(void);
	void setAmax(long);	
	void setVmax(double);	
	void setTargetPos(long targetPos);
	bool positionReached(void);
	bool isMoving(void);
	void setDir(bool);
	void setEvents(unsigned);
	void resetEvents(unsigned);
	void abort(void);
	void resetAbort(void);
	void initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId);
	void initMc5160(TMC5160Stepper *driverP, SemaphoreHandle_t, long);
  void setRatios(long fkHz);
  long decelDistance(double speed, unsigned long aMax);
  void enable(void);
  void disable(void);


	volatile int dirPin, stepPin;
	double currentSpeed;
	long currentPos, currentPosImage;	
  long d1;

	void programSpeed(double speed);
	void positionMode(void);
	long getDelta(void);
#ifdef DBG_STEPDIR 
	void logMotion(uint8_t, long, double);
#endif
	unsigned getEvents(void);

	QueueHandle_t motQueue;
	EventGroupHandle_t motEvents;

	double aMax, dMax, vMax, vStop, vSlow;
	double newSpeed, speedAdjustment;
  long targetPos, delta;
  unsigned long stopDistance;
	bool edgePos = false;

  void state(PositionState newState);
  PositionState state(void);
	PositionState posState;

#ifdef DBG_STEPDIR 
	LOG_ENTRY logTable[LOG_SIZE];
	LOG_ENTRY *lP = logTable;
#endif



private:
	// Internal variables
	bool initialized = false;
	TaskHandle_t motTaskHandle;

	// Processor-specific variables
#ifdef __arm__
	void *timerP;
#endif

#ifdef __ESP32__
	hw_timer_t *timerP = NULL;
#endif	

};
void StepDirInterruptHandler(StepDir *sdP);
