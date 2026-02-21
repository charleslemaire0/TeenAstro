import 'dart:convert';
import 'dart:typed_data';

/// Mount state enums and model - mirrors TeenAstroMountStatus

enum TrackState { off, on, slewing, unknown }
enum SiderealMode { star, sun, moon, target, unknown }
enum ParkState { unparked, parked, failed, parking, unknown }
enum PierSide { east, west, unknown }
enum MountType { gem, fork, altAz, forkAlt, undefined }
enum SpeedLevel { guide, slow, medium, fast, max, unknown }
enum MountError { none, motorFault, alt, limitSense, limitA1, limitA2, underPole, meridian, sync }

class MountState {
  final TrackState tracking;
  final SiderealMode sidereal;
  final ParkState parkState;
  final bool atHome;
  final SpeedLevel speed;
  final bool spiralRunning;
  final bool pulseGuiding;
  final String guidingEW; // '>' east, '<' west, ' ' none
  final String guidingNS; // '^' north, '_' south, ' ' none
  final bool trackCorrected;
  final bool aligned;
  final MountType mountType;
  final PierSide pierSide;
  final int gnssFlags;
  final MountError error;
  final int enableFlags;
  final bool valid;

  // Position strings
  final String ra;
  final String dec;
  final String ha;
  final String az;
  final String alt;
  final String targetRa;
  final String targetDec;

  // Time strings (from :GXAS# bytes 6-11; local from UTC + byte 62 timezone)
  final String utcTime;
  final String utcDate;
  final String localTime;
  final String localDate;
  final String siderealTime;
  /// Timezone offset in hours (local = UTC − timezoneOffset). From GXAS byte 62.
  final double timezoneOffset;

  // Version info
  final String productName;
  final String versionNum;
  final String boardVersion;
  final String driverType;

  // Site location (degrees, from :Gtf# / :Ggf#)
  final double latitude;
  final double longitude;

  // Focuser
  final String focuserStatus;
  final bool focuserConnected;
  final int focuserPos;    // steps (from :GXAS# binary packet)
  final int focuserSpeed;  // speed units (from :GXAS# binary packet)

  // Tracking rates (×10000, from :GXAS# bytes 48-63; same as :GXRr#/:GXRd#/:GXRe#/:GXRf#)
  final int trackRateRa;
  final int trackRateDec;
  final int storedTrackRateRa;
  final int storedTrackRateDec;

  const MountState({
    this.tracking = TrackState.unknown,
    this.sidereal = SiderealMode.unknown,
    this.parkState = ParkState.unknown,
    this.atHome = false,
    this.speed = SpeedLevel.unknown,
    this.spiralRunning = false,
    this.pulseGuiding = false,
    this.guidingEW = ' ',
    this.guidingNS = ' ',
    this.trackCorrected = false,
    this.aligned = false,
    this.mountType = MountType.undefined,
    this.pierSide = PierSide.unknown,
    this.gnssFlags = 0,
    this.error = MountError.none,
    this.enableFlags = 0,
    this.valid = false,
    this.ra = '?',
    this.dec = '?',
    this.ha = '?',
    this.az = '?',
    this.alt = '?',
    this.targetRa = '',
    this.targetDec = '',
    this.utcTime = '?',
    this.utcDate = '?',
    this.localTime = '?',
    this.localDate = '?',
    this.siderealTime = '?',
    this.timezoneOffset = 0.0,
    this.productName = '',
    this.versionNum = '',
    this.boardVersion = '',
    this.driverType = '',
    this.latitude = 0.0,
    this.longitude = 0.0,
    this.focuserStatus = '',
    this.focuserConnected = false,
    this.focuserPos = 0,
    this.focuserSpeed = 0,
    this.trackRateRa = 0,
    this.trackRateDec = 0,
    this.storedTrackRateRa = 0,
    this.storedTrackRateDec = 0,
  });

