#include "SmartController.h"
#include "SHC_text.h"
#include <CommandCodec.h>
#include <TeenAstroPad.h>
#include <cmath>

// Gear-check goto ladder: sync A → goto B (mostly axis1) → recenter →
// goto C (mostly axis2) → recenter → report miss and measured gear.
// Measured gear = cfg_gear * commanded / true, with true = commanded + miss
// on the primary instrument axis (:GXP1# / :GXP2#).

static const double kMinCmdDeg = 5.0; // need a meaningful slew for scale

static double wrap180(double deg)
{
  while (deg > 180.0) deg -= 360.0;
  while (deg < -180.0) deg += 360.0;
  return deg;
}

static void formatArcmin(char* out, size_t len, double deg)
{
  const double am = deg * 60.0;
  snprintf(out, len, "%+.0f'", am);
}

static bool computeMeasuredGear(float gearCfg, double cmdDeg, double missDeg,
                                float& gearMeas)
{
  const double trueDeg = cmdDeg + missDeg;
  if (fabs(cmdDeg) < kMinCmdDeg || fabs(trueDeg) < 1e-3)
    return false;
  // Same sign expected for a mostly-axis move; still allow if true is usable.
  gearMeas = (float)((double)gearCfg * cmdDeg / trueDeg);
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
  if (!dmsToDouble(&a1Deg, b1, true, true))
    return false;
  if (!dmsToDouble(&a2Deg, b2, true, true))
    return false;
  return true;
}

bool SmartHandController::gearCheckWaitSlewSettled()
{
  DisplayMessage(T_GC_WAIT_SLEW, NULL, 500);
  const unsigned long t0 = millis();
  while (millis() - t0 < 180000UL)
  {
    ta_MountStatus.updateMount();
    if (ta_MountStatus.getTrackingState() != TeenAstroMountStatus::TRK_SLEWING)
    {
      delay(400);
      ta_MountStatus.updateMount();
      return true;
    }
    tickButtons();
    delay(80);
    if (eventbuttons[0] == E_LONGPRESS || eventbuttons[0] == E_LONGPRESSTART)
    {
      m_client->stopSlew();
      return false;
    }
  }
  return false;
}

bool SmartHandController::gearCheckRecenterAndMeasure(double& miss1Deg, double& miss2Deg)
{
  double before1 = 0, before2 = 0;
  if (!gearCheckReadAxes(before1, before2))
    return false;

  buttonPad.setControlerMode();
  DisplayMessage(T_RECENTER, T_GC_RECENTER_OK, 800);

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
        display->drawUTF8(0, 40, T_GC_RECENTER_OK);
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

  miss1Deg = wrap180(after1 - before1);
  miss2Deg = wrap180(after2 - before2);
  return true;
}

SmartHandController::MENU_RESULT SmartHandController::gearCheckRunLeg(
  const char* pickPrompt,
  int primaryAxis,
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

  MENU_RESULT cat = menuCatalogAlign(NAV_GOTO);
  if (cat == MR_CANCEL)
    return MR_CANCEL;

  if (!gearCheckWaitSlewSettled())
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

  // RecenterAndMeasure re-reads settle as "before" — same moment for miss.
  if (!gearCheckRecenterAndMeasure(out.miss1Deg, out.miss2Deg))
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

  char rowMiss[28], rowGear[28];
  char am[16];
  formatArcmin(am, sizeof(am), out.missPrimaryDeg);
  snprintf(rowMiss, sizeof(rowMiss), "A%d %s", primaryAxis, am);
  if (out.gearValid)
    snprintf(rowGear, sizeof(rowGear), "%s %.1f>%.1f", T_GEAR, (double)out.gearCfg, (double)out.gearMeas);
  else
    snprintf(rowGear, sizeof(rowGear), "%s %s", T_GEAR, T_FAILED);

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
  const double threshDeg = 2.0 / 60.0; // 2'

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

  // Measured gear: configured → estimated (from cmd/(cmd+miss)).
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

SmartHandController::MENU_RESULT SmartHandController::menuGearCheckEq()
{
  DisplayLongMessage(T_GEARCHECK_EQ, T_GC_SYNC_FIRST, T_GC_THEN_GOTO, T_GC_RECENTER_EACH, -1);

  if (display->UserInterfaceMessage(&buttonPad, T_SYNC, T_STAR " A?", "", T_NO "\n" T_YES) != 2)
    return MR_CANCEL;

  MENU_RESULT syncRes = menuSyncGoto(NAV_SYNC);
  if (syncRes == MR_CANCEL)
    return MR_CANCEL;

  GearCheckLegResult leg1, leg2;
  if (gearCheckRunLeg(T_GC_PICK_HA, 1, leg1) != MR_OK)
    return MR_CANCEL;
  if (gearCheckRunLeg(T_GC_PICK_DEC, 2, leg2) != MR_OK)
    return MR_CANCEL;

  gearCheckReport(leg1, leg2, "HA", "Dec");
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuGearCheckAltAz()
{
  DisplayLongMessage(T_GEARCHECK_AZ, T_GC_SYNC_FIRST, T_GC_THEN_GOTO, T_GC_RECENTER_EACH, -1);

  if (display->UserInterfaceMessage(&buttonPad, T_SYNC, T_STAR " A?", "", T_NO "\n" T_YES) != 2)
    return MR_CANCEL;

  MENU_RESULT syncRes = menuSyncGoto(NAV_SYNC);
  if (syncRes == MR_CANCEL)
    return MR_CANCEL;

  GearCheckLegResult leg1, leg2;
  if (gearCheckRunLeg(T_GC_PICK_AZ, 1, leg1) != MR_OK)
    return MR_CANCEL;
  if (gearCheckRunLeg(T_GC_PICK_ALT, 2, leg2) != MR_OK)
    return MR_CANCEL;

  gearCheckReport(leg1, leg2, "Az", "Alt");
  return MR_OK;
}

SmartHandController::MENU_RESULT SmartHandController::menuGearCheck()
{
  ta_MountStatus.updateMount();
  const TeenAstroMountStatus::Mount mt = ta_MountStatus.getMount();
  if (mt == TeenAstroMountStatus::MOUNT_UNDEFINED)
  {
    DisplayLongMessage("!" T_WARNING "!", NULL, T_MOUNTTYPE, T_NOTDEFINED "!", -1);
    return MR_CANCEL;
  }

  if (mt == TeenAstroMountStatus::MOUNT_TYPE_GEM || mt == TeenAstroMountStatus::MOUNT_TYPE_FORK)
    return menuGearCheckEq();
  if (mt == TeenAstroMountStatus::MOUNT_TYPE_ALTAZM || mt == TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT)
    return menuGearCheckAltAz();

  DisplayMessage(T_MOUNTTYPE, T_FAILED, -1);
  return MR_CANCEL;
}
