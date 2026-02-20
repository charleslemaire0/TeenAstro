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

  // Time strings
  final String utcTime;
  final String utcDate;
  final String siderealTime;

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
    this.siderealTime = '?',
    this.productName = '',
    this.versionNum = '',
    this.boardVersion = '',
    this.driverType = '',
    this.latitude = 0.0,
    this.longitude = 0.0,
    this.focuserStatus = '',
    this.focuserConnected = false,
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
    String? siderealTime,
    String? productName,
    String? versionNum,
    String? boardVersion,
    String? driverType,
    double? latitude,
    double? longitude,
    String? focuserStatus,
    bool? focuserConnected,
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
      siderealTime: siderealTime ?? this.siderealTime,
      productName: productName ?? this.productName,
      versionNum: versionNum ?? this.versionNum,
      boardVersion: boardVersion ?? this.boardVersion,
      driverType: driverType ?? this.driverType,
      latitude: latitude ?? this.latitude,
      longitude: longitude ?? this.longitude,
      focuserStatus: focuserStatus ?? this.focuserStatus,
      focuserConnected: focuserConnected ?? this.focuserConnected,
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
