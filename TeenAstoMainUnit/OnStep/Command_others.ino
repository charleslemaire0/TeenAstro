//   $ - Set parameter
  //   $B - Antibacklash 
  //  :$BDddd# Set Dec Antibacklash
  //          Return: 0 on failure
  //                  1 on success
  //  :$BRddd# Set RA Antibacklash
  //          Return: 0 on failure
  //                  1 on success
  //         Set the Backlash values.  Units are arc-seconds

void Command_dollar()
{
  switch (command[1])
  {
  case '$':
    for (int i = 0; i < EEPROM.length(); i++)
    {
      EEPROM.write(i, 0);
    }
  case '!':
    Serial.end();
    Serial1.end();
    Serial2.end();
    delay(1000);
    _reboot_Teensyduino_();
    break;
  case 'B':
  {
    int i;
    if ((strlen(parameter) > 1) && (strlen(parameter) < 5))
    {
      if ((atoi2((char *)&parameter[1], &i)) && ((i >= 0) && (i <= 999)))
      {
        if (parameter[0] == 'D')
        {
          backlashAxis2 = (int)round(((double)i *
            (double)StepsPerDegreeAxis2
            ) / 3600.0);
          EEPROM_writeInt(EE_backlashAxis2, backlashAxis2);
        }
        else if (parameter[0] == 'R')
        {
          backlashAxis1 = (int)round(((double)i *
            (double)StepsPerDegreeAxis1
            ) / 3600.0);
          EEPROM_writeInt(EE_backlashAxis1, backlashAxis1);
        }
        else
          commandError = true;
      }
      else
        commandError = true;
    }
  }
  break;
  case 'G':
  {
    int i;
    if ((parameter[0] == 'D' || parameter[0] == 'R' )
      && strlen(parameter) > 1 && strlen(parameter) < 11
      && atoi2( &parameter[1], &i) )
    {
      if (parameter[0] == 'D')
      {
        GearAxis2 = (unsigned int)i;
        EEPROM_writeInt(EE_GearAxis2, i);
      }
      else
      {
        GearAxis1 = (unsigned int)i;
        EEPROM_writeInt(EE_GearAxis1, i);
      }
      unsetPark();
      updateRatios();
    }
    else
      commandError = true;
  }
  break;
  case 'S':
  {
    int i;
    if ((parameter[0] == 'D' || parameter[0] == 'R')
      && (strlen(parameter) > 1) && (strlen(parameter) < 11)
      && atoi2((char *)&parameter[1], &i))
    {
      if (parameter[0] == 'D')
      {
        StepRotAxis2 = (unsigned int)i;
        EEPROM_writeInt(EE_StepRotAxis2, i);
      }
      else 
      {
        StepRotAxis1 = (unsigned int)i;
        EEPROM_writeInt(EE_StepRotAxis1, i);
      }
      unsetPark();
      updateRatios();
    }
    else
      commandError = true;
  }
  break;
  case 'M':
  {
    int i;
    if ((parameter[0] == 'D' || parameter[0] == 'R')
      && (strlen(parameter) > 1) && (strlen(parameter) < 4)
      && atoi2( &parameter[1], &i)  && ((i >= 4) && (i < 9)))
    {
      if (parameter[0] == 'D')
      {
        double fact = pow(2., i - MicroAxis2);
        cli();
        posAxis2 = fact *posAxis2;
        sei();
        StopAxis2();
        MicroAxis2 = i;       
        motorAxis2.setMicrostep(MicroAxis2);
        EEPROM.write(EE_MicroAxis2, MicroAxis2);
      }
      else
      {
        double fact = pow(2., i - MicroAxis1);
        cli();
        posAxis1 = fact *posAxis1;
        sei();
        StopAxis1();
        MicroAxis1 = i;
        motorAxis1.setMicrostep(MicroAxis1);
        EEPROM.write(EE_MicroAxis1, MicroAxis1);
      }
      unsetPark();
      updateRatios();
    }
    else
      commandError = true;
  }
  break;
  case 'R':
  {
    if ((parameter[0] == 'D' || parameter[0] == 'R') 
      && (strlen(parameter) > 1) && (strlen(parameter) < 3)
      && (parameter[1]== '0' || parameter[1] ==  '1'))
    {
      if (parameter[0] == 'D')
      {
        ReverseAxis2 = parameter[1] == '1' ? true : false;
        EEPROM.write(EE_ReverseAxis2, ReverseAxis2);
      }
      else
      {
        ReverseAxis1 = parameter[1] == '1' ? true : false;
        EEPROM.write(EE_ReverseAxis1, ReverseAxis1);
      }
    }
    else
      commandError = true;
  }
  break;
  case 'c':
  case 'C':
  {
    int i;
    if ((strlen(parameter) > 1) && (strlen(parameter) < 5)
      && atoi2((char *)&parameter[1], &i)
      && ((i >= 0) && (i <= 255)))
    {
      if (parameter[0] == 'D')
      {
        if (command[1] == 'C')
        {
          HighCurrAxis2 = (u_int8_t)i;
          EEPROM.write(EE_HighCurrAxis2, HighCurrAxis2);
        }
        else
        {
          LowCurrAxis2 = (u_int8_t)i;
          EEPROM.write(EE_LowCurrAxis2, LowCurrAxis2);
          motorAxis2.setCurrent((unsigned int)LowCurrAxis2 * 10);
        }
      }
      else if (parameter[0] == 'R')
      {
        if (command[1] == 'C')
        {
          HighCurrAxis1 = (u_int8_t)i ;
          EEPROM.write(EE_HighCurrAxis1, HighCurrAxis1);
        }
        else 
        {
          LowCurrAxis1 = (u_int8_t)i;
          EEPROM.write(EE_LowCurrAxis1, LowCurrAxis1);
          motorAxis1.setCurrent((unsigned int)LowCurrAxis1 * 10);
        }
      }
    }
    else
      commandError = true;
  }
  break;
  case 'F':
  {
    int i;
    if ((parameter[0] == 'D' || parameter[0] == 'R')
      && (strlen(parameter) > 1) && (strlen(parameter) < 5)
      && atoi2((char *)&parameter[1], &i)
      && ((i >= 0) && (i <= 127)))
    {
      i = i - 64;
      
      if (parameter[0] == 'D')
      {
        motorAxis2.setSG(i);
      }
      else
      {
        motorAxis1.setSG(i);
      }
    }
    else
      commandError = true;
  }
  break;
  default:
    commandError = true;
    break;
  }

}