  // GNSS convenience
  bool get hasGNSS => (gnssFlags & 0x01) != 0;
  bool get gnssValid => (gnssFlags & 0x02) != 0;
  bool get gnssTimeSync => (gnssFlags & 0x04) != 0;
  bool get gnssLocationSync => (gnssFlags & 0x08) != 0;

  // Enable convenience
  bool get encodersEnabled => (enableFlags & 0x01) != 0;
  bool get motorsEnabled => (enableFlags & 0x08) != 0;
  bool get isAltAz => mountType == MountType.altAz || mountType == MountType.forkAlt;
  bool get isGEM => mountType == MountType.gem;
  bool get isParked => parkState == ParkState.parked;
  bool get isTracking => tracking == TrackState.on;
  bool get isSlewing => tracking == TrackState.slewing;

  // ---------------------------------------------------------------------------
  // Coordinate format helpers
  // ---------------------------------------------------------------------------

  static String _p2(int n) => n.toString().padLeft(2, '0');

  /// Convert UTC (h,m,s,month,day,year2) + timezone offset (hours) to local (timeStr, dateStr).
  /// Convention: local = UTC − timezoneOffset.
  static (String, String) _utcToLocal(
    int utcH, int utcM, int utcS, int utcMo, int utcD, int utcY, double tzHours,
  ) {
    int utcSec = utcH * 3600 + utcM * 60 + utcS;
    int rawLocalSec = utcSec - (tzHours * 3600).round();
    int dayDelta = 0;
    if (rawLocalSec < 0) dayDelta = -1;
    if (rawLocalSec >= 86400) dayDelta = 1;
    while (rawLocalSec < 0) rawLocalSec += 86400;
    while (rawLocalSec >= 86400) rawLocalSec -= 86400;
    int lh = rawLocalSec ~/ 3600;
    int lm = (rawLocalSec % 3600) ~/ 60;
    int ls = rawLocalSec % 60;
    final timeStr = '${_p2(lh)}:${_p2(lm)}:${_p2(ls)}';

    int d = utcD + dayDelta;
    int mo = utcMo;
    int y = utcY;
    const dim = [0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];
    int dims(int month, int year2) {
      final fullYear = 2000 + year2;
      if (month == 2 && (fullYear % 4 == 0 && (fullYear % 100 != 0 || fullYear % 400 == 0))) return 29;
      return dim[month];
    }
    if (d < 1) {
      mo--;
      if (mo < 1) { mo = 12; y--; }
      d = dims(mo, y);
    } else if (d > dims(mo, y)) {
      d = 1;
      mo++;
      if (mo > 12) { mo = 1; y++; }
    }
    final dateStr = '${_p2(mo)}/${_p2(d)}/${_p2(y)}';
    return (timeStr, dateStr);
  }

  static String _formatRa(double hours) {
    while (hours < 0) hours += 24;
    while (hours >= 24) hours -= 24;
    final h  = hours.floor();
    final mf = (hours - h) * 60;
    final m  = mf.floor();
    var   s  = ((mf - m) * 60).round();
    if (s >= 60) s = 59;
    return '${_p2(h)}:${_p2(m)}:${_p2(s)}';
  }

  static String _formatDeg(double deg) {
    final sign = deg >= 0 ? '+' : '-';
    final abs  = deg.abs();
    final d    = abs.floor();
    final mf   = (abs - d) * 60;
    final m    = mf.floor();
    var   s    = ((mf - m) * 60).round();
    if (s >= 60) s = 59;
    return '$sign${_p2(d)}*${_p2(m)}:${_p2(s)}';
  }

  static String _formatAz(double deg) {
    while (deg < 0) deg += 360;
    while (deg >= 360) deg -= 360;
    final d  = deg.floor();
    final mf = (deg - d) * 60;
    final m  = mf.floor();
    var   s  = ((mf - m) * 60).round();
    if (s >= 60) s = 59;
    return '${d.toString().padLeft(3, '0')}*${_p2(m)}:${_p2(s)}';
  }

