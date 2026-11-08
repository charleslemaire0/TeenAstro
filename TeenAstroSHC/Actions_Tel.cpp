#include "SmartController.h"
#include "SHC_text.h"

void SmartHandController::menuSpeedRate()
{
  ta_MountStatus.updateMount();
  if (ta_MountStatus.isPushTo())
    return;
  buttonPad.setMenuMode();
  const char* string_list_Speed = T_GUIDE "\n" T_SLOW "\n" T_MEDIUM "\n" T_FAST "\n" T_MAX;
  static unsigned char current_selection_speed = 3;
  TeenAstroMountStatus::GuidingRate cur_GR = ta_MountStatus.getGuidingRate();
  if (cur_GR == TeenAstroMountStatus::GuidingRate::UNKNOW)
    return;
  current_selection_speed = static_cast<unsigned char>(cur_GR);
  uint8_t selected_speed = display->UserInterfaceSelectionList(&buttonPad, T_SETSPEED, current_selection_speed + 1, string_list_Speed);
  if (selected_speed > 0)
  {
    m_client->setSpeed(selected_speed - 1);
    current_selection_speed = selected_speed;
  }
  buttonPad.setControlerMode();
}

#ifdef NO_SPEED_MENU
void SmartHandController::increaseSpeed(bool increase)
{
  ta_MountStatus.updateMount();
  if (ta_MountStatus.isPushTo())
    return;
  const char* string_list_Speed = T_GUIDE "\n" T_SLOW "\n" T_MEDIUM "\n" T_FAST "\n" T_MAX;
  static unsigned char current_speed = 3;
  TeenAstroMountStatus::GuidingRate cur_GR = ta_MountStatus.getGuidingRate();
  if (cur_GR == TeenAstroMountStatus::GuidingRate::UNKNOW)
    return;
  current_speed = static_cast<unsigned char>(cur_GR);
  if (increase && cur_GR < TeenAstroMountStatus::MAX )
  {    
    m_client->setSpeed(current_speed + 1);
  }
  else if (!increase && cur_GR > TeenAstroMountStatus::GUIDING )
  {
    m_client->setSpeed(current_speed - 1);
  }
}
#endif

void SmartHandController::menuTelActionPushTo()
{
  buttonPad.setMenuMode();
  telescoplocked = false;
  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    const char* string_list = T_PUSHTO "\n" T_SYNC "\n" T_ALIGN "\n" T_SIDEOFPIER "\n" T_SAVE " RADEC";
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, s_sel, string_list);
    s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
    MENU_RESULT answer = MR_CANCEL;
    switch (tmp_sel)
    {
    case 0:
      exitMenu = true;
      break;
    case 1:
      answer = menuSyncGoto(NAV_PUSHTO);
      answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
      break;
    case 2:
      answer = menuSyncGoto(NAV_SYNC);
      answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
      break;
    case 3:
      answer = menuAlignment();
      answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
      break;
    case 4:
      menuPier();
      break;
    case 5:
      if (m_client->saveUserPosition() == LX200_VALUESET)
      {
        DisplayMessage("RA DEC", T_SAVED, 500);
      }
      else
      {
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
      }
      break;
    default:
      break;
    }
  }
  if (!sleepDisplay)
  {
    buttonPad.setControlerMode();
  }

}