//----------------------------------------------------------------------------------
//   % - Return parameter
//  :%BD# Get Dec Antibacklash
//          Return: d#
//  :%BR# Get RA Antibacklash
//          Return: d#
//          Get the Backlash values.  Units are arc-seconds
void Command_pct()
{
  switch (command[1])
  {
  case 'X':
    initmotor();
    break;
  case 'B':
  {
    int i;
    if (parameter[0] == 'D')
    {
      i = (int)round(((double)backlashAxis2 * 3600.0) /
        (double)StepsPerDegreeAxis2);
      if (i < 0) i = 0;
      if (i > 999) i = 999;
      sprintf(reply, "%d", i);
      quietReply = true;
    }
    else if (parameter[0] == 'R')
    {
      i = (int)round(((double)backlashAxis1 * 3600.0) /
        (double)StepsPerDegreeAxis1);
      if (i < 0) i = 0;
      if (i > 999) i = 999;
      sprintf(reply, "%d", i);
      quietReply = true;
    }
    else
      commandError = true;
  }
  break;
  case 'G':
  {
    if (parameter[0] == 'D')
    {
      sprintf(reply, "%u", GearAxis2);
      quietReply = true;
    }
    else if (parameter[0] == 'R')
    {
      sprintf(reply, "%u", GearAxis1);
      quietReply = true;
    }
    else
      commandError = true;

  }
  break;
  case 'S':
  {
    if (parameter[0] == 'D')
    {
      sprintf(reply, "%u", StepRotAxis2);
      quietReply = true;
    }
    else if (parameter[0] == 'R')
    {
      sprintf(reply, "%u", StepRotAxis1);
      quietReply = true;
    }
    else
      commandError = true;
  }
  break;
  case 'M':
  {
    if (parameter[0] == 'D')
    {
      sprintf(reply, "%u", (unsigned  int)MicroAxis2);
      quietReply = true;
    }
    else if (parameter[0] == 'R')
    {
      sprintf(reply, "%u", (unsigned  int)MicroAxis1);
      quietReply = true;
    }
    else
      commandError = true;
  }
  break;
  case 'R':
  {
    if (parameter[0] == 'D')
    {
      sprintf(reply, "%u", (unsigned  int)ReverseAxis2);
      quietReply = true;
    }
    else if (parameter[0] == 'R')
    {
      sprintf(reply, "%u", (unsigned  int)ReverseAxis1);
      quietReply = true;
    }
    else
      commandError = true;
  }
  break;
  case 'C':
  {
    if (parameter[0] == 'D')
    {
      sprintf(reply, "%u", HighCurrAxis2);
      quietReply = true;
    }
    else if (parameter[0] == 'R')
    {
      sprintf(reply, "%u", HighCurrAxis1);
      quietReply = true;
    }
    else
      commandError = true;
  }
  break;
  case 'c':
  {
    if (parameter[0] == 'D')
    {
      sprintf(reply, "%u", LowCurrAxis2);
      quietReply = true;
    }
    else if (parameter[0] == 'R')
    {
      sprintf(reply, "%u", LowCurrAxis1);
      quietReply = true;
    }
    else
      commandError = true;
  }
  break;
  case 'L':
  {
    if ((parameter[0] == 'D' || parameter[0] == 'R'))
    {
      if (parameter[0] == 'D')
      {
        sprintf(reply, "%d", motorAxis2.getSG());
        quietReply = true;
      }
      else if (parameter[0] == 'R')
      {
        sprintf(reply, "%d", motorAxis1.getSG());
        quietReply = true;
      }
      else
        commandError = true;
    }
    else
      commandError = true;
  }
  break;
  case 'I':
  {
    if ((parameter[0] == 'D' || parameter[0] == 'R'))
    {
      if (parameter[0] == 'D')
      {
        sprintf(reply, "%u", motorAxis1.getCurrent());
        quietReply = true;
      }
      else if (parameter[0] == 'R')
      {
        sprintf(reply, "%u", motorAxis2.getCurrent());
        quietReply = true;
      }
      else
        commandError = true;
    }
    else
      commandError = true;
  }
  break;

  default:
    commandError = true;
    break;
  }
}


