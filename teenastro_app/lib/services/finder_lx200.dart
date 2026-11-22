import 'dart:math' as math;

import '../models/equinox_precession.dart';
import '../models/planet_positions.dart';
import 'phone_finder.dart' show parseMountDeg, parseMountHours;

/// Local sidereal time, in hours. [lonEastDeg] is positive east.
double localSiderealHours(DateTime time, double lonEastDeg) {
  final jd = julianDate(time.toUtc());
  final d = jd - 2451545.0;
  final t = d / 36525.0;
  final gmst =
      280.46061837 + 360.98564736629 * d + 0.000387933 * t * t - t * t * t / 38710000.0;
  var deg = (gmst + lonEastDeg) % 360;
  if (deg < 0) deg += 360;
  return deg / 15;
}

/// What the finder knows at one instant. Coordinates are JNow.
class FinderSnapshot {
  final double? latDeg;
  final double? lonEastDeg;
  final double? raH;
  final double? decD;

  /// Local wall time, with its zone offset.
  final DateTime local;

  const FinderSnapshot({
    required this.local,
    this.latDeg,
    this.lonEastDeg,
    this.raH,
    this.decD,
  });

  bool get hasSite => latDeg != null && lonEastDeg != null;

  double get lstH => localSiderealHours(local, lonEastDeg ?? 0);

  /// The last solve. Before the first solve, the zenith.
  (double, double) get pointing {
    final ra = raH;
    final dec = decD;
    if (ra != null && dec != null) return (ra, dec);
    return (lstH, latDeg ?? 0);
  }
}

/// Meade LX200 replies for SkySafari, from the phone's plate solve.
/// The finder has no motors: slews are refused and syncs change nothing.
class FinderLx200 {
  /// Called once both :Sr and :Sd have been received, in JNow.
  final void Function(double raH, double decD)? onTarget;

  bool highPrecision = true;
  double? _targetRa;
  double? _targetDec;
  String _buffer = '';

  FinderLx200({this.onTarget});

  /// Feed received text. Returns the replies to send, in order.
  List<String> feed(String data, FinderSnapshot sky) {
    final out = <String>[];
    _buffer += data;
    while (_buffer.isNotEmpty) {
      if (_buffer.codeUnitAt(0) == 0x06) {
        out.add('A');
        _buffer = _buffer.substring(1);
        continue;
      }
      final start = _buffer.indexOf(':');
      if (start < 0) {
        final ack = _buffer.indexOf('\x06');
        _buffer = ack < 0 ? '' : _buffer.substring(ack);
        continue;
      }
      final ack = _buffer.indexOf('\x06');
      if (ack >= 0 && ack < start) {
        _buffer = _buffer.substring(ack);
        continue;
      }
      final end = _buffer.indexOf('#', start);
      if (end < 0) {
        _buffer = _buffer.substring(start);
        break;
      }
      final reply = command(_buffer.substring(start, end + 1), sky);
      if (reply != null) out.add(reply);
      _buffer = _buffer.substring(end + 1);
    }
    return out;
  }

  /// Reply to one command such as `:GR#`. Null when the command has no reply.
  String? command(String cmd, FinderSnapshot sky) {
    var body = cmd;
    if (body.startsWith(':')) body = body.substring(1);
    if (body.endsWith('#')) body = body.substring(0, body.length - 1);
    if (body.isEmpty) return null;

    switch (body) {
      case 'GR':
        return '${formatRa(sky.pointing.$1, highPrecision)}#';
      case 'GD':
        return '${formatDec(sky.pointing.$2, highPrecision)}#';
      case 'GA':
        return '${formatDec(_altAz(sky).$1, highPrecision)}#';
      case 'GZ':
        return '${formatAzimuth(_altAz(sky).$2, highPrecision)}#';
      case 'GS':
        return '${formatRaDecLx200(sky.lstH, 0).$1}#';
      case 'GL':
        return '${_two(sky.local.hour)}:${_two(sky.local.minute)}:${_two(sky.local.second)}#';
      case 'GC':
        return '${_two(sky.local.month)}/${_two(sky.local.day)}/${_two(sky.local.year % 100)}#';
      case 'GG':
        return '${formatUtcOffset(sky.local.timeZoneOffset)}#';
      case 'Gt':
        return '${formatLatitude(sky.latDeg ?? 0)}#';
      case 'Gg':
        return '${formatMeadeLongitude(sky.lonEastDeg ?? 0)}#';
      case 'GW':
        return 'AT2#';
      case 'GVP':
        return 'TeenAstro Finder#';
      case 'GVN':
        return '1.0#';
      case 'U':
        highPrecision = !highPrecision;
        return null;
      case 'D':
        return '#';
      case 'MS':
        return '1';
      case 'CM':
        return 'N/A#';
    }

    if (body.startsWith('Sr')) {
      final ra = parseMountHours(body.substring(2).trim());
      if (ra == null || ra < 0 || ra >= 24) return '0';
      _targetRa = ra;
      _emitTarget();
      return '1';
    }
    if (body.startsWith('Sd')) {
      final dec = parseMountDeg(body.substring(2).trim());
      if (dec == null || dec.abs() > 90) return '0';
      _targetDec = dec;
      _emitTarget();
      return '1';
    }
    if (body.startsWith('S')) return '1';

    // Motion, halt, rate, park, tracking and focuser commands have no reply.
    final first = body[0];
    if (first == 'Q' || first == 'M' || first == 'R' || first == 'h' ||
        first == 'T' || first == 'F') {
      return null;
    }
    return '#';
  }

