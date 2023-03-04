typedef struct
{
	int     minAlt;                       // the minimum altitude, in degrees, for goTo's (so we don't try to point too low)
	int     maxAlt;                       // the maximum altitude, in degrees, for goTo's (to keep the telescope tube away from the mount/tripod)
	long    minutesPastMeridianGOTOE;     // for goto's, how far past the meridian to allow before we do a flip (if on the East side of the pier)- one hour of RA is the default = 60.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
	long    minutesPastMeridianGOTOW;     // as above, if on the West side of the pier.  If left alone, the mount will stop tracking when it hits the this limit.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.
	double  underPoleLimitGOTO;           // maximum allowed hour angle (+/-) under the celestial pole. OnStep will flip the mount and move the Dec. >90 degrees (+/-) once past this limit.  Sometimes used for Fork mounts in Align mode.  Ignored on Alt/Azm mounts.	
} Limits;



bool checkAltitude(void);
void initLimit(void);
void initLimitMinAxis1(void);
void initLimitMaxAxis1(void);
void initLimitMinAxis2(void);
void initLimitMaxAxis2(void);