  // ---------------------------------------------------------------------------
  // Parse binary :GXAS# response (88-char base64 → 66-byte packet)
  // ---------------------------------------------------------------------------
  //
  // Packet layout (little-endian): fixed data first; optional focuser before checksum.
  //   Bytes 12-39: positions (7 × float32). Bytes 40-55: tracking rates (4 × int32 LE).
  //   Bytes 56-61: focuser position (uint32) + speed (uint16), optional. Byte 65: checksum.
  MountState parseBinaryState(String base64Str) {
    if (base64Str.length != 88) return copyWith(valid: false);

    final Uint8List bytes;
    try {
      bytes = base64.decode(base64Str);
    } catch (_) {
      return copyWith(valid: false);
    }
    if (bytes.length < 66) return copyWith(valid: false);

    // Verify XOR checksum: XOR of bytes 0–64 must equal byte 65.
    int xorChk = 0;
    for (int i = 0; i < 65; i++) xorChk ^= bytes[i];
    if (xorChk != bytes[65]) return copyWith(valid: false);

    final bd = ByteData.sublistView(bytes);

    // ── Byte 0 ───────────────────────────────────────────────────────────
    final b0 = bytes[0];
    final trackVal = switch (b0 & 0x3) {
      0 => TrackState.off,
      1 => TrackState.on,
      _ => TrackState.slewing,
    };
    final sidIdx = (b0 >> 2) & 0x3;
    final sidVal = (sidIdx < SiderealMode.values.length - 1)
        ? SiderealMode.values[sidIdx]
        : SiderealMode.unknown;
    final parkIdx = (b0 >> 4) & 0x3;
    final parkVal = switch (parkIdx) {
      0 => ParkState.unparked,
      1 => ParkState.parking,
      2 => ParkState.parked,
      3 => ParkState.failed,
      _ => ParkState.unknown,
    };
    final atHomeVal = ((b0 >> 6) & 0x1) == 1;
    final pierVal   = ((b0 >> 7) & 0x1) == 0 ? PierSide.east : PierSide.west;

    // ── Byte 1 ───────────────────────────────────────────────────────────
    final b1 = bytes[1];
    final speedIdx   = b1 & 0x7;
    final speedVal   = (speedIdx <= 4)
        ? SpeedLevel.values[speedIdx] : SpeedLevel.unknown;
    final alignedVal = ((b1 >> 3) & 0x1) == 1;
    final mtIdx      = (b1 >> 4) & 0x7;
    final mountVal   = switch (mtIdx) {
      1 => MountType.gem,
      2 => MountType.fork,
      3 => MountType.altAz,
      4 => MountType.forkAlt,
      _ => MountType.undefined,
    };
    final spiralVal  = ((b1 >> 7) & 0x1) == 1;

    // ── Byte 2 ───────────────────────────────────────────────────────────
    final b2 = bytes[2];
    final ewIdx   = b2 & 0x3;
    final ewStr   = switch (ewIdx) { 1 => '>', 2 => '<', 3 => 'b', _ => ' ' };
    final nsIdx   = (b2 >> 2) & 0x3;
    final nsStr   = switch (nsIdx) { 1 => '^', 2 => '_', 3 => 'b', _ => ' ' };
    final trkComp = ((b2 >> 4) & 0x3) > 0;
    final pulse   = ((b2 >> 7) & 0x1) == 1;

    // ── Bytes 3-5 ────────────────────────────────────────────────────────
    final gFlags   = bytes[3];
    final errIdx   = bytes[4];
    final errVal   = (errIdx < MountError.values.length)
        ? MountError.values[errIdx] : MountError.none;
    final eFlags     = bytes[5] & 0x0F;
    final hasFoc     = ((bytes[5] >> 4) & 0x1) == 1;

    // ── UTC bytes 6-11 ────────────────────────────────────────────────────
    final utcH  = bytes[6];  final utcM = bytes[7];  final utcS = bytes[8];
    final utcMo = bytes[9];  final utcD = bytes[10]; final utcY = bytes[11];
    final utcTimeStr = '${_p2(utcH)}:${_p2(utcM)}:${_p2(utcS)}';
    final utcDateStr = '${_p2(utcMo)}/${_p2(utcD)}/${_p2(utcY)}';

    // ── Byte 62: timezone offset (int8, toff×10; local = UTC − toff) ───────
    final tz10Raw = bytes[62];
    final tz10 = tz10Raw > 127 ? tz10Raw - 256 : tz10Raw;
    final tzHours = tz10 / 10.0;
    final local = _utcToLocal(utcH, utcM, utcS, utcMo, utcD, utcY, tzHours);
    final localTimeStr = local.$1;
    final localDateStr = local.$2;

    // ── Positions bytes 12-39 ─────────────────────────────────────────────
    final raH   = bd.getFloat32(12, Endian.little);
    final decD  = bd.getFloat32(16, Endian.little);
    final altD  = bd.getFloat32(20, Endian.little);
    final azD   = bd.getFloat32(24, Endian.little);
    final lstH  = bd.getFloat32(28, Endian.little);
    final tRaH  = bd.getFloat32(32, Endian.little);
    final tDecD = bd.getFloat32(36, Endian.little);

    final raStr   = _formatRa(raH);
    final decStr  = _formatDeg(decD);
    final altStr  = _formatDeg(altD);
    final azStr   = _formatAz(azD);
    final lstStr  = _formatRa(lstH);
    final tRaStr  = _formatRa(tRaH);
    final tDecStr = _formatDeg(tDecD);

    // ── Tracking rates bytes 40-55 (int32 LE) ──────────────────────────────
    final trRa = bd.getInt32(40, Endian.little);
    final trDec = bd.getInt32(44, Endian.little);
    final stRa = bd.getInt32(48, Endian.little);
    final stDec = bd.getInt32(52, Endian.little);

    // ── Focuser bytes 56-61 (optional when hasFocuser) ────────────────────
    final fPos = bd.getUint32(56, Endian.little);
    final fSpd = bd.getUint16(60, Endian.little);

    return copyWith(
      tracking: trackVal,
      sidereal: sidVal,
      parkState: parkVal,
      atHome: atHomeVal,
      speed: speedVal,
      spiralRunning: spiralVal,
      pulseGuiding: pulse,
      guidingEW: ewStr,
      guidingNS: nsStr,
      trackCorrected: trkComp,
      aligned: alignedVal,
      mountType: mountVal,
      pierSide: pierVal,
      gnssFlags: gFlags,
      error: errVal,
      enableFlags: eFlags,
      ra: raStr,
      dec: decStr,
      alt: altStr,
      az: azStr,
      siderealTime: lstStr,
      targetRa: tRaStr,
      targetDec: tDecStr,
      utcTime: utcTimeStr,
      utcDate: utcDateStr,
      localTime: localTimeStr,
      localDate: localDateStr,
      timezoneOffset: tzHours,
      focuserConnected: hasFoc,
      focuserPos: fPos,
      focuserSpeed: fSpd,
      trackRateRa: trRa,
      trackRateDec: trDec,
      storedTrackRateRa: stRa,
      storedTrackRateDec: stDec,
      valid: true,
    );
  }

