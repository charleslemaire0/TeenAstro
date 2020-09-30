#pragma once
// EEPROM Info --------------------------------------------------------------------------------------------------------------
// 0-1023 bytes
// general purpose storage 0..99

#define EE_mountType        0
#define EE_posAxis1         1
#define EE_posAxis2         5

#define EE_corr_track       9
#define EE_parkStatus       10
#define EE_parkSaved        11
#define EE_currentSite      12
#define EE_ApparentPole     13
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

// limits
#define EE_minAlt           33
#define EE_maxAlt           34
#define EE_dpmE             35
#define EE_dpmW             36
#define EE_dup              37

//motor1
#define EE_motorA1gear          58
#define EE_motorA1stepRot       62
#define EE_motorA1micro         64
#define EE_motorA1reverse       65
#define EE_motorA1lowCurr       66
#define EE_motorA1highCurr      67
#define EE_backlashAxis1        68
#define EE_motorA1silent        70

//motor2
#define EE_motorA2gear          72
#define EE_motorA2stepRot       76
#define EE_motorA2micro         78
#define EE_motorA2reverse       79
#define EE_motorA2lowCurr       80
#define EE_motorA2highCurr      81
#define EE_backlashAxis2        82
#define EE_motorA2silent        84

#define EE_siderealInterval     88
#define EE_autoInitKey          96

#define EE_sites            100
#define EE_site_lat         0
#define EE_site_long        4
#define EE_site_height      8
#define EE_site_time        10
#define EE_site_name        11

//transformation matrix
#define EE_T11              EE_sites +  27 * 4
#define EE_T12              EE_T11 + 4
#define EE_T13              EE_T11 + 8
#define EE_T21              EE_T11 + 12
#define EE_T22              EE_T11 + 16
#define EE_T23              EE_T11 + 20
#define EE_T31              EE_T11 + 24
#define EE_T32              EE_T11 + 28
#define EE_T33              EE_T11 + 32
#define EE_Tvalid           EE_T11 + 36