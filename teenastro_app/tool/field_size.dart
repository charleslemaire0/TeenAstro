import 'dart:math' as math;

/// One star with a pixel and a sky position.
///
/// x grows right and y grows down. Right ascension and declination are degrees.
class SkyPixel {
  final double x;
  final double y;
  final double raDeg;
  final double decDeg;

  const SkyPixel(this.x, this.y, this.raDeg, this.decDeg);
}

/// Longer-side field, in degrees, from a solved center and its stars.
///
/// The scale is the gnomonic plate that best maps pixels onto the sky.
/// The longer side is then the angle from one edge to the other through
/// the middle. Returns null when the stars do not determine that scale.
double? longSideFieldDeg({
  required double centerRaDeg,
  required double centerDecDeg,
  required double width,
  required double height,
  required List<SkyPixel> stars,
}) {
  if (width < 2 || height < 2 || stars.length < 6) return null;
  final ra0 = centerRaDeg * math.pi / 180;
  final dec0 = centerDecDeg * math.pi / 180;
  final sinDec0 = math.sin(dec0);
  final cosDec0 = math.cos(dec0);
  final centerX = (width - 1) / 2;
  final centerY = (height - 1) / 2;
  var uu = 0.0;
  var uv = 0.0;
  var vv = 0.0;
  var uXi = 0.0;
  var vXi = 0.0;
  var uEta = 0.0;
  var vEta = 0.0;
  var used = 0;
  for (final star in stars) {
    final ra = star.raDeg * math.pi / 180;
    final dec = star.decDeg * math.pi / 180;
    final dra = ra - ra0;
    final sinDec = math.sin(dec);
    final cosDec = math.cos(dec);
    final cosC = sinDec * sinDec0 + cosDec * cosDec0 * math.cos(dra);
    if (cosC <= 0.2) continue;
    final xi = cosDec * math.sin(dra) / cosC;
    final eta = (sinDec * cosDec0 - cosDec * sinDec0 * math.cos(dra)) / cosC;
    final u = star.x - centerX;
    final v = star.y - centerY;
    uu += u * u;
    uv += u * v;
    vv += v * v;
    uXi += u * xi;
    vXi += v * xi;
    uEta += u * eta;
    vEta += v * eta;
    used++;
  }
  if (used < 6) return null;
  final det = uu * vv - uv * uv;
  if (det.abs() < 1) return null;
  final a = (vv * uXi - uv * vXi) / det;
  final b = (uu * vXi - uv * uXi) / det;
  final c = (vv * uEta - uv * vEta) / det;
  final d = (uu * vEta - uv * uEta) / det;
  final across = math.sqrt(a * a + b * b);
  final down = math.sqrt(c * c + d * d);
  if (across <= 0 || down <= 0) return null;
  final ratio = across > down ? across / down : down / across;
  if (ratio > 1.4) return null;
  final scale = (across + down) / 2;
  final long = math.max(width, height);
  return 2 * math.atan(scale * long / 2) * 180 / math.pi;
}

/// Longer-side field when the image width's field is already known.
double longSideFromWidthDeg(double widthDeg, double width, double height) {
  final half = math.tan(widthDeg * math.pi / 360);
  final heightDeg = 2 * math.atan(half * height / width) * 180 / math.pi;
  return math.max(widthDeg, heightDeg);
}