  /// Parse the raw :GXI# status string into a new MountState,
  /// preserving position/time/version fields from previous state.
  MountState parseStatus(String raw) {
    if (raw.length < 17) return copyWith(valid: false);

    final trackVal = switch (raw[0]) {
      '2' || '3' => TrackState.slewing,
      '1' => TrackState.on,
      '0' => TrackState.off,
      _ => TrackState.unknown,
    };

    final sidVal = switch (raw[1]) {
      '0' => SiderealMode.star,
      '1' => SiderealMode.sun,
      '2' => SiderealMode.moon,
      '3' => SiderealMode.target,
      _ => SiderealMode.unknown,
    };

    final parkVal = switch (raw[2]) {
      'P' => ParkState.parked,
      'p' => ParkState.unparked,
      'I' => ParkState.parking,
      'F' => ParkState.failed,
      _ => ParkState.unknown,
    };

    final speedIdx = raw.codeUnitAt(4) - 0x30;
    final speedVal = (speedIdx >= 0 && speedIdx <= 4)
        ? SpeedLevel.values[speedIdx]
        : SpeedLevel.unknown;

    final mountVal = switch (raw[12]) {
      'E' => MountType.gem,
      'K' => MountType.fork,
      'A' => MountType.altAz,
      'k' => MountType.forkAlt,
      _ => MountType.undefined,
    };

    final pierVal = switch (raw[13]) {
      'E' => PierSide.east,
      'W' => PierSide.west,
      _ => PierSide.unknown,
    };

    final gFlags = raw.codeUnitAt(14) - 0x41;  // char - 'A'
    final errIdx = raw.codeUnitAt(15) - 0x30;
    final errVal = (errIdx >= 0 && errIdx <= 8)
        ? MountError.values[errIdx]
        : MountError.none;
    final eFlags = raw.codeUnitAt(16) - 0x41;

    return copyWith(
      tracking: trackVal,
      sidereal: sidVal,
      parkState: parkVal,
      atHome: raw[3] == 'H',
      speed: speedVal,
      spiralRunning: raw[5] == '@',
      pulseGuiding: raw[6] == '*',
      guidingEW: raw[7] == ' ' ? ' ' : String.fromCharCode(raw.codeUnitAt(7)),
      guidingNS: raw[8] == ' ' ? ' ' : String.fromCharCode(raw.codeUnitAt(8)),
      trackCorrected: raw[10] == 'c',
      aligned: raw[11] == '1',
      mountType: mountVal,
      pierSide: pierVal,
      gnssFlags: gFlags >= 0 ? gFlags : 0,
      error: errVal,
      enableFlags: eFlags >= 0 ? eFlags : 0,
      valid: true,
    );
  }

