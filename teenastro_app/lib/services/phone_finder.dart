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

  /// Root-mean-square of the matched stars, in degrees.
  final double rmsDeg;

  const FinderSolution({
    required this.raH,
    required this.decD,
    required this.rollRad,
    required this.matches,
    this.rmsDeg = 0,
  });
}

/// A plate solve, or why it was rejected.
class FieldSolve {
  final FinderSolution? solution;
  final String? reason;
  const FieldSolve({this.solution, this.reason});
}

/// Horizontal and vertical cones of the camera axis in the telescope frame.
/// Both are radians. Positive Hz is to the right in the frame, positive Ve is up.
class FinderCones {
  final double hzRad;
  final double veRad;
  const FinderCones({this.hzRad = 0, this.veRad = 0});

  bool get isZero => hzRad == 0 && veRad == 0;
}

/// Field angle, in degrees, of the longer side of a lens.
///
/// [widthMm] and [heightMm] are the sensor's physical size. [focalMm] is the
/// lens focal length. The angle is the longer side of the frame.
double longerSideFieldDeg(double widthMm, double heightMm, double focalMm) {
  final side = math.max(widthMm, heightMm);
  if (focalMm <= 0 || side <= 0) {
    throw ArgumentError('The lens size must be positive');
  }
  return 2 * math.atan(side / (2 * focalMm)) * 180 / math.pi;
}

/// Great-circle distance between two sky positions, in degrees.
double sphereSeparationDeg(
  double raH,
  double decD,
  double otherRa,
  double otherDec,
) {
  final d1 = decD * math.pi / 180;
  final d2 = otherDec * math.pi / 180;
  final dRa = (raH - otherRa) * 15 * math.pi / 180;
  final cosC =
      math.sin(d1) * math.sin(d2) + math.cos(d1) * math.cos(d2) * math.cos(dRa);
  return math.acos(cosC.clamp(-1.0, 1.0)) * 180 / math.pi;
}

/// Longer-side field, in degrees, from stars with known sky and pixel positions.
///
/// Each pair gives a degrees-per-pixel scale. The result is the median of those
/// scales times the longer side of the frame. Pairs closer than 30 pixels or
/// half a degree are left out. Returns null when no pair can be measured.
double? longSideFieldFromStars({
  required List<({double raH, double decD, double x, double y})> stars,
  required double width,
  required double height,
}) {
  final long = math.max(width, height);
  if (stars.length < 2 || long < 2) return null;
  final fields = <double>[];
  for (var i = 0; i < stars.length; i++) {
    for (var j = i + 1; j < stars.length; j++) {
      final a = stars[i];
      final b = stars[j];
      final pixels =
          math.sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
      final sky = sphereSeparationDeg(a.raH, a.decD, b.raH, b.decD);
      if (pixels < 30 || sky < 0.5) continue;
      fields.add(sky / pixels * long);
    }
  }
  if (fields.isEmpty) return null;
  fields.sort();
  final mid = fields.length ~/ 2;
  if (fields.length.isOdd) return fields[mid];
  return (fields[mid - 1] + fields[mid]) / 2;
}

double wrapHours(double h) {
  var x = h % 24.0;
  if (x < 0) x += 24.0;
  return x;
}