//----------------------------------------------------------------------------------
//   A - Alignment Commands
//  :AW#  Align Write to EEPROM
//         Returns: 1 on success
void Command_A()
{
  if (command[1] == 'W')
  {
    saveAlignModel();
  }
  else    //  :A?#  Align status
          //         Returns: mno#
          //         where m is the maximum number of alignment stars
          //               n is the current alignment star (0 otherwise)
          //               o is the last required alignment star when an alignment is in progress (0 otherwise)
    if (command[1] == '?')
    {
      reply[0] = maxAlignNumStar;
      reply[1] = '0' + alignThisStar;
      reply[2] = '0' + alignNumStars;
      reply[3] = 0;
      quietReply = true;
    }
    else    //  :An#  Start Telescope Manual Alignment Sequence
            //         This is to initiate a one or two-star alignment:
            //         1) Before calling this function, the telescope should be in the polar-home position
            //         2) Call this function
            //         3) Set the target location (RA/Dec) to a bright star, etc. (near the celestial equator in the western sky)
            //         4) Issue a goto command
            //         5) Center the star/object using the guide commands (as needed)
            //         6) Call :A+# command to accept the correction
            //         ( for two-star alignment )
            //         7) Set the target location (RA/Dec) to a bright star, etc. (near the celestial equator in the southern sky)
            //         8) Issue a goto command
            //         9) Center the star/object using the guide commands (as needed)
            //         10) Call :A+# command to accept the correction
            //         Returns:
            //         1: When ready for your goto commands
            //         0: If mount is busy
      if ((command[1] >= '1') && (command[1] <= '9'))
      {
        // set current time and date before calling this routine
        // Two star and three star align not supported with Fork mounts in alternate mode
        
        if (mountType == MOUNT_TYPE_FORK_ALT && alignThisStar != 1)
        {
          commandError = true;
          alignNumStars = 0;
          alignThisStar = 1;
          return;
        }
          // telescope should be set in the polar home (CWD) for a starting point
          // this command sets indexAxis1, indexAxis2, azmCor=0; altCor=0;
          setHome();

          // enable the stepper drivers
          enable_Axis(true);
          delay(10);

          // start tracking
          trackingState = TrackingON;
          lastSetTrakingEnable = millis();
          // start align...
          alignNumStars = command[1] - '0';
          alignThisStar = 1;


        if (commandError)
        {
          alignNumStars = 0;
          alignThisStar = 1;
        }
      }
      else    //  :A+#  Manual Alignment, set target location
              //         Returns:
              //         1: If correction is accepted
              //         0: Failure, Manual align mode not set or distance too far
        if (command[1] == '+')
        {
          // after last star turn meridian flips off when align is done
          if ((alignNumStars == alignThisStar) && (meridianFlip == MeridianFlipAlign))
            meridianFlip = MeridianFlipNever;

          // AltAz Taki method
          if (mountType == MOUNT_TYPE_ALTAZM && (alignNumStars > 1) && (alignThisStar <= alignNumStars))
          {
            cli();

            // get the Azm/Alt
            double  F = (double)(posAxis1 /*+ indexAxis1Steps*/) / (double)StepsPerDegreeAxis1;
            double  H = (double)(posAxis2 /*+ indexAxis2Steps*/) / (double)StepsPerDegreeAxis2;
            sei();

            // B=RA, D=Dec, H=Elevation (instr), F=Azimuth (instr), all in degrees
            Align.addStar(alignThisStar, alignNumStars, haRange(rtk.LST() * 15.0 - newTargetRA), newTargetDec, H, F);
            alignThisStar++;
          }
          else if (alignThisStar <= alignNumStars)
            {
              // RA, Dec (in degrees)
              if (GeoAlign.addStar(alignThisStar, alignNumStars,
                newTargetRA, newTargetDec))
                alignThisStar++;
              else
                commandError = true;
            }
            else
              commandError = true;

          if (commandError)
          {
            alignNumStars = 0;
            alignThisStar = 0;
          }
        }
        else
          commandError = true;
}


