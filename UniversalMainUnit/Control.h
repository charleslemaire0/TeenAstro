#pragma once

#define CTL_MAX_MESSAGE_SIZE 10   // in words
#define CTL_QUEUE_SIZE  (10 * CTL_MAX_MESSAGE_SIZE * sizeof(unsigned)) 

// Messages to Control Task that require an async action
enum CTL_MESSAGES {CTL_MSG_GOTO, CTL_MSG_GOTO_HOME, CTL_MSG_START_TRACKING, CTL_MSG_STOP_TRACKING,  
                    CTL_MSG_MOVE_AXIS1, CTL_MSG_MOVE_AXIS2, CTL_MSG_STOP_AXIS1, CTL_MSG_STOP_AXIS2, 
                    CTL_MSG_SET_SLEW_SPEED
         };

enum CTL_MODE {CTL_MODE_IDLE, CTL_MODE_GOTO, CTL_MODE_TRACKING, CTL_MODE_STOPPING};


// Global Events
#define EV_ABORT            (1<<2)
#define EV_AT_HOME          (1<<3)
#define EV_GOING_HOME       (1<<4)
#define EV_SLEWING          (1<<5)
#define EV_TRACKING         (1<<6)
#define EV_EAST             (1<<7)
#define EV_WEST             (1<<8)
#define EV_NORTH            (1<<9)
#define EV_SOUTH            (1<<10)
#define EV_GUIDING_AXIS1    (1<<11)
#define EV_GUIDING_AXIS2    (1<<12)
#define EV_CENTERING        (1<<13)
#define EV_START_TRACKING   (1<<14)
#define EV_SPEED_CHANGE     (1<<15)
#define EV_SPIRAL           (1<<16)


bool isSlewing(void);     // replaces movingTo global variable
bool isTracking(void);    // replaces siderealTracking
void stopMoving(void);
void controlTask(void *arg);
void DecayModeGoto(void);
void DecayModeTracking(void);
void setEvents(unsigned ev);
void resetEvents(unsigned ev);
bool getEvent(unsigned ev);
void waitSlewing(void);
void startTracking(void);
void stopTracking(void);
void MoveAxis1(const byte dir);
void MoveAxis1AtRate(double speed, const byte dir);
void StopAxis1(void);
void MoveAxis2(const byte dir);
void MoveAxis2AtRate(double speed, const byte dir);
void StopAxis2(void);
byte goTo(Steps *sP);
void adjustSpeeds(void);
void setSlewSpeed(double speed);
