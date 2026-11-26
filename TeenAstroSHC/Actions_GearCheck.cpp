#include "SmartController.h"
#include "SHC_text.h"
#include <string.h>
#include <CommandCodec.h>
#include <TeenAstroPad.h>
#include <cmath>

// Gear check turns one motor axis by 90, 180 or 360 degrees (:MGn,ddd#).
// The user marks the shaft, lets it turn, then jogs the mark onto the
// expected place. Measured gear = cfg * (commanded + jog) / commanded,
// with both angles in the mount's step scale (:GXP1# / :GXP2#).

static double wrap180(double deg)
{
  while (deg > 180.0) deg -= 360.0;
  while (deg < -180.0) deg += 360.0;
  return deg;
}

static void formatArcmin(char* out, size_t len, double deg)
{
  snprintf(out, len, "%+.0f'", deg * 60.0);
}

static bool computeMeasuredGear(float gearCfg, double cmdDeg, double missDeg,
                                float& gearMeas)
{
  const double trueDeg = cmdDeg + missDeg;
  if (fabs(cmdDeg) < 5.0 || fabs(trueDeg) < 1e-3)
    return false;
  gearMeas = (float)((double)gearCfg * trueDeg / cmdDeg);
  if (!isfinite(gearMeas) || gearMeas <= 0.0f || gearMeas > 100000.0f)
    return false;
  return true;
}

bool SmartHandController::gearCheckReadAxes(double& a1Deg, double& a2Deg)
{
  char b1[20] = "";
  char b2[20] = "";
  if (m_client->getAxisDegrees(1, b1, sizeof(b1)) != LX200_VALUEGET)
    return false;
  if (m_client->getAxisDegrees(2, b2, sizeof(b2)) != LX200_VALUEGET)
    return false;
  // :GXP is full-range signed ("+180*30:00", "-180*00:00"). dmsToDouble()
  // is the declination parser (2 digits, ±90) and rejects every axis reply.
  if (!dmsFullRangeToDouble(&a1Deg, b1))
    return false;
  if (!dmsFullRangeToDouble(&a2Deg, b2))
    return false;
  return true;
}

bool SmartHandController::gearCheckWaitSlewSettled(int degrees)
{
  // Slow mounts: about 15 s per commanded degree, plus a minute.
  // Long Shift sends :Q# and ends the turn. The hint stays on screen.
  const unsigned long budget = 60000UL + (unsigned long)degrees * 15000UL;
  const unsigned long t0 = millis();
  unsigned long lastDraw = 0;
  while ((unsigned long)(millis() - t0) < budget)
  {
    ta_MountStatus.updateMount();
    if (ta_MountStatus.getTrackingState() != TeenAstroMountStatus::TRK_SLEWING)
    {
      delay(400);
      ta_MountStatus.updateMount();
      if (ta_MountStatus.getTrackingState() != TeenAstroMountStatus::TRK_SLEWING)
      {
        display->setFont(u8g2_font_helvR12_te);
        return true;
      }
    }
    tickButtons();
    if (eventbuttons[0] == E_LONGPRESS || eventbuttons[0] == E_LONGPRESSTART)
    {
      m_client->stopSlew();
      display->setFont(u8g2_font_helvR12_te);
      return false;
    }
    const unsigned long now = millis();
    if ((unsigned long)(now - lastDraw) > 250UL)
    {
      lastDraw = now;
      display->setFont(u8g2_font_helvR12_te);
      display->firstPage();
      do
      {
        display->drawUTF8(0, 26, T_GC_WAIT_SLEW);
        display->drawUTF8(0, 48, T_GC_STOP_NOW);
      }
      while (display->nextPage());
    }
    delay(50);
  }
  m_client->stopSlew();
  display->setFont(u8g2_font_helvR12_te);
  return false;
}

bool SmartHandController::gearCheckPrepareCatalog()
{
  double lat = 0, lst = 0;
  bool gotLat = false, gotLst = false;
  for (int i = 0; i < 4 && !gotLat; ++i)
    gotLat = ta_MountStatus.getLat(lat);
  for (int i = 0; i < 4 && !gotLst; ++i)
    gotLst = ta_MountStatus.getLstT0(lst);
  if (!gotLat || !gotLst)
  {
    DisplayMessage(T_GETVEALUE, T_FAILED, -1);
    return false;
  }
  cat_mgr.setLat(lat);
  cat_mgr.setLstT0(lst);
  return cat_mgr.isInitialized();
}

