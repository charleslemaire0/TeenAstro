import 'dart:math' as math;
import 'dart:typed_data';

import 'phone_finder.dart';

/// One Astrometry.net index, the same files StellarSolver searches.
class StellarIndex {
  final String name;
  final double scaleLow;
  final double scaleHigh;
  final Uint32List quads;
  final Float64List codes;
  final Float64List stars;
  final Map<int, List<int>> buckets;

  const StellarIndex({
    required this.name,
    required this.scaleLow,
    required this.scaleHigh,
    required this.quads,
    required this.codes,
    required this.stars,
    required this.buckets,
  });

  int get starCount => stars.length ~/ 3;
}

/// Read the 4100-series index files StellarSolver ships with.
List<StellarIndex> parseStellarIndexes(Map<String, Uint8List> files) {
  final loaded = <StellarIndex>[];
  for (final name in files.keys.toList()..sort()) {
    final index = _parseIndex(name, files[name]!);
    if (index != null) loaded.add(index);
  }
  return loaded;
}

/// Plate solve. [longSideDeg] is the longer side of the frame.
/// [hintRaH] and [hintDecD] are an optional J2000 position, in hours and
/// degrees. When both are set, the search starts around that position.
/// The result is J2000. Star x grows right and y grows down.
FieldSolve solveStellarField({
  required List<PixelStar> stars,
  required List<StellarIndex> indexes,
  required double width,
  required double height,
  required double longSideDeg,
  double? hintRaH,
  double? hintDecD,
  double tolerance = 0.20,
}) {
  if (stars.length < 4) {
    return const FieldSolve(reason: 'Not enough stars in the frame');
  }
  if (indexes.isEmpty || width < 2 || height < 2 || longSideDeg <= 0) {
    return const FieldSolve(reason: 'StellarSolver has no index for this photo');
  }
  final widthDeg = longSideDeg * width / math.max(width, height);
  final low = widthDeg * (1 - tolerance);
  final high = widthDeg * (1 + tolerance);
  final deadline = DateTime.now().add(const Duration(seconds: 8));
  final hint = hintRaH == null || hintDecD == null
      ? null
      : _near(hintRaH, hintDecD, longSideDeg, indexes);
  final hinted = hint == null
      ? null
      : _searchScales(stars, width, height, low, high, indexes, deadline, hint);
  if (hinted != null && hinted.matches >= 6) return _solution(hinted);
  final blind =
      _searchScales(stars, width, height, low, high, indexes, deadline, null);
  if (blind == null || blind.matches < 6) {
    return const FieldSolve(reason: 'StellarSolver found no match in this photo');
  }
  return _solution(blind);
}

_Pose? _searchScales(
  List<PixelStar> stars,
  double width,
  double height,
  double low,
  double high,
  List<StellarIndex> indexes,
  DateTime deadline,
  _Near? near,
) {
  _Pose? best;
  for (final yDown in [true, false]) {
    for (final scale in [(low + high) / 2, low, high]) {
      if (DateTime.now().isAfter(deadline)) break;
      final pose = _search(
        stars,
        width,
        height,
        scale,
        yDown,
        indexes,
        deadline,
        near,
      );
      if (pose == null) continue;
      if (best == null ||
          pose.matches > best.matches ||
          (pose.matches == best.matches && pose.rms < best.rms)) {
        best = pose;
      }
      if (pose.matches >= 12 && pose.rms < 0.15) return best;
    }
  }
  return best;
}

FieldSolve _solution(_Pose pose) {
  final center = _mul(_poseRotation(pose), const [0, 0, 1]);
  var ra = math.atan2(center[1], center[0]);
  if (ra < 0) ra += 2 * math.pi;
  final dec = math.asin(center[2].clamp(-1.0, 1.0));
  final right = _mul(_poseRotation(pose), const [1, 0, 0]);
  final north = [
    -math.sin(dec) * math.cos(ra),
    -math.sin(dec) * math.sin(ra),
    math.cos(dec),
  ];
  final east = [-math.sin(ra), math.cos(ra), 0.0];
  final roll = math.atan2(_dot(right, north), _dot(right, east));
  return FieldSolve(
    solution: FinderSolution(
      raH: ra * 12 / math.pi,
      decD: dec * 180 / math.pi,
      rollRad: roll,
      matches: pose.matches,
      rmsDeg: pose.rms,
    ),
  );
}

