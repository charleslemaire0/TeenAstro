#pragma once
// EEPROM Info --------------------------------------------------------------------------------------------------------------
// 0-1023 bytes
// general purpose storage 0..99

#define EE_mountType        0
#define EE_posAxis1         1
#define EE_posAxis2         5

#define EE_parkStatus       10
#define EE_parkSaved        11

#define EE_pulseGuideRate   22
#define EE_maxRate          23
#define EE_degAcc           25

#define EE_minAlt           26
#define EE_maxAlt           27

#define EE_dpmE             28
#define EE_dpmW             29
#define EE_dup              30

#define EE_doCor            42
#define EE_pdCor            46
#define EE_altCor           50
#define EE_azmCor           54

#define EE_GearAxis1        58
#define EE_StepRotAxis1     62
#define EE_MicroAxis1       64
#define EE_ReverseAxis1     65
#define EE_LowCurrAxis1     66
#define EE_HighCurrAxis1    67
#define EE_backlashAxis1    68

#define EE_GearAxis2        72
#define EE_StepRotAxis2     76
#define EE_MicroAxis2       78
#define EE_ReverseAxis2     79
#define EE_LowCurrAxis2     80
#define EE_HighCurrAxis2    81
#define EE_backlashAxis2    82

#define EE_siderealInterval 88

#define EE_autoInitKey      96