  void _emitTarget() {
    final ra = _targetRa;
    final dec = _targetDec;
    if (ra != null && dec != null) onTarget?.call(ra, dec);
  }

  (double, double) _altAz(FinderSnapshot sky) {
    final (ra, dec) = sky.pointing;
    final lat = (sky.latDeg ?? 0) * math.pi / 180;
    final ha = (sky.lstH - ra) * 15 * math.pi / 180;
    final d = dec * math.pi / 180;
    final sinAlt = math.sin(d) * math.sin(lat) + math.cos(d) * math.cos(lat) * math.cos(ha);
    final alt = math.asin(sinAlt.clamp(-1.0, 1.0));
    final y = -math.cos(d) * math.sin(ha);
    final x = math.sin(d) * math.cos(lat) - math.cos(d) * math.sin(lat) * math.cos(ha);
    var az = math.atan2(y, x) * 180 / math.pi;
    if (az < 0) az += 360;
    return (alt * 180 / math.pi, az);
  }
}

String _two(int v) => v.toString().padLeft(2, '0');

/// `HH:MM:SS`, or `HH:MM.T` in low precision.
String formatRa(double raH, bool high) {
  if (high) return formatRaDecLx200(raH, 0).$1;
  var tenths = (raH * 600).round() % 14400;
  if (tenths < 0) tenths += 14400;
  final h = tenths ~/ 600;
  final m = (tenths % 600) ~/ 10;
  return '${_two(h)}:${_two(m)}.${tenths % 10}';
}

/// `sDD*MM:SS`, or `sDD*MM` in low precision.
String formatDec(double decD, bool high) {
  if (high) return formatRaDecLx200(0, decD).$2.replaceFirst(':', '*');
  final sign = decD < 0 ? '-' : '+';
  final minutes = (decD.abs() * 60).round();
  return '$sign${_two(minutes ~/ 60)}*${_two(minutes % 60)}';
}

/// `DDD*MM:SS`, or `DDD*MM` in low precision.
String formatAzimuth(double azD, bool high) {
  if (high) {
    var seconds = (azD * 3600).round() % (360 * 3600);
    if (seconds < 0) seconds += 360 * 3600;
    final d = seconds ~/ 3600;
    final m = (seconds % 3600) ~/ 60;
    return '${d.toString().padLeft(3, '0')}*${_two(m)}:${_two(seconds % 60)}';
  }
  var minutes = (azD * 60).round() % (360 * 60);
  if (minutes < 0) minutes += 360 * 60;
  return '${(minutes ~/ 60).toString().padLeft(3, '0')}*${_two(minutes % 60)}';
}

/// `sDD*MM`.
String formatLatitude(double latD) => formatDec(latD, false);

/// Meade longitude: degrees west, 0 to 360, `DDD*MM`.
String formatMeadeLongitude(double lonEastDeg) {
  var minutes = (-lonEastDeg * 60).round() % (360 * 60);
  if (minutes < 0) minutes += 360 * 60;
  return '${(minutes ~/ 60).toString().padLeft(3, '0')}*${_two(minutes % 60)}';
}

/// Hours added to local time to give UTC, `sHH.H`.
String formatUtcOffset(Duration zoneOffset) {
  final hours = -zoneOffset.inMinutes / 60;
  final sign = hours < 0 ? '-' : '+';
  return '$sign${hours.abs().toStringAsFixed(1).padLeft(4, '0')}';
}