class _Pose {
  final List<double> rotation;
  final int matches;
  final double rms;
  const _Pose(this.rotation, this.matches, this.rms);
}

List<double> _poseRotation(_Pose pose) => pose.rotation;

_Pose? _search(
  List<PixelStar> stars,
  double width,
  double height,
  double widthDeg,
  bool yDown,
  List<StellarIndex> indexes,
  DateTime deadline,
  _Near? near,
) {
  final vectors = _camera(stars, width, height, widthDeg, yDown);
  final pool = math.min(vectors.length, 26);
  final hits = <_Hit>[];
  for (var i = 0; i < pool; i++) {
    for (var j = i + 1; j < pool; j++) {
      if (DateTime.now().isAfter(deadline)) break;
      final separation = math.acos(_dot(vectors[i], vectors[j]).clamp(-1.0, 1.0));
      final usable = [
        for (final index in indexes)
          if (index.scaleLow * 0.85 <= separation &&
              separation <= index.scaleHigh * 1.15)
            index,
      ];
      if (usable.isEmpty) continue;
      for (var k = 0; k < pool; k++) {
        if (k == i || k == j) continue;
        final code = _triangleCode([vectors[i], vectors[j], vectors[k]]);
        if (code == null) continue;
        for (final index in usable) {
          for (final hit in _lookup(index, code)) {
            if (near != null && !_quadNear(near, index, hit.$2)) continue;
            hits.add(_Hit(hit.$1, index, hit.$2, i, j, k));
          }
        }
      }
    }
  }
  hits.sort((a, b) => a.distance.compareTo(b.distance));
  _Pose? best;
  var checked = 0;
  for (final hit in hits) {
    if (DateTime.now().isAfter(deadline) || checked >= 40) break;
    if (hit.distance > 0.004) continue;
    checked++;
    final ids = hit.quad * 3;
    final camera = [vectors[hit.a], vectors[hit.b], vectors[hit.c]];
    final catalog = [
      _star(hit.index, hit.index.quads[ids]),
      _star(hit.index, hit.index.quads[ids + 1]),
      _star(hit.index, hit.index.quads[ids + 2]),
    ];
    var rotation = _rotation(camera, catalog);
    final only = near?.stars[hit.index];
    var matched = _match(vectors, rotation, hit.index, 0.4, only);
    if (matched.length >= 6) {
      rotation = _rotation(
        [for (final pair in matched) vectors[pair.$1]],
        [for (final pair in matched) _star(hit.index, pair.$2)],
      );
      matched = _match(vectors, rotation, hit.index, 0.25, only);
    }
    if (near != null) {
      final center = [rotation[2], rotation[5], rotation[8]];
      if (_dot(center, near.axis) < near.centerDot) continue;
    }
    if (matched.length < 6) continue;
    var error = 0.0;
    for (final pair in matched) {
      final sky = _mul(rotation, vectors[pair.$1]);
      final angle = math.acos(_dot(sky, _star(hit.index, pair.$2)).clamp(-1.0, 1.0));
      error += angle * angle;
    }
    final rms = math.sqrt(error / matched.length) * 180 / math.pi;
    if (best == null ||
        matched.length > best.matches ||
        (matched.length == best.matches && rms < best.rms)) {
      best = _Pose(rotation, matched.length, rms);
      if (matched.length >= 12 && rms < 0.15) break;
    }
  }
  return best;
}

class _Hit {
  final double distance;
  final StellarIndex index;
  final int quad;
  final int a;
  final int b;
  final int c;
  const _Hit(this.distance, this.index, this.quad, this.a, this.b, this.c);
}

List<List<double>> _camera(
  List<PixelStar> stars,
  double width,
  double height,
  double widthDeg,
  bool yDown,
) {
  final centerX = (width - 1) / 2;
  final centerY = (height - 1) / 2;
  final focal = (width / 2) / math.tan(widthDeg * math.pi / 360);
  return [
    for (final star in stars)
      _unit([
        (star.x - centerX) / focal,
        ((yDown ? star.y - centerY : centerY - star.y) / focal),
        1,
      ]),
  ];
}