  MountState copyWith({
    TrackState? tracking,
    SiderealMode? sidereal,
    ParkState? parkState,
    bool? atHome,
    SpeedLevel? speed,
    bool? spiralRunning,
    bool? pulseGuiding,
    String? guidingEW,
    String? guidingNS,
    bool? trackCorrected,
    bool? aligned,
    MountType? mountType,
    PierSide? pierSide,
    int? gnssFlags,
    MountError? error,
    int? enableFlags,
    bool? valid,
    String? ra,
    String? dec,
    String? ha,
    String? az,
    String? alt,
    String? targetRa,
    String? targetDec,
    String? utcTime,
    String? utcDate,
    String? localTime,
    String? localDate,
    String? siderealTime,
    double? timezoneOffset,
    String? productName,
    String? versionNum,
    String? boardVersion,
    String? driverType,
    double? latitude,
    double? longitude,
    String? focuserStatus,
    bool? focuserConnected,
    int? focuserPos,
    int? focuserSpeed,
    int? trackRateRa,
    int? trackRateDec,
    int? storedTrackRateRa,
    int? storedTrackRateDec,
  }) {
    return MountState(
      tracking: tracking ?? this.tracking,
      sidereal: sidereal ?? this.sidereal,
      parkState: parkState ?? this.parkState,
      atHome: atHome ?? this.atHome,
      speed: speed ?? this.speed,
      spiralRunning: spiralRunning ?? this.spiralRunning,
      pulseGuiding: pulseGuiding ?? this.pulseGuiding,
      guidingEW: guidingEW ?? this.guidingEW,
      guidingNS: guidingNS ?? this.guidingNS,
      trackCorrected: trackCorrected ?? this.trackCorrected,
      aligned: aligned ?? this.aligned,
      mountType: mountType ?? this.mountType,
      pierSide: pierSide ?? this.pierSide,
      gnssFlags: gnssFlags ?? this.gnssFlags,
      error: error ?? this.error,
      enableFlags: enableFlags ?? this.enableFlags,
      valid: valid ?? this.valid,
      ra: ra ?? this.ra,
      dec: dec ?? this.dec,
      ha: ha ?? this.ha,
      az: az ?? this.az,
      alt: alt ?? this.alt,
      targetRa: targetRa ?? this.targetRa,
      targetDec: targetDec ?? this.targetDec,
      utcTime: utcTime ?? this.utcTime,
      utcDate: utcDate ?? this.utcDate,
      localTime: localTime ?? this.localTime,
      localDate: localDate ?? this.localDate,
      siderealTime: siderealTime ?? this.siderealTime,
      timezoneOffset: timezoneOffset ?? this.timezoneOffset,
      productName: productName ?? this.productName,
      versionNum: versionNum ?? this.versionNum,
      boardVersion: boardVersion ?? this.boardVersion,
      driverType: driverType ?? this.driverType,
      latitude: latitude ?? this.latitude,
      longitude: longitude ?? this.longitude,
      focuserStatus: focuserStatus ?? this.focuserStatus,
      focuserConnected: focuserConnected ?? this.focuserConnected,
      focuserPos: focuserPos ?? this.focuserPos,
      focuserSpeed: focuserSpeed ?? this.focuserSpeed,
      trackRateRa: trackRateRa ?? this.trackRateRa,
      trackRateDec: trackRateDec ?? this.trackRateDec,
      storedTrackRateRa: storedTrackRateRa ?? this.storedTrackRateRa,
      storedTrackRateDec: storedTrackRateDec ?? this.storedTrackRateDec,
    );
  }