/// Angular separation of two sky positions, in arcminutes.
double separationArcmin(
  double raH,
  double decD,
  double originRa,
  double originDec,
) {
  final (east, north) = skyOffset(raH, decD, originRa, originDec);
  return math.sqrt(east * east + north * north) * 60.0;
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

/// Camera axis and roll from at least three named stars.
///
/// [fovDeg] is the longer side of the frame. Each star predicts where the
/// axis must be for a given roll. The roll that makes those predictions
/// agree is kept, and the axis is their mean.
FinderSolution? poseFromNamedStars({
  required List<({double raH, double decD, double x, double y})> stars,
  required double width,
  required double height,
  required double fovDeg,
}) {
  if (stars.length < 3 || width < 2 || height < 2 || fovDeg <= 0) return null;
  final scale = fovDeg / math.max(width, height);
  final measured = <(double right, double up)>[
    for (final s in stars)
      ((s.x - width / 2) * scale, (height / 2 - s.y) * scale),
  ];

  ({double ra, double dec, double roll, double scatter})? best;
  void consider(double roll) {
    final centers = <(double, double)>[];
    for (var i = 0; i < stars.length; i++) {
      final (east, north) =
          telescopeToSky(measured[i].$1, measured[i].$2, roll);
      centers.add(offsetToSky(stars[i].raH, stars[i].decD, -east, -north));
    }
    final mean = _meanPositions(centers);
    var scatter2 = 0.0;
    for (final c in centers) {
      final (east, north) = skyOffset(c.$1, c.$2, mean.$1, mean.$2);
      scatter2 += east * east + north * north;
    }
    final scatter = math.sqrt(scatter2 / centers.length);
    if (best == null || scatter < best!.scatter) {
      best = (ra: mean.$1, dec: mean.$2, roll: roll, scatter: scatter);
    }
  }

  for (var deg = 0; deg < 360; deg++) {
    consider(deg * math.pi / 180);
  }
  var roll = best!.roll;
  var ra = best!.ra;
  var dec = best!.dec;
  for (var iter = 0; iter < 8; iter++) {
    var sinSum = 0.0;
    var cosSum = 0.0;
    for (var i = 0; i < stars.length; i++) {
      final (east, north) = skyOffset(stars[i].raH, stars[i].decD, ra, dec);
      final weight = math.sqrt(east * east + north * north);
      if (weight < 0.05) continue;
      final delta =
          math.atan2(north, east) - math.atan2(measured[i].$2, measured[i].$1);
      sinSum += weight * math.sin(delta);
      cosSum += weight * math.cos(delta);
    }
    if (sinSum != 0 || cosSum != 0) roll = math.atan2(sinSum, cosSum);
    final centers = <(double, double)>[];
    for (var i = 0; i < stars.length; i++) {
      final (east, north) =
          telescopeToSky(measured[i].$1, measured[i].$2, roll);
      centers.add(offsetToSky(stars[i].raH, stars[i].decD, -east, -north));
    }
    final mean = _meanPositions(centers);
    ra = mean.$1;
    dec = mean.$2;
  }
  best = (ra: ra, dec: dec, roll: roll, scatter: best!.scatter);

  var error2 = 0.0;
  for (var i = 0; i < stars.length; i++) {
    final (east, north) =
        skyOffset(stars[i].raH, stars[i].decD, best!.ra, best!.dec);
    final (right, up) = skyToTelescope(east, north, best!.roll);
    final rightError = right - measured[i].$1;
    final upError = up - measured[i].$2;
    error2 += rightError * rightError + upError * upError;
  }
  return FinderSolution(
    raH: best!.ra,
    decD: best!.dec,
    rollRad: best!.roll,
    matches: stars.length,
    rmsDeg: math.sqrt(error2 / stars.length),
  );
}

(double, double) _meanPositions(List<(double, double)> positions) {
  var east = 0.0;
  var north = 0.0;
  var dec = 0.0;
  for (final p in positions) {
    final angle = p.$1 * math.pi / 12;
    east += math.cos(angle);
    north += math.sin(angle);
    dec += p.$2;
  }
  return (
    wrapHours(math.atan2(north, east) * 12 / math.pi),
    dec / positions.length,
  );
}

/// Radial phone lens: u' = u (1 + k1 r² + k2 r⁴ + k3 r⁶).
///
/// [r] is the pinhole radius in focal-length units. A stored single term is
/// k1, with k2 and k3 at zero.
class LensTerms {
  final double k1;
  final double k2;
  final double k3;

  const LensTerms(this.k1, this.k2, this.k3);

  const LensTerms.none()
      : k1 = 0,
        k2 = 0,
        k3 = 0;

  String describe() =>
      'k1 ${k1.toStringAsFixed(3)}, k2 ${k2.toStringAsFixed(3)}, k3 ${k3.toStringAsFixed(3)}';
}

double _lensPoly(double r2, LensTerms lens) =>
    1 + r2 * (lens.k1 + r2 * (lens.k2 + r2 * lens.k3));

/// False when the radial map folds inside the frame.
bool _lensOpen(double f, LensTerms lens, double width, double height) {
  if (f < 50) return false;
  final half = 0.5 * math.sqrt(width * width + height * height);
  final r2 = (half / f) * (half / f);
  final scale = _lensPoly(r2, lens);
  final slope = 1 + r2 * (3 * lens.k1 + r2 * (5 * lens.k2 + r2 * 7 * lens.k3));
  return scale > 0.2 && slope > 0.05;
}

/// Wide-field camera fitted to named stars.
///
/// The plate is the pinhole view of a real camera: north up puts east to the
/// left. [fovDeg] is the longer side after the focal length is fitted.
/// The optical center is the middle of the picture.
class WideCamera {
  final FinderSolution solution;
  final double fovDeg;
  final double focalPx;
  final LensTerms lens;
  final double originX;
  final double originY;
  final double _width;
  final double _height;
  final List<({double x, double y, double z})> _toSky;

  const WideCamera({
    required this.solution,
    required this.fovDeg,
    required this.focalPx,
    required this.lens,
    required this.originX,
    required this.originY,
    required double width,
    required double height,
    required List<({double x, double y, double z})> toSky,
  })  : _width = width,
        _height = height,
        _toSky = toSky;

  /// Sky direction of a pixel. [x] grows right, [y] grows down.
  ({double x, double y, double z}) direction(double x, double y) {
    final ray = _wideRay(
      x,
      y,
      _width,
      _height,
      focalPx,
      lens,
      originX: originX,
      originY: originY,
    );
    return _wideApply(_toSky, ray);
  }

  /// Pixel of a sky direction. Null when the star is behind the camera
  /// or the lens inverse does not land on that direction.
  ({double x, double y})? pixel(({double x, double y, double z}) sky) {
    final cam = (
      x: _toSky[0].x * sky.x + _toSky[1].x * sky.y + _toSky[2].x * sky.z,
      y: _toSky[0].y * sky.x + _toSky[1].y * sky.y + _toSky[2].y * sky.z,
      z: _toSky[0].z * sky.x + _toSky[1].z * sky.y + _toSky[2].z * sky.z,
    );
    if (cam.z <= 0.05) return null;
    final tx = cam.x / cam.z;
    final ty = cam.y / cam.z;
    final target = math.sqrt(tx * tx + ty * ty);
    var radius = target;
    if (target > 1e-8) {
      var settled = false;
      for (var i = 0; i < 12; i++) {
        final r2 = radius * radius;
        final slope =
            1 + r2 * (3 * lens.k1 + r2 * (5 * lens.k2 + r2 * 7 * lens.k3));
        if (slope.abs() < 1e-4) return null;
        final next = radius - (radius * _lensPoly(r2, lens) - target) / slope;
        if (next < 0 || next > 6) return null;
        if ((next - radius).abs() < 1e-8) {
          radius = next;
          settled = true;
          break;
        }
        radius = next;
      }
      if (!settled) return null;
    }
    final gain = target < 1e-8 ? 0.0 : radius / target;
    final u = tx * gain;
    final v = ty * gain;
    // A negative radial term folds distant directions back into the frame.
    // Keep a pixel only when the forward lens puts it on this same direction.
    final scale = _lensPoly(u * u + v * v, lens);
    final bx = u * scale - tx;
    final by = v * scale - ty;
    if (bx * bx + by * by > 1e-4) return null;
    return (x: originX - u * focalPx, y: originY - v * focalPx);
  }
}

/// Clicks the catalog did not accept, and catalog stars in the frame
/// that no click landed on.
class WideFieldReport {
  final FinderSolution? solution;
  final String? reason;
  final List<({String label, double x, double y})> notInCatalog;
  final List<({String name, double x, double y})> missing;
  final List<
      ({
        String name,
        double x,
        double y,
        double arcmin,
        double vx,
        double vy
      })> matched;

  const WideFieldReport({
    this.solution,
    this.reason,
    this.notInCatalog = const [],
    this.missing = const [],
    this.matched = const [],
  });
}

/// Pose of a rectilinear frame from at least three named stars.
///
/// The focal length starts from [fovDeg]. The lens is a pinhole and the
/// optical center is the middle of the picture.
WideCamera? widePoseFromNamedStars({
  required List<({double raH, double decD, double x, double y})> stars,
  required double width,
  required double height,
  required double fovDeg,
}) {
  if (stars.length < 3 || width < 2 || height < 2 || fovDeg <= 1) return null;
  final long = math.max(width, height);
  final f0 = long / 2 / math.tan(fovDeg * math.pi / 360);
  var bestF = f0;
  const bestLens = LensTerms.none();
  var bestErr = 1e9;
  final cx = width / 2;
  final cy = height / 2;
  final step = math.max(10.0, f0 / 80);
  final separations = <double>[];
  for (var i = 0; i < stars.length; i++) {
    for (var j = i + 1; j < stars.length; j++) {
      separations.add(sphereSeparationDeg(
        stars[i].raH,
        stars[i].decD,
        stars[j].raH,
        stars[j].decD,
      ));
    }
  }
  double pairError(double f, LensTerms lens) {
    if (!_lensOpen(f, lens, width, height)) return 1e9;
    final rays = [
      for (final s in stars)
        _wideRay(s.x, s.y, width, height, f, lens, originX: cx, originY: cy),
    ];
    var err = 0.0;
    var n = 0;
    for (var i = 0; i < stars.length; i++) {
      for (var j = i + 1; j < stars.length; j++) {
        final plate = _wideAngle(rays[i], rays[j]);
        final delta = plate - separations[n];
        err += delta * delta;
        n++;
      }
    }
    return math.sqrt(err / n);
  }

  void consider(double f, LensTerms lens) {
    final rms = pairError(f, lens);
    if (rms < bestErr) {
      bestErr = rms;
      bestF = f;
    }
  }

  for (var f = f0 * 0.75; f <= f0 * 1.25; f += step) {
    consider(f, bestLens);
  }

  var baseA = 0;
  var baseB = 1;
  var baseSep = -1.0;
  for (var i = 0; i < stars.length; i++) {
    for (var j = i + 1; j < stars.length; j++) {
      final sep = sphereSeparationDeg(
        stars[i].raH,
        stars[i].decD,
        stars[j].raH,
        stars[j].decD,
      );
      if (sep > baseSep) {
        baseSep = sep;
        baseA = i;
        baseB = j;
      }
    }
  }
  final camA = _wideRay(
    stars[baseA].x,
    stars[baseA].y,
    width,
    height,
    bestF,
    bestLens,
    originX: cx,
    originY: cy,
  );
  final camB = _wideRay(
    stars[baseB].x,
    stars[baseB].y,
    width,
    height,
    bestF,
    bestLens,
    originX: cx,
    originY: cy,
  );
  final skyA = _wideSky(stars[baseA].raH, stars[baseA].decD);
  final skyB = _wideSky(stars[baseB].raH, stars[baseB].decD);
  final rays = [
    for (final s in stars)
      _wideRay(
        s.x,
        s.y,
        width,
        height,
        bestF,
        bestLens,
        originX: cx,
        originY: cy,
      ),
  ];
  final sky = [for (final s in stars) _wideSky(s.raH, s.decD)];
  List<({double x, double y, double z})>? toSky;
  var namedRms = 1e9;
  for (final flip in [false, true]) {
    final rot = _wideAlign(camA, camB, skyA, skyB, flip);
    var err = 0.0;
    for (var i = 0; i < stars.length; i++) {
      final ang = _wideAngle(_wideApply(rot, rays[i]), sky[i]);
      err += ang * ang;
    }
    final rms = math.sqrt(err / stars.length);
    if (rms < namedRms) {
      namedRms = rms;
      toSky = rot;
    }
  }
  if (toSky == null || namedRms > _wideMatchDeg) return null;
  final axis = _wideApply(toSky, (x: 0, y: 0, z: 1));
  final right = _wideApply(toSky, (x: 1, y: 0, z: 0));
  final dec = math.asin(axis.z.clamp(-1.0, 1.0)) * 180 / math.pi;
  var ra = math.atan2(axis.y, axis.x);
  if (ra < 0) ra += 2 * math.pi;
  final raH = ra * 12 / math.pi;
  final east = (x: -math.sin(ra), y: math.cos(ra), z: 0.0);
  final north = (
    x: -math.sin(dec * math.pi / 180) * math.cos(ra),
    y: -math.sin(dec * math.pi / 180) * math.sin(ra),
    z: math.cos(dec * math.pi / 180),
  );
  final roll = math.atan2(_wideDot(right, north), _wideDot(right, east));
  final fitted = 2 * math.atan(long / (2 * bestF)) * 180 / math.pi;
  return WideCamera(
    solution: FinderSolution(
      raH: raH,
      decD: dec,
      rollRad: roll,
      matches: stars.length,
      rmsDeg: namedRms,
    ),
    fovDeg: fitted,
    focalPx: bestF,
    lens: bestLens,
    originX: cx,
    originY: cy,
    width: width,
    height: height,
    toSky: toSky,
  );
}

/// Catalog match on a wide frame whose pose comes from the named marks.
///
/// A click counts when a catalog star lies within [_wideMatchDeg]. Clicks
/// farther out are reported and left unmatched.
WideFieldReport solveWideField({
  required List<
          ({
            String name,
            bool named,
            double raH,
            double decD,
            double x,
            double y
          })>
      marks,
  required List<FinderStar> catalog,
  required List<String> catalogNames,
  List<double>? catalogMags,
  List<FinderStar>? identifyCatalog,
  List<String>? identifyNames,
  List<double>? identifyMags,
  double? magnitudeLimit,
  double missingMag = 4.5,
  required double width,
  required double height,
  required double fovDeg,
  List<String>? trace,
}) {
  void note(String line) => trace?.add(line);
  final named = [
    for (final m in marks)
      if (m.named) (raH: m.raH, decD: m.decD, x: m.x, y: m.y),
  ];
  final camera = widePoseFromNamedStars(
    stars: named,
    width: width,
    height: height,
    fovDeg: fovDeg,
  );
  if (camera == null) {
    const reason = 'The named stars do not form one camera pose';
    note(reason);
    return const WideFieldReport(reason: reason);
  }
  final pose = camera.solution;
  note(
    'Catalog to magnitude ${wideCatalogMag.toStringAsFixed(1)}. '
    'Match radius ${_wideMatchDeg.toStringAsFixed(1)}°. '
    'Focal length ${camera.focalPx.toStringAsFixed(0)} px, '
    'long side ${camera.fovDeg.toStringAsFixed(1)}°.',
  );
  var rollDeg = pose.rollRad * 180 / math.pi;
  rollDeg %= 360;
  if (rollDeg < 0) rollDeg += 360;
  note(
    'Camera axis ${_traceHours(pose.raH)}, ${_traceDegrees(pose.decD)}, '
    'roll ${rollDeg.toStringAsFixed(1)}° from the named stars. '
    'A click is kept only inside ${_wideMatchDeg.toStringAsFixed(1)}°.',
  );
  final sky = <({double x, double y, double z})>[
    for (final s in catalog) _wideSky(s.raH, s.decD),
  ];
  ({Map<int, ({int cat, double d})> taken, double sumSq}) assign(
      WideCamera cam) {
    final pairs = <({double d, int i, int j})>[];
    for (var i = 0; i < marks.length; i++) {
      final dir = cam.direction(marks[i].x, marks[i].y);
      for (var j = 0; j < sky.length; j++) {
        final d = _wideAngle(dir, sky[j]);
        if (d <= _wideMatchDeg) pairs.add((d: d, i: i, j: j));
      }
    }
    pairs.sort((a, b) => a.d.compareTo(b.d));
    final usedMark = List<bool>.filled(marks.length, false);
    final usedCat = List<bool>.filled(catalog.length, false);
    final taken = <int, ({int cat, double d})>{};
    var sumSq = 0.0;
    for (final p in pairs) {
      if (usedMark[p.i] || usedCat[p.j]) continue;
      usedMark[p.i] = true;
      usedCat[p.j] = true;
      taken[p.i] = (cat: p.j, d: p.d);
      sumSq += p.d * p.d;
    }
    return (taken: taken, sumSq: sumSq);
  }

  var fitted = camera;
  var assigned = assign(camera);
  if (assigned.taken.length >= 3) {
    final again = widePoseFromNamedStars(
      stars: [
        for (final entry in assigned.taken.entries)
          (
            raH: catalog[entry.value.cat].raH,
            decD: catalog[entry.value.cat].decD,
            x: marks[entry.key].x,
            y: marks[entry.key].y,
          ),
      ],
      width: width,
      height: height,
      fovDeg: fovDeg,
    );
    if (again != null) {
      final retry = assign(again);
      final firstMin = (math.sqrt(assigned.sumSq / assigned.taken.length) * 60)
          .toStringAsFixed(1);
      final secondMin = retry.taken.length < 3
          ? 'fewer than 3 stars'
          : "${(math.sqrt(retry.sumSq / retry.taken.length) * 60).toStringAsFixed(1)}'";
      note(
        'Solving again from ${assigned.taken.length} matched stars. '
        'Residual $firstMin\' from the named-star guess, $secondMin from the matched stars.',
      );
      if (retry.taken.length >= 3) {
        fitted = again;
        assigned = retry;
      }
    }
  }
  var labelNames = catalogNames;
  var namedFrom = catalog;
  var published = assigned;
  final identify = identifyCatalog;
  final identifyLabel = identifyNames;
  List<FinderStar>? plotStars;
  List<String>? plotNames;
  if (identify != null &&
      identifyLabel != null &&
      identify.length == identifyLabel.length &&
      identify.isNotEmpty) {
    final named = _nameClicks(
      cam: fitted,
      marks: marks,
      catalog: identify,
      mags: identifyMags,
      radius: _wideMatchDeg,
      tie: _wideTieDeg,
    );
    if (named.taken.length >= 3) {
      var chosen = named;
      var chosenNames = identifyLabel;
      final limit = magnitudeLimit ??
          recognizedMagnitudeLimit([
            for (final hit in named.taken.values)
              if (identifyMags != null && hit.cat < identifyMags.length)
                identifyMags[hit.cat],
          ]);
      if (limit != null && identifyMags != null) {
        final keptStars = <FinderStar>[];
        final keptNames = <String>[];
        final keptMags = <double>[];
        for (var j = 0; j < identify.length; j++) {
          final mag = j < identifyMags.length ? identifyMags[j] : 99.0;
          if (mag > limit) continue;
          keptStars.add(identify[j]);
          keptNames.add(identifyLabel[j]);
          keptMags.add(mag);
        }
        final again = _nameClicks(
          cam: fitted,
          marks: marks,
          catalog: keptStars,
          mags: keptMags,
          radius: _wideMatchDeg,
          tie: _wideTieDeg,
        );
        if (magnitudeLimit != null || again.taken.length >= 3) {
          chosen = again;
          chosenNames = keptNames;
          plotStars = keptStars;
          plotNames = keptNames;
          note(
            magnitudeLimit != null
                ? 'Magnitude limit ${limit.toStringAsFixed(1)}.'
                : 'Magnitude limit ${limit.toStringAsFixed(1)} from the recognized stars. '
                    'A brighter star within ${(_wideTieDeg * 60).toStringAsFixed(0)}\' '
                    'of the nearest one keeps the name.',
          );
        }
      }
      if (plotStars == null) {
        note(
          'Naming each click from stars to magnitude ${wideIdentifyMag.toStringAsFixed(1)}. '
          'A brighter star within ${(_wideTieDeg * 60).toStringAsFixed(0)}\' '
          'of the nearest one keeps the name.',
        );
      }
      published = chosen;
      labelNames = chosenNames;
      namedFrom = plotStars ?? identify;
    }
  }
  final taken = published.taken;
  final sumSq = published.sumSq;
  final deepNames = !identical(labelNames, catalogNames);
  final usedMark = List<bool>.filled(marks.length, false);
  final usedCat = List<bool>.filled(catalog.length, false);
  final matchedNames = <String>{};
  for (final entry in taken.entries) {
    usedMark[entry.key] = true;
    matchedNames.add(labelNames[entry.value.cat]);
    if (!deepNames) usedCat[entry.value.cat] = true;
  }
  final poseNow = fitted.solution;
  for (var i = 0; i < marks.length; i++) {
    final label = marks[i].named ? marks[i].name : 'Unnamed ${marks[i].name}';
    final hit = taken[i];
    if (hit == null) {
      note(
          '$label  no catalog star within ${_wideMatchDeg.toStringAsFixed(1)}°.');
      continue;
    }
    final who = labelNames[hit.cat];
    note("$label  $who  ${(hit.d * 60).toStringAsFixed(1)}'");
  }
  final notInCatalog = <({String label, double x, double y})>[
    for (var i = 0; i < marks.length; i++)
      if (!usedMark[i])
        (
          label: marks[i].named ? marks[i].name : 'Unnamed ${marks[i].name}',
          x: marks[i].x,
          y: marks[i].y,
        ),
  ];
  final missing = <({String name, double x, double y})>[];
  final plotted = plotStars;
  final plottedNames = plotNames;
  if (plotted != null && plottedNames != null) {
    for (var j = 0; j < plotted.length; j++) {
      if (matchedNames.contains(plottedNames[j])) continue;
      final place = fitted.pixel(_wideSky(plotted[j].raH, plotted[j].decD));
      if (place == null) continue;
      if (place.x < 0 || place.y < 0 || place.x > width || place.y > height) {
        continue;
      }
      missing.add((name: plottedNames[j], x: place.x, y: place.y));
    }
  } else {
    for (var j = 0; j < catalog.length; j++) {
      if (deepNames) {
        if (matchedNames.contains(catalogNames[j])) continue;
      } else if (usedCat[j]) {
        continue;
      }
      if (catalogMags != null && catalogMags[j] > missingMag) continue;
      final place = fitted.pixel(sky[j]);
      if (place == null) continue;
      if (place.x < 0 || place.y < 0 || place.x > width || place.y > height) {
        continue;
      }
      missing.add((name: catalogNames[j], x: place.x, y: place.y));
    }
  }
  final matched = <({
    String name,
    double x,
    double y,
    double arcmin,
    double vx,
    double vy
  })>[];
  for (final entry in taken.entries) {
    final mark = marks[entry.key];
    final cat = entry.value.cat;
    var vx = 0.0;
    var vy = 0.0;
    if (cat >= 0 && cat < namedFrom.length) {
      final place =
          fitted.pixel(_wideSky(namedFrom[cat].raH, namedFrom[cat].decD));
      if (place != null) {
        vx = place.x - mark.x;
        vy = place.y - mark.y;
      }
    }
    matched.add((
      name: labelNames[cat],
      x: mark.x,
      y: mark.y,
      arcmin: entry.value.d * 60,
      vx: vx,
      vy: vy,
    ));
  }
  missing.sort((a, b) => a.name.compareTo(b.name));
  note(notInCatalog.isEmpty
      ? 'Not in the catalog: none.'
      : 'Not in the catalog: ${notInCatalog.length}.');
  for (final star in notInCatalog) {
    note('  ${star.label}');
  }
  note(missing.isEmpty
      ? 'Missing in the frame: none.'
      : 'Missing in the frame: ${missing.length}.');
  for (final star in missing) {
    note('  ${star.name}');
  }
  if (taken.length < 3) {
    const reason = 'The field did not match the catalog';
    note(reason);
    return WideFieldReport(
      reason: reason,
      notInCatalog: notInCatalog,
      missing: missing,
      matched: matched,
    );
  }
  final rms = math.sqrt(sumSq / taken.length);
  note(
    'Camera axis ${_traceHours(poseNow.raH)}, ${_traceDegrees(poseNow.decD)}. '
    '${taken.length} stars, residual ${(rms * 60).toStringAsFixed(1)}\'.',
  );
  return WideFieldReport(
    solution: FinderSolution(
      raH: poseNow.raH,
      decD: poseNow.decD,
      rollRad: poseNow.rollRad,
      matches: taken.length,
      rmsDeg: rms,
    ),
    notInCatalog: notInCatalog,
    missing: missing,
    matched: matched,
  );
}

/// Clicks farther than this from every catalog star stay unmatched.
const double _wideMatchDeg = 1.0;

/// Two catalog stars this close, in degrees, compete and the brighter one is named.
const double _wideTieDeg = 0.08;

/// Faintest star used when a solved plate names a click.
const double wideIdentifyMag = 8.0;

/// Gap, in magnitudes, that ends the run of recognized stars.
const double _wideMagGap = 0.5;

/// Recognized stars required before a gap can set the magnitude limit.
const int _wideMagRun = 8;

/// Faintest magnitude in the continuous run of recognized stars.
///
/// A gap wider than [_wideMagGap] ends the run. The result is null when fewer
/// than [_wideMagRun] stars sit on the bright side of such a gap.
double? recognizedMagnitudeLimit(List<double> magnitudes) {
  if (magnitudes.length < _wideMagRun) return null;
  final sorted = [...magnitudes]..sort();
  for (var i = _wideMagRun; i < sorted.length; i++) {
    if (sorted[i] - sorted[i - 1] > _wideMagGap) return sorted[i - 1];
  }
  return null;
}

({Map<int, ({int cat, double d})> taken, double sumSq}) _nameClicks({
  required WideCamera cam,
  required List<
          ({
            String name,
            bool named,
            double raH,
            double decD,
            double x,
            double y
          })>
      marks,
  required List<FinderStar> catalog,
  required List<double>? mags,
  required double radius,
  required double tie,
}) {
  final sky = <({double x, double y, double z})>[
    for (final s in catalog) _wideSky(s.raH, s.decD),
  ];
  final hits = <List<({double d, int j})>>[];
  for (var i = 0; i < marks.length; i++) {
    final dir = cam.direction(marks[i].x, marks[i].y);
    final row = <({double d, int j})>[];
    for (var j = 0; j < sky.length; j++) {
      final d = _wideAngle(dir, sky[j]);
      if (d <= radius) row.add((d: d, j: j));
    }
    row.sort((a, b) => a.d.compareTo(b.d));
    hits.add(row);
  }
  final order = [for (var i = 0; i < marks.length; i++) i]..sort((a, b) {
      final da = hits[a].isEmpty ? 99.0 : hits[a].first.d;
      final db = hits[b].isEmpty ? 99.0 : hits[b].first.d;
      return da.compareTo(db);
    });
  final used = List<bool>.filled(catalog.length, false);
  final taken = <int, ({int cat, double d})>{};
  var sumSq = 0.0;
  for (final i in order) {
    final open = <({double d, int j})>[
      for (final hit in hits[i])
        if (!used[hit.j]) hit,
    ];
    if (open.isEmpty) continue;
    final nearest = open.first.d;
    var pick = open.first;
    if (mags != null) {
      for (final hit in open) {
        if (hit.d > nearest + tie) break;
        final mag = hit.j < mags.length ? mags[hit.j] : 99.0;
        final best = pick.j < mags.length ? mags[pick.j] : 99.0;
        if (mag < best || (mag == best && hit.d < pick.d)) pick = hit;
      }
    }
    used[pick.j] = true;
    taken[i] = (cat: pick.j, d: pick.d);
    sumSq += pick.d * pick.d;
  }
  return (taken: taken, sumSq: sumSq);
}

/// A typed center this far from the axis can still be solved, in degrees.
const double _wideHintDeg = 8.0;

class _MarkVote {
  final double raH;
  final double decD;
  final double east;
  final double north;
  final double ang;
  const _MarkVote(this.raH, this.decD, this.east, this.north, this.ang);
}

class _WideBin {
  final votes = <int, _MarkVote>{};

  void add(int mark, _MarkVote vote) {
    final prev = votes[mark];
    if (prev != null && prev.ang <= vote.ang) return;
    votes[mark] = vote;
  }

  int get n => votes.length;

  (double, double) mean() => _meanPositions([
        for (final v in votes.values) (v.raH, v.decD),
      ]);

  double scatter() {
    if (votes.isEmpty) return 1e9;
    var se = 0.0;
    var sn = 0.0;
    for (final v in votes.values) {
      se += v.east;
      sn += v.north;
    }
    final me = se / n;
    final mn = sn / n;
    var ss = 0.0;
    for (final v in votes.values) {
      final de = v.east - me;
      final dn = v.north - mn;
      ss += de * de + dn * dn;
    }
    return ss / n;
  }
}

class _WideAssign {
  final Map<int, ({int cat, double d})> taken;
  final double sumSq;
  const _WideAssign(this.taken, this.sumSq);

  int get count => taken.length;
  double get rms => count == 0 ? 1e9 : math.sqrt(sumSq / count);
}

class _ScalePick {
  final int scale;
  final double rollDeg;
  final double raH;
  final double decD;
  final int votes;
  final double scatter;
  const _ScalePick({
    required this.scale,
    required this.rollDeg,
    required this.raH,
    required this.decD,
    required this.votes,
    required this.scatter,
  });
}

class _Polished {
  final double raH;
  final double decD;
  final double rollDeg;
  final _WideAssign asg;
  const _Polished(this.raH, this.decD, this.rollDeg, this.asg);
}

/// Plate solve from a typed center when fewer than three stars are named.
///
/// The picture is the pinhole used for named stars. Roll is searched, and a
/// center within [_wideHintDeg] of [hintRa] and [hintDec] can still match.
/// The typed field is tried from 90% to 110%. One or two named marks fix
/// the center for each roll. Three or more matches are then fitted with
/// [widePoseFromNamedStars].
WideFieldReport solveWideFromHint({
  required List<
          ({
            String name,
            bool named,
            double raH,
            double decD,
            double x,
            double y
          })>
      marks,
  required List<FinderStar> catalog,
  required List<String> catalogNames,
  List<double>? catalogMags,
  List<FinderStar>? identifyCatalog,
  List<String>? identifyNames,
  List<double>? identifyMags,
  double? magnitudeLimit,
  double missingMag = 4.5,
  required double width,
  required double height,
  required double fovDeg,
  required double hintRa,
  required double hintDec,
  double? latDeg,
  double? lstH,
  List<String>? trace,
}) {
  void note(String line) => trace?.add(line);
  if (marks.length < 4 ||
      catalog.isEmpty ||
      width < 2 ||
      height < 2 ||
      fovDeg <= 1) {
    const reason = 'Not enough stars in the frame';
    note('$reason (${marks.length}).');
    return const WideFieldReport(reason: reason);
  }
  final haveSite = latDeg != null && lstH != null;
  const lens = LensTerms.none();
  final longSide = math.max(width, height);
  final focal0 = longSide / 2 / math.tan(fovDeg * math.pi / 360);
  final sky = <({double x, double y, double z})>[
    for (final s in catalog) _wideSky(s.raH, s.decD),
  ];
  final nearby = <int>[];
  var below = 0;
  for (var j = 0; j < catalog.length; j++) {
    final s = catalog[j];
    if (sphereSeparationDeg(s.raH, s.decD, hintRa, hintDec) > fovDeg * 0.75) {
      continue;
    }
    if (haveSite && altitudeDeg(latDeg, lstH, s.raH, s.decD) <= 0) {
      below++;
      continue;
    }
    nearby.add(j);
  }
  note(
    'Hint ${_traceHours(hintRa)}, ${_traceDegrees(hintDec)}. '
    'Search radius ${_wideHintDeg.toStringAsFixed(0)}°. '
    'Catalog stars near the hint: ${nearby.length}. '
    'Below the horizon and left out: $below.',
  );
  note('The typed field is tried from 90% to 110%.');
  if (nearby.length < 4) {
    final reason = below > 0
        ? 'Not enough stars above the horizon'
        : 'Not enough catalog stars near the hint';
    note(reason);
    return WideFieldReport(reason: reason);
  }

  final anchors = <int>[
    for (var i = 0; i < marks.length; i++)
      if (marks[i].named) i,
  ];
  final anchorSky = <({double x, double y, double z})>[
    for (final i in anchors) _wideSky(marks[i].raH, marks[i].decD),
  ];
  if (anchors.isNotEmpty) {
    note(anchors.length == 1
        ? '1 named mark fixes the center for each roll.'
        : '${anchors.length} named marks fix the center for each roll.');
  }

  final scales = <double>[
    for (var i = 0; i <= 8; i++) 0.90 + i * 0.025,
  ];
  final raySets = <List<({double x, double y, double z})>>[
    for (final scale in scales)
      [
        for (final m in marks)
          _wideRay(
            m.x,
            m.y,
            width,
            height,
            focal0 * scale,
            lens,
          ),
      ],
  ];

  (double, double)? anchorCenter(
    List<({double x, double y, double z})> rays,
    double roll,
  ) {
    final found = <(double, double)>[];
    for (var n = 0; n < anchors.length; n++) {
      final axis = _axisForRay(
        ray: rays[anchors[n]],
        star: anchorSky[n],
        roll: roll,
      );
      if (axis == null) return null;
      found.add(axis);
    }
    final mean = found.length == 1 ? found.first : _meanPositions(found);
    if (sphereSeparationDeg(mean.$1, mean.$2, hintRa, hintDec) > _wideHintDeg) {
      return null;
    }
    return mean;
  }

  final found = <_ScalePick>[];
  for (var s = 0; s < scales.length; s++) {
    final rays = raySets[s];
    for (var deg = 0; deg < 360; deg++) {
      final roll = deg * math.pi / 180;
      if (anchors.isNotEmpty) {
        final center = anchorCenter(rays, roll);
        if (center == null) continue;
        final asg = _assignWide(
          basis: _wideBasis(center.$1, center.$2, roll),
          rays: rays,
          nearby: nearby,
          sky: sky,
          radius: _wideMatchDeg,
        );
        if (asg.count < 3) continue;
        found.add(_ScalePick(
          scale: s,
          rollDeg: deg.toDouble(),
          raH: center.$1,
          decD: center.$2,
          votes: asg.count,
          scatter: asg.rms,
        ));
        continue;
      }
      final vote = _bestCenterVote(
        rays: rays,
        nearby: nearby,
        sky: sky,
        roll: roll,
        hintRa: hintRa,
        hintDec: hintDec,
      );
      if (vote == null) continue;
      found.add(_ScalePick(
        scale: s,
        rollDeg: deg.toDouble(),
        raH: vote.$1,
        decD: vote.$2,
        votes: vote.$3,
        scatter: vote.$4,
      ));
    }
  }
  if (found.isEmpty) {
    note('360 rolls at 1°. No group of 3 stars agreed.');
    const reason = 'The field did not match the catalog';
    note(reason);
    return const WideFieldReport(reason: reason);
  }
  found.sort((a, b) {
    if (a.votes != b.votes) return b.votes.compareTo(a.votes);
    return a.scatter.compareTo(b.scatter);
  });
  final shortlist = found.take(12).toList();
  final lead = shortlist.first;
  note(
    'Strongest agreement ${lead.votes} at roll ${lead.rollDeg.toStringAsFixed(1)}°, '
    'field scale ${scales[lead.scale].toStringAsFixed(2)}.',
  );

  _Polished? chosen;
  for (final pick in shortlist) {
    final polished = _polishHint(
      rays: raySets[pick.scale],
      nearby: nearby,
      sky: sky,
      catalog: catalog,
      rollDeg: pick.rollDeg,
      raH: pick.raH,
      decD: pick.decD,
      hintRa: hintRa,
      hintDec: hintDec,
      anchorAt: anchors.isEmpty
          ? null
          : (double roll) => anchorCenter(raySets[pick.scale], roll),
    );
    if (polished == null) continue;
    final asg = polished.asg;
    final kept = chosen;
    if (kept != null &&
        (asg.count < kept.asg.count ||
            (asg.count == kept.asg.count && asg.rms >= kept.asg.rms))) {
      continue;
    }
    chosen = polished;
  }
  final polished = chosen;
  if (polished == null) {
    const reason = 'The field did not match the catalog';
    note(reason);
    return const WideFieldReport(reason: reason);
  }
  var rollShown = polished.rollDeg % 360;
  if (rollShown < 0) rollShown += 360;
  note(
    'Pinhole camera around ${_traceHours(hintRa)}, ${_traceDegrees(hintDec)}, '
    'roll ${rollShown.toStringAsFixed(1)}°.',
  );
  note(
    '${polished.asg.count} clicks agree inside ${_wideMatchDeg.toStringAsFixed(1)}°. '
    'Fitting the camera from those stars.',
  );
  final seeded = <({
    String name,
    bool named,
    double raH,
    double decD,
    double x,
    double y
  })>[];
  for (var i = 0; i < marks.length; i++) {
    final hit = polished.asg.taken[i];
    if (hit == null) {
      seeded.add(marks[i]);
      continue;
    }
    final star = catalog[hit.cat];
    final label = marks[i].named ? marks[i].name : 'Unnamed ${marks[i].name}';
    seeded.add((
      name: label,
      named: true,
      raH: star.raH,
      decD: star.decD,
      x: marks[i].x,
      y: marks[i].y,
    ));
  }
  return solveWideField(
    marks: seeded,
    catalog: catalog,
    catalogNames: catalogNames,
    catalogMags: catalogMags,
    identifyCatalog: identifyCatalog,
    identifyNames: identifyNames,
    identifyMags: identifyMags,
    magnitudeLimit: magnitudeLimit,
    missingMag: missingMag,
    width: width,
    height: height,
    fovDeg: fovDeg,
    trace: trace,
  );
}

/// Strongest cluster of implied centers for one roll. Null when fewer than
/// three clicks agree. The record is ra hours, dec degrees, count, scatter.
(double, double, int, double)? _bestCenterVote({
  required List<({double x, double y, double z})> rays,
  required List<int> nearby,
  required List<({double x, double y, double z})> sky,
  required double roll,
  required double hintRa,
  required double hintDec,
}) {
  final basis = _wideBasis(hintRa, hintDec, roll);
  final pointed = [for (final ray in rays) _wideApply(basis, ray)];
  final bins0 = <int, _WideBin>{};
  final bins1 = <int, _WideBin>{};
  final near = math.cos(_wideHintDeg * math.pi / 180);
  for (final j in nearby) {
    final star = sky[j];
    for (var i = 0; i < pointed.length; i++) {
      final dot = _wideDot(pointed[i], star);
      if (dot < near) continue;
      final ang = math.acos(dot.clamp(-1.0, 1.0)) * 180 / math.pi;
      if (ang > _wideHintDeg) continue;
      final axis = _axisForRay(ray: rays[i], star: star, roll: roll);
      if (axis == null) continue;
      if (sphereSeparationDeg(axis.$1, axis.$2, hintRa, hintDec) >
          _wideHintDeg) {
        continue;
      }
      final (east, north) = skyOffset(axis.$1, axis.$2, hintRa, hintDec);
      final vote = _MarkVote(axis.$1, axis.$2, east, north, ang);
      _dropVote(bins0, east, north, 0, i, vote);
      _dropVote(bins1, east, north, 0.5, i, vote);
    }
  }
  _WideBin? best;
  for (final bins in [bins0, bins1]) {
    for (final bin in bins.values) {
      if (bin.n < 3) continue;
      if (best == null ||
          bin.n > best.n ||
          (bin.n == best.n && bin.scatter() < best.scatter())) {
        best = bin;
      }
    }
  }
  if (best == null) return null;
  final mean = best.mean();
  return (mean.$1, mean.$2, best.n, best.scatter());
}

void _dropVote(
  Map<int, _WideBin> bins,
  double east,
  double north,
  double origin,
  int mark,
  _MarkVote vote,
) {
  const bin = 1.0;
  final ie = ((east - origin) / bin).round();
  final inn = ((north - origin) / bin).round();
  bins.putIfAbsent(ie * 1000 + inn, () => _WideBin()).add(mark, vote);
}

_WideAssign _assignWide({
  required List<({double x, double y, double z})> basis,
  required List<({double x, double y, double z})> rays,
  required List<int> nearby,
  required List<({double x, double y, double z})> sky,
  required double radius,
}) {
  final pointed = [for (final ray in rays) _wideApply(basis, ray)];
  final pairs = <({double d, int i, int j})>[];
  final near = math.cos(radius * math.pi / 180);
  for (var i = 0; i < pointed.length; i++) {
    for (final j in nearby) {
      final dot = _wideDot(pointed[i], sky[j]);
      if (dot < near) continue;
      final d = math.acos(dot.clamp(-1.0, 1.0)) * 180 / math.pi;
      if (d <= radius) pairs.add((d: d, i: i, j: j));
    }
  }
  pairs.sort((a, b) => a.d.compareTo(b.d));
  final usedMark = List<bool>.filled(rays.length, false);
  final usedCat = List<bool>.filled(sky.length, false);
  final taken = <int, ({int cat, double d})>{};
  var sumSq = 0.0;
  for (final p in pairs) {
    if (usedMark[p.i] || usedCat[p.j]) continue;
    usedMark[p.i] = true;
    usedCat[p.j] = true;
    taken[p.i] = (cat: p.j, d: p.d);
    sumSq += p.d * p.d;
  }
  return _WideAssign(taken, sumSq);
}

_Polished? _polishHint({
  required List<({double x, double y, double z})> rays,
  required List<int> nearby,
  required List<({double x, double y, double z})> sky,
  required List<FinderStar> catalog,
  required double rollDeg,
  required double raH,
  required double decD,
  required double hintRa,
  required double hintDec,
  required (double, double)? Function(double roll)? anchorAt,
}) {
  _Polished? best;
  void consider(double ra, double dec, double roll) {
    if (sphereSeparationDeg(ra, dec, hintRa, hintDec) > _wideHintDeg) return;
    final asg = _assignWide(
      basis: _wideBasis(ra, dec, roll * math.pi / 180),
      rays: rays,
      nearby: nearby,
      sky: sky,
      radius: _wideMatchDeg,
    );
    if (asg.count < 3) return;
    final kept = best;
    if (kept != null &&
        (asg.count < kept.asg.count ||
            (asg.count == kept.asg.count && asg.rms >= kept.asg.rms))) {
      return;
    }
    best = _Polished(ra, dec, roll, asg);
  }

  (double, double) shifted(
      double ra, double dec, double roll, _WideAssign asg) {
    final basis = _wideBasis(ra, dec, roll * math.pi / 180);
    final centers = <(double, double)>[];
    for (final entry in asg.taken.entries) {
      final pred = _wideApply(basis, rays[entry.key]);
      final where = _vecToRaDec(pred);
      final star = catalog[entry.value.cat];
      final (east, north) = skyOffset(star.raH, star.decD, where.$1, where.$2);
      centers.add(offsetToSky(ra, dec, east, north));
    }
    return _meanPositions(centers);
  }

  var ra = raH;
  var dec = decD;
  final anchored = anchorAt;
  if (anchored != null) {
    final fixed = anchored(rollDeg * math.pi / 180);
    if (fixed == null) return null;
    ra = fixed.$1;
    dec = fixed.$2;
    consider(ra, dec, rollDeg);
  } else {
    for (var iter = 0; iter < 4; iter++) {
      final asg = _assignWide(
        basis: _wideBasis(ra, dec, rollDeg * math.pi / 180),
        rays: rays,
        nearby: nearby,
        sky: sky,
        radius: _wideMatchDeg,
      );
      consider(ra, dec, rollDeg);
      if (asg.count < 3) break;
      final next = shifted(ra, dec, rollDeg, asg);
      if (sphereSeparationDeg(next.$1, next.$2, hintRa, hintDec) >
          _wideHintDeg) {
        break;
      }
      if (sphereSeparationDeg(next.$1, next.$2, ra, dec) < 0.01) {
        ra = next.$1;
        dec = next.$2;
        consider(ra, dec, rollDeg);
        break;
      }
      ra = next.$1;
      dec = next.$2;
    }
  }

  final baseRa = ra;
  final baseDec = dec;
  for (var step = -10; step <= 10; step++) {
    final roll = rollDeg + step * 0.1;
    if (anchored != null) {
      final fixed = anchored(roll * math.pi / 180);
      if (fixed == null) continue;
      consider(fixed.$1, fixed.$2, roll);
      continue;
    }
    var tra = baseRa;
    var tdec = baseDec;
    final asg = _assignWide(
      basis: _wideBasis(tra, tdec, roll * math.pi / 180),
      rays: rays,
      nearby: nearby,
      sky: sky,
      radius: _wideMatchDeg,
    );
    if (asg.count >= 3) {
      final next = shifted(tra, tdec, roll, asg);
      if (sphereSeparationDeg(next.$1, next.$2, hintRa, hintDec) <=
          _wideHintDeg) {
        tra = next.$1;
        tdec = next.$2;
      }
    }
    consider(tra, tdec, roll);
  }
  return best;
}

/// Axis that places [ray] on [star] at [roll]. Null when that roll cannot.
(double, double)? _axisForRay({
  required ({double x, double y, double z}) ray,
  required ({double x, double y, double z}) star,
  required double roll,
}) {
  final c = math.cos(roll);
  final s = math.sin(roll);
  final ex = ray.x * c - ray.y * s;
  final ny = ray.x * s + ray.y * c;
  final az = ray.z;
  final h = math.sqrt(ny * ny + az * az);
  if (h < 1e-8) return null;
  final phi = math.atan2(ny, az);
  final base = math.asin((star.z / h).clamp(-1.0, 1.0));
  (double, double)? best;
  var bestErr = 1e9;
  for (final psi in [base, math.pi - base]) {
    var dec = psi - phi;
    while (dec > math.pi) {
      dec -= 2 * math.pi;
    }
    while (dec < -math.pi) {
      dec += 2 * math.pi;
    }
    if (dec.abs() > math.pi / 2) continue;
    final sinD = math.sin(dec);
    final cosD = math.cos(dec);
    final p = ex;
    final q = -ny * sinD + az * cosD;
    final norm = p * p + q * q;
    if (norm < 1e-8) continue;
    final cosA = (q * star.x + p * star.y) / norm;
    final sinA = (q * star.y - p * star.x) / norm;
    final ra = math.atan2(sinA, cosA);
    final axis = (
      x: cosD * math.cos(ra),
      y: cosD * math.sin(ra),
      z: sinD,
    );
    final east = (x: -math.sin(ra), y: math.cos(ra), z: 0.0);
    final north = (
      x: -sinD * math.cos(ra),
      y: -sinD * math.sin(ra),
      z: cosD,
    );
    final pred = (
      x: ex * east.x + ny * north.x + az * axis.x,
      y: ex * east.y + ny * north.y + az * axis.y,
      z: ex * east.z + ny * north.z + az * axis.z,
    );
    final err = _wideAngle(pred, star);
    if (err < bestErr) {
      bestErr = err;
      var hours = ra * 12 / math.pi;
      if (hours < 0) hours += 24;
      best = (hours, dec * 180 / math.pi);
    }
  }
  if (best == null || bestErr > 0.05) return null;
  return best;
}

/// Camera basis at [raH], [decD] and [roll]. Rows map a camera vector to the sky.
List<({double x, double y, double z})> _wideBasis(
  double raH,
  double decD,
  double roll,
) {
  final ra = raH * math.pi / 12;
  final dec = decD * math.pi / 180;
  final cosD = math.cos(dec);
  final sinD = math.sin(dec);
  final cosA = math.cos(ra);
  final sinA = math.sin(ra);
  final axis = (x: cosD * cosA, y: cosD * sinA, z: sinD);
  final east = (x: -sinA, y: cosA, z: 0.0);
  final north = (x: -sinD * cosA, y: -sinD * sinA, z: cosD);
  final c = math.cos(roll);
  final s = math.sin(roll);
  final camX = (
    x: c * east.x + s * north.x,
    y: c * east.y + s * north.y,
    z: c * east.z + s * north.z,
  );
  final camY = (
    x: -s * east.x + c * north.x,
    y: -s * east.y + c * north.y,
    z: -s * east.z + c * north.z,
  );
  return [
    (x: camX.x, y: camY.x, z: axis.x),
    (x: camX.y, y: camY.y, z: axis.y),
    (x: camX.z, y: camY.z, z: axis.z),
  ];
}

(double, double) _vecToRaDec(({double x, double y, double z}) v) {
  final dec = math.asin(v.z.clamp(-1.0, 1.0)) * 180 / math.pi;
  var ra = math.atan2(v.y, v.x);
  if (ra < 0) ra += 2 * math.pi;
  return (ra * 12 / math.pi, dec);
}

/// Faintest star the wide-field catalog uses when assigning a click.
const double wideCatalogMag = 5.5;

({double x, double y, double z}) _wideRay(
  double x,
  double y,
  double width,
  double height,
  double f,
  LensTerms lens, {
  double? originX,
  double? originY,
}) {
  // East is to the left of a picture whose top is north.
  var dx = (originX ?? width / 2) - x;
  var dy = (originY ?? height / 2) - y;
  final r2 = (dx * dx + dy * dy) / (f * f);
  final scale = _lensPoly(r2, lens);
  dx *= scale;
  dy *= scale;
  final n = math.sqrt(dx * dx + dy * dy + f * f);
  return (x: dx / n, y: dy / n, z: f / n);
}

({double x, double y, double z}) _wideSky(double raH, double decD) {
  final ra = raH * 15 * math.pi / 180;
  final dec = decD * math.pi / 180;
  return (
    x: math.cos(dec) * math.cos(ra),
    y: math.cos(dec) * math.sin(ra),
    z: math.sin(dec),
  );
}

({double x, double y, double z}) _wideCross(
  ({double x, double y, double z}) a,
  ({double x, double y, double z}) b,
) {
  return (
    x: a.y * b.z - a.z * b.y,
    y: a.z * b.x - a.x * b.z,
    z: a.x * b.y - a.y * b.x,
  );
}

({double x, double y, double z}) _wideUnit(({double x, double y, double z}) v) {
  final n = math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return (x: v.x / n, y: v.y / n, z: v.z / n);
}

double _wideDot(
  ({double x, double y, double z}) a,
  ({double x, double y, double z}) b,
) =>
    a.x * b.x + a.y * b.y + a.z * b.z;

double _wideAngle(
  ({double x, double y, double z}) a,
  ({double x, double y, double z}) b,
) =>
    math.acos(_wideDot(a, b).clamp(-1.0, 1.0)) * 180 / math.pi;

({double x, double y, double z}) _wideApply(
  List<({double x, double y, double z})> rot,
  ({double x, double y, double z}) v,
) {
  return (
    x: rot[0].x * v.x + rot[0].y * v.y + rot[0].z * v.z,
    y: rot[1].x * v.x + rot[1].y * v.y + rot[1].z * v.z,
    z: rot[2].x * v.x + rot[2].y * v.y + rot[2].z * v.z,
  );
}

List<({double x, double y, double z})> _wideAlign(
  ({double x, double y, double z}) camA,
  ({double x, double y, double z}) camB,
  ({double x, double y, double z}) skyA,
  ({double x, double y, double z}) skyB,
  bool flip,
) {
  final c2 = _wideUnit(_wideCross(camA, camB));
  final c1 = _wideUnit(_wideCross(c2, camA));
  var s2 = _wideUnit(_wideCross(skyA, skyB));
  if (flip) s2 = (x: -s2.x, y: -s2.y, z: -s2.z);
  final s1 = _wideUnit(_wideCross(s2, skyA));
  ({double x, double y, double z}) row(int i) {
    double pick(({double x, double y, double z}) v) =>
        i == 0 ? v.x : (i == 1 ? v.y : v.z);
    return (
      x: pick(skyA) * camA.x + pick(s1) * c1.x + pick(s2) * c2.x,
      y: pick(skyA) * camA.y + pick(s1) * c1.y + pick(s2) * c2.y,
      z: pick(skyA) * camA.z + pick(s1) * c1.z + pick(s2) * c2.z,
    );
  }

  return [row(0), row(1), row(2)];
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

/// Altitude of a sky position, in degrees. Positive is above the horizon.
double altitudeDeg(double latDeg, double lstH, double raH, double decD) {
  final lat = latDeg * math.pi / 180;
  final dec = decD * math.pi / 180;
  var ha = (lstH - raH) * 15 * math.pi / 180;
  while (ha > math.pi) {
    ha -= 2 * math.pi;
  }
  while (ha < -math.pi) {
    ha += 2 * math.pi;
  }
  final sinAlt = math.sin(lat) * math.sin(dec) +
      math.cos(lat) * math.cos(dec) * math.cos(ha);
  return math.asin(sinAlt.clamp(-1.0, 1.0)) * 180 / math.pi;
}


String _traceHours(double hours) {
  var total = (wrapHours(hours) * 3600).round();
  final h = total ~/ 3600;
  total -= h * 3600;
  final m = total ~/ 60;
  final s = total - m * 60;
  return '${h.toString().padLeft(2, '0')}:'
      '${m.toString().padLeft(2, '0')}:'
      '${s.toString().padLeft(2, '0')}';
}

String _traceDegrees(double degrees) {
  final sign = degrees < 0 ? '-' : '+';
  var total = (degrees.abs() * 60).round();
  final d = total ~/ 60;
  final m = total - d * 60;
  return "$sign$d°${m.toString().padLeft(2, '0')}'";
}


/// One automatic alignment place. [raH] is hours, [decD] is degrees.
class PhoneAlignPlace {
  final double raH;
  final double decD;
  const PhoneAlignPlace(this.raH, this.decD);
}

/// Places for a from-home phone alignment. [flipAfterThird] is the 3+3 pier change.
class PhoneAlignPlan {
  final List<PhoneAlignPlace> places;
  final bool flipAfterThird;
  const PhoneAlignPlan({required this.places, required this.flipAfterThird});
}

double _wrap360(double deg) {
  var x = deg % 360.0;
  if (x < 0) x += 360.0;
  return x;
}

double _placeAltitude(double latDeg, double lstH, double raH, double decD) =>
    altitudeDeg(latDeg, lstH, raH, decD);

bool _placeReaches(
  double raH,
  double decD, {
  required double lstH,
  required double latDeg,
  required double minAlt,
  required double maxAlt,
}) {
  final alt = _placeAltitude(latDeg, lstH, raH, decD);
  if (alt < minAlt + 12) return false;
  if (alt > maxAlt - 8) return false;
  if (alt > 78) return false;
  return true;
}

List<PhoneAlignPlace>? _placesFromHours({
  required List<double> hours,
  required List<double> decOff,
  required double lstH,
  required double dec0,
  required double latDeg,
  required double minAlt,
  required double maxAlt,
}) {
  final places = <PhoneAlignPlace>[];
  for (var i = 0; i < hours.length; i++) {
    final dec = dec0 + decOff[i];
    final raH = _wrap360(lstH * 15.0 - hours[i] * 15.0) / 15.0;
    if (!_placeReaches(raH, dec,
        lstH: lstH, latDeg: latDeg, minAlt: minAlt, maxAlt: maxAlt)) {
      return null;
    }
    places.add(PhoneAlignPlace(raH, dec));
  }
  return places;
}

List<PhoneAlignPlace>? _onePier({
  required int count,
  required double sign,
  required double lstH,
  required double dec0,
  required double latDeg,
  required double minAlt,
  required double maxAlt,
}) {
  final List<double> hours;
  final List<double> decOff;
  if (count == 2) {
    hours = [2.2, 5.0];
    decOff = [-12.0, 8.0];
  } else if (count == 4) {
    hours = [1.6, 3.0, 4.4, 5.8];
    decOff = [-18.0, -4.0, 8.0, 14.0];
  } else if (count == 3) {
    hours = [2.0, 3.6, 5.2];
    decOff = [-18.0, 0.0, 12.0];
  } else {
    return null;
  }
  return _placesFromHours(
    hours: [for (final h in hours) sign * h],
    decOff: decOff,
    lstH: lstH,
    dec0: dec0,
    latDeg: latDeg,
    minAlt: minAlt,
    maxAlt: maxAlt,
  );
}

List<PhoneAlignPlace>? _spread({
  required int count,
  required double lstH,
  required double dec0,
  required double latDeg,
  required double minAlt,
  required double maxAlt,
}) {
  final List<double> hours;
  final List<double> decOff;
  if (count == 2) {
    hours = [-3.5, 3.5];
    decOff = [-10.0, 8.0];
  } else if (count == 4) {
    hours = [-4.8, -2.0, 2.0, 4.8];
    decOff = [-12.0, 6.0, -4.0, 14.0];
  } else if (count == 6) {
    hours = [-5.0, -3.2, -1.4, 1.4, 3.2, 5.0];
    decOff = [-10.0, 8.0, -16.0, 12.0, 0.0, -8.0];
  } else {
    return null;
  }
  return _placesFromHours(
    hours: hours,
    decOff: decOff,
    lstH: lstH,
    dec0: dec0,
    latDeg: latDeg,
    minAlt: minAlt,
    maxAlt: maxAlt,
  );
}

double _clampDec(double latDeg, double limit) {
  if (latDeg > limit) return limit;
  if (latDeg < -limit) return -limit;
  return latDeg;
}

/// From-home plans, preferred hour-angle sign first.
/// An empty list means no set of places stays inside the altitude margins.
/// [altAz] spreads the places. Every other mount keeps 2 and 4 on one pier,
/// and 6 is three, a flip, then three on the other pier.
List<PhoneAlignPlan> phoneAlignPlans({
  required int stars,
  required double lstH,
  required double latDeg,
  required bool altAz,
  double minAlt = 0,
  double maxAlt = 90,
}) {
  if (stars != 2 && stars != 4 && stars != 6) return const [];
  if (altAz) {
    final places = _spread(
      count: stars,
      lstH: lstH,
      dec0: _clampDec(latDeg, 40),
      latDeg: latDeg,
      minAlt: minAlt,
      maxAlt: maxAlt,
    );
    if (places == null) return const [];
    return [PhoneAlignPlan(places: places, flipAfterThird: false)];
  }
  final dec0 = _clampDec(latDeg, 50);
  if (stars == 6) {
    final plans = <PhoneAlignPlan>[];
    for (final sign in [1.0, -1.0]) {
      final first = _onePier(
        count: 3,
        sign: sign,
        lstH: lstH,
        dec0: dec0,
        latDeg: latDeg,
        minAlt: minAlt,
        maxAlt: maxAlt,
      );
      final second = _onePier(
        count: 3,
        sign: -sign,
        lstH: lstH,
        dec0: dec0,
        latDeg: latDeg,
        minAlt: minAlt,
        maxAlt: maxAlt,
      );
      if (first == null || second == null) continue;
      plans.add(PhoneAlignPlan(
        places: [...first, ...second],
        flipAfterThird: true,
      ));
    }
    return plans;
  }
  final plans = <PhoneAlignPlan>[];
  for (final sign in [1.0, -1.0]) {
    final places = _onePier(
      count: stars,
      sign: sign,
      lstH: lstH,
      dec0: dec0,
      latDeg: latDeg,
      minAlt: minAlt,
      maxAlt: maxAlt,
    );
    if (places == null) continue;
    plans.add(PhoneAlignPlan(places: places, flipAfterThird: false));
  }
  return plans;
}

/// Stars the automatic exposure aims for in one frame.
const int targetStarCount = 30;

/// First shutter of a solve, in microseconds. One second.
const int startShutterUs = 1000000;

/// Opening shutter: one second, clipped to the camera's range.
int openingShutter({required int minUs, required int maxUs}) {
  if (maxUs < minUs) return maxUs;
  return startShutterUs.clamp(minUs, maxUs);
}

/// Shutter time to trust after a capture.
///
/// [deliveredUs] is what the camera reported. A much shorter delivery means
/// the camera refused the request, so later steps must not ask for longer.
({int us, int maxUs}) deliveredShutter({
  required int requestedUs,
  required int deliveredUs,
  required int minUs,
  required int maxUs,
}) {
  final asked = requestedUs.clamp(minUs, maxUs);
  if (deliveredUs <= 0) return (us: asked, maxUs: maxUs);
  final got = deliveredUs.clamp(minUs, maxUs);
  if (got < (asked * 4) ~/ 5) return (us: got, maxUs: got);
  return (us: got, maxUs: maxUs);
}

/// Next shutter and ISO that move [starCount] toward [target].
///
/// The shutter is lengthened or shortened first. ISO changes only when the
/// shutter is already at the end of the camera's range. Times are microseconds.
/// Returns null when the count is close enough, or nothing further can change.
({int us, int iso})? nextShutter({
  required int starCount,
  required int us,
  required int iso,
  required int minUs,
  required int maxUs,
  required int minIso,
  required int maxIso,
  int target = targetStarCount,
  int tolerance = 6,
}) {
  if (maxUs < minUs || maxIso < minIso) return null;
  if ((starCount - target).abs() <= tolerance) return null;
  int clampUs(int v) => v.clamp(minUs, maxUs);
  int clampIso(int v) => v.clamp(minIso, maxIso);
  if (starCount < target) {
    if (us < maxUs) {
      final factor = starCount < 8 ? 4 : 2;
      final grown = clampUs(math.max(us + 1, us * factor));
      if (grown != us) return (us: grown, iso: iso);
    }
    if (iso < maxIso) {
      final grown = clampIso(math.max(iso + 1, iso * 2));
      if (grown != iso) return (us: us, iso: grown);
    }
    return null;
  }
  if (us > minUs) {
    final shorter = clampUs(math.max(minUs, us ~/ 2));
    if (shorter != us) return (us: shorter, iso: iso);
  }
  if (iso > minIso) {
    final lower = clampIso(math.max(minIso, iso ~/ 2));
    if (lower != iso) return (us: us, iso: lower);
  }
  return null;
}

/// Next exposure compensation, in EV, that moves [starCount] toward [target].
///
/// One EV doubles the light. A large miss takes a full stop; a small miss
/// takes one camera step. Returns null when the count is already close, or
/// when the offset is already at the end of the camera range.
double? nextExposureOffset({
  required int starCount,
  required double offsetEv,
  required double minEv,
  required double maxEv,
  double stepEv = 0.5,
  int target = targetStarCount,
  int tolerance = 6,
}) {
  if ((starCount - target).abs() <= tolerance) return null;
  if (maxEv <= minEv) return null;
  final base = stepEv <= 0 ? 0.5 : stepEv;
  final gap = (target - starCount).abs();
  final nudge = gap > 20
      ? math.max(base, 1.0)
      : gap > 8
          ? math.max(base, 0.5)
          : base;
  final next = (starCount < target ? offsetEv + nudge : offsetEv - nudge)
      .clamp(minEv, maxEv);
  if ((next - offsetEv).abs() < 1e-6) return null;
  return next;
}

/// Brightness-weighted middle of the star nearest [x], [y].
///
/// The search stays inside [radius] pixels of the click. Null when nothing
/// there is brighter than the surrounding sky.
({double x, double y})? starMiddle(
  img.Image src,
  double x,
  double y, {
  int radius = 28,
}) {
  final w = src.width;
  final h = src.height;
  final x0 = x.round();
  final y0 = y.round();
  if (x0 < 0 || y0 < 0 || x0 >= w || y0 >= h || radius < 2) return null;
  int lumAt(int px, int py) {
    final p = src.getPixel(px, py);
    return (0.299 * p.r + 0.587 * p.g + 0.114 * p.b).round();
  }

  final samples = <int>[];
  var peak = 0;
  var px = x0;
  var py = y0;
  final reach = radius * radius;
  for (var dy = -radius; dy <= radius; dy++) {
    final yy = y0 + dy;
    if (yy < 0 || yy >= h) continue;
    for (var dx = -radius; dx <= radius; dx++) {
      if (dx * dx + dy * dy > reach) continue;
      final xx = x0 + dx;
      if (xx < 0 || xx >= w) continue;
      final v = lumAt(xx, yy);
      samples.add(v);
      if (v > peak) {
        peak = v;
        px = xx;
        py = yy;
      }
    }
  }
  if (samples.length < 9) return null;
  samples.sort();
  final background = samples[(samples.length * 0.4).floor()];
  if (peak - background < 15) return null;

  const aperture = 10;
  var weight = 0.0;
  var sx = 0.0;
  var sy = 0.0;
  for (var dy = -aperture; dy <= aperture; dy++) {
    final yy = py + dy;
    if (yy < 0 || yy >= h) continue;
    for (var dx = -aperture; dx <= aperture; dx++) {
      if (dx * dx + dy * dy > aperture * aperture) continue;
      final xx = px + dx;
      if (xx < 0 || xx >= w) continue;
      final above = lumAt(xx, yy) - background;
      if (above <= 0) continue;
      weight += above;
      sx += above * xx;
      sy += above * yy;
    }
  }
  if (weight <= 0) return null;
  return (x: sx / weight, y: sy / weight);
}

/// Bright peaks in a camera frame, brightest first.
List<PixelStar> detectStars(img.Image src, {int maxStars = targetStarCount}) {
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

StarFrame? readStarFrame(Uint8List bytes, {int maxStars = targetStarCount}) {
  final decoded = img.decodeImage(bytes);
  if (decoded == null) return null;
  return StarFrame(
    detectStars(decoded, maxStars: maxStars),
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
