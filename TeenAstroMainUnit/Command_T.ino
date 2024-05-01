
#define EnableTest

//----------------------------------------------------------------------------------
//   T - Tracking Commands
//  :T+#   Master sidereal clock faster by 0.1 Hertz (I use a fifth of the LX200 standard, stored in XEEPROM) Returns: Nothing
//  :T-#   Master sidereal clock slower by 0.1 Hertz (stored in XEEPROM) Returns: Nothing
//  :TS#   Track rate solar Returns: Nothing
//  :TL#   Track rate lunar Returns: Nothing
//  :TQ#   Track rate sidereal Returns: Nothing
//  :TT#   Track rate target Returns: Nothing
//  :TR#   Master sidereal clock reset (to calculated sidereal rate, stored in EEPROM) Returns: Nothing
//  :Te#   Tracking enable  (replies 0/1)
//  :Td#   Tracking disable (replies 0/1)

//  :T0#   Track compensation disable (replies 0/1)
//  :T1#   Track compensation only in RA (replies 0/1)
//  :T2#   Track compensation BOTH (replies 0/1)
void Command_T()
{

  switch (command[1])

  {
  case '+':
    siderealClockSpeed -= HzCf * (0.02);
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), siderealClockSpeed * 16);
    updateSideral();
    replyNothing();
    break;
  case '-':
    siderealClockSpeed += HzCf * (0.02);
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), siderealClockSpeed * 16);
    updateSideral();
    replyNothing();
    break;
  case 'S':
    // solar tracking rate 60Hz
    SetTrackingRate(TrackingSolar, 0);
    sideralMode = SIDM_SUN;
    replyNothing();
    break;
  case 'L':
    // lunar tracking rate 57.9Hz
    SetTrackingRate(TrackingLunar, 0);
    sideralMode = SIDM_MOON;
    replyNothing();
    break;
  case 'Q':
    // sidereal tracking rate
    SetTrackingRate(TrackingStar, 0);
    sideralMode = SIDM_STAR;
    replyNothing();
    break;
  case 'R':
    // reset master sidereal clock interval
    siderealClockSpeed = mastersiderealClockSpeed;
    SetTrackingRate(TrackingStar, 0);
    sideralMode = SIDM_STAR;
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), siderealClockSpeed * 16);
    updateSideral();
    replyNothing();
    break;
  case 'T':
    // set user defined Target tracking rate
    SetTrackingRate(1.0 - (double)storedTrakingRateRA / 10000.0, (double)storedTrakingRateDEC / 10000.0);
    sideralMode = SIDM_TARGET;
    replyNothing();
    break;
  case 'e':
    if (parkStatus == PRK_UNPARKED)
    {
      lastSetTrakingEnable = millis();
      atHome = false;
      if (enableMotor)
      {
        StartSideralTracking();
        replyShortTrue();
      }
      else
      {
        replyShortFalse();
      }
    }
    else
      replyShortFalse();
    break;
  case 'd':
    if (parkStatus == PRK_UNPARKED)
    {
      sideralTracking = false;
      replyShortTrue();
    }
    else
      replyShortFalse();
    break;
  case '1':
    // turn compensation RA only
    if (isAltAZ())
    {
      replyShortFalse();
    }
    else
    {
      trackComp = TC_RA;
      computeTrackingRate(true);
      XEEPROM.update(getMountAddress(EE_TC_Axis), 1);
      replyShortTrue();
    }
    break;
  case '2':
    // turn compensation BOTH
    if (isAltAZ())
    {
      replyShortFalse();
    }
    else
    {
      trackComp = TC_BOTH;
      computeTrackingRate(true);
      XEEPROM.update(getMountAddress(EE_TC_Axis), 2);
      replyShortTrue();
    }
    break;
