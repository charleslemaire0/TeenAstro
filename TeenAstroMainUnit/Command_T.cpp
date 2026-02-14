/**
 * Tracking commands: T (sidereal rate, solar/lunar/target, enable/disable, compensation).
 */
#include "Command.h"  // CommandGlobals provides Coord_LO, alignment, etc.

#define EnableTest

// -----------------------------------------------------------------------------
//   T - Tracking  :T+# :T-# :TS# :TL# :TQ# :TT# :TR# :Te# :Td# :T0# :T1# :T2#
// -----------------------------------------------------------------------------
void Command_T() {
  switch (commandState.command[1]) {
  case '+':
    mount.tracking.siderealClockSpeed -= HzCf * (0.02);
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), mount.tracking.siderealClockSpeed * 16);
    mount.updateSideral();
    replyNothing();
    break;
  case '-':
    mount.tracking.siderealClockSpeed += HzCf * (0.02);
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), mount.tracking.siderealClockSpeed * 16);
    mount.updateSideral();
    replyNothing();
    break;
  case 'S':
    // solar tracking rate 60Hz
    mount.setTrackingRate(TrackingSolar, 0);
    mount.tracking.sideralMode = SIDM_SUN;
    replyNothing();
    break;
  case 'L':
    // lunar tracking rate 57.9Hz
    mount.setTrackingRate(TrackingLunar, 0);
    mount.tracking.sideralMode = SIDM_MOON;
    replyNothing();
    break;
  case 'Q':
    // sidereal tracking rate
    mount.setTrackingRate(TrackingStar, 0);
    mount.tracking.sideralMode = SIDM_STAR;
    replyNothing();
    break;
  case 'R':
    // reset master sidereal clock interval
    mount.tracking.siderealClockSpeed = mastersiderealClockSpeed;
    mount.setTrackingRate(TrackingStar, 0);
    mount.tracking.sideralMode = SIDM_STAR;
    XEEPROM.writeLong(getMountAddress(EE_siderealClockSpeed), mount.tracking.siderealClockSpeed * 16);
    mount.updateSideral();
    replyNothing();
    break;
  case 'T':
    // set user defined Target tracking rate
    mount.setTrackingRate(1.0 - (double)mount.tracking.storedTrakingRateRA / 10000.0, (double)mount.tracking.storedTrakingRateDEC / 10000.0);
    mount.tracking.sideralMode = SIDM_TARGET;
    replyNothing();
    break;
  case 'e':
    if (!mount.isParked())
    {
      mount.tracking.lastSetTrakingEnable = millis();
      mount.parkHome.atHome = false;
      if (mount.motorsEncoders.enableMotor)
      {
        mount.startSideralTracking();
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
    if (!mount.isParked())
    {
      mount.tracking.sideralTracking = false;
      replyShortTrue();
    }
    else
      replyShortFalse();
    break;
  case '1':
    // turn compensation RA only
    if (mount.isAltAZ())
    {
      replyShortFalse();
    }
    else
    {
      mount.tracking.trackComp = TC_RA;
      mount.computeTrackingRate(true);
      XEEPROM.update(getMountAddress(EE_TC_Axis), 1);
      replyShortTrue();
    }
    break;
  case '2':
    // turn compensation BOTH
    if (mount.isAltAZ())
    {
      replyShortFalse();
    }
    else
    {
      mount.tracking.trackComp = TC_BOTH;
      mount.computeTrackingRate(true);
      XEEPROM.update(getMountAddress(EE_TC_Axis), 2);
      replyShortTrue();
    }
    break;
#ifdef EnableTest
  case 'X':
    //Test Commands
  {
    switch (commandState.command[2])
    {
    case 'A':
    { 
      //Taki's observation for more explanation take a look to
      // ..\libraries\TeenAstroCoordConv\matrix_method_rev_e.pdf
      // pages 37 to 41
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
      switch (commandState.command[3])
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
        char txt[512];
        CoordConv test_alignment;
        Coord_EQ EQ1(0, Dec_1, (k * (t_1 - t_0) - Ra_1));
        Coord_EQ EQ2(0, Dec_2, (k * (t_2 - t_0) - Ra_2));
        Coord_LO LO1(0, Ia2_1, Ia1_1);
        Coord_LO LO2(0, Ia2_2, Ia1_2);
        sprintf(txt, "%s \t [%f, %f]", "point 1", EQ1.Ha() * 180. / PI, EQ1.Dec() * 180. / PI);
        Serial.println(txt);
        sprintf(txt, "%s \t [%f, %f]", "point 2", EQ2.Ha() * 180. / PI, EQ2.Dec() * 180. / PI);
        Serial.println(txt);
        test_alignment.addReference(EQ1.direct_Ha(), EQ1.Dec(), LO1.Axis1(), LO1.Axis2());
        test_alignment.addReference(EQ2.direct_Ha(), EQ2.Dec(), LO2.Axis1(), LO2.Axis2());
             
  
        Coord_EQ EQ3(0, Dec_3, (k * (t_3 - t_0) - Ra_3));
        Coord_LO LO3 = EQ3.To_Coord_LO(test_alignment.Tinv);
        Serial.println(LO3.Axis1() * 180 / PI);
        Serial.println(LO3.Axis2() * 180 / PI);
        Serial.println(LO3.Axis3() * 180 / PI);

        
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
        test_alignment.addReference(HO2.direct_Az_S(), HO2.Alt(), Ia1_2, Ia2_2);

        Serial.println("error");
        Serial.println(test_alignment.getError() * 180. / PI);

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
        double offset2 = random(-10, 10) * PI / 180;
        double offset1 = random(-10, 10) * PI / 180;
        Ia1_1 += offset1;
        Ia1_2 += offset1;
        Ia2_1 += offset2;
        Ia2_2 += offset2;

        double Lat = random(10,80) * PI / 180;

        LA3::RefrOpt refraction = { false,10,101 };
        Coord_HO HO1 = Coord_EQ(0., Dec_1, k * (t_1 - t_0) - Ra_1).To_Coord_HO(Lat, refraction);
        Coord_HO HO2 = Coord_EQ(0., Dec_2, k * (t_2 - t_0) - Ra_2).To_Coord_HO(Lat, refraction);
        Coord_HO HO3 = Coord_EQ(0., Dec_3, k * (t_3 - t_0) - Ra_3).To_Coord_HO(Lat, refraction);
        CoordConv test_alignment;
        Serial.print("Azimuth Pt3: ");
        Serial.println(HO3.Az());
        test_alignment.addReference(HO1.direct_Az_S(), HO1.Alt(), Ia1_1, Ia2_1);
        test_alignment.addReference(HO2.direct_Az_S(), HO2.Alt(), Ia1_2, Ia2_2);
        test_alignment.minimizeAxis2();
        test_alignment.minimizeAxis1(mount.config.identity.mountType == MOUNT_TYPE_GEM ? (Lat >= 0 ? M_PI_2 : -M_PI_2) : 0);

        Serial.println("error");
        Serial.println(test_alignment.getError() * 180. / PI * 60.);

        Coord_IN IN3 = HO3.To_Coord_IN(test_alignment.Tinv);


        Serial.println((IN3.Axis1_direct()) * 180. / PI );
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
      switch (commandState.command[3])
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
