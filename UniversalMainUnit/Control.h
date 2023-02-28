#pragma once

#define CTL_MAX_MESSAGE_SIZE 10   // in words
#define CTL_QUEUE_SIZE  (10 * CTL_MAX_MESSAGE_SIZE * sizeof(unsigned)) 

// Messages to Control Task that require an async action
enum CTL_MESSAGES {CTL_MSG_GOTO, CTL_MSG_GOTO_HOME, CTL_MSG_START_TRACKING, CTL_MSG_STOP_TRACKING,  
           CTL_MSG_MOVE_AXIS1, CTL_MSG_MOVE_AXIS2, CTL_MSG_STOP_AXIS1, CTL_MSG_STOP_AXIS2, CTL_MSG_SET_SLEW_SPEED};

enum CTL_MODE {CTL_MODE_IDLE, CTL_MODE_GOTO, CTL_MODE_TRACKING, CTL_MODE_ERR};


// Global Events
#define EV_ABORT        (1<<0)
#define EV_AT_HOME      (1<<1)
#define EV_GOING_HOME   (1<<2)
#define EV_PARKED       (1<<3)
#define EV_SLEWING      (1<<4)
#define EV_TRACKING     (1<<5)
#define EV_GUIDING_E    (1<<6)
#define EV_GUIDING_W    (1<<7)
#define EV_GUIDING_N    (1<<8)
#define EV_GUIDING_S    (1<<9)
#define EV_ERROR        (1<<10)


bool isSlewing(void);     // replaces movingTo global variable
bool isTracking(void);    // replaces siderealTracking
void abortSlew(void);
void controlTask(void *arg);
void DecayModeGoto(void);
void DecayModeTracking(void);
void setEvents(unsigned ev);
void resetEvents(unsigned ev);
bool getEvent(unsigned ev);
void waitSlewing(void);
