#pragma once

#define SD_MAX_MESSAGE_SIZE 10		// in words
#define SD_QUEUE_SIZE  (10 *	SD_MAX_MESSAGE_SIZE * sizeof(unsigned)) 

// Messages to Motor Task that require an async action
enum MT_MESSAGES {MSG_SET_CUR_POS, MSG_SET_TARGET_POS, MSG_SET_AMAX, MSG_SET_VMAX, MSG_ADJUST_SPEED};

// Internal mode
enum PositionState {PS_IDLE, PS_ACCEL, PS_CRUISE, PS_DECEL};

// Events describing motor state
#define EV_MOT_GOTO    			(1 << 0)
#define EV_MOT_ABORT			(1 << 3)

#ifdef DBG_STEPDIR 
#define LOG_SIZE 4000
typedef struct
{
	unsigned t;
	unsigned pos;
	unsigned speed;
} LOG_ENTRY;
#endif


class StepDir : public MotionControl
{
public:
	void setCurrentPos(long);
	long getCurrentPos(void);
	long getTargetPos(void);
	double getSpeed(void);
	void setAmax(long);	
	void setVmax(double);	
	double getVmax(void);
	void adjustSpeed(double percent);
	void setTargetPos(long targetPos);
	bool positionReached(void);
	bool isSlewing(void);
	void setDir(bool);
	void setEvents(unsigned);
	void resetEvents(unsigned);
	void abort(void);
	void resetAbort(void);
	void initStepDir(int DirPin, int StepPin, void (*isrP)(), unsigned timerId);
	void initMc5160(TMC5160Stepper *driverP);

	volatile int dirPin, stepPin;
	double currentSpeed;
	long currentPos, currentPosImage;	

	void programSpeed(double speed);
	void positionMode(void);
	long getDelta(void);
#ifdef DBG_STEPDIR 
	void logMotion(long , long);
#endif
	unsigned getEvents(void);

	QueueHandle_t motQueue;
	EventGroupHandle_t motEvents;

	double aMax, vMax, vStop, vSlow;
	double speedAdjustment;
	long targetPos;
	long stopDistance;

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
	IntervalTimer *timerP;
#endif

#ifdef __ESP32__
	hw_timer_t *timerP = NULL;
#endif	

};
void StepDirInterruptHandler(StepDir *sdP);
