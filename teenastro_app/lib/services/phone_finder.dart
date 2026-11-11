import 'dart:math' as math;
import 'dart:typed_data';

import 'package:image/image.dart' as img;

/// A catalog star. Coordinates are the same equinox as the mount.
class FinderStar {
  final double raH;
  final double decD;
  const FinderStar(this.raH, this.decD);
}

/// A star peak in a captured frame. [x] grows right, [y] grows down.
class PixelStar {
  final double x;
  final double y;
  const PixelStar(this.x, this.y);
}

/// Where the camera axis points, and how the frame is rolled on the sky.
class FinderSolution {
  final double raH;
  final double decD;

  /// Image right-axis, measured from east toward north, in radians.
  final double rollRad;
  final int matches;

  const FinderSolution({
    required this.raH,
    required this.decD,
    required this.rollRad,
    required this.matches,
  });
}

/// Horizontal and vertical cones of the camera axis in the telescope frame.
/// Both are radians. Positive Hz is to the right in the frame, positive Ve is up.
class FinderCones {
  final double hzRad;
  final double veRad;
  const FinderCones({this.hzRad = 0, this.veRad = 0});

  bool get isZero => hzRad == 0 && veRad == 0;
}

double wrapHours(double h) {
  var x = h % 24.0;
  if (x < 0) x += 24.0;
  return x;
}

/// East and north separation of [raH], [decD] from an origin, in degrees.
(double east, double north) skyOffset(
  double raH,
  double decD,
  double originRa,
  double originDec,
) {
  var dRa = raH - originRa;
  if (dRa > 12) dRa -= 24;
  if (dRa < -12) dRa += 24;
  final c = math.cos(originDec * math.pi / 180);
  return (dRa * 15.0 * c, decD - originDec);
}

(double raH, double decD) offsetToSky(
  double originRa,
  double originDec,
  double eastDeg,
  double northDeg,
) {
  final c = math.cos(originDec * math.pi / 180);
  final safe = c.abs() < 0.05 ? (c.isNegative ? -0.05 : 0.05) : c;
  final dec = (originDec + northDeg).clamp(-89.0, 89.0);
  return (wrapHours(originRa + (eastDeg / safe) / 15.0), dec);
}

/// Sky offset into the telescope frame. [rollRad] is the frame's right axis.
(double hz, double ve) skyToTelescope(
  double eastDeg,
  double northDeg,
  double rollRad,
) {
  final c = math.cos(rollRad);
  final s = math.sin(rollRad);
  return (eastDeg * c + northDeg * s, -eastDeg * s + northDeg * c);
}

(double east, double north) telescopeToSky(
  double hzDeg,
  double veDeg,
  double rollRad,
) {
  final c = math.cos(rollRad);
  final s = math.sin(rollRad);
  return (hzDeg * c - veDeg * s, hzDeg * s + veDeg * c);
}

/// Camera axis minus telescope axis, in the telescope frame.
FinderCones measureCones({
  required double telescopeRa,
  required double telescopeDec,
  required FinderSolution camera,
}) {
  final (east, north) =
      skyOffset(camera.raH, camera.decD, telescopeRa, telescopeDec);
  final (hz, ve) = skyToTelescope(east, north, camera.rollRad);
  return FinderCones(hzRad: hz * math.pi / 180, veRad: ve * math.pi / 180);
}

/// Telescope axis obtained by removing the stored cones from a camera solve.
(double raH, double decD) telescopeAxis(
    FinderSolution camera, FinderCones cones) {
  final hz = cones.hzRad * 180 / math.pi;
  final ve = cones.veRad * 180 / math.pi;
  final (east, north) = telescopeToSky(hz, ve, camera.rollRad);
  // Inverse of skyOffset: north is camera minus telescope, and the east
  // scale uses the telescope's declination.
  final dec = camera.decD - north;
  final c = math.cos(dec * math.pi / 180);
  final safe = c.abs() < 0.05 ? (c.isNegative ? -0.05 : 0.05) : c;
  return (wrapHours(camera.raH - (east / safe) / 15.0), dec);
}

