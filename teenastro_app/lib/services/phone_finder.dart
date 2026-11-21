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
/// lens focal length. This is the plate scale [solveField] expects.
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
      final delta = math.atan2(north, east) -
          math.atan2(measured[i].$2, measured[i].$1);
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

/// Wide-field camera fitted to named stars.
///
/// The plate is the pinhole view of a real camera: north up puts east to the
/// left. [fovDeg] is the longer side after the focal length is fitted.
class WideCamera {
  final FinderSolution solution;
  final double fovDeg;
  final double focalPx;
  final double distortion;
  final double _width;
  final double _height;
  final List<({double x, double y, double z})> _toSky;

  const WideCamera({
    required this.solution,
    required this.fovDeg,
    required this.focalPx,
    required this.distortion,
    required double width,
    required double height,
    required List<({double x, double y, double z})> toSky,
  })  : _width = width,
        _height = height,
        _toSky = toSky;

  /// Sky direction of a pixel. [x] grows right, [y] grows down.
  ({double x, double y, double z}) direction(double x, double y) {
    final ray = _wideRay(x, y, _width, _height, focalPx, distortion);
    return _wideApply(_toSky, ray);
  }

  /// Pixel of a sky direction. Null when the star is behind the camera.
  ({double x, double y})? pixel(({double x, double y, double z}) sky) {
    final cam = (
      x: _toSky[0].x * sky.x + _toSky[1].x * sky.y + _toSky[2].x * sky.z,
      y: _toSky[0].y * sky.x + _toSky[1].y * sky.y + _toSky[2].y * sky.z,
      z: _toSky[0].z * sky.x + _toSky[1].z * sky.y + _toSky[2].z * sky.z,
    );
    if (cam.z <= 0.05) return null;
    final tx = cam.x / cam.z;
    final ty = cam.y / cam.z;
    var u = tx;
    var v = ty;
    for (var i = 0; i < 8; i++) {
      final denom = 1 + distortion * (u * u + v * v);
      if (denom.abs() < 1e-4) return null;
      u = tx / denom;
      v = ty / denom;
    }
    return (x: _width / 2 - u * focalPx, y: _height / 2 - v * focalPx);
  }
}