List<(int, int)> _match(
  List<List<double>> vectors,
  List<double> rotation,
  StellarIndex index,
  double gateDeg,
  List<int>? only,
) {
  final gate = math.cos(gateDeg * math.pi / 180);
  final count = only?.length ?? index.starCount;
  final pairs = <(int, int)>[];
  for (var i = 0; i < vectors.length; i++) {
    final sky = _mul(rotation, vectors[i]);
    var best = -1;
    var bestDot = gate;
    for (var n = 0; n < count; n++) {
      final s = only == null ? n : only[n];
      final dot = sky[0] * index.stars[s * 3] +
          sky[1] * index.stars[s * 3 + 1] +
          sky[2] * index.stars[s * 3 + 2];
      if (dot > bestDot) {
        bestDot = dot;
        best = s;
      }
    }
    if (best >= 0) pairs.add((i, best));
  }
  return pairs;
}

/// Index stars around a J2000 position. The center must land inside the window.
class _Near {
  final List<double> axis;
  final double centerDot;
  final Map<StellarIndex, List<int>> stars;
  final Map<StellarIndex, Set<int>> ids;
  const _Near(this.axis, this.centerDot, this.stars, this.ids);
}

_Near _near(
  double raH,
  double decD,
  double longSideDeg,
  List<StellarIndex> indexes,
) {
  final ra = raH * math.pi / 12;
  final dec = decD * math.pi / 180;
  final axis = [
    math.cos(dec) * math.cos(ra),
    math.cos(dec) * math.sin(ra),
    math.sin(dec),
  ];
  final centerDeg = math.max(15.0, longSideDeg);
  final starDeg = math.min(180.0, centerDeg + longSideDeg * 0.75);
  final starDot = math.cos(starDeg * math.pi / 180);
  final stars = <StellarIndex, List<int>>{};
  final ids = <StellarIndex, Set<int>>{};
  for (final index in indexes) {
    final kept = <int>[];
    for (var s = 0; s < index.starCount; s++) {
      final dot = axis[0] * index.stars[s * 3] +
          axis[1] * index.stars[s * 3 + 1] +
          axis[2] * index.stars[s * 3 + 2];
      if (dot >= starDot) kept.add(s);
    }
    stars[index] = kept;
    ids[index] = kept.toSet();
  }
  return _Near(axis, math.cos(centerDeg * math.pi / 180), stars, ids);
}

bool _quadNear(_Near near, StellarIndex index, int quad) {
  final allowed = near.ids[index];
  if (allowed == null || allowed.length < 6) return false;
  final at = quad * 3;
  return allowed.contains(index.quads[at]) &&
      allowed.contains(index.quads[at + 1]) &&
      allowed.contains(index.quads[at + 2]);
}

List<double> _rotation(List<List<double>> camera, List<List<double>> catalog) {
  final matrix = List<double>.filled(9, 0);
  for (var n = 0; n < camera.length; n++) {
    for (var i = 0; i < 3; i++) {
      for (var j = 0; j < 3; j++) {
        matrix[i * 3 + j] += catalog[n][i] * camera[n][j];
      }
    }
  }
  final svd = _svd(matrix);
  var rotation = _mulMat(svd.u, svd.vh);
  if (_det(rotation) < 0) {
    for (var i = 0; i < 3; i++) {
      svd.u[i * 3 + 2] *= -1;
    }
    rotation = _mulMat(svd.u, svd.vh);
  }
  return rotation;
}

List<double> _star(StellarIndex index, int id) => [
      index.stars[id * 3],
      index.stars[id * 3 + 1],
      index.stars[id * 3 + 2],
    ];

List<(double, int)> _lookup(StellarIndex index, (double, double) code) {
  const step = 0.01;
  final ix = (code.$1 / step).floor();
  final iy = (code.$2 / step).floor();
  final hits = <(double, int)>[];
  for (var dx = -1; dx <= 1; dx++) {
    for (var dy = -1; dy <= 1; dy++) {
      final bucket = index.buckets[(ix + dx) * 100000 + (iy + dy)];
      if (bucket == null) continue;
      for (final quad in bucket) {
        final storedX = index.codes[quad * 2];
        final storedY = index.codes[quad * 2 + 1];
        final distance = math.max((storedX - code.$1).abs(), (storedY - code.$2).abs());
        if (distance <= 0.006) hits.add((distance, quad));
      }
    }
  }
  return hits;
}

