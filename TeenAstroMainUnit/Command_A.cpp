/**
 * A - Alignment. One file per letter (plan).
 */
#include "Command.h"
#include "ValueToString.h"

namespace {

void alignmentPolarFinalizeFromCurrentTarget()
{
  // Mechanical pole pass complete: discard the provisional two-star soft model
  // and rebuild RAM to the cold-boot baseline (synthetic refs for an ideal
  // polar mount, hasValid=false, EE_Tvalid cleared). Mount stays synced on the
  // recentered alignment star so :MS# pointing remains correct against the now
  // mechanically-true pole.
  double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
  double Lat = *localSite.latitude();
  Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
  Coord_HO HO_T = EQ_T.To_Coord_HO(Lat * DEG_TO_RAD, mount.refrOptForGoto());

  // Cold-baseline conv: clean(), seed synthetic ideal-polar refs, hasValid=false,
  // EE_Tvalid cleared in EEPROM. See initTransformation() in EEPROM.cpp.
  initTransformation(true);

  // Sync encoders so the current observing direction == alignment star.
  mount.syncAzAlt(&HO_T, mount.getPoleSide());

  mount.alignment.alignPolarThirdPending = false;
  mount.alignment.alignPhase   = ALIGN_IDLE;
  mount.alignment.alignStarNum = 0;
  mount.alignment.alignNumStarsSession = 2;
  mount.config.peripherals.PushtoStatus = PT_OFF;
}

/// Parse an ",r<n>" suffix requesting a rigid six degree of freedom session.
/// Returns the star count (COORDCONV_MIN_RIGID_STARS..COORDCONV_MAX_STARS) or 0
/// when the suffix is absent. \p bad is set when the count is out of range.
uint8_t parseRigidSuffix(const char *cmd, bool &bad)
{
  bad = false;
  if (cmd[0] != ',' || cmd[1] != 'r')
    return 0;
  const char n = cmd[2];
  if (n < '0' || n > '9' || cmd[3] != 0)
  {
    bad = true;
    return 0;
  }
  const uint8_t stars = (uint8_t)(n - '0');
  if (stars < COORDCONV_MIN_RIGID_STARS || stars > COORDCONV_MAX_STARS)
  {
    bad = true;
    return 0;
  }
  return stars;
}

/// Close out an alignment session: refine T with the legacy minimisers, or run
/// the rigid fit when enough stars were collected, then sync on the last star.
void alignmentFinalize(Coord_HO &HO_T, double Lat)
{
  MountAlignment &al = mount.alignment;

  // The rigid fit replaces the legacy minimize* fudges only if it actually
  // solved something. It drops any head term the star distribution cannot
  // separate, and a set clustered in altitude can leave it with none at all;
  // in that case we must still fall back, or a long rigid session would end up
  // worse than the two star path it was meant to improve on.
  bool fitted = false;
  if (al.isRigidSession() && al.conv.getStars() >= COORDCONV_MIN_RIGID_STARS)
  {
    // Up to six unknowns: T contributes three, the head at most three more. The
    // two star Taki solution already seeded T, so it converges in a couple of
    // iterations.
    fitted = fitRigidAlignModel() && al.conv.hasHead();
  }
  if (!fitted)
  {
    al.conv.minimizeAxis2();
    al.conv.minimizeAxis1(mount.config.identity.mountType == MOUNT_TYPE_GEM ? (Lat >= 0 ? M_PI_2 : -M_PI_2) : 0);
  }
  mount.syncAzAlt(&HO_T, mount.getPoleSide());
  al.hasValid = true;
  al.alignPhase   = ALIGN_IDLE;
  al.alignStarNum = 0;
  al.alignPolarThirdPending = false;
}

} // namespace