void SmartHandController::menuTelActionPushToGoto()
{
  buttonPad.setMenuMode();

  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    TeenAstroMountStatus::ParkState currentstate = ta_MountStatus.getParkState();

    if (currentstate == TeenAstroMountStatus::PRK_PARKED)
    {
      const char* string_list_main_ParkedL0 = T_UNPARK;
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, s_sel, string_list_main_ParkedL0);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        m_client->unpark();
        exitMenu = true;
        break;
      default:
        break;
      }
    }
    else if (currentstate == TeenAstroMountStatus::PRK_UNPARKED)
    {
      const char* string_list_main_UnParkedL0 = telescoplocked ? T_UNLOCK : T_GOTO "\n" T_PUSHTO "\n" T_SYNC "\n" T_ALIGN "\n" T_GEARCHECK "\n" T_TRACKING "\n" T_SIDEOFPIER "\n" T_SAVE " RADEC\n" T_LOCK "\n" T_SPIRAL;
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, s_sel, string_list_main_UnParkedL0);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      MENU_RESULT answer = MR_CANCEL;
      if (telescoplocked)
      {
        switch (tmp_sel)
        {
        case 0:
          exitMenu = true;
          break;
        case 1:
          telescoplocked = false;
          exitMenu = true;
          break;
        default:
          break;
        }
      }
      else
      {
        switch (tmp_sel)
        {
        case 0:
          exitMenu = true;
          break;
        case 1:
          answer = menuSyncGoto(NAV_GOTO);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 2:
          answer = menuSyncGoto(NAV_PUSHTO);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 3:
          answer = menuSyncGoto(NAV_SYNC);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 4:
          answer = menuAlignment();
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 5:
          answer = menuGearCheck();
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 6:
          menuTrack();
          break;
        case 7:
          menuPier();
          break;
        case 8:
          if (m_client->saveUserPosition() == LX200_VALUESET)
          {
            DisplayMessage("RA DEC", T_SAVED, 500);
          }
          else
          {
            DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
          }
          break;
        case 9:
          telescoplocked = true;
          exitMenu = true;
          break;
        case 10:
        {
          menuSpirale();
          break;
        }
        default:
          break;
        }
      }
    }
  }
  if (!sleepDisplay)
  {
    buttonPad.setControlerMode();
  }
}

void SmartHandController::menuTelActionGoto()
{
  buttonPad.setMenuMode();

  static uint8_t s_sel = 1;
  uint8_t tmp_sel;
  while (!exitMenu)
  {
    ta_MountStatus.updateMount();
    TeenAstroMountStatus::ParkState currentstate = ta_MountStatus.getParkState();

    if (currentstate == TeenAstroMountStatus::PRK_PARKED)
    {
      const char* string_list_main_ParkedL0 = T_UNPARK;
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, s_sel, string_list_main_ParkedL0);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      switch (tmp_sel)
      {
      case 0:
        exitMenu = true;
        break;
      case 1:
        m_client->unpark();
        exitMenu = true;
        break;
      default:
        break;
      }
    }
    else if (currentstate == TeenAstroMountStatus::PRK_UNPARKED)
    {
      const char* string_list_main_UnParkedL0 = telescoplocked ? T_UNLOCK : T_GOTO "\n" T_SYNC "\n" T_ALIGN "\n" T_GEARCHECK "\n" T_TRACKING "\n" T_SIDEOFPIER "\n" T_SAVE " RADEC\n" T_LOCK "\n" T_SPIRAL;
      tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TELESCOPEACTION, s_sel, string_list_main_UnParkedL0);
      s_sel = tmp_sel > 0 ? tmp_sel : s_sel;
      MENU_RESULT answer = MR_CANCEL;
      if (telescoplocked)
      {
        switch (tmp_sel)
        {
        case 0:
          exitMenu = true;
          break;
        case 1:
          telescoplocked = false;
          exitMenu = true;
          break;
        default:
          break;
        }
      }
      else
      {
        switch (tmp_sel)
        {
        case 0:
          exitMenu = true;
          break;
        case 1:
          answer = menuSyncGoto(NAV_GOTO);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 2:
          answer = menuSyncGoto(NAV_SYNC);
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 3:
          answer = menuAlignment();
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 4:
          answer = menuGearCheck();
          answer == MR_OK || answer == MR_QUIT ? exitMenu = true : exitMenu = false;
          break;
        case 5:
          menuTrack();
          break;
        case 6:
          menuPier();
          break;
        case 7:
          if (m_client->saveUserPosition() == LX200_VALUESET)
          {
            DisplayMessage("RA DEC", T_SAVED, 500);
          }
          else
          {
            DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
          }
          break;
        case 8:
          telescoplocked = true;
          exitMenu = true;
          break;
        case 9:
        {
          menuSpirale();
          break;
        }
        default:
          break;
        }
      }
    }
  }
  if (!sleepDisplay)
  {
    buttonPad.setControlerMode();
  }
}