(double, double)? _triangleCode(List<List<double>> xyz) {
  final mid = _unit([
    xyz[0][0] + xyz[1][0],
    xyz[0][1] + xyz[1][1],
    xyz[0][2] + xyz[1][2],
  ]);
  final projected = [_coords(xyz[0], mid), _coords(xyz[1], mid), _coords(xyz[2], mid)];
  if (projected.any((item) => item == null)) return null;
  final (ay, ax) = projected[0]!;
  final (by, bx) = projected[1]!;
  final (cy, cx) = projected[2]!;
  final abx = bx - ax;
  final aby = by - ay;
  final scale = abx * abx + aby * aby;
  if (scale <= 0) return null;
  final inv = 1 / scale;
  final cost = (aby + abx) * inv;
  final sint = (aby - abx) * inv;
  final adx = cx - ax;
  final ady = cy - ay;
  var x = adx * cost + ady * sint;
  var y = -adx * sint + ady * cost;
  if (x > 0.5) {
    x = 1 - x;
    y = 1 - y;
  }
  return (x, y);
}

(double, double)? _coords(List<double> star, List<double> center) {
  final denom = _dot(star, center);
  if (denom <= 0) return null;
  var ix = center[1];
  var iy = -center[0];
  final norm = math.sqrt(ix * ix + iy * iy);
  if (norm < 1e-12) return null;
  ix /= norm;
  iy /= norm;
  final jx = iy * center[2];
  final jy = -ix * center[2];
  final jz = ix * center[1] - iy * center[0];
  final sx = star[0] * ix + star[1] * iy;
  final sy = star[0] * jx + star[1] * jy + star[2] * jz;
  return (sy / denom, sx / denom);
}

StellarIndex? _parseIndex(String name, Uint8List bytes) {
  final hdus = _fits(bytes);
  if (hdus.isEmpty) return null;
  final low = double.tryParse(hdus.first.cards['SCALE_L'] ?? '');
  final high = double.tryParse(hdus.first.cards['SCALE_U'] ?? '');
  Uint8List? quads;
  Uint8List? codes;
  Uint8List? stars;
  for (final hdu in hdus) {
    switch (hdu.cards['TTYPE1']) {
      case 'quads':
        quads = hdu.data;
      case 'kdtree_data_codes':
        codes = hdu.data;
      case 'kdtree_data_stars':
        stars = hdu.data;
    }
  }
  if (low == null || high == null || quads == null || codes == null || stars == null) {
    return null;
  }
  final quadView = ByteData.sublistView(quads);
  final quadCount = quads.length ~/ 12;
  final quadIds = Uint32List(quadCount * 3);
  for (var i = 0; i < quadIds.length; i++) {
    quadIds[i] = quadView.getUint32(i * 4, Endian.little);
  }
  final codeView = ByteData.sublistView(codes);
  final codeCount = codes.length ~/ 4;
  const codeLow = -0.20710678118654752;
  const codeHigh = 1.2071067811865475;
  final decodedCodes = Float64List(codeCount * 2);
  for (var i = 0; i < decodedCodes.length; i++) {
    final raw = codeView.getUint16(i * 2, Endian.little);
    decodedCodes[i] = codeLow + raw * (codeHigh - codeLow) / 65535.0;
  }
  final starView = ByteData.sublistView(stars);
  final starCount = stars.length ~/ 12;
  final decodedStars = Float64List(starCount * 3);
  for (var i = 0; i < decodedStars.length; i++) {
    final raw = starView.getUint32(i * 4, Endian.little);
    decodedStars[i] = -1 + 2 * raw / 4294967296.0;
  }
  const step = 0.01;
  final buckets = <int, List<int>>{};
  for (var i = 0; i < codeCount; i++) {
    final key = (decodedCodes[i * 2] / step).floor() * 100000 +
        (decodedCodes[i * 2 + 1] / step).floor();
    (buckets[key] ??= []).add(i);
  }
  return StellarIndex(
    name: name,
    scaleLow: low,
    scaleHigh: high,
    quads: quadIds,
    codes: decodedCodes,
    stars: decodedStars,
    buckets: buckets,
  );
}

class _Hdu {
  final Map<String, String> cards;
  final Uint8List data;
  const _Hdu(this.cards, this.data);
}

