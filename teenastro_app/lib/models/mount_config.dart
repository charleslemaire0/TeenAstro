import 'dart:convert';
import 'dart:typed_data';

/// Configuration data for one motor axis, unpacked from the :GXCS# packet.
class AxisConfig {
  /// Raw gear value (divide by 1000 to get the gear ratio as a double).
  final int gear;

  /// Steps per motor rotation.
  final int stepRot;

  /// Backlash amount (steps).
  final int backlash;

  /// Backlash take-up rate (steps/s).
  final int backlashRate;

  /// Low (hold) current in mA.
  final int lowCurr;

  /// High (run) current in mA.
  final int highCurr;

  /// Microstep divider index (0–8).
  final int micro;

  /// Direction reversal enabled.
  final bool reverse;

  /// StealthChop / silent mode enabled.
  final bool silent;

  const AxisConfig({
    required this.gear,
    required this.stepRot,
    required this.backlash,
    required this.backlashRate,
    required this.lowCurr,
    required this.highCurr,
    required this.micro,
    required this.reverse,
    required this.silent,
  });

  /// Gear ratio as a floating-point value.
  double get gearRatio => gear / 1000.0;

  static const empty = AxisConfig(
    gear: 0,
    stepRot: 0,
    backlash: 0,
    backlashRate: 0,
    lowCurr: 0,
    highCurr: 0,
    micro: 0,
    reverse: false,
    silent: false,
  );
}

/// Full mount configuration snapshot, decoded from a single :GXCS# response.
///
/// Replaces ~40 individual LX200 queries (`:GXMx#`, `:GXRx#`, `:GXLx#`,
/// `:GXEx#`). The data is static — it should be fetched once at startup and
/// refreshed only when a setting is changed.
class MountConfig {
  // ── Validity ────────────────────────────────────────────────────────────
  final bool valid;

  // ── Motor axes ───────────────────────────────────────────────────────────
  /// Axis 1 (RA / Azimuth).
  final AxisConfig axis1;

  /// Axis 2 (Dec / Altitude).
  final AxisConfig axis2;

  // ── Rates / Speed ────────────────────────────────────────────────────────
  /// Guiding rate (arcsec/s).
  final double guideRate;

  /// Slow centering rate (arcsec/s).
  final double slowRate;

  /// Medium centering rate (arcsec/s).
  final double mediumRate;

  /// Fast slewing rate (arcsec/s).
  final double fastRate;

  /// Acceleration distance (degrees).
  final double acceleration;

  /// Maximum slew rate (µs/step) from EEPROM.
  final int maxRate;

  /// Default rate index (0–4).
  final int defaultRate;

  /// Settle duration after slew (seconds).
  final int settleTime;

  // ── Limits ───────────────────────────────────────────────────────────────
  /// Meridian east limit (arcmin past meridian).
  final int meridianE;

  /// Meridian west limit (arcmin past meridian).
  final int meridianW;

  /// Axis 1 min travel (in 0.1-degree units, stored in EEPROM).
  final int axis1Min;

  /// Axis 1 max travel (in 0.1-degree units).
  final int axis1Max;

  /// Axis 2 min travel (in 0.1-degree units).
  final int axis2Min;

  /// Axis 2 max travel (in 0.1-degree units).
  final int axis2Max;

  /// Under-pole limit × 10 (e.g. 120 → 12.0 h).
  final int underPole10;

  /// Minimum altitude limit (degrees, signed).
  final int minAlt;

  /// Maximum altitude limit (degrees, signed).
  final int maxAlt;

  /// Minimum distance from pole for continuous tracking (degrees).
  final int minDistPole;

  /// Refraction correction applied during tracking.
  final bool refrTracking;

  /// Refraction correction applied during goto.
  final bool refrGoto;

  /// Refraction correction applied for polar alignment.
  final bool refrPole;

  // ── Encoders ─────────────────────────────────────────────────────────────
  /// Axis 1 encoder pulses per degree × 100.
  final int ppd1;

  /// Axis 2 encoder pulses per degree × 100.
  final int ppd2;

  /// Encoder auto-sync mode (0 = off; see EncoderSync enum in firmware).
  final int encSyncMode;

  /// Axis 1 encoder direction reversed.
  final bool encReverse1;

  /// Axis 2 encoder direction reversed.
  final bool encReverse2;

  // ── Options ──────────────────────────────────────────────────────────────
  /// Active mount index (0-based, selects the mount profile in the firmware).
  final int mountIdx;