bool SmartHandController::gearCheckRecenterAndMeasure(double& miss1Deg, double& miss2Deg,
                                                     const char* prompt, bool wrapMiss)
{
  double before1 = 0, before2 = 0;
  if (!gearCheckReadAxes(before1, before2))
    return false;

  buttonPad.setControlerMode();
  DisplayMessage(T_RECENTER, prompt, 800);

  unsigned long lastDraw = 0;
  for (;;)
  {
    tickButtons();
    bool moving = false;
    manualMove(moving);
    const unsigned long now = millis();
    if (now - lastDraw > 250)
    {
      lastDraw = now;
      display->setFont(u8g2_font_helvR10_te);
      display->firstPage();
      do
      {
        display->drawUTF8(0, 20, T_GEARCHECK);
        display->drawUTF8(0, 40, prompt);
      }
      while (display->nextPage());
      display->setFont(u8g2_font_helvR12_te);
    }
    if (eventbuttons[0] == E_CLICK)
      break;
    delay(20);
  }

  buttonPad.setMenuMode();
  delay(150);
  double after1 = 0, after2 = 0;
  if (!gearCheckReadAxes(after1, after2))
    return false;

  miss1Deg = wrapMiss ? wrap180(after1 - before1) : (after1 - before1);
  miss2Deg = wrapMiss ? wrap180(after2 - before2) : (after2 - before2);
  return true;
}

