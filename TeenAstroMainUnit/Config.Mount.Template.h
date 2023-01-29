#pragma once
//This this the mount template for fix mount definition

#define FirmwareSubName "Template"

// MOUNT_TYPE_GEM = 1, MOUNT_TYPE_FORK = 2, MOUNT_TYPE_ALTAZM = 3, MOUNT_TYPE_FORK_ALT = 4
#define D_mountType 2 

#define D_motorA1gear 180       // = physical ratio * 100
#define D_motorA1stepRot 180
#define D_motorA1micro 4        // microstep 0 to 8 : 1, 2, 4 ,8, 16, 32, 64, 128, 256
#define D_motorA1reverse 0
#define D_motorA1highCurr 2000
#define D_motorA1lowCurr 1000
#define D_motorA1silent 0

#define D_motorA2gear 180       // = physical ratio * 100
#define D_motorA2stepRot 180
#define D_motorA2micro 4        // microstep 0 to 8 : 1, 2, 4 ,8, 16, 32, 64, 128, 256
#define D_motorA2reverse 0
#define D_motorA2highCurr 2000
#define D_motorA2lowCurr 1000
#define D_motorA2silent 0

#define D_encoderA1plusePerDegree 400
#define D_encoderA1reverse 0

#define D_encoderA2plusePerDegree 400
#define D_encoderA2reverse 0

#define DefaultR4 960