  const MountConfig({
    this.valid = false,
    this.axis1 = AxisConfig.empty,
    this.axis2 = AxisConfig.empty,
    this.guideRate = 0,
    this.slowRate = 0,
    this.mediumRate = 0,
    this.fastRate = 0,
    this.acceleration = 0,
    this.maxRate = 0,
    this.defaultRate = 0,
    this.settleTime = 0,
    this.meridianE = 0,
    this.meridianW = 0,
    this.axis1Min = 0,
    this.axis1Max = 0,
    this.axis2Min = 0,
    this.axis2Max = 0,
    this.underPole10 = 0,
    this.minAlt = 0,
    this.maxAlt = 90,
    this.minDistPole = 0,
    this.refrTracking = false,
    this.refrGoto = false,
    this.refrPole = false,
    this.ppd1 = 0,
    this.ppd2 = 0,
    this.encSyncMode = 0,
    this.encReverse1 = false,
    this.encReverse2 = false,
    this.mountIdx = 0,
  });

  // ── Helpers ──────────────────────────────────────────────────────────────
  /// Meridian east limit as degrees (positive = east of meridian).
  double get meridianEDeg => meridianE / 60.0;

  /// Meridian west limit as degrees.
  double get meridianWDeg => meridianW / 60.0;

  /// Under-pole limit as hours.
  double get underPoleH => underPole10 / 10.0;

  /// Axis 1 min travel in degrees.
  double get axis1MinDeg => axis1Min / 10.0;

  /// Axis 1 max travel in degrees.
  double get axis1MaxDeg => axis1Max / 10.0;

  /// Axis 2 min travel in degrees.
  double get axis2MinDeg => axis2Min / 10.0;

  /// Axis 2 max travel in degrees.
  double get axis2MaxDeg => axis2Max / 10.0;

  /// Axis 1 encoder pulses per degree (true value).
  double get ppd1Real => ppd1 / 100.0;

  /// Axis 2 encoder pulses per degree (true value).
  double get ppd2Real => ppd2 / 100.0;

  // ── Parser ───────────────────────────────────────────────────────────────
  /// Decode a 120-character base64 string (the :GXCS# response, without '#')
  /// into a [MountConfig].  Returns a [MountConfig] with `valid = false` on
  /// any error (wrong length, bad base64, checksum mismatch).
  static MountConfig parseBinaryConfig(String base64Str) {
    final trimmed = base64Str.endsWith('#')
        ? base64Str.substring(0, base64Str.length - 1)
        : base64Str;

    if (trimmed.length != 120) return const MountConfig();

    final Uint8List bytes;
    try {
      bytes = base64.decode(trimmed);
    } catch (_) {
      return const MountConfig();
    }

    if (bytes.length < 90) return const MountConfig();

    // Verify XOR checksum: XOR of bytes 0–88 must equal byte 89.
    int xorChk = 0;
    for (int i = 0; i < 89; i++) xorChk ^= bytes[i];
    if (xorChk != bytes[89]) return const MountConfig();

    final bd = ByteData.sublistView(bytes);

    AxisConfig unpackAxis(int base) {
      final flags = bytes[base + 15];
      return AxisConfig(
        gear:         bd.getUint32(base + 0, Endian.little),
        stepRot:      bd.getUint16(base + 4, Endian.little),
        backlash:     bd.getUint16(base + 6, Endian.little),
        backlashRate: bd.getUint16(base + 8, Endian.little),
        lowCurr:      bd.getUint16(base + 10, Endian.little),
        highCurr:     bd.getUint16(base + 12, Endian.little),
        micro:        bytes[base + 14],
        reverse:      (flags & 0x01) != 0,
        silent:       (flags & 0x02) != 0,
      );
    }

    final refrFlags = bytes[73];
    final encFlags  = bytes[83];

    return MountConfig(
      valid: true,

      axis1: unpackAxis(0),
      axis2: unpackAxis(16),

      guideRate:    bd.getFloat32(32, Endian.little),
      slowRate:     bd.getFloat32(36, Endian.little),
      mediumRate:   bd.getFloat32(40, Endian.little),
      fastRate:     bd.getFloat32(44, Endian.little),
      acceleration: bd.getFloat32(48, Endian.little),
      maxRate:      bd.getUint16(52, Endian.little),
      defaultRate:  bytes[54],
      settleTime:   bytes[55],

      meridianE:    bd.getInt16(56, Endian.little),
      meridianW:    bd.getInt16(58, Endian.little),
      axis1Min:     bd.getInt16(60, Endian.little),
      axis1Max:     bd.getInt16(62, Endian.little),
      axis2Min:     bd.getInt16(64, Endian.little),
      axis2Max:     bd.getInt16(66, Endian.little),
      underPole10:  bd.getUint16(68, Endian.little),
      minAlt:       bytes[70].toSigned(8),
      maxAlt:       bytes[71].toSigned(8),
      minDistPole:  bytes[72],
      refrTracking: (refrFlags & 0x01) != 0,
      refrGoto:     (refrFlags & 0x02) != 0,
      refrPole:     (refrFlags & 0x04) != 0,

      ppd1:         bd.getUint32(74, Endian.little),
      ppd2:         bd.getUint32(78, Endian.little),
      encSyncMode:  bytes[82],
      encReverse1:  (encFlags & 0x01) != 0,
      encReverse2:  (encFlags & 0x02) != 0,

      mountIdx:     bytes[84],
    );
  }
}