void SmartHandController::menuTrack()
{
  ta_MountStatus.updateMount();
  TeenAstroMountStatus::TrackState currentstate = ta_MountStatus.getTrackingState();
  uint8_t tmp_sel;
  if (currentstate == TeenAstroMountStatus::TRK_ON)
  {
    const char* string_list_tracking = T_STOPTRACKING "\n" T_SIDEREAL "\n" T_LUNAR "\n" T_SOLAR "\n" T_TARGET;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGSTATE, 0, string_list_tracking);
    switch (tmp_sel)
    {
    case 1:
      char out[20];
      memset(out, 0, sizeof(out));
      if (m_client->enableTracking(false) == LX200_VALUESET)
      {
        DisplayMessage(T_TRACKING, T_OFF, 500);
        exitMenu = true;
      }
      else
      {
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
      }
      break;
    case 2:
      exitMenu = DisplayMessageLX200(m_client->setTrackRateSidereal());
      break;
    case 3:
      exitMenu = DisplayMessageLX200(m_client->setTrackRateLunar());
      break;
    case 4:
      exitMenu = DisplayMessageLX200(m_client->setTrackRateSolar());
      break;
    case 5:
      exitMenu = DisplayMessageLX200(m_client->setTrackRateUser());
      break;
    default:
      break;
    }
  }
  else if (currentstate == TeenAstroMountStatus::TRK_OFF)
  {
    const char* string_list_tracking = T_STARTTRACKING;
    tmp_sel = display->UserInterfaceSelectionList(&buttonPad, T_TRACKINGSTATE, 0, string_list_tracking);
    switch (tmp_sel)
    {
    case 1:
      if (m_client->enableTracking(true) == LX200_VALUESET)
      {
        DisplayMessage(T_TRACKING, T_ON, 500);
        exitMenu = true;
      }
      else
      {
        DisplayMessage(T_LX200COMMAND, T_FAILED, 1000);
      }
      break;
    default:
      break;
    }
  }
  else
  {
    DisplayMessage(T_CURRENTLYTRACKING, T_CANNOTBECHANGED, 1000);
  }
}

/// Format the rigid head geometry (cone, axis2 non-perpendicularity, axis2
/// index) for the alignment error screen, all in arcseconds. Returns false when
/// the model has none, i.e. when no rigid multi-star session fitted them, so the
/// extra screen only appears where it means something.
///
/// A term the star distribution could not separate is shown as "n/a" rather
/// than as a measured zero, so the display never implies a measurement that
/// was not actually made. CH stays "n/a" until each pier side has 3 stars;
/// the line then shows the current split, for example "CH n/a 1+5".
static bool formatRigidHead(LX200Client& client, char(&cone)[20], char(&perp)[20], char(&idx2)[20])
{
  double c = 0.0, p = 0.0, i2 = 0.0;
  char mask[8] = { 0 };
  uint8_t nIn = 0, nOut = 0;
  const bool haveSides = client.getAlignPierSides(nIn, nOut) == LX200_VALUEGET;
  if (client.getAlignHeadCone(c) != LX200_VALUEGET
      || client.getAlignHeadPerp(p) != LX200_VALUEGET
      || client.getAlignHeadIndex2(i2) != LX200_VALUEGET
      || client.getAlignFittedTerms(mask, sizeof(mask)) != LX200_VALUEGET)
    return false;
  if (strchr(mask, 'C') == NULL && strchr(mask, 'P') == NULL && strchr(mask, 'I') == NULL)
    return false;
  if (strchr(mask, 'C'))
    snprintf(cone, sizeof(cone), "CH %+.0f\"", c);
  else if (haveSides && (nIn < 3 || nOut < 3))
    snprintf(cone, sizeof(cone), "CH n/a %u+%u", (unsigned)nIn, (unsigned)nOut);
  else
    snprintf(cone, sizeof(cone), "CH n/a");
  if (strchr(mask, 'P')) snprintf(perp, sizeof(perp), "NP %+.0f\"", p);
  else                   snprintf(perp, sizeof(perp), "NP n/a");
  if (strchr(mask, 'I')) snprintf(idx2, sizeof(idx2), "ID %+.0f\"", i2);
  else                   snprintf(idx2, sizeof(idx2), "ID n/a");
  return true;
}