//----------------------------------------------------------------------------------
//   B - Reticule/Accessory Control
//  :B+#   Increase reticule Brightness
//         Returns: Nothing
//  :B-#   Decrease Reticule Brightness
//         Returns: Nothing
void Command_B()
{
  if (command[1] != '+' && command[1] != '-')
    return;
#ifdef RETICULE_LED_PINS
  int scale;
  if (reticuleBrightness > 255 - 8)
    scale = 1;
  else if (reticuleBrightness > 255 - 32)
    scale = 4;
  else if (reticuleBrightness > 255 - 64)
    scale = 12;
  else if (reticuleBrightness > 255 - 128)
    scale = 32;
  else
    scale = 64;
  if (command[1] == '-') reticuleBrightness += scale;
  if (reticuleBrightness > 255) reticuleBrightness = 255;
  if (command[1] == '+') reticuleBrightness -= scale;
  if (reticuleBrightness < 0) reticuleBrightness = 0;
  analogWrite(ReticulePin, reticuleBrightness);
#endif
  quietReply = true;
}

//   C - Sync Control
//  :CS#   Synchonize the telescope with the current right ascension and declination coordinates
//         Returns: Nothing (Sync's fail silently)
//  :CM#   Synchonize the telescope with the current database object (as above)
//         Returns: "N/A#" on success, "En#" on failure where n is the error code per the :MS# command

void Command_C()
{
  if ((parkStatus == NotParked) &&
      (trackingState != TrackingMoveTo) &&
      ( command[1] == 'M' || command[1] == 'S'))
  {
    if (newTargetPierSide == PierSideEast || newTargetPierSide == PierSideWest)
    {
      pierSide = newTargetPierSide;
      newTargetPierSide = 0;
    }
    i = syncEqu(newTargetRA, newTargetDec);
    i = 0;
    if (command[1] == 'M')
    {
      if (i == 0) strcpy(reply, "N/A");
      if (i>0) { reply[0] = 'E'; reply[1] = '0' + i; reply[2] = 0; }
    }
    quietReply = true;
  }
  else
  {
    commandError = true;
  }
}


