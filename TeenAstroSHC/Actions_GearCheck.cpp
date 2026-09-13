#include "SmartController.h"
#include "SHC_text.h"
#include <CommandCodec.h>
#include <TeenAstroPad.h>
#include <cmath>

// Gear-check goto ladder: sync A → goto B (mostly axis1) → recenter →
// goto C (mostly axis2) → recenter → report which axis miss grew.
// EQ and AltAz use separate prompts/labels; miss is from instrument axes
// (:GXP1# / :GXP2#) so it maps to motor gear / steps-per-degree.

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
      // Brief settle so the reported position is stable.
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
  double& miss1Deg,
  double& miss2Deg)
{
  DisplayLongMessage(T_GEARCHECK, pickPrompt, T_SELECTASTAR, "", -1);

  MENU_RESULT cat = menuCatalogAlign(NAV_GOTO);
  if (cat == MR_CANCEL)
    return MR_CANCEL;

  if (!gearCheckWaitSlewSettled())
  {
    DisplayMessage(T_SELECTION, T_ABORTED, -1);
    return MR_CANCEL;
  }

  if (!gearCheckRecenterAndMeasure(miss1Deg, miss2Deg))
  {
    DisplayMessage(T_GETVEALUE, T_FAILED, -1);
    return MR_CANCEL;
  }

  char row1[28], row2[28];
  char am1[16], am2[16];
  formatArcmin(am1, sizeof(am1), miss1Deg);
  formatArcmin(am2, sizeof(am2), miss2Deg);
  snprintf(row1, sizeof(row1), "A1 %s", am1);
  snprintf(row2, sizeof(row2), "A2 %s", am2);
  DisplayLongMessage(T_GEARCHECK, row1, row2, "", -1);
  return MR_OK;
}

void SmartHandController::gearCheckReport(
  double miss1Leg1,
  double miss2Leg1,
  double miss1Leg2,
  double miss2Leg2,
  const char* label1,
  const char* label2)
{
  // Primary miss on the intended axis of each leg.
  const double score1 = fabs(miss1Leg1);
  const double score2 = fabs(miss2Leg2);
  const double threshDeg = 2.0 / 60.0; // 2'

  char r1[28], r2[28];
  char am1[16], am2[16];
  formatArcmin(am1, sizeof(am1), miss1Leg1);
  formatArcmin(am2, sizeof(am2), miss2Leg2);
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

  // Keep cross-axis numbers available if useful (leg1 A2 / leg2 A1).
  (void)miss2Leg1;
  (void)miss1Leg2;
}

SmartHandController::MENU_RESULT SmartHandController::menuGearCheckEq()
{
  DisplayLongMessage(T_GEARCHECK_EQ, T_GC_SYNC_FIRST, T_GC_THEN_GOTO, T_GC_RECENTER_EACH, -1);

  if (display->UserInterfaceMessage(&buttonPad, T_SYNC, T_STAR " A?", "", T_NO "\n" T_YES) != 2)
    return MR_CANCEL;

  MENU_RESULT syncRes = menuSyncGoto(NAV_SYNC);
  if (syncRes == MR_CANCEL)
    return MR_CANCEL;

  double m11 = 0, m12 = 0, m21 = 0, m22 = 0;
  if (gearCheckRunLeg(T_GC_PICK_HA, m11, m12) != MR_OK)
    return MR_CANCEL;
  if (gearCheckRunLeg(T_GC_PICK_DEC, m21, m22) != MR_OK)
    return MR_CANCEL;

  gearCheckReport(m11, m12, m21, m22, "HA", "Dec");
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

  double m11 = 0, m12 = 0, m21 = 0, m22 = 0;
  if (gearCheckRunLeg(T_GC_PICK_AZ, m11, m12) != MR_OK)
    return MR_CANCEL;
  if (gearCheckRunLeg(T_GC_PICK_ALT, m21, m22) != MR_OK)
    return MR_CANCEL;

  gearCheckReport(m11, m12, m21, m22, "Az", "Alt");
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

  // Explicit GEM/FORK vs AltAz — same care as menuAlignment (do not use isAltAz()).
  if (mt == TeenAstroMountStatus::MOUNT_TYPE_GEM || mt == TeenAstroMountStatus::MOUNT_TYPE_FORK)
    return menuGearCheckEq();
  if (mt == TeenAstroMountStatus::MOUNT_TYPE_ALTAZM || mt == TeenAstroMountStatus::MOUNT_TYPE_FORK_ALT)
    return menuGearCheckAltAz();

  DisplayMessage(T_MOUNTTYPE, T_FAILED, -1);
  return MR_CANCEL;
}