  /// Human-readable labels
  String get trackingLabel => switch (tracking) {
        TrackState.off => 'Off',
        TrackState.on => 'Tracking',
        TrackState.slewing => 'Slewing',
        TrackState.unknown => '?',
      };

  String get siderealLabel => switch (sidereal) {
        SiderealMode.star => 'Sidereal',
        SiderealMode.sun => 'Solar',
        SiderealMode.moon => 'Lunar',
        SiderealMode.target => 'User',
        SiderealMode.unknown => '?',
      };

  String get parkLabel => switch (parkState) {
        ParkState.parked => 'Parked',
        ParkState.unparked => 'Unparked',
        ParkState.parking => 'Parking...',
        ParkState.failed => 'Park Failed',
        ParkState.unknown => '?',
      };

  String get mountLabel => switch (mountType) {
        MountType.gem => 'GEM',
        MountType.fork => 'Fork EQ',
        MountType.altAz => 'Alt-Az',
        MountType.forkAlt => 'Fork Alt-Az',
        MountType.undefined => '?',
      };

  String get pierLabel => switch (pierSide) {
        PierSide.east => 'East',
        PierSide.west => 'West',
        PierSide.unknown => '?',
      };

  String get speedLabel => switch (speed) {
        SpeedLevel.guide => 'Guide',
        SpeedLevel.slow => 'Slow',
        SpeedLevel.medium => 'Medium',
        SpeedLevel.fast => 'Fast',
        SpeedLevel.max => 'Max',
        SpeedLevel.unknown => '?',
      };

  String get errorLabel => switch (error) {
        MountError.none => '',
        MountError.motorFault => 'Motor Fault',
        MountError.alt => 'Altitude Limit',
        MountError.limitSense => 'Limit Sense',
        MountError.limitA1 => 'Axis 1 Limit',
        MountError.limitA2 => 'Axis 2 Limit',
        MountError.underPole => 'Under Pole',
        MountError.meridian => 'Meridian',
        MountError.sync => 'Sync Error',
      };
}