//----------------------------------------------------------------------------------
//   D - Distance Bars
//  :D#    returns an "\0x7f#" if the mount is moving, otherwise returns "#".
void Command_D()
{
  if (command[1] != 0)
    return;
  if (trackingState == TrackingMoveTo)
  {
    reply[0] = (char)127;
    reply[1] = 0;
  }
  else
  {
    reply[0] = 0;
  }
  quietReply = true;
}

//----------------------------------------------------------------------------------
//   F - Focuser
void Command_F()
{
  quietReply = true; // the focuser is responding
  boolean focuserNoResponse = false;
  boolean focuserShortResponse = false;
  char command_out[30];
  Serial2.flush();
  while (Serial2.available() > 0) Serial2.read();

  switch (command[1])
  {
  case '+':
  case '-':
  case 'P':
  case 'Q':
  case 'O':
  case 'o':
  case 'I':
  case 'i':
  case '?':
  case '~':
  case 'M':
  case 'V':
    memcpy(command_out, ":F_#", sizeof(":F_#"));
    command_out[2] = command[1];
    break;
  case 'g':
  case 'x':
    command_out[0] = ':';
    command_out[1] = 'F';
    command_out[2] = command[1];
    command_out[3] = parameter[0];
    command_out[4] = '#';
    command_out[5] = 0;
    break;
  case 's':
  {
    command_out[0] = ':';
    command_out[1] = 'F';
    command_out[2] = command[1];
    command_out[3] = parameter[0];
    command_out[4] = ' ';
    int pos = 5;
    for (unsigned int k = 0; k+1 < strlen(parameter); k++)
    {
      command_out[pos] = parameter[k + 1];
      pos++;
    }
    command_out[pos] = '#';
    command_out[pos + 1] = 0;
  }
    break;
  case 'S':
  case 'G':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case 'c':
  case 'C':
  case 'm':
  {
    command_out[0] = ':';
    command_out[1] = 'F';
    command_out[2] = command[1];
    int k = strlen(parameter);
    int p = 3;
    if (k > 0)
    {
      command_out[p] = ' ';
      p++;
      memcpy(&command_out[p], &parameter[0], k * sizeof(char));
    }
    command_out[p + k] = '#';
    command_out[p + k + 1] = 0;

    break;
  }

  break;
  default:
  {
    break;
  }
  }
 
  switch (command[1])
  {
  case '+':
  case '-':
  case 'P':
  case 'Q':
  case 'G':
  case 'g':
  case 's':
  case 'S':
    focuserNoResponse = true;
    break;
  case 'x':
  case '?':
  case '~':
  case 'M':
  case 'V':
    focuserNoResponse = false;
    focuserShortResponse = false;
    break;
  case 'O':
  case 'o':
  case 'I':
  case 'i':
  case 'W':
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case 'c':
  case 'C':
  case 'm':
    focuserNoResponse = false;
    focuserShortResponse = true;
    break;
  default:
  {
    commandError = true;
    return;
    break;
  }
  }

  Serial2.print(command_out);

  if (!focuserNoResponse)
  {
    unsigned long start = millis();
    int pos = 0;
    char b = 0;
    while ( millis() - start < 10)
    {
      if (Serial2.available()>0)
      {        
        b = Serial2.read();

        if (b == '#' && !focuserShortResponse)
        {
          quietReply = true;
          return;
        }
        reply[pos] = b;
        pos++;
        if (pos > 49)
        {
          commandError = true;
          return;
        }
        reply[pos] = 0;
        if (focuserShortResponse)
        {
          quietReply = false;
          if (b != '1')
            commandError = true;
          return;
        }
      }

    }
    commandError = true;
  }
}
//----------------------------------------------------------------------------------
//  h - Home Position Commands
void Command_h()
{
  switch (command[1])
  {
  case 'F':
    //  :hF#   Reset telescope at the home position.  This position is required for a Cold Start.
    //         Point to the celestial pole with the counterweight pointing downwards (CWD position).
    //         Returns: Nothing
    setHome();
    quietReply = true;
    break;
  case 'C':
    //  :hC#   Moves telescope to the home position
    //          Return: 0 on failure
    //                  1 on success
    if (!goHome()) commandError = true;
    break;
  case 'O':
    // : hO#   Reset telescope at the Park position if Park position is stored.
    //          Return: 0 on failure
    //                  1 on success
    if (!syncAtPark()) commandError = true;
    break;
  case 'P':
    // : hP#   Goto the Park Position
      //         Returns: Nothing
    if (park()) commandError = true;
    break;
  case 'Q':
    //  :hQ#   Set the park position
    //          Return: 0 on failure
    //                  1 on success
    if (!setPark()) commandError = true;
    break;
  case 'R':
    //  :hR#   Restore parked telescope to operation
    //          Return: 0 on failure
    //                  1 on success
    if (!unpark()) commandError = true;
    break;
  default:
    commandError = true;
    break;
  }
}

