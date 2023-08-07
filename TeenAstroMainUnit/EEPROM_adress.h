#pragma once
// EEPROM Info --------------------------------------------------------------------------------------------------------------
// 0-1023 bytes
// general purpose storage 0..99

uint8_t             midx;

#define EE_autoInitKey      0
#define EE_currentMount     4

#define EE_sites            10
#define EE_site_lat         0
#define EE_site_long        4
#define EE_site_height      8
#define EE_site_time        10
#define EE_site_name        11


#define MountNameLen     15 // withnull character!
#define MountSize        200
#define maxNumMount      2


#define EE_Mounts           100
#define MountSize           200

#define EE_mountType        0
#define EE_posAxis1         1
#define EE_posAxis2         5

#define EE_TC_Axis          9
#define EE_parkStatus       10
#define EE_parkSaved        11
#define EE_currentSite      12
#define EE_refraction       13
// user defined RA DEC 
#define EE_RA               14 // in degree
#define EE_DEC              18 // in degree

// Rates
#define EE_Rate0            22
#define EE_Rate1            23
#define EE_Rate2            24
#define EE_Rate3            25
#define EE_maxRate          26
#define EE_DefaultRate      28
#define EE_degAcc           29
#define EE_TakeupRate       30


// limits Localsite
#define EE_minAlt           33
#define EE_maxAlt           34
// limits AXIS
#define EE_minAxis1         36
#define EE_maxAxis1         38
#define EE_minAxis2         40
#define EE_maxAxis2         42
// Drift Rates
#define EE_RA_Drift          44
#define EE_DEC_Drift         48
               
// limits GEM
#define EE_dpmE             52
#define EE_dpmW             53
#define EE_dup              54
#define EE_dpmDistanceFromPole  55

//user defined homeposition
#define EE_homeSaved        56
#define EE_homePosAxis1     57 // in degree
#define EE_homePosAxis2     61 // in degree

#define EE_siderealClockSpeed     88


//motor1
#define EE_motorA1gear            100
#define EE_motorA1stepRot         104
#define EE_motorA1micro           106
#define EE_motorA1reverse         107
#define EE_motorA1lowCurr         108
#define EE_motorA1highCurr        109
#define EE_motorA1backlashAmount  110
#define EE_motorA1backlashRate    113
#define EE_motorA1silent          112

//motor2
#define EE_motorA2gear            114
#define EE_motorA2stepRot         118
#define EE_motorA2micro           120
#define EE_motorA2reverse         121
#define EE_motorA2lowCurr         122
#define EE_motorA2highCurr        123
#define EE_motorA2backlashAmount  124
#define EE_motorA2backlashRate    127
#define EE_motorA2silent          126

//encoder1
#define EE_encoderA1pulsePerDegree 130
#define EE_encoderA1reverse        134
//encoder2
#define EE_encoderA2pulsePerDegree 135
#define EE_encoderA2reverse        139

#define EE_encoderSync            140

#define EE_mountName              141

//transformation matrix
#define EE_T11              160
#define EE_T12              EE_T11 + 4
#define EE_T13              EE_T11 + 8
#define EE_T21              EE_T11 + 12
#define EE_T22              EE_T11 + 16
#define EE_T23              EE_T11 + 20
#define EE_T31              EE_T11 + 24
#define EE_T32              EE_T11 + 28
#define EE_T33              EE_T11 + 32
#define EE_Tvalid           EE_T11 + 36

int getMountAddress(int adress)
{
  return (int)EE_Mounts + (int)MountSize * (int)midx + adress;
}

int getMountAddress(int adress, int idx)
{
  return (int)EE_Mounts + (int)MountSize * (int)idx + adress;
}