// -----------------------------------------------------------------------------
//   A - Alignment  :A0# :A0,2# :A0,m# :A*# :A*,m# :A1# .. :A9# :AP# :AB# :AC# :AA# :AE# :AW#
// -----------------------------------------------------------------------------
void Command_A() {
  switch (commandState.command[1]) {
  case '0': {
    // :A0#  LX200 standard (alignment menu 0); :A0,2# two-star; :A0,m# mechanical pole (two-star + bolt pass)
    // :A0,r<n># rigid six degree of freedom session with n=3..9 stars (TeenAstro extension)
    bool rigidBad = false;
    const uint8_t rigidStars = parseRigidSuffix(&commandState.command[2], rigidBad);
    if (rigidBad) {
      replyNothing();
      break;
    }
    if (rigidStars == 0) {
      if (commandState.command[2] == ',' && commandState.command[3] == '3' && commandState.command[4] == 0) {
        replyNothing();
        break;
      }
      if (commandState.command[2] == ',' && commandState.command[4] != 0) {
        replyNothing();
        break;
      }
      if (commandState.command[2] == ',' && commandState.command[3] != '2' && commandState.command[3] != 'm') {
        replyNothing();
        break;
      }
    }
    uint8_t numStarsSession = 2;
    if (rigidStars != 0)
      numStarsSession = rigidStars;
    else if (commandState.command[2] == ',' && commandState.command[3] == 'm' && commandState.command[4] == 0)
      numStarsSession = 3;
    else if (commandState.command[2] == ',' && commandState.command[3] == '2' && commandState.command[4] == 0)
      numStarsSession = 2;
    initTransformation(true);
    mount.syncAtHome();
    mount.axes.enable(true);
    delay(10);
    if (mount.motorsEncoders.enableMotor)
      mount.startSideralTracking();
    mount.alignment.alignPhase   = ALIGN_SELECT;
    mount.alignment.alignStarNum = 1;
    mount.alignment.alignStarName[0] = '\0';
    mount.alignment.alignPolarThirdPending = false;
    mount.alignment.alignNumStarsSession = numStarsSession;
    mount.alignment.alignRigidStars = rigidStars;
    replyShortTrue();
    break;
  }
  case '*': {
    // :A*#  LX200 standard (telescope at target); :A*,m# same + mechanical pole session (defer on 2nd star)
    // :A*,r<n># rigid six degree of freedom session with n=3..9 stars (TeenAstro extension)
    bool rigidBadStar = false;
    const uint8_t rigidStarsAtTarget = parseRigidSuffix(&commandState.command[2], rigidBadStar);
    if (rigidBadStar) {
      replyNothing();
      break;
    }
    if (rigidStarsAtTarget == 0) {
      if (commandState.command[2] == ',' && commandState.command[3] == '3' && commandState.command[4] == 0) {
        replyNothing();
        break;
      }
      if (commandState.command[2] == ',' && commandState.command[4] != 0) {
        replyNothing();
        break;
      }
      if (commandState.command[2] == ',' && commandState.command[3] != 'm') {
        replyNothing();
        break;
      }
    }
    const bool mechanicalPole = (commandState.command[2] == ',' && commandState.command[3] == 'm' && commandState.command[4] == 0);
    initTransformation(true);
    mount.axes.enable(true);
    delay(10);
    if (mount.motorsEncoders.enableMotor)
      mount.startSideralTracking();
    PoleSide targetPoleSide = mount.getPoleSide();
    if (mount.targetCurrent.newTargetPoleSide != POLE_NOTVALID) {
      targetPoleSide = mount.targetCurrent.newTargetPoleSide;
      mount.targetCurrent.newTargetPoleSide = POLE_NOTVALID;
    }
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO(Lat * DEG_TO_RAD, mount.refrOptForGoto());
    mount.syncAzAlt(&HO_T, targetPoleSide);
    Coord_IN IN_T = mount.getInstr();
    mount.alignment.conv.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    mount.alignment.alignPhase   = ALIGN_SELECT;
    mount.alignment.alignStarNum = 2;
    mount.alignment.alignNumStarsSession = rigidStarsAtTarget != 0 ? rigidStarsAtTarget : (mechanicalPole ? 3 : 2);
    mount.alignment.alignRigidStars = rigidStarsAtTarget;
    mount.alignment.alignPolarThirdPending = false;
    replyShortTrue();
    break;
  }
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    uint8_t starIdx = commandState.command[1] - '0';
    double newTargetHA = haRange(rtk.LST() * 15.0 - mount.targetCurrent.newTargetRA);
    double Lat = *localSite.latitude();
    Coord_EQ EQ_T(0, mount.targetCurrent.newTargetDec * DEG_TO_RAD, newTargetHA * DEG_TO_RAD);
    Coord_HO HO_T = EQ_T.To_Coord_HO(Lat * DEG_TO_RAD, mount.refrOptForGoto());

    const bool rigidSession = mount.alignment.isRigidSession();

    // :A3# does not finalize the polar pass (that is :AP#), and :A3# outside any
    // session would corrupt conv. A rigid session is the one case where :A3# is
    // legitimate: stars three and up are what the six parameter fit needs.
    if (starIdx == 3 && !rigidSession) {
      replyNothing();
      break;
    }

    // Only the very first star of a session may sync the encoders; after that
    // the axes must be read where they actually are, which is what makes the
    // residuals meaningful. getRefs() alone is not enough to spot the first
    // star: it drops back to 0 once the two star pass has built T.
    if (mount.alignment.conv.getRefs() == 0 && mount.alignment.conv.getStars() == 0)
      mount.syncAzAlt(&HO_T, mount.getPoleSide());
    Coord_IN IN_T = mount.getInstr();

    // Every star is recorded for the rigid fit, but only the first two seed T
    // through the two star pass. Feeding it more would restart it and leave T
    // rebuilt from an arbitrary pair.
    const bool seedTaki = !rigidSession || mount.alignment.conv.getStars() < 2;
    mount.alignment.conv.addStar(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    if (seedTaki)
      mount.alignment.conv.addReference(HO_T.direct_Az_S(), HO_T.Alt(), IN_T.Axis1_direct(), IN_T.Axis2());
    if (rigidSession && starIdx < mount.alignment.alignRigidStars) {
      // Still collecting. The first two stars already seeded T, so gotos work
      // and the user can slew to the next star, but the model is not final.
      mount.alignment.alignPhase   = ALIGN_SELECT;
      mount.alignment.alignStarNum = starIdx + 1;
      mount.alignment.hasValid = mount.alignment.conv.isReady();
    } else if (mount.alignment.conv.isReady()) {
      const bool deferThird = !rigidSession && !mount.isAltAZ() && mount.alignment.alignNumStarsSession >= 3 && starIdx == 2;
      if (deferThird) {
        mount.syncAzAlt(&HO_T, mount.getPoleSide());
        mount.alignment.alignPolarThirdPending = true;
        mount.alignment.alignPhase   = ALIGN_RECENTER;
        mount.alignment.alignStarNum = 3;
        mount.alignment.hasValid = false;
      } else {
        alignmentFinalize(HO_T, Lat);
      }
    } else {
      mount.alignment.alignPhase   = ALIGN_SELECT;
      mount.alignment.alignStarNum = starIdx + 1;
    }
    mount.config.peripherals.PushtoStatus = PT_OFF;
    replyShortTrue();
    break;
  }
  case 'P':
    // :AP#  Polar / mechanical bolt pass complete (after :A0,m# / :A*,m# defer); TeenAstro extension
    if (commandState.command[2] != 0) {
      replyNothing();
      break;
    }
    if (!mount.alignment.alignPolarThirdPending) {
      replyNothing();
      break;
    }
    alignmentPolarFinalizeFromCurrentTarget();
    replyShortTrue();
    break;
  case 'E': {
    // :AE#  LX200 standard (alignment error / AltAz)
    double val = mount.alignment.conv.getError() * RAD_TO_DEG;
    doubleToDms(commandState.reply, &val, false, true, true);
    strcat(commandState.reply, "#");
    break;
  }
  case 'C':
  case 'A':
    // :AC# :AA#  LX200 standard
    initTransformation(true);
    mount.syncAtHome();
    mount.alignment.autoAlignmentBySync = (commandState.command[1] == 'A');
    mount.alignment.alignPhase   = ALIGN_IDLE;
    mount.alignment.alignStarNum = 0;
    mount.alignment.alignPolarThirdPending = false;
    replyShortTrue();
    break;
  case 'B':
    // :AB#  Abort alignment in progress (clear refs without syncing at home).
    // Must reseed synthetic T/Tinv — conv.clean() alone zeros T and leaves Tinv
    // stale, so reported Alt/Az and RA/Dec diverge until the next initTransformation.
    initTransformation(true);
    mount.alignment.alignPhase   = ALIGN_IDLE;
    mount.alignment.alignStarNum = 0;
    mount.alignment.alignPolarThirdPending = false;
    mount.alignment.alignStarName[0] = '\0';
    mount.alignment.autoAlignmentBySync = false;
    replyShortTrue();
    break;
  case 'W':
    // :AW#  LX200 standard (save alignment)
    saveAlignModel();
    replyShortTrue();
    break;
  default:
    replyNothing();
    break;
  }
}