//----------------------------------------------------------------------------------
//   Q - Halt Movement Commands
void Command_Q()
{
  switch (command[1])
  {
  case 0:
    //  :Q#    Halt all slews, stops goto
    //         Returns: Nothing
    if ((parkStatus == NotParked) || (parkStatus == Parking))
    {
      if (trackingState == TrackingMoveTo)
      {
        abortSlew = true;
      }
      else if (GuidingState == GuidingRecenter || GuidingState == GuidingST4)
      {
        if (guideDirAxis1)
          StopAxis1();
        if (guideDirAxis2)
          StopAxis2();
      }
      else
      {
        if (guideDirAxis1)
          guideDirAxis1 = 'b';
        if (guideDirAxis2)
          guideDirAxis2 = 'b';
      }
    }

    quietReply = true;
    break;


  case 'e':
  case 'w':
    //  :Qe# & Qw#   Halt east/westward Slews
    //         Returns: Nothing
  {
    if ((parkStatus == NotParked) && (trackingState != TrackingMoveTo))
    {
      if (guideDirAxis1)
        StopAxis1();
    }
    quietReply = true;
  }
  break;

  case 'n':
  case 's':
    //  :Qn# & Qs#   Halt north/southward Slews
    //         Returns: Nothing
  {
    if ((parkStatus == NotParked) && (trackingState != TrackingMoveTo))
    {
      if (guideDirAxis2)
        StopAxis2();
    }
    quietReply = true;
  }
  break;

  default:
    commandError = true;
    break;
  }
}

//----------------------------------------------------------------------------------
//   R - Slew Rate Commands
void Command_R()
{
  //  :RG#   Set Slew rate to Guiding Rate (slowest) 1X
  //  :RC#   Set Slew rate to Centering rate (2nd slowest) 8X
  //  :RM#   Set Slew rate to Find Rate (2nd Fastest) 24X
  //  :RS#   Set Slew rate to max (fastest) ?X (1/2 of MaxRate)
  //  :Rn#   Set Slew rate to n, where n=0..9
  //         Returns: Nothing
  int i = 5;
  switch (command[1])
  {
  case 'G':
    i = 2;
    break;
  case 'C':
    i = 5;
    break;
  case 'M':
    i = 6;
    break;
  case 'S':
    i = 8;
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    i = command[1] - '0';
    break;
  default:
    commandError = true;
    return;
  }
  if (trackingState != TrackingMoveTo && GuidingState == GuidingOFF)
  {
    enableGuideRate(i, false);
  }
  else
  {
    commandError = true;
  }

  quietReply = true;
}