#ifdef EnableTest
  case 'X':
    //Test Commands
  {
    switch (command[2])
    {
    case 'A':
    {  
      double k = 1.002737908;
      double t_0 = 5.497787;
      // alpha And
      double t_1 = 5.619669;
      double Ra_1 = 0.034470;
      double Dec_1 = 0.506809;
      double Ia1_1 = 1.732239;
      double Ia2_1 = 1.463808;
      // alpha Umi
      double t_2 = 5.659376;
      double Ra_2 = 0.618501;
      double Dec_2 = 1.557218;
      double Ia1_2 = 5.427625;
      double Ia2_2 = 0.611563;
      // beta Cet
      double t_3 = 5.725553;
      double Ra_3 = 0.188132;
      double Dec_3 = -0.314822;
      double Ia1_3 = 2.272546;
      double Ia2_3 = 0.656449;
      switch (command[3])
      {

      case '0':
      {
        CoordConv test_alignment;
        test_alignment.addReference(k * (t_1 - t_0) - Ra_1, Dec_1, -Ia1_1, Ia2_1);
        test_alignment.addReference(k * (t_2 - t_0) - Ra_2, Dec_2, -Ia1_2, Ia2_2);
        break;
      }

      case '1':
        //TXA1 Test the 2 stars alignment
      {

        CoordConv test_alignment;
        test_alignment.addReference(-(k * (t_1 - t_0) - Ra_1), Dec_1, Ia1_1, Ia2_1);
        test_alignment.addReference(-(k * (t_2 - t_0) - Ra_2), Dec_2, Ia1_2, Ia2_2);
        Coord_EQ EQ3(0, Dec_3, -(k * (t_3 - t_0) - Ra_3));
        Coord_LO LO3 = EQ3.To_Coord_LO(test_alignment.Tinv);
        Serial.println(LO3.Axis1() * 180 / PI);
        Serial.println(LO3.Axis2() * 180 / PI);
        Serial.println(LO3.Axis3() * 180 / PI);
        Coord_EQ EQ3_test = LO3.To_Coord_EQ(test_alignment.T);

        //Serial.println((k * (t_3 - t_0) - EQ3_test.Ha() - Ra_3) * 180 / PI * 60);
        //Serial.println((EQ3_test.Dec() - Dec_3) * 180 / PI * 60);
        
      }
      break;
      case '2':
      {
        double Lat = 35*PI/180;
        LA3::RefrOpt refraction = { false,10,101 };
        Coord_HO HO1 = Coord_EQ(0., Dec_1, (k * (t_1 - t_0) - Ra_1)).To_Coord_HO(Lat, refraction);
        Coord_HO HO2 = Coord_EQ(0., Dec_2, (k * (t_2 - t_0) - Ra_2)).To_Coord_HO(Lat, refraction);
        Coord_HO HO3 = Coord_EQ(0., Dec_3, (k * (t_3 - t_0) - Ra_3)).To_Coord_HO(Lat, refraction);
        CoordConv test_alignment;
        
        test_alignment.addReference(HO1.direct_Az_S(), HO1.Alt(), Ia1_1, Ia2_1);
        test_alignment.addReference(HO2.direct_Az_S(), HO2.Alt(),  Ia1_2, Ia2_2);

        Serial.println("error");
        Serial.println(test_alignment.getError() * 180. / PI * 3600.);

        Coord_IN IN3 = HO3.To_Coord_IN(test_alignment.Tinv);
 
        Serial.println((IN3.Axis1_direct()) * 180. / PI);
        Serial.println((IN3.Axis2()) * 180. / PI);
        Serial.println((IN3.Axis3()) * 180. / PI);
        //Coord_EQ EQ3_test = IN3.To_Coord_EQ(test_alignment.T, refraction, Lat);
        //Serial.println((k * (t_3 - t_0) - EQ3_test.Ha() - Ra_3) * 180 / PI * 60);
        //Serial.println((EQ3_test.Dec() - Dec_3) * 180 / PI * 60);
      }
      break;
      case '3':
      {
        double offset1 = random(-10, 10) * PI / 180;
        double offset2 = random(-10, 10) * PI / 180;
        Ia1_1 += offset1;


        double Lat = random(0, 90) * PI / 180;
        LA3::RefrOpt refraction = { false,10,101 };
        Coord_HO HO1 = Coord_EQ(0., Dec_1, k * (t_1 - t_0) - Ra_1).To_Coord_HO(Lat, refraction);
        Coord_HO HO2 = Coord_EQ(0., Dec_2, k * (t_2 - t_0) - Ra_2).To_Coord_HO(Lat, refraction);
        Coord_HO HO3 = Coord_EQ(0., Dec_3, k * (t_3 - t_0) - Ra_3).To_Coord_HO(Lat, refraction);
        CoordConv test_alignment;

        test_alignment.addReference(HO1.direct_Az_S(), HO1.Alt(), Ia1_1, Ia2_1);
        test_alignment.addReference(HO2.direct_Az_S(), HO2.Alt(), Ia1_2, Ia2_2);

        Serial.println("error");
        Serial.println(test_alignment.getError() * 180. / PI * 60.);

        Coord_IN IN3 = HO3.To_Coord_IN(test_alignment.Tinv);

        Serial.println((IN3.Axis1()) * 180. / PI);
        Serial.println((IN3.Axis2()) * 180. / PI);
        Serial.println((IN3.Axis3()) * 180. / PI);
        //Coord_EQ EQ3_test = IN3.To_Coord_EQ(test_alignment.T, refraction, Lat);
        //Serial.println((k * (t_3 - t_0) - EQ3_test.Ha() - Ra_3) * 180 / PI * 60);
        //Serial.println((EQ3_test.Dec() - Dec_3) * 180 / PI * 60);
      }
      break;
      default:
        replyNothing();
      }
    }
      break;
    case 'T':
    {
      switch (command[3])
      {
      case '1':
        //TXT1 Test EQ to HO and HO to EQ
      {
        double Ha = 54.382617 / 180 * PI;
        double Dec = 36.466667 / 180 * PI;
        double Lat = 52.5 / 180 * PI;
        double Alt = 49.169122 / 180 * PI;
        double Az = 269.14634 / 180 * PI - 2*PI;
        LA3::RefrOpt refraction = { false,10,101 };
        Coord_HO H1 = Coord_EQ(0, Dec, Ha).To_Coord_HO(Lat, refraction);
        Serial.println(H1.Az() - Az);
        Serial.println(H1.Alt() - Alt);
        Coord_EQ EQ1 = H1.To_Coord_EQ(Lat);
        Serial.println(EQ1.Ha() - Ha);
        Serial.println(EQ1.Dec() - Dec);
      }
      break;
      default:
        replyNothing();
      }

   
    }
    break;
    default:
      replyNothing();
      break;
    }
  }
    break;
#endif
  default:
    replyNothing();
    break;
  }


}