/// "+DD*MM:SS" from :AE# / :GXAawz# becomes "+DD°MM'SS\"".
static void prettyDmsInPlace(char* s, size_t n)
{
  if (s == NULL)
    return;
  const size_t len = strlen(s);
  if (len < 9 || n < len + 1)
    return;
  if ((s[0] != '+' && s[0] != '-') || s[3] != '*' || s[6] != ':')
    return;
  s[3] = '\xB0';
  s[6] = '\'';
  if (len == 9 && n > 10)
  {
    s[9] = '\"';
    s[10] = '\0';
  }
}

void SmartHandController::showAlignmentResult()
{
  DisplayMessage(T_ALIGNMENT, T_SUCESS "!", -1);
  // The key that dismisses "Success" must not also skip the error.
  showAlignErrorReport(800);
}

void SmartHandController::showAlignErrorReport(int settleMs)
{
  char title[22];
  strncpy(title, T_ERROR, sizeof(title) - 1);
  title[sizeof(title) - 1] = '\0';

  // Prefer the rigid multi-star RMS (:GXAr#). :AE# is only the two-star seed
  // anglediff and stays misleading after 4-star / 3+3 close-out.
  double rmsAs = 0.0;
  if (m_client->getAlignRigidRms(rmsAs) == LX200_VALUEGET && rmsAs > 0.0)
  {
    if (rmsAs >= 3600.0)
      snprintf(title, sizeof(title), "RMS %.1f\xB0", rmsAs / 3600.0);
    else if (rmsAs >= 60.0)
      snprintf(title, sizeof(title), "RMS %.1f'", rmsAs / 60.0);
    else
      snprintf(title, sizeof(title), "RMS %.1f\"", rmsAs);
  }

  char errAz[20] = { "?" };
  char errAlt[20] = { "?" };
  char errPol[20] = { "?" };
  const bool havePolar =
      m_client->getAlignErrorPolar(errPol, sizeof(errPol)) == LX200_VALUEGET
      && m_client->getAlignErrorAz(errAz, sizeof(errAz)) == LX200_VALUEGET
      && m_client->getAlignErrorAlt(errAlt, sizeof(errAlt)) == LX200_VALUEGET;
  if (havePolar)
  {
    prettyDmsInPlace(errPol, sizeof(errPol));
    prettyDmsInPlace(errAz, sizeof(errAz));
    prettyDmsInPlace(errAlt, sizeof(errAlt));
    char w[24], ma[24], me[24];
    snprintf(w, sizeof(w), "W  %s", errPol);
    snprintf(ma, sizeof(ma), "MA %s", errAz);
    snprintf(me, sizeof(me), "ME %s", errAlt);
    DisplayLongMessage(title, w, ma, me, -1, settleMs);
  }
  else
  {
    char text[24] = { 0 };
    if (m_client->getAlignError(text, sizeof(text)) == LX200_VALUEGET)
    {
      prettyDmsInPlace(text, sizeof(text));
      DisplayMessage(title, text, -1, settleMs);
    }
    else
      DisplayMessage(title, "?", -1, settleMs);
  }

  char hc[20], hp[20], hi[20];
  if (formatRigidHead(*m_client, hc, hp, hi))
    DisplayLongMessage("Head geometry:", hc, hp, hi, -1, settleMs);
}