//----------------------------------------------------------------------------------
//   T - Tracking Commands
//  :T+#   Master sidereal clock faster by 0.1 Hertz (I use a fifth of the LX200 standard, stored in EEPROM)
//  :T-#   Master sidereal clock slower by 0.1 Hertz (stored in EEPROM)
//  :TS#   Track rate solar
//  :TL#   Track rate lunar
//  :TQ#   Track rate sidereal
//  :TR#   Master sidereal clock reset (to calculated sidereal rate, stored in EEPROM)
//  :TK#   Track rate king
//  :Te#   Tracking enable  (OnStep only, replies 0/1)
//  :Td#   Tracking disable (OnStep only, replies 0/1)
//  :To#   OnTrack enable   (OnStep only, replies 0/1)
//  :Tr#   Track refraction enable  (OnStep only, replies 0/1)
//  :Tn#   Track refraction disable (OnStep only, replies 0/1)
//         Returns: Nothing
void Command_T()
{

  switch (command[1])

  {
  case '+':
    siderealInterval -= HzCf * (0.02);
    quietReply = true;
    break;
  case '-':
    siderealInterval += HzCf * (0.02);
    quietReply = true;
    break;
  case 'S':
    // solar tracking rate 60Hz 
    SetTrackingRate(TrackingSolar);
    refraction = false;
    quietReply = true;
    break;
  case 'L':
    // lunar tracking rate 57.9Hz
    SetTrackingRate(TrackingLunar);
    refraction = false;
    quietReply = true;
    break;
  case 'Q':
    // sidereal tracking rate
    SetTrackingRate(default_tracking_rate);
    quietReply = true;
    break;
  case 'R':
    // reset master sidereal clock interval
    siderealInterval = masterSiderealInterval;
    quietReply = true;
    break;
  case 'K':
    // king tracking rate 60.136Hz

    SetTrackingRate(0.99953004401);
    refraction = false;
    quietReply = true;
    break;
  case 'e':
    if (parkStatus == NotParked)
    {
      if ((trackingState == TrackingON || trackingState == TrackingOFF))
      {
        trackingState = TrackingON;
        lastSetTrakingEnable = millis();
        atHome = false;
      }
      if (trackingState == TrackingMoveTo)
      {
        lastTrackingState = TrackingON;
      }
    }
    else
      commandError = true;
    break;
  case 'd':
    if (parkStatus == NotParked)
    {
      if ((trackingState == TrackingON || trackingState == TrackingOFF))
      {
        trackingState = TrackingOFF;
        lastSetTrakingEnable = millis();
        atHome = false;
      }
      if (trackingState == TrackingMoveTo)
      {
        lastTrackingState = TrackingOFF;
      }
    }
    else
      commandError = true;
    break;
    if (trackingState == TrackingON || trackingState == TrackingOFF)
    {
      trackingState = TrackingOFF;
    }
    else
      commandError = true;
    break;

  case 'o':
    // turn full compensation on, defaults to base sidereal tracking rate
    refraction = refraction_enable;
    onTrack = true;
    SetTrackingRate(default_tracking_rate);
    break;

  case 'r':
    // turn refraction compensation on, defaults to base sidereal tracking rate
    refraction = refraction_enable;
    onTrack = false;
    SetTrackingRate(default_tracking_rate);
    break;

  case 'n':
    // turn refraction off, sidereal tracking rate resumes     
    refraction = false;
    onTrack = false;
    SetTrackingRate(default_tracking_rate);
    break;

  default:
    commandError = true;
    break;
}
            
  // Only burn the new rate if changing the sidereal interval
  if ((!commandError) && ((command[1] == '+') || (command[1] == '-') ||
    (command[1] == 'R')))
  {
    EEPROM_writeLong(EE_siderealInterval, siderealInterval);
    updateSideral();
  }
}

//   U - Precision Toggle
//  :U#    Toggle between low/hi precision positions
//         Low -  RA/Dec/etc. displays and accepts HH:MM.M sDD*MM
//         High - RA/Dec/etc. displays and accepts HH:MM:SS sDD*MM:SS
//         Returns Nothing

void Command_U()
{
  if (command[1] == 0)
  {
    highPrecision = !highPrecision;
    quietReply = true;
  }
  else
    commandError = true;
}

//   W - Site Select/Site get
//  :Wn#
//         Sets current site to n, 0..3 or queries site with ?
//         Returns: Nothing or current site ?#
void Command_W()
{
  if ((command[1] >= '0') && (command[1] <= '3'))
  {
    uint8_t currentSite = command[1] - '0';
    EEPROM.write(EE_currentSite, currentSite);
    localSite.ReadSiteDefinition(currentSite);
    quietReply = true;
  }
  else
    if (command[1] == '?') {
      quietReply = true;
      sprintf(reply, "%d", localSite.siteIndex());
    }
    else
      commandError = true;
}