/// Hinted plate solve. [fovDeg] is the longer side of the frame.
/// The hint is only where to look; the result is the camera axis.
FinderSolution? solveField({
  required List<PixelStar> stars,
  required List<FinderStar> catalog,
  required double hintRa,
  required double hintDec,
  required double width,
  required double height,
  required double fovDeg,
}) {
  if (stars.length < 4 || catalog.isEmpty || width < 2 || height < 2) {
    return null;
  }
  final scale = fovDeg / math.max(width, height);
  final reach = fovDeg * 0.65;
  final nearby = <FinderStar>[];
  for (final s in catalog) {
    final (e, n) = skyOffset(s.raH, s.decD, hintRa, hintDec);
    if (e * e + n * n <= reach * reach) nearby.add(s);
  }
  if (nearby.length < 4) return null;

  final det = <(double right, double up)>[];
  for (final p in stars) {
    det.add(((p.x - width / 2) * scale, (height / 2 - p.y) * scale));
  }

  const bin = 0.3;
  FinderSolution? best;
  var bestCount = 0;

  for (var deg = 0; deg < 360; deg++) {
    final roll = deg * math.pi / 180;
    final bins = <int, _Vote>{};
    for (final star in nearby) {
      final (e, n) = skyOffset(star.raH, star.decD, hintRa, hintDec);
      final (right0, up0) = skyToTelescope(e, n, roll);
      for (final d in det) {
        final tr = d.$1 - right0;
        final tu = d.$2 - up0;
        if (tr.abs() > reach || tu.abs() > reach) continue;
        final key = (tr / bin).round() * 100000 + (tu / bin).round();
        final vote = bins.putIfAbsent(key, () => _Vote());
        vote.add(tr, tu);
      }
    }
    _Vote? top;
    for (final v in bins.values) {
      if (top == null || v.n > top.n) top = v;
    }
    if (top == null || top.n < 4 || top.n < bestCount) continue;
    final tr = top.sr / top.n;
    final tu = top.su / top.n;
    final (east, north) = telescopeToSky(-tr, -tu, roll);
    final (ra, dec) = offsetToSky(hintRa, hintDec, east, north);
    if (top.n > bestCount) {
      bestCount = top.n;
      best = FinderSolution(raH: ra, decD: dec, rollRad: roll, matches: top.n);
    }
  }
  return best;
}

class _Vote {
  int n = 0;
  double sr = 0;
  double su = 0;
  void add(double r, double u) {
    n++;
    sr += r;
    su += u;
  }
}

/// Bright peaks in a camera frame, brightest first.
List<PixelStar> detectStars(img.Image src, {int maxStars = 18}) {
  final w = src.width;
  final h = src.height;
  if (w < 16 || h < 16) return const [];
  final step = math.max(1, math.max(w, h) ~/ 480);
  final sw = w ~/ step;
  final sh = h ~/ step;
  final lum = Int32List(sw * sh);
  for (var y = 0; y < sh; y++) {
    for (var x = 0; x < sw; x++) {
      final p = src.getPixel(x * step, y * step);
      lum[y * sw + x] = (0.299 * p.r + 0.587 * p.g + 0.114 * p.b).round();
    }
  }
  final sorted = Int32List.fromList(lum)..sort();
  final thr = math.max(40, sorted[(sorted.length * 0.985).floor()]);
  const rad = 3;
  final peaks = <({int x, int y, int v})>[];
  for (var y = rad; y < sh - rad; y++) {
    for (var x = rad; x < sw - rad; x++) {
      final v = lum[y * sw + x];
      if (v < thr) continue;
      var isMax = true;
      for (var dy = -rad; dy <= rad && isMax; dy++) {
        for (var dx = -rad; dx <= rad; dx++) {
          if (dx == 0 && dy == 0) continue;
          if (lum[(y + dy) * sw + (x + dx)] > v) {
            isMax = false;
            break;
          }
        }
      }
      if (isMax) peaks.add((x: x, y: y, v: v));
    }
  }
  peaks.sort((a, b) => b.v.compareTo(a.v));
  return [
    for (final p in peaks.take(maxStars))
      PixelStar(
          p.x * step.toDouble() + step / 2, p.y * step.toDouble() + step / 2),
  ];
}

class StarFrame {
  final List<PixelStar> stars;
  final double width;
  final double height;
  const StarFrame(this.stars, this.width, this.height);
}

StarFrame? readStarFrame(Uint8List bytes) {
  final decoded = img.decodeImage(bytes);
  if (decoded == null) return null;
  return StarFrame(
    detectStars(decoded),
    decoded.width.toDouble(),
    decoded.height.toDouble(),
  );
}

double? parseMountHours(String s) {
  if (s.isEmpty || s == '?') return null;
  final parts = s.split(':');
  if (parts.isEmpty) return null;
  final h = double.tryParse(parts[0]);
  if (h == null) return null;
  final m = parts.length > 1 ? (double.tryParse(parts[1]) ?? 0) : 0;
  final sec = parts.length > 2 ? (double.tryParse(parts[2]) ?? 0) : 0;
  return h + m / 60 + sec / 3600;
}

double? parseMountDeg(String s) {
  if (s.isEmpty || s == '?') return null;
  final sign = s.trim().startsWith('-') ? -1.0 : 1.0;
  final cleaned = s.replaceAll('*', ':').replaceAll(RegExp(r'[^0-9.:]'), '');
  final parts = cleaned.split(':');
  if (parts.isEmpty || parts[0].isEmpty) return null;
  final d = double.tryParse(parts[0]);
  if (d == null) return null;
  final m = parts.length > 1 ? (double.tryParse(parts[1]) ?? 0) : 0;
  final sec = parts.length > 2 ? (double.tryParse(parts[2]) ?? 0) : 0;
  return sign * (d + m / 60 + sec / 3600);
}

String formatArcmin(double radians) {
  final arcmin = radians * 180 / math.pi * 60;
  final sign = arcmin >= 0 ? '+' : '';
  return "$sign${arcmin.toStringAsFixed(1)}'";
}