void SmartHandController::menuKnownErrors()
{
  bool on = false;
  if (m_client->getKnownGeomUse(on) != LX200_VALUEGET)
  {
    DisplayMessage(T_LX200COMMAND, T_FAILED, -1);
    return;
  }
  const uint8_t choice = display->UserInterfaceSelectionList(&buttonPad, T_KNOWN_ERRORS, on ? 2 : 1, T_OFF "\n" T_ON);
  if (choice == 0)
    return;
  const bool want = choice == 2;
  if (want != on && m_client->setKnownGeomUse(want) != LX200_VALUESET)
  {
    DisplayMessage(T_LX200COMMAND, T_FAILED, -1);
    return;
  }
  if (!want)
    return;

  double d = 0;
  float cone = 0.f;
  float perp = 0.f;
  if (m_client->getKnownCone(d) == LX200_VALUEGET)
    cone = (float)d;
  if (!display->UserInterfaceInputValueFloat(&buttonPad, T_CONE, "", &cone, -5.f, 5.f, 6, 3, " deg"))
    return;
  if (m_client->setKnownCone(cone) != LX200_VALUESET)
  {
    DisplayMessage(T_LX200COMMAND, T_FAILED, -1);
    return;
  }
  d = 0;
  if (m_client->getKnownPerp(d) == LX200_VALUEGET)
    perp = (float)d;
  if (!display->UserInterfaceInputValueFloat(&buttonPad, T_PERP, "", &perp, -5.f, 5.f, 6, 3, " deg"))
    return;
  if (m_client->setKnownPerp(perp) != LX200_VALUESET)
    DisplayMessage(T_LX200COMMAND, T_FAILED, -1);
}

