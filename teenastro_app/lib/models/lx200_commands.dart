/// Reply types for LX200 commands (mirrors CMDREPLY enum from CommandEnums.h)
enum CmdReply { none, short_, shortBool, long_, invalid }

/// Goto error codes (mirrors ErrorsGoTo from CommandEnums.h)
enum GotoError {
  none, belowHorizon, noObjectSelected, sameSide, parked, slewing,
  limits, guidingBusy, aboveOverhead, motor, unknown10, motorFault,
  alt, limitSense, axis1, axis2, underPole, meridian
}

/// Human-readable root cause for goto/slew failures.
String gotoErrorCause(GotoError e) {
  switch (e) {
    case GotoError.none:
      return 'OK';
    case GotoError.belowHorizon:
      return 'Target is below the horizon';
    case GotoError.noObjectSelected:
      return 'No target selected';
    case GotoError.sameSide:
      return 'Target on same pier side (meridian flip may be needed)';
    case GotoError.parked:
      return 'Mount is parked; unpark first';
    case GotoError.slewing:
      return 'Mount is already slewing';
    case GotoError.limits:
      return 'Target outside slew limits';
    case GotoError.guidingBusy:
      return 'Guiding in progress';
    case GotoError.aboveOverhead:
      return 'Target above overhead limit';
    case GotoError.motor:
    case GotoError.motorFault:
      return 'Motor fault';
    case GotoError.alt:
      return 'Altitude limit';
    case GotoError.limitSense:
      return 'Limit sensor triggered';
    case GotoError.axis1:
      return 'Axis 1 limit';
    case GotoError.axis2:
      return 'Axis 2 limit';
    case GotoError.underPole:
      return 'Target under the pole';
    case GotoError.meridian:
      return 'Meridian limit (flip or reposition)';
    case GotoError.unknown10:
      return 'Goto rejected by mount';
  }
}

/// Navigation mode
enum NavMode { sync, goto_, pushTo }

/// LX200 command constants
class LX200 {
  // Movement
  static const moveNorth = ':Mn#';
  static const moveSouth = ':Ms#';
  static const moveEast  = ':Me#';
  static const moveWest  = ':Mw#';
  static const stopNorth = ':Qn#';
  static const stopSouth = ':Qs#';
  static const stopEast  = ':Qe#';
  static const stopWest  = ':Qw#';
  static const stopAll   = ':Q#';
  static const meridianFlip = ':MF#';

  // Speed
  static String setSpeed(int level) => ':R$level#';

  // Tracking
  static const trackOn  = ':Te#';
  static const trackOff = ':Td#';
  static const trackSidereal = ':TQ#';
  static const trackLunar    = ':TL#';
  static const trackSolar    = ':TS#';
  static const trackUser     = ':TT#';
  static const trackIncr     = ':T+#';
  static const trackDecr     = ':T-#';
  static const trackReset    = ':TR#';
  static const trackDualOn   = ':T2#';
  static const trackDualOff  = ':T1#';

  // Park / Home
  static const park     = ':hP#';
  static const unpark   = ':hR#';
  static const setPark  = ':hQ#';
  static const goHome   = ':hC#';
  static const setHome  = ':hB#';
  static const resetHome = ':hb#';

  // Bulk all-state query (48-byte binary, base64-encoded, 64 chars + '#')
  static const getAllState  = ':GXAS#';
  static const getAllConfig = ':GXCS#';

  // Status
  static const getStatus  = ':GXI#';
  static const getRa      = ':GR#';
  static const getDec     = ':GD#';
  static const getHa      = ':GXT3#';
  static const getAz      = ':GZ#';
  static const getAlt     = ':GA#';
  static const getUtcTime = ':GXT0#';
  static const getUtcDate = ':GXT1#';
  static const getSidereal = ':GS#';
  static const getTargetRa  = ':Gr#';
  static const getTargetDec = ':Gd#';

  // Version
  static const getProductName  = ':GVP#';
  static const getVersionNum   = ':GVN#';
  static const getVersionDate  = ':GVD#';
  static const getBoardVersion = ':GVB#';
  static const getDriverType   = ':GVb#';

  // Target setting
  static String setTargetRa(String hms)  => ':Sr$hms#';
  static String setTargetDec(String dms) => ':Sd$dms#';
  static String setTargetAz(String dms)  => ':Sz$dms#';
  static String setTargetAlt(String dms) => ':Sa$dms#';

  // Goto / Sync
  static const gotoTarget     = ':MS#';
  static const gotoTargetJNow = ':MA#';
  static const syncTarget     = ':CM#';
  static const pushToTarget   = ':EMS#';
  static const gotoUser       = ':MU#';
  static const syncUser       = ':CU#';

  // Alignment
  static const alignStart      = ':A0#';
  static const alignAcceptStar = ':A*#';
  static const alignAtHome     = ':AA#';
  static const alignSave       = ':AW#';
  static const alignClear      = ':AC#';
  static const getAlignError   = ':AE#';
  /// Add alignment star n (OnStepX-style: n=1 first star, n=2 second, n=3 third, etc.)
  static String alignAddStar(int n) => ':A$n#';
  static const pierEast  = ':SmE#';
  static const pierWest  = ':SmW#';
  static const pierNone  = ':SmN#';