/// Clicks the catalog did not accept, and catalog stars in the frame
/// that no click landed on.
class WideFieldReport {
  final FinderSolution? solution;
  final String? reason;
  final List<({String label, double x, double y})> notInCatalog;
  final List<({String name, double x, double y})> missing;
  final List<({String name, double x, double y})> matched;

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
/// The focal length starts from [fovDeg]. [distortion] is a stored radial
/// term. With neither [distortion] nor [fitDistortion], the lens is treated
/// as having no radial distortion.
WideCamera? widePoseFromNamedStars({
  required List<({double raH, double decD, double x, double y})> stars,
  required double width,
  required double height,
  required double fovDeg,
  double? distortion,
  bool fitDistortion = false,
}) {
  if (stars.length < 3 || width < 2 || height < 2 || fovDeg <= 1) return null;
  final long = math.max(width, height);
  final f0 = long / 2 / math.tan(fovDeg * math.pi / 360);
  var bestF = f0;
  var bestK = distortion ?? 0.0;
  var bestErr = 1e9;
  final step = math.max(10.0, f0 / 80);
  double pairError(double f, double k) {
    var err = 0.0;
    var pairs = 0;
    for (var i = 0; i < stars.length; i++) {
      for (var j = i + 1; j < stars.length; j++) {
        final plate = _wideAngle(
          _wideRay(stars[i].x, stars[i].y, width, height, f, k),
          _wideRay(stars[j].x, stars[j].y, width, height, f, k),
        );
        final sky = sphereSeparationDeg(
          stars[i].raH,
          stars[i].decD,
          stars[j].raH,
          stars[j].decD,
        );
        err += (plate - sky) * (plate - sky);
        pairs++;
      }
    }
    return math.sqrt(err / pairs);
  }

  void consider(double f, double k) {
    if (f < 50) return;
    final rms = pairError(f, k);
    if (rms < bestErr) {
      bestErr = rms;
      bestF = f;
      bestK = k;
    }
  }

  if (distortion != null) {
    for (var f = f0 * 0.75; f <= f0 * 1.25; f += step) {
      consider(f, distortion);
    }
  } else if (fitDistortion) {
    for (var f = f0 * 0.75; f <= f0 * 1.25; f += step) {
      for (var k100 = -20; k100 <= 10; k100 += 2) {
        consider(f, k100 / 100.0);
      }
    }
    final coarseF = bestF;
    final coarseK = bestK;
    final fine = math.max(4.0, step / 5);
    for (var f = coarseF - step; f <= coarseF + step; f += fine) {
      for (var k1000 = ((coarseK - 0.03) * 1000).round();
          k1000 <= ((coarseK + 0.03) * 1000).round();
          k1000 += 5) {
        consider(f, k1000 / 1000.0);
      }
    }
  } else {
    for (var f = f0 * 0.75; f <= f0 * 1.25; f += step) {
      consider(f, 0);
    }
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
  final camA = _wideRay(stars[baseA].x, stars[baseA].y, width, height, bestF, bestK);
  final camB = _wideRay(stars[baseB].x, stars[baseB].y, width, height, bestF, bestK);
  final skyA = _wideSky(stars[baseA].raH, stars[baseA].decD);
  final skyB = _wideSky(stars[baseB].raH, stars[baseB].decD);
  final rays = [
    for (final s in stars)
      _wideRay(s.x, s.y, width, height, bestF, bestK),
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
  final fitted =
      2 * math.atan(long / (2 * bestF)) * 180 / math.pi;
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
    distortion: bestK,
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
  required List<({String name, bool named, double raH, double decD, double x, double y})>
      marks,
  required List<FinderStar> catalog,
  required List<String> catalogNames,
  List<double>? catalogMags,
  double missingMag = 4.5,
  double? distortion,
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
    distortion: distortion,
  );
  if (camera == null) {
    const reason = 'The named stars do not form one camera pose';
    note(reason);
    return const WideFieldReport(reason: reason);
  }
  final pose = camera.solution;
  if (distortion != null) {
    final plain = widePoseFromNamedStars(
      stars: named,
      width: width,
      height: height,
      fovDeg: fovDeg,
      distortion: 0,
    );
    if (plain != null) {
      note(
        'Named-star residual ${(pose.rmsDeg * 60).toStringAsFixed(1)}\' '
        'with the stored lens, '
        '${(plain.solution.rmsDeg * 60).toStringAsFixed(1)}\' without it.',
      );
    }
  }
  note(
    'Catalog to magnitude ${wideCatalogMag.toStringAsFixed(1)}. '
    'Match radius ${_wideMatchDeg.toStringAsFixed(1)}°. '
    'Focal length ${camera.focalPx.toStringAsFixed(0)} px, '
    'distortion ${camera.distortion.toStringAsFixed(3)}'
    '${distortion == null ? ', no lens calibration' : ', stored lens'}, '
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
  ({Map<int, ({int cat, double d})> taken, double sumSq}) assign(WideCamera cam) {
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
      distortion: camera.distortion,
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
  final taken = assigned.taken;
  final sumSq = assigned.sumSq;
  final usedMark = List<bool>.filled(marks.length, false);
  final usedCat = List<bool>.filled(catalog.length, false);
  for (final entry in taken.entries) {
    usedMark[entry.key] = true;
    usedCat[entry.value.cat] = true;
  }
  final poseNow = fitted.solution;
  for (var i = 0; i < marks.length; i++) {
    final label = marks[i].named ? marks[i].name : 'Unnamed ${marks[i].name}';
    final hit = taken[i];
    if (hit == null) {
      note('$label  no catalog star within ${_wideMatchDeg.toStringAsFixed(1)}°.');
      continue;
    }
    final who = catalogNames[hit.cat];
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
  for (var j = 0; j < catalog.length; j++) {
    if (usedCat[j]) continue;
    if (catalogMags != null && catalogMags[j] > missingMag) continue;
    final place = fitted.pixel(sky[j]);
    if (place == null) continue;
    if (place.x < 0 ||
        place.y < 0 ||
        place.x > width ||
        place.y > height) {
      continue;
    }
    missing.add((name: catalogNames[j], x: place.x, y: place.y));
  }
  final matched = <({String name, double x, double y})>[
    for (final entry in taken.entries)
      (
        name: catalogNames[entry.value.cat],
        x: marks[entry.key].x,
        y: marks[entry.key].y,
      ),
  ];
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

/// Faintest star the wide-field catalog uses when assigning a click.
const double wideCatalogMag = 5.5;

({double x, double y, double z}) _wideRay(
  double x,
  double y,
  double width,
  double height,
  double f,
  double k,
) {
  // East is to the left of a picture whose top is north.
  var dx = width / 2 - x;
  var dy = height / 2 - y;
  final r2 = (dx * dx + dy * dy) / (f * f);
  final scale = 1 + k * r2;
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

/// Hinted plate solve. [fovDeg] is the longer side of the frame.
/// The hint is only where to look; the result is the camera axis.
///
/// A candidate is kept only when the matched stars agree to within
/// [_maxRmsDeg], they span a real patch of the frame, and — when [latDeg]
/// and [lstH] are given — the axis is above the horizon.
FieldSolve solveField({
  required List<PixelStar> stars,
  required List<FinderStar> catalog,
  required double hintRa,
  required double hintDec,
  required double width,
  required double height,
  required double fovDeg,
  double? latDeg,
  double? lstH,
  List<String>? trace,
  List<({double raH, double decD, double x, double y})>? identified,
}) {
  void note(String line) => trace?.add(line);
  if (stars.length < 4 || catalog.isEmpty || width < 2 || height < 2) {
    note('Not enough stars in the frame (${stars.length}).');
    return const FieldSolve(reason: 'Not enough stars in the frame');
  }
  final haveSite = latDeg != null && lstH != null;
  final scale = fovDeg / math.max(width, height);
  final reach = fovDeg * 0.65;
  note(
      '${stars.length} marked stars on a ${width.toStringAsFixed(0)}×${height.toStringAsFixed(0)} frame.');
  note('Long side ${fovDeg.toStringAsFixed(1)}°, '
      '${scale.toStringAsFixed(4)}° per pixel. '
      'Search radius ${reach.toStringAsFixed(1)}°.');
  note('Hint ${_traceHours(hintRa)}, ${_traceDegrees(hintDec)}.');
  final nearby = <FinderStar>[];
  var below = 0;
  for (final s in catalog) {
    final (e, n) = skyOffset(s.raH, s.decD, hintRa, hintDec);
    if (e * e + n * n > reach * reach) continue;
    if (haveSite && altitudeDeg(latDeg, lstH, s.raH, s.decD) <= 0) {
      below++;
      continue;
    }
    nearby.add(s);
  }
  note('Catalog stars near the hint: ${nearby.length}. '
      'Below the horizon and left out: $below.');
  if (nearby.length < 4) {
    final reason = below > 0
        ? 'Not enough stars above the horizon'
        : 'Not enough catalog stars near the hint';
    note(reason);
    return FieldSolve(reason: reason);
  }

  final det = <(double right, double up)>[];
  for (final p in stars) {
    det.add(((p.x - width / 2) * scale, (height / 2 - p.y) * scale));
  }
  final need = det.length >= 15 ? 6 : 4;
  final minSpan = math.max(2.0, fovDeg * 0.12);
  note(
      'A solution needs $need matches spanning at least ${minSpan.toStringAsFixed(1)}°.');

  const bin = 0.3;
  FinderSolution? best;
  var sawHorizon = false;
  double? worstRms;
  var votes = 0;
  var measured = 0;
  var bestShort = 0;
  double? anchorDeg;
  var anchorRms = 1e9;
  var refining = false;

  String rollText(double deg) {
    var shown = deg % 360;
    if (shown < 0) shown += 360;
    return shown.toStringAsFixed(1);
  }

  void measure(double deg) {
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
    if (top == null || top.n < 4) {
      if (!refining && trace != null && deg % 90 == 0) {
        note('Roll ${rollText(deg)}°: strongest vote ${top?.n ?? 0}.');
      }
      return;
    }
    votes++;
    final tr = top.sr / top.n;
    final tu = top.su / top.n;
    final (east, north) = telescopeToSky(-tr, -tu, roll);
    var sky = offsetToSky(hintRa, hintDec, east, north);
    var fit = _matchField(
      det: det,
      catalog: nearby,
      centerRa: sky.$1,
      centerDec: sky.$2,
      roll: roll,
    );
    if (fit == null) return;
    final (corrE, corrN) = telescopeToSky(-fit.biasR, -fit.biasU, roll);
    final refined = offsetToSky(sky.$1, sky.$2, corrE, corrN);
    final fit2 = _matchField(
      det: det,
      catalog: nearby,
      centerRa: refined.$1,
      centerDec: refined.$2,
      roll: roll,
    );
    if (fit2 != null && fit2.count >= fit.count && fit2.rms <= fit.rms) {
      fit = fit2;
      sky = refined;
    }
    measured++;
    final short = fit.count < need || fit.span < minSpan;
    if (!short && fit.rms < anchorRms) {
      anchorDeg = deg;
      anchorRms = fit.rms;
    }
    if (refining) {
      if (!short) {
        note(
          'Roll ${rollText(deg)}°: ${fit.count} matches, '
          'residual ${(fit.rms * 60).toStringAsFixed(1)}\' '
          'at ${_traceHours(sky.$1)}, ${_traceDegrees(sky.$2)}.',
        );
      }
    } else if (trace != null && (short ? fit.count > bestShort : true)) {
      final where =
          'Roll ${rollText(deg)}°: ${fit.count} matches, span ${fit.span.toStringAsFixed(1)}°, '
          'residual ${(fit.rms * 60).toStringAsFixed(1)}\' '
          'at ${_traceHours(sky.$1)}, ${_traceDegrees(sky.$2)}.';
      if (short) {
        bestShort = fit.count;
        note('$where Short of the match or the span.');
      } else if (fit.rms > _maxRmsDeg) {
        final worst = worstRms;
        if (worst == null || fit.rms < worst) {
          note('$where Residual is above 24\'.');
        }
      } else if (haveSite && altitudeDeg(latDeg, lstH, sky.$1, sky.$2) <= 0) {
        note('$where Below the horizon.');
      } else {
        final kept = best;
        if (kept == null ||
            fit.count > kept.matches ||
            (fit.count == kept.matches && fit.rms < kept.rmsDeg)) {
          note('$where Kept.');
        }
      }
    }
    if (short) return;
    if (fit.rms > _maxRmsDeg) {
      final worst = worstRms;
      if (worst == null || fit.rms < worst) worstRms = fit.rms;
      return;
    }
    if (haveSite && altitudeDeg(latDeg, lstH, sky.$1, sky.$2) <= 0) {
      sawHorizon = true;
      return;
    }
    final kept = best;
    if (kept != null &&
        (fit.count < kept.matches ||
            (fit.count == kept.matches && fit.rms >= kept.rmsDeg))) {
      return;
    }
    best = FinderSolution(
      raH: sky.$1,
      decD: sky.$2,
      rollRad: roll,
      matches: fit.count,
      rmsDeg: fit.rms,
    );
  }

  for (var deg = 0; deg < 360; deg++) {
    measure(deg.toDouble());
  }
  note('360 rolls at 1°. $votes had a vote of 4 or more. $measured were measured.');
  final solved = best;
  final around = solved == null
      ? anchorDeg
      : solved.rollRad * 180 / math.pi;
  if (around != null) {
    note(
      'Roll refined in steps of 0.1° around ${rollText(around)}°. '
      'A step of 1° is coarser than the 24\' residual.',
    );
    refining = true;
    for (var step = -10; step <= 10; step++) {
      measure(around + step * 0.1);
    }
  }
  final chosen = best;
  if (chosen != null) {
    note('Camera axis ${_traceHours(chosen.raH)}, ${_traceDegrees(chosen.decD)}. '
        '${chosen.matches} stars, residual ${(chosen.rmsDeg * 60).toStringAsFixed(1)}\'.');
    if (identified != null) {
      _collectMatches(
        det: det,
        stars: stars,
        catalog: nearby,
        centerRa: chosen.raH,
        centerDec: chosen.decD,
        roll: chosen.rollRad,
        into: identified,
      );
    }
    return FieldSolve(solution: chosen);
  }
  if (sawHorizon) {
    note('The solution is below the horizon.');
    return const FieldSolve(reason: 'The solution is below the horizon');
  }
  final high = worstRms;
  if (high != null) {
    final arcmin = (high * 60).toStringAsFixed(0);
    final reason = "Residuals too large ($arcmin')";
    note(reason);
    return FieldSolve(reason: reason);
  }
  note('The field did not match the catalog.');
  return const FieldSolve(reason: 'The field did not match the catalog');
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

const double _matchDeg = 0.75;
const double _maxRmsDeg = 0.40;

class _StarFit {
  final int count;
  final double rms;
  final double span;
  final double biasR;
  final double biasU;
  const _StarFit(this.count, this.rms, this.span, this.biasR, this.biasU);
}

/// One-to-one matches between detections and catalog stars at [centerRa]/[centerDec].
_StarFit? _matchField({
  required List<(double right, double up)> det,
  required List<FinderStar> catalog,
  required double centerRa,
  required double centerDec,
  required double roll,
}) {
  final proj = <(double right, double up)>[];
  for (final s in catalog) {
    final (e, n) = skyOffset(s.raH, s.decD, centerRa, centerDec);
    proj.add(skyToTelescope(e, n, roll));
  }
  final pairs = <({double d, int i, int j})>[];
  for (var i = 0; i < det.length; i++) {
    for (var j = 0; j < proj.length; j++) {
      final dr = det[i].$1 - proj[j].$1;
      final du = det[i].$2 - proj[j].$2;
      final dist = math.sqrt(dr * dr + du * du);
      if (dist <= _matchDeg) pairs.add((d: dist, i: i, j: j));
    }
  }
  pairs.sort((a, b) => a.d.compareTo(b.d));
  final usedI = List<bool>.filled(det.length, false);
  final usedJ = List<bool>.filled(proj.length, false);
  var sumSq = 0.0;
  var biasR = 0.0;
  var biasU = 0.0;
  final matched = <(double right, double up)>[];
  for (final p in pairs) {
    if (usedI[p.i] || usedJ[p.j]) continue;
    usedI[p.i] = true;
    usedJ[p.j] = true;
    sumSq += p.d * p.d;
    biasR += det[p.i].$1 - proj[p.j].$1;
    biasU += det[p.i].$2 - proj[p.j].$2;
    matched.add(det[p.i]);
  }
  final n = matched.length;
  if (n < 4) return null;
  var span = 0.0;
  for (var a = 0; a < n; a++) {
    for (var b = a + 1; b < n; b++) {
      final dr = matched[a].$1 - matched[b].$1;
      final du = matched[a].$2 - matched[b].$2;
      final dist = math.sqrt(dr * dr + du * du);
      if (dist > span) span = dist;
    }
  }
  return _StarFit(n, math.sqrt(sumSq / n), span, biasR / n, biasU / n);
}

/// Catalog stars that landed on a detection at the reported solution.
/// This does not decide whether the solution is kept.
void _collectMatches({
  required List<(double right, double up)> det,
  required List<PixelStar> stars,
  required List<FinderStar> catalog,
  required double centerRa,
  required double centerDec,
  required double roll,
  required List<({double raH, double decD, double x, double y})> into,
}) {
  final proj = <(double right, double up)>[];
  for (final s in catalog) {
    final (e, n) = skyOffset(s.raH, s.decD, centerRa, centerDec);
    proj.add(skyToTelescope(e, n, roll));
  }
  final pairs = <({double d, int i, int j})>[];
  for (var i = 0; i < det.length; i++) {
    for (var j = 0; j < proj.length; j++) {
      final dr = det[i].$1 - proj[j].$1;
      final du = det[i].$2 - proj[j].$2;
      final dist = math.sqrt(dr * dr + du * du);
      if (dist <= _matchDeg) pairs.add((d: dist, i: i, j: j));
    }
  }
  pairs.sort((a, b) => a.d.compareTo(b.d));
  final usedI = List<bool>.filled(det.length, false);
  final usedJ = List<bool>.filled(catalog.length, false);
  for (final p in pairs) {
    if (usedI[p.i] || usedJ[p.j]) continue;
    usedI[p.i] = true;
    usedJ[p.j] = true;
    into.add((
      raH: catalog[p.j].raH,
      decD: catalog[p.j].decD,
      x: stars[p.i].x,
      y: stars[p.i].y,
    ));
  }
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