List<_Hdu> _fits(Uint8List bytes) {
  final hdus = <_Hdu>[];
  var pos = 0;
  while (pos + 80 <= bytes.length) {
    final cards = <String, String>{};
    var ended = false;
    while (!ended) {
      if (pos + 2880 > bytes.length) return hdus;
      for (var card = 0; card < 36; card++) {
        final text = String.fromCharCodes(bytes, pos, pos + 80);
        pos += 80;
        if (text.substring(0, 8).trim() == 'END') {
          ended = true;
          final remainder = pos % 2880;
          if (remainder != 0) pos += 2880 - remainder;
          break;
        }
        if (text.length < 10 || text.substring(8, 10) != '= ') continue;
        final key = text.substring(0, 8).trim();
        var value = text.substring(10).trim();
        final slash = value.indexOf('/');
        if (!value.startsWith("'") && slash >= 0) value = value.substring(0, slash).trim();
        if (value.startsWith("'")) {
          final end = value.lastIndexOf("'");
          value = end > 0 ? value.substring(1, end).trim() : '';
        }
        cards[key] = value;
      }
    }
    final naxis = int.tryParse(cards['NAXIS'] ?? '') ?? 0;
    var dataBytes = 0;
    if (naxis > 0) {
      final bitpix = (int.tryParse(cards['BITPIX'] ?? '') ?? 8).abs();
      var count = 1;
      for (var axis = 1; axis <= naxis; axis++) {
        count *= int.tryParse(cards['NAXIS$axis'] ?? '') ?? 0;
      }
      dataBytes = count * bitpix ~/ 8;
    }
    final padded = ((dataBytes + 2879) ~/ 2880) * 2880;
    if (pos + dataBytes > bytes.length) return hdus;
    hdus.add(_Hdu(cards, Uint8List.sublistView(bytes, pos, pos + dataBytes)));
    pos += padded;
  }
  return hdus;
}

double _dot(List<double> a, List<double> b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

List<double> _unit(List<double> v) {
  final n = math.sqrt(_dot(v, v));
  return [v[0] / n, v[1] / n, v[2] / n];
}

List<double> _mul(List<double> m, List<double> v) => [
      m[0] * v[0] + m[1] * v[1] + m[2] * v[2],
      m[3] * v[0] + m[4] * v[1] + m[5] * v[2],
      m[6] * v[0] + m[7] * v[1] + m[8] * v[2],
    ];

List<double> _mulMat(List<double> a, List<double> b) {
  final out = List<double>.filled(9, 0);
  for (var i = 0; i < 3; i++) {
    for (var j = 0; j < 3; j++) {
      out[i * 3 + j] = a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
    }
  }
  return out;
}

double _det(List<double> m) =>
    m[0] * (m[4] * m[8] - m[5] * m[7]) -
    m[1] * (m[3] * m[8] - m[5] * m[6]) +
    m[2] * (m[3] * m[7] - m[4] * m[6]);

({List<double> u, List<double> vh}) _svd(List<double> a) {
  final columns = [
    [a[0], a[3], a[6]],
    [a[1], a[4], a[7]],
    [a[2], a[5], a[8]],
  ];
  final v = [
    [1.0, 0.0, 0.0],
    [0.0, 1.0, 0.0],
    [0.0, 0.0, 1.0],
  ];
  for (var sweep = 0; sweep < 8; sweep++) {
    for (final pair in const [(0, 1), (0, 2), (1, 2)]) {
      final p = pair.$1;
      final q = pair.$2;
      var pp = _dot(columns[p], columns[p]);
      var qq = _dot(columns[q], columns[q]);
      var pq = _dot(columns[p], columns[q]);
      if (pq.abs() < 1e-15 * math.sqrt(pp * qq + 1e-30)) continue;
      final tau = (qq - pp) / (2 * pq);
      final t = (tau >= 0 ? 1 : -1) / (tau.abs() + math.sqrt(1 + tau * tau));
      final c = 1 / math.sqrt(1 + t * t);
      final s = t * c;
      for (var i = 0; i < 3; i++) {
        final cp = columns[p][i];
        final cq = columns[q][i];
        columns[p][i] = c * cp - s * cq;
        columns[q][i] = s * cp + c * cq;
        final vp = v[p][i];
        final vq = v[q][i];
        v[p][i] = c * vp - s * vq;
        v[q][i] = s * vp + c * vq;
      }
    }
  }
  final u = List<double>.filled(9, 0);
  for (var col = 0; col < 3; col++) {
    final n = math.sqrt(_dot(columns[col], columns[col]));
    for (var row = 0; row < 3; row++) {
      u[row * 3 + col] = n < 1e-15 ? (row == col ? 1 : 0) : columns[col][row] / n;
    }
  }
  final vh = List<double>.filled(9, 0);
  for (var row = 0; row < 3; row++) {
    for (var col = 0; col < 3; col++) {
      vh[row * 3 + col] = v[row][col];
    }
  }
  return (u: u, vh: vh);
}