  // Focuser
  static const focuserIn   = ':F-#';
  static const focuserOut  = ':F+#';
  static const focuserStop = ':FQ#';

  // Site / Location reading
  static const getLatitude  = ':Gtf#';
  static const getLongitude = ':Ggf#';
  static const getElevation = ':Ge#';

  // Time / Date setting
  static String setLocalTime(String hms)   => ':SL$hms#';
  static String setDate(String mmddyy)     => ':SC$mmddyy#';
  static String setTimeZone(String offset) => ':SG$offset#';

  // Location setting
  static String setLatitude(String dms)  => ':St$dms#';
  static String setLongitude(String dms) => ':Sg$dms#';

  // GNSS sync (TeenAstro-specific)
  static const gnssSyncFull = ':gs#';
  static const gnssSyncTime = ':gt#';

  // Refraction
  static const getRefractionEnabled = ':GXrt#';
  static String enableRefraction(bool on) => ':SXrt,${on ? "y" : "n"}#';

  // Spiral
  static String spiral(int fovArcMin) =>
      ':M@${fovArcMin.toString().padLeft(3, '0')}#';

  // Reboot / Factory
  static const reboot = ':\$!#';
  static const factoryReset = ':\$\$#';
}

/// Determine expected reply type for a command string.
/// Ported from CommandMeta.h getReplyType().
CmdReply getReplyType(String cmd) {
  if (cmd.isEmpty) return CmdReply.invalid;
  if (cmd[0] != ':') return CmdReply.invalid;
  if (cmd.length < 3) return CmdReply.invalid;

  final c1 = cmd[1];
  final c2 = cmd[2];

  switch (c1) {
    case 'A': // Alignment (:A0# :A1# .. :A9# :A*# :AE# etc.)
      if ('*0123456789CWA'.contains(c2)) return CmdReply.shortBool;
      if (c2 == 'E') return CmdReply.long_;
      return CmdReply.invalid;
    case 'B': // Reticule
      if ('+-'.contains(c2)) return CmdReply.none;
      return CmdReply.invalid;
    case 'C': // Sync
      if ('AMU'.contains(c2)) return CmdReply.long_;
      if (c2 == 'S') return CmdReply.none;
      return CmdReply.invalid;
    case 'D': // Distance
      if (c2 == '#') return CmdReply.long_;
      return CmdReply.invalid;
    case 'E': // Encoder
      if ('ACD'.contains(c2)) return CmdReply.long_;
      if (c2 == 'M' && cmd.length > 3 && 'ASUQ'.contains(cmd[3])) return CmdReply.short_;
      return CmdReply.invalid;
    case 'F': // Focuser
      if ('+-gGPQsS\$!'.contains(c2)) return CmdReply.none;
      if ('OoIi:012345678cCmrW'.contains(c2)) return CmdReply.shortBool;
      if ('x?~MV'.contains(c2)) return CmdReply.long_;
      return CmdReply.invalid;
    case 'g': // GNSS
      if ('ts'.contains(c2)) return CmdReply.shortBool;
      return CmdReply.invalid;
    case 'G': // Get
      if ('AaCcDdefgGhLMNOPmnoRrSTtVXWZ'.contains(c2)) return CmdReply.long_;
      return CmdReply.invalid;
    case 'h': // Home/Park
      if (c2 == 'F') return CmdReply.none;
      if ('BbCOPQRS'.contains(c2)) return CmdReply.shortBool;
      return CmdReply.invalid;
    case 'M': // Move/Slew
      if ('ewnsg'.contains(c2)) return CmdReply.none;
      if ('SAUF?'.contains(c2)) return CmdReply.short_;
      if ('12@'.contains(c2)) return CmdReply.shortBool;
      return CmdReply.invalid;
    case 'Q': // Halt
      if ('#ewns'.contains(c2)) return CmdReply.none;
      return CmdReply.invalid;
    case 'R': // Rate
      if ('GCMS01234'.contains(c2)) return CmdReply.none;
      return CmdReply.invalid;
    case 'S': // Set
      if ('!aBCedgGhLmMnNoOrtTUXz'.contains(c2)) return CmdReply.shortBool;
      return CmdReply.invalid;
    case 'T': // Tracking
      if ('R+-TSLQ'.contains(c2)) return CmdReply.none;
      if ('ed012'.contains(c2)) return CmdReply.shortBool;
      return CmdReply.invalid;
    case 'U': // Precision
      if (c2 == '#') return CmdReply.none;
      return CmdReply.invalid;
    case 'W': // Site
      if ('0123'.contains(c2)) return CmdReply.none;
      if (c2 == '?') return CmdReply.long_;
      return CmdReply.invalid;
    case '\$': // Reset
      if ('\$!'.contains(c2)) return CmdReply.none;
      if (c2 == 'X') return CmdReply.shortBool;
      return CmdReply.invalid;
    default:
      return CmdReply.invalid;
  }
}