SmartHandController::MENU_RESULT SmartHandController::menuAlignment()
{
  bool alignInProgress = ta_MountStatus.isAligning();
  static int current_selection = 1;
  while (true)
  {
    // Deferred mechanical-pole alignment is GEM/FORK only. Do not use isAltAz() here:
    // MountState::isAltAz() is true for MOUNT_UNDEFINED, which hides this menu line
    // until :GXAS# has set mount type — bad UX on EQ mounts before the first decode.
    const TeenAstroMountStatus::Mount mtAlign = ta_MountStatus.getMount();
    const bool showThreeStar =
        (mtAlign == TeenAstroMountStatus::MOUNT_TYPE_GEM
         || mtAlign == TeenAstroMountStatus::MOUNT_TYPE_FORK
         || mtAlign == TeenAstroMountStatus::MOUNT_UNDEFINED);
    // 3+3 needs the other pier side, and only a German equatorial flips.
    const bool showSix =
        (mtAlign == TeenAstroMountStatus::MOUNT_TYPE_GEM
         || mtAlign == TeenAstroMountStatus::MOUNT_UNDEFINED);
    char string_list[256];
    if (alignInProgress)
      strcpy(string_list, T_CANCEL);
    else
    {
      strcpy(string_list, "2 " T_STARS "\n" T_ALIGN_PERP);
      if (showSix)
        strcat(string_list, "\n" T_ALIGN_CONE);
      if (showThreeStar)
        strcat(string_list, "\n" T_TWO_STARS_MECH);
      strcat(string_list, "\n" T_PC " " T_ALIGNMENT);
      if (ta_MountStatus.isAligned())
        strcat(string_list, "\n" T_SAVE "\n" T_Clear "\nShow align. error");
      strcat(string_list, "\n" T_KNOWN_ERRORS);
    }
    int selection = display->UserInterfaceSelectionList(&buttonPad, T_ALIGNMENT, current_selection, string_list);
    if (selection == 0) return MR_CANCEL;
    current_selection = selection;
    // The cases below are numbered as if the 3+3 line were always shown.
    int choice = selection;
    if (!alignInProgress && !showSix && choice >= 3)
      choice++;
    if (!alignInProgress)
    {
      // Last item on every alignment list. Handled here so the star-count
      // cases below keep their numbers when the mechanical-pole line is hidden.
      const int knownSel = ta_MountStatus.isAligned()
        ? (showThreeStar ? 9 : 8)
        : (showThreeStar ? 6 : 5);
      if (choice == knownSel)
      {
        menuKnownErrors();
        continue;
      }
    }
    switch (choice)
    {
    case 1:
      if (alignInProgress)
      {
        m_client->alignAbort();
        ta_MountStatus.stopAlign();
        DisplayMessage(T_ALIGNMENT, T_CANCELED, -1);
        return MR_QUIT;
      }
      else
      {
        bool useKnown = false;
        m_client->getKnownGeomUse(useKnown);
        int ret = display->UserInterfaceMessage(&buttonPad, T_SELECTMODE, "2 " T_STAR,
          useKnown ? T_USE_IN_2STAR : T_ALIGNMENT, T_HOME "\n" T_STAR);
        if (ret == 1)
        {
          DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
          if (m_client->alignStart() == LX200_VALUESET)
          {
            ta_MountStatus.startAlign(TeenAstroMountStatus::AlignMode::ALIM_TWO);
            return MR_QUIT;
          }
          else
          {
            DisplayMessage(T_INITIALISATION, T_FAILED, -1);
          }   
        }
        else if (ret == 2)
        {
          const uint8_t choice = pickPierSide();
          bool ok = false;
          if (choice)
          {
            ok = sendPierSide(choice);
            if (ok)
            {
              DisplayMessage("Please Sync", "with a Target", 1000);
              cat_mgr.select(0);
              char title[40] = "";
              cat_mgr.filtersClear();
              cat_mgr.filterAdd(FM_OBJ_HAS_NAME);
              cat_mgr.filterAdd(FM_ABOVE_HORIZON, 1);

              strcat(title, cat_mgr.catalogTitle());
              double lat, LT0;
              while (!ta_MountStatus.getLat(lat))
              {
              }
              while (!ta_MountStatus.getLstT0(LT0))
              {
              }
              cat_mgr.setLat(lat);
              cat_mgr.setLstT0(LT0);
              if (cat_mgr.isInitialized())
              {
                if (cat_mgr.setIndex(cat_mgr.getIndex()))
                {
                  if (display->UserInterfaceCatalog(&buttonPad, title))
                  {
                    LX200RETURN out = SyncGotoCatLX200(*m_client, NAV_SYNC);
                    if (out == LX200_SYNCED)
                    {
                      DisplayMessageLX200(m_client->alignAcceptStar());
                      ta_MountStatus.startAlignSecondStar(TeenAstroMountStatus::AlignMode::ALIM_TWO);
                      return MR_QUIT;
                    }
                    else
                    {
                      DisplayMessageLX200(out);
                    }
                  }
                }
                else DisplayMessage(cat_mgr.catalogTitle(), "No Object", -1);
              }
              else DisplayMessage(cat_mgr.catalogTitle(), "Not Init'd?", -1);
              cat_mgr.filtersClear();

              DisplayMessageLX200(m_client->setPierSideNone());
       
            }
          }
        }
      }
      break;
    case 2:
      if (!alignInProgress)
      {
        // Four stars. German equatorial and equatorial fork stay on one pier
        // side. An alt-az never goes over the zenith, so the stars cover the
        // sky. :A0,r4# solves NP. Cone stays unpublished.
        const TeenAstroMountStatus::Mount mt4 = ta_MountStatus.getMount();
        const char* fourWhere = T_SAME_SIDE;
        if (mt4 == TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT
            || mt4 == TeenAstroMountStatus::MOUNT_TYPE_ALTAZM)
          fourWhere = T_AROUND_SKY;
        if (display->UserInterfaceMessage(&buttonPad, "4 " T_STARS,
            fourWhere, T_MEASURES_PERP, T_NO "\n" T_YES) != 2)
          break;
        DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
        if (m_client->alignStartRigid(4) == LX200_VALUESET)
        {
          ta_MountStatus.startAlign(TeenAstroMountStatus::AlignMode::ALIM_FOUR);
          return MR_QUIT;
        }
        DisplayMessage(T_INITIALISATION, T_FAILED, -1);
      }
      break;
    case 3:
      if (!alignInProgress)
      {
        // Three stars, then the other pier side, then three more. :A0,r6#.
        // Cone is solved only when the split really is 3 and 3.
        if (display->UserInterfaceMessage(&buttonPad, T_ALIGN_CONE, T_FLIP_AFTER_3, T_MEASURES_CONE, T_NO "\n" T_YES) != 2)
          break;
        DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
        if (m_client->alignStartRigid(6) == LX200_VALUESET)
        {
          ta_MountStatus.startAlign(TeenAstroMountStatus::AlignMode::ALIM_SIX);
          return MR_QUIT;
        }
        DisplayMessage(T_INITIALISATION, T_FAILED, -1);
      }
      break;
    case 4:
      if (showThreeStar)
      {
        int ret = display->UserInterfaceMessage(&buttonPad, T_SELECTMODE, T_TWO_STARS_MECH, T_ALIGNMENT , T_HOME "\n" T_STAR);
        if (ret == 1)
        {
          DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
          if (m_client->alignStartMechanicalPole() == LX200_VALUESET)
          {
            ta_MountStatus.startAlign(TeenAstroMountStatus::AlignMode::ALIM_THREE);
            return MR_QUIT;
          }
          else
          {
            DisplayMessage(T_INITIALISATION, T_FAILED, -1);
          }
        }
        else if (ret == 2)
        {
          const uint8_t choice = pickPierSide();
          bool ok = false;
          if (choice)
          {
            ok = sendPierSide(choice);
            if (ok)
            {
              DisplayMessage("Please Sync", "with a Target", 1000);
              cat_mgr.select(0);
              char title[40] = "";
              cat_mgr.filtersClear();
              cat_mgr.filterAdd(FM_OBJ_HAS_NAME);
              cat_mgr.filterAdd(FM_ABOVE_HORIZON, 1);

              strcat(title, cat_mgr.catalogTitle());
              double lat, LT0;
              while (!ta_MountStatus.getLat(lat))
              {
              }
              while (!ta_MountStatus.getLstT0(LT0))
              {
              }
              cat_mgr.setLat(lat);
              cat_mgr.setLstT0(LT0);
              if (cat_mgr.isInitialized())
              {
                if (cat_mgr.setIndex(cat_mgr.getIndex()))
                {
                  if (display->UserInterfaceCatalog(&buttonPad, title))
                  {
                    LX200RETURN out = SyncGotoCatLX200(*m_client, NAV_SYNC);
                    if (out == LX200_SYNCED)
                    {
                      DisplayMessageLX200(m_client->alignAcceptStarMechanicalPole());
                      ta_MountStatus.startAlignSecondStar(TeenAstroMountStatus::AlignMode::ALIM_THREE);
                      return MR_QUIT;
                    }
                    else
                    {
                      DisplayMessageLX200(out);
                    }
                  }
                }
                else DisplayMessage(cat_mgr.catalogTitle(), "No Object", -1);
              }
              else DisplayMessage(cat_mgr.catalogTitle(), "Not Init'd?", -1);
              cat_mgr.filtersClear();

              DisplayMessageLX200(m_client->setPierSideNone());
            }
          }
        }
      }
      else
      {
        DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
        if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, T_PC, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
        {
          if (m_client->alignAtHome() == LX200_VALUESET)
          {
            DisplayMessage(T_MOUNTSYNCED, T_ATHOME, -1);
            return MR_QUIT;
          }
          else
          {
            DisplayMessage(T_Clear, T_FAILED, -1);
          }
        }
      }
      break;
    case 5:
      if (showThreeStar)
      {
        DisplayLongMessage("!" T_WARNING "!", T_THEMOUNTMUSTBEATHOME1, T_THEMOUNTMUSTBEATHOME2, T_THEMOUNTMUSTBEATHOME3, -1);
        if (display->UserInterfaceMessage(&buttonPad, T_READYFOR, T_PC, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
        {
          if (m_client->alignAtHome() == LX200_VALUESET)
          {
            DisplayMessage(T_MOUNTSYNCED, T_ATHOME, -1);
            return MR_QUIT;
          }
          else
          {
            DisplayMessage(T_Clear, T_FAILED, -1);
          }
        }
      }
      else if (display->UserInterfaceMessage(&buttonPad, T_SAVE, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (m_client->alignSave() == LX200_VALUESET)
        {
          DisplayMessage(T_ALIGNMENT, T_SAVED, -1);
          return MR_QUIT;
        }
        else
        {
          DisplayMessage(T_SAVING, T_FAILED, -1);
        }
      }
      break;
    case 6:
      if (showThreeStar)
      {
        if (display->UserInterfaceMessage(&buttonPad, T_SAVE, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
        {
          if (m_client->alignSave() == LX200_VALUESET)
          {
            DisplayMessage(T_ALIGNMENT, T_SAVED, -1);
            return MR_QUIT;
          }
          else
          {
            DisplayMessage(T_SAVING, T_FAILED, -1);
          }
        }
      }
      else if (display->UserInterfaceMessage(&buttonPad, T_Clear, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
      {
        if (m_client->alignClear() == LX200_VALUESET)
        {
          DisplayMessage(T_MOUNTSYNCED, T_ATHOME, -1);
          return MR_QUIT;
        }
        else
        {
          DisplayMessage(T_Clear, T_FAILED, -1);
        }
      }
      break;
    case 7:
      if (showThreeStar)
      {
        if (display->UserInterfaceMessage(&buttonPad, T_Clear, T_STAR, T_ALIGNMENT "?", T_NO "\n" T_YES) == 2)
        {
          if (m_client->alignClear() == LX200_VALUESET)
          {
            DisplayMessage(T_MOUNTSYNCED, T_ATHOME, -1);
            return MR_QUIT;
          }
          else
          {
            DisplayMessage(T_Clear, T_FAILED, -1);
          }
        }
      }
      else
        showAlignErrorReport(0);
      break;
    case 8:
      showAlignErrorReport(0);
      break;
    }
  }
}

bool SmartHandController::SelectStarAlign()
{
  buttonPad.setMenuMode();
  double lat, LT0;
  while (!ta_MountStatus.getLat(lat))
  {
  }
  while (!ta_MountStatus.getLstT0(LT0))
  {
  }
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(LT0);
  const MENU_RESULT result = menuCatalogAlign(ta_MountStatus.isPushTo() ? NAV_PUSHTO : NAV_GOTO, true);
  if (result == MENU_RESULT::MR_QUIT)
    noteAlignStar();
  buttonPad.setControlerMode();
  return result != MENU_RESULT::MR_CANCEL;
}

void SmartHandController::noteAlignStar()
{
  if (m_alignPicked >= 6) return;
  m_alignRa[m_alignPicked] = (float)cat_mgr.ra();
  m_alignDec[m_alignPicked] = (float)cat_mgr.dec();
  m_alignPicked++;
}

void SmartHandController::dropAlignStar()
{
  if (m_alignPicked > 0) m_alignPicked--;
}