import 'dart:math' as math;

import 'phone_finder.dart' show sphereSeparationDeg;

/// How far to move the telescope from where it points to the target.
class PushTo {
  /// Great-circle distance, in degrees.
  final double separationDeg;

  /// Azimuth change, -180 to 180. Positive turns right (toward east from north).
  final double? dAzDeg;

  /// Altitude change. Positive is up.
  final double? dAltDeg;

  /// Used when the site is unknown. RA in hours, Dec in degrees.
  final double? dRaH;
  final double? dDecDeg;

  const PushTo({
    required this.separationDeg,
    this.dAzDeg,
    this.dAltDeg,
    this.dRaH,
    this.dDecDeg,
  });

  bool get altAz => dAzDeg != null && dAltDeg != null;
}

/// Offset from ([fromRa], [fromDec]) to ([toRa], [toDec]). All JNow.
/// With [latDeg] and [lstH], the offset is in azimuth and altitude.
PushTo pushTo({
  required double fromRa,
  required double fromDec,
  required double toRa,
  required double toDec,
  double? latDeg,
  double? lstH,
}) {
  final sep = sphereSeparationDeg(fromRa, fromDec, toRa, toDec);
  if (latDeg != null && lstH != null) {
    final from = altAzDeg(latDeg, lstH, fromRa, fromDec);
    final to = altAzDeg(latDeg, lstH, toRa, toDec);
    return PushTo(
      separationDeg: sep,
      dAzDeg: _wrap180(to.$2 - from.$2),
      dAltDeg: to.$1 - from.$1,
    );
  }
  var dRa = toRa - fromRa;
  if (dRa > 12) dRa -= 24;
  if (dRa < -12) dRa += 24;
  return PushTo(separationDeg: sep, dRaH: dRa, dDecDeg: toDec - fromDec);
}

/// Altitude and azimuth in degrees. Azimuth is from north through east.
(double, double) altAzDeg(double latDeg, double lstH, double raH, double decD) {
  final lat = latDeg * math.pi / 180;
  final dec = decD * math.pi / 180;
  final ha = (lstH - raH) * 15 * math.pi / 180;
  final sinAlt =
      math.sin(dec) * math.sin(lat) + math.cos(dec) * math.cos(lat) * math.cos(ha);
  final alt = math.asin(sinAlt.clamp(-1.0, 1.0));
  final y = -math.cos(dec) * math.sin(ha);
  final x = math.sin(dec) * math.cos(lat) - math.cos(dec) * math.sin(lat) * math.cos(ha);
  var az = math.atan2(y, x) * 180 / math.pi;
  if (az < 0) az += 360;
  return (alt * 180 / math.pi, az);
}

double _wrap180(double deg) {
  var d = deg % 360;
  if (d > 180) d -= 360;
  if (d <= -180) d += 360;
  return d;
}

/// One line such as `3.4°  right 2.9°  up 1.8°`.
String formatPushTo(PushTo p) {
  final total = '${p.separationDeg.toStringAsFixed(1)}°';
  if (p.altAz) {
    final az = p.dAzDeg!;
    final alt = p.dAltDeg!;
    return '$total  ${az >= 0 ? 'right' : 'left'} ${az.abs().toStringAsFixed(1)}°'
        '  ${alt >= 0 ? 'up' : 'down'} ${alt.abs().toStringAsFixed(1)}°';
  }
  final ra = p.dRaH! * 60;
  final dec = p.dDecDeg!;
  return '$total  RA ${ra >= 0 ? '+' : '-'}${ra.abs().toStringAsFixed(1)}m'
      '  Dec ${dec >= 0 ? '+' : '-'}${dec.abs().toStringAsFixed(1)}°';
}
