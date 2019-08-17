// -----------------------------------------------------------------------------------
// PROGRAM FOR POINTING A TELESCOPE BY T. TAKI
//
// CONVERSION TO C++/Arduino BY Howard Dutton, 10/12/2016

class instrumental_stepper
{
public:
  long m_axis1;
  long m_axis2;
  //instrumental_angle toInstrumentalAngle()
  //{
  //  return instrumental_angle(((double)m_axis1) / StepsPerDegreeAxis1, ((double)m_axis2) / StepsPerDegreeAxis2);
  //}
  void setAtMount()
  {
    cli();
    m_axis1 = posAxis1;
    m_axis2 = posAxis2;
    sei();
  }
  PierSide getPierSide()
  {
    return -quaterRotAxis2 <= m_axis2 && m_axis2 <= quaterRotAxis2 ? PIER_EAST : PIER_WEST;
  }

  //for GEM
  bool checkPole(CheckMode mode)
  {
    double underPoleLimit = (mode == CHECKMODE_GOTO) ? underPoleLimitGOTO : underPoleLimitGOTO + 5.0 / 60;
    return (m_axis1 > quaterRotAxis1 - underPoleLimit * 15. * StepsPerDegreeAxis1) && (m_axis1 < quaterRotAxis1 + underPoleLimit * 15. * StepsPerDegreeAxis1);
    return true;
  }
  bool checkMeridian(CheckMode mode)
  {
    bool ok = true;
    double MinutesPastMeridianW = (mode == CHECKMODE_GOTO) ? minutesPastMeridianGOTOW : minutesPastMeridianGOTOW + 5;
    double MinutesPastMeridianE = (mode == CHECKMODE_GOTO) ? minutesPastMeridianGOTOE : minutesPastMeridianGOTOE + 5;
    switch (getPierSide())
    {
    case PIER_WEST:
      if (m_axis1 > (12. + MinutesPastMeridianW / 60.)* 15.0 * StepsPerDegreeAxis1) ok = false;
      break;
    case PIER_EAST:
      if (m_axis1 < -MinutesPastMeridianE / 60. * 15.0 * StepsPerDegreeAxis1) ok = false;
      break;
    default:
      ok = false;
      break;
    }
    return ok;
  }
  //for Eq Fork and Alt
  boolean checkAxis2LimitEQ()
  {
    return m_axis2 > MinAxis2EQ * StepsPerDegreeAxis2 && m_axis2 < MaxAxis2EQ * StepsPerDegreeAxis2;
  }
    boolean checkAxis2LimitAZALT()
  {
    return m_axis2 > MinAxis2AZALT * StepsPerDegreeAxis2 && m_axis2 < MaxAxis2AZALT * StepsPerDegreeAxis2;
  }

};