SmartHandController::MENU_RESULT SmartHandController::menuGearCheckRotate()
{
  ta_MountStatus.updateMount();
  if (ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_UNDEFINED)
  {
    DisplayLongMessage("!" T_WARNING "!", NULL, T_MOUNTTYPE, T_NOTDEFINED "!", -1);
    return MR_CANCEL;
  }

  const uint8_t axisSel = display->UserInterfaceSelectionList(
    &buttonPad, T_GEARCHECK, 1, T_GC_AXIS1 "\n" T_GC_AXIS2);
  if (axisSel == 0)
    return MR_CANCEL;
  const int axis = (int)axisSel;

  const uint8_t angSel = display->UserInterfaceSelectionList(
    &buttonPad, axis == 1 ? T_GC_AXIS1 : T_GC_AXIS2, 1,
    T_GC_90 "\n" T_GC_180 "\n" T_GC_360);
  if (angSel == 0)
    return MR_CANCEL;
  const int degrees = (angSel == 1) ? 90 : (angSel == 2) ? 180 : 360;

  if (display->UserInterfaceMessage(&buttonPad, "!" T_WARNING "!",
        T_GC_NOLIMIT, T_GC_STOP, T_NO "\n" T_YES) != 2)
    return MR_CANCEL;

  DisplayLongMessage(T_GEARCHECK, T_GC_MARK, T_GC_THEN_OK, "", -1);

  double start1 = 0, start2 = 0;
  if (!gearCheckReadAxes(start1, start2))
  {
    DisplayMessage(T_GETVEALUE, T_FAILED, -1);
    return MR_CANCEL;
  }

  m_client->enableTracking(false);
  if (!DisplayMessageLX200(m_client->gearCheckRotate((uint8_t)axis, degrees), true))
    return MR_CANCEL;

  if (!gearCheckWaitSlewSettled(degrees))
  {
    DisplayMessage(T_SELECTION, T_ABORTED, -1);
    return MR_CANCEL;
  }

  double settle1 = 0, settle2 = 0;
  if (!gearCheckReadAxes(settle1, settle2))
  {
    DisplayMessage(T_GETVEALUE, T_FAILED, -1);
    return MR_CANCEL;
  }

  double miss1 = 0, miss2 = 0;
  if (!gearCheckRecenterAndMeasure(miss1, miss2, T_GC_ALIGN_MARK, false))
  {
    DisplayMessage(T_GETVEALUE, T_FAILED, -1);
    return MR_CANCEL;
  }

  const double start = (axis == 1) ? start1 : start2;
  const double settle = (axis == 1) ? settle1 : settle2;
  const double miss = (axis == 1) ? miss1 : miss2;
  const double cmd = settle - start;

  ta_MountStatus.updateAllConfig(true);
  const float gearCfg = ta_MountStatus.hasConfig()
    ? (ta_MountStatus.getCfgGear(axis - 1) / 1000.0f)
    : 0.0f;
  float gearMeas = 0;
  const bool gearValid = computeMeasuredGear(gearCfg, cmd, miss, gearMeas);

  char rowMiss[28], rowGear[28];
  snprintf(rowMiss, sizeof(rowMiss), "A%d %+.0f'", axis, miss * 60.0);
  if (gearValid)
    snprintf(rowGear, sizeof(rowGear), "%.1f>%.1f", (double)gearCfg, (double)gearMeas);
  else
    snprintf(rowGear, sizeof(rowGear), "%s", T_FAILED);
  DisplayLongMessage(T_GEARCHECK, rowMiss, rowGear, "", -1);

  if (gearValid &&
      display->UserInterfaceMessage(&buttonPad, T_GEAR, T_GC_SET, "", T_NO "\n" T_YES) == 2)
  {
    DisplayMessageLX200(m_client->writeTotGear((uint8_t)axis, gearMeas), false);
  }
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::gearCheckPickStar(
  NAV mode, int leg, int mountKind, bool gemSamePier,
  const float* prevRa, const float* prevDec, int nPrev,
  float& pickedRa, float& pickedDec)
{
  if (!gearCheckPrepareCatalog())
    return MR_CANCEL;

  cat_mgr.select(0);
  cat_mgr.filtersClear();
  cat_mgr.filterAdd(FM_OBJ_HAS_NAME);
  cat_mgr.filterAdd(FM_ABOVE_HORIZON, 1);
  cat_mgr.buildGearRank(mountKind, leg, gemSamePier, prevRa, prevDec, nPrev);

  char title[40] = "";
  if (mode == NAV_SYNC)
    strcpy(title, "Sync ");
  else if (mode == NAV_GOTO)
    strcpy(title, "Goto ");
  else
  {
    cat_mgr.filtersClear();
    return MR_CANCEL;
  }
  strcat(title, cat_mgr.catalogTitle());

  MENU_RESULT result = MR_CANCEL;
  if (!cat_mgr.isInitialized())
    DisplayMessage(cat_mgr.catalogTitle(), "Not Init'd?", -1);
  else if (!cat_mgr.setIndex(cat_mgr.getIndex()))
    DisplayMessage(cat_mgr.catalogTitle(), "No Object", -1);
  else
  {
    while (display->UserInterfaceCatalog(&buttonPad, title))
    {
      pickedRa = (float)cat_mgr.ra();
      pickedDec = (float)cat_mgr.dec();
      if (DisplayMessageLX200(SyncGotoCatLX200(*m_client, mode), false))
      {
        result = MR_QUIT;
        break;
      }
    }
  }
  cat_mgr.filtersClear();
  return result;
}

SmartHandController::MENU_RESULT SmartHandController::gearCheckRunLeg(
  const char* pickPrompt,
  int primaryAxis,
  int leg,
  int mountKind,
  bool gemSamePier,
  float* histRa,
  float* histDec,
  int& nHist,
  GearCheckLegResult& out)
{
  out = GearCheckLegResult();
  out.primaryAxis = primaryAxis;

  DisplayLongMessage(T_GEARCHECK, pickPrompt, T_SELECTASTAR, "", -1);

  double start1 = 0, start2 = 0;
  if (!gearCheckReadAxes(start1, start2))
  {
    DisplayMessage(T_GETVEALUE, T_FAILED, -1);
    return MR_CANCEL;
  }

  float pickedRa = 0, pickedDec = 0;
  if (gearCheckPickStar(NAV_GOTO, leg, mountKind, gemSamePier,
                        histRa, histDec, nHist, pickedRa, pickedDec) != MR_QUIT)
    return MR_CANCEL;
  if (nHist < 2)
  {
    histRa[nHist] = pickedRa;
    histDec[nHist] = pickedDec;
    nHist++;
  }

  // A catalog slew can cross a large part of the sky.
  if (!gearCheckWaitSlewSettled(240))
  {
    DisplayMessage(T_SELECTION, T_ABORTED, -1);
    return MR_CANCEL;
  }

  double settle1 = 0, settle2 = 0;
  if (!gearCheckReadAxes(settle1, settle2))
  {
    DisplayMessage(T_GETVEALUE, T_FAILED, -1);
    return MR_CANCEL;
  }

  if (!gearCheckRecenterAndMeasure(out.miss1Deg, out.miss2Deg, T_GC_RECENTER_OK, true))
  {
    DisplayMessage(T_GETVEALUE, T_FAILED, -1);
    return MR_CANCEL;
  }

  const double cmd1 = wrap180(settle1 - start1);
  const double cmd2 = wrap180(settle2 - start2);
  out.cmdPrimaryDeg = (primaryAxis == 1) ? cmd1 : cmd2;
  out.missPrimaryDeg = (primaryAxis == 1) ? out.miss1Deg : out.miss2Deg;

  ta_MountStatus.updateAllConfig(true);
  const float gearCfg = ta_MountStatus.hasConfig()
    ? (ta_MountStatus.getCfgGear(primaryAxis - 1) / 1000.0f)
    : 0.0f;
  out.gearCfg = gearCfg;
  out.gearValid = computeMeasuredGear(gearCfg, out.cmdPrimaryDeg, out.missPrimaryDeg, out.gearMeas);

  char rowMiss[28], rowGear[28], am[16];
  formatArcmin(am, sizeof(am), out.missPrimaryDeg);
  snprintf(rowMiss, sizeof(rowMiss), "A%d %s", primaryAxis, am);
  if (out.gearValid)
    snprintf(rowGear, sizeof(rowGear), "%.1f>%.1f", (double)out.gearCfg, (double)out.gearMeas);
  else
    snprintf(rowGear, sizeof(rowGear), "%s", T_FAILED);
  DisplayLongMessage(T_GEARCHECK, rowMiss, rowGear, "", -1);
  return MR_OK;
}

void SmartHandController::gearCheckReport(
  const GearCheckLegResult& leg1,
  const GearCheckLegResult& leg2,
  const char* label1,
  const char* label2)
{
  const double score1 = fabs(leg1.missPrimaryDeg);
  const double score2 = fabs(leg2.missPrimaryDeg);
  const double threshDeg = 2.0 / 60.0;

  char r1[28], r2[28];
  char am1[16], am2[16];
  formatArcmin(am1, sizeof(am1), leg1.missPrimaryDeg);
  formatArcmin(am2, sizeof(am2), leg2.missPrimaryDeg);
  snprintf(r1, sizeof(r1), "%s %s", label1, am1);
  snprintf(r2, sizeof(r2), "%s %s", label2, am2);

  const char* verdict = T_GC_SIMILAR;
  if (score1 >= threshDeg || score2 >= threshDeg)
  {
    if (score1 > 2.0 * score2 && score1 >= threshDeg)
      verdict = T_GC_CHECK_A1;
    else if (score2 > 2.0 * score1 && score2 >= threshDeg)
      verdict = T_GC_CHECK_A2;
    else if (score1 >= threshDeg && score2 >= threshDeg)
      verdict = (score1 >= score2) ? T_GC_CHECK_A1 : T_GC_CHECK_A2;
  }

  DisplayLongMessage(T_GC_LEG1, r1, T_GC_LEG2, r2, -1);
  DisplayLongMessage(T_GEARCHECK, verdict, "", "", -1);

  char g1[28], g2[28];
  if (leg1.gearValid)
    snprintf(g1, sizeof(g1), "A1 %.1f>%.1f", (double)leg1.gearCfg, (double)leg1.gearMeas);
  else
    snprintf(g1, sizeof(g1), "A1 %s", T_FAILED);
  if (leg2.gearValid)
    snprintf(g2, sizeof(g2), "A2 %.1f>%.1f", (double)leg2.gearCfg, (double)leg2.gearMeas);
  else
    snprintf(g2, sizeof(g2), "A2 %s", T_FAILED);
  DisplayLongMessage(T_GEAR, g1, g2, "", -1);
}

SmartHandController::MENU_RESULT SmartHandController::menuGearCheckEq(bool gemSamePier)
{
  DisplayLongMessage(T_GEARCHECK_EQ, T_GC_SYNC_FIRST, T_GC_THEN_GOTO, T_GC_RECENTER_EACH, -1);

  if (display->UserInterfaceMessage(&buttonPad, T_SYNC, T_STAR " A?", "", T_NO "\n" T_YES) != 2)
    return MR_CANCEL;

  float histRa[2] = {}, histDec[2] = {};
  int nHist = 0;
  float syncRa = 0, syncDec = 0;
  if (gearCheckPickStar(NAV_SYNC, 0, 0, gemSamePier, NULL, NULL, 0, syncRa, syncDec) != MR_QUIT)
    return MR_CANCEL;
  histRa[0] = syncRa;
  histDec[0] = syncDec;
  nHist = 1;

  GearCheckLegResult leg1, leg2;
  if (gearCheckRunLeg(T_GC_PICK_HA, 1, 1, 0, gemSamePier, histRa, histDec, nHist, leg1) != MR_OK)
    return MR_CANCEL;
  if (gearCheckRunLeg(T_GC_PICK_DEC, 2, 2, 0, gemSamePier, histRa, histDec, nHist, leg2) != MR_OK)
    return MR_CANCEL;

  gearCheckReport(leg1, leg2, "HA", "Dec");
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuGearCheckAltAz()
{
  DisplayLongMessage(T_GEARCHECK_AZ, T_GC_SYNC_FIRST, T_GC_THEN_GOTO, T_GC_RECENTER_EACH, -1);

  if (display->UserInterfaceMessage(&buttonPad, T_SYNC, T_STAR " A?", "", T_NO "\n" T_YES) != 2)
    return MR_CANCEL;

  float histRa[2] = {}, histDec[2] = {};
  int nHist = 0;
  float syncRa = 0, syncDec = 0;
  if (gearCheckPickStar(NAV_SYNC, 0, 1, false, NULL, NULL, 0, syncRa, syncDec) != MR_QUIT)
    return MR_CANCEL;
  histRa[0] = syncRa;
  histDec[0] = syncDec;
  nHist = 1;

  GearCheckLegResult leg1, leg2;
  if (gearCheckRunLeg(T_GC_PICK_AZ, 1, 1, 1, false, histRa, histDec, nHist, leg1) != MR_OK)
    return MR_CANCEL;
  if (gearCheckRunLeg(T_GC_PICK_ALT, 2, 2, 1, false, histRa, histDec, nHist, leg2) != MR_OK)
    return MR_CANCEL;

  gearCheckReport(leg1, leg2, "Az", "Alt");
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuGearCheckStars()
{
  ta_MountStatus.updateMount();
  const TeenAstroMountStatus::Mount mt = ta_MountStatus.getMount();
  if (mt == TeenAstroMountStatus::MOUNT_TYPE_GEM)
    return menuGearCheckEq(true);
  if (mt == TeenAstroMountStatus::MOUNT_TYPE_FORK)
    return menuGearCheckEq(false);
  if (mt == TeenAstroMountStatus::MOUNT_TYPE_ALTAZM || mt == TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT)
    return menuGearCheckAltAz();
  DisplayMessage(T_MOUNTTYPE, T_FAILED, -1);
  return MR_CANCEL;
}

SmartHandController::MENU_RESULT SmartHandController::menuGearCheck()
{
  ta_MountStatus.updateMount();
  if (ta_MountStatus.getMount() == TeenAstroMountStatus::MOUNT_UNDEFINED)
  {
    DisplayLongMessage("!" T_WARNING "!", NULL, T_MOUNTTYPE, T_NOTDEFINED "!", -1);
    return MR_CANCEL;
  }

  const uint8_t choice = display->UserInterfaceSelectionList(
    &buttonPad, T_GEARCHECK, 1, T_GC_STARS "\n" T_GC_ROTATE);
  if (choice == 0)
    return MR_CANCEL;
  if (choice == 1)
    return menuGearCheckStars();
  return menuGearCheckRotate();
}
