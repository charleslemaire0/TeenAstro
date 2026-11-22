// Tetra on a 10% crop slide, every crop extracted and solved from scratch.
//
//   dart run tool/bench_sweep.dart [--stars=N] [--pattern=N] photo.jpg long-side-deg [out.json] [reference.json]
//
// Cedar runs through tool/cedar/cedar_native.dll and Tetra through
// tool/tetra/tetra.dll, both in this process. No frame reuses another.
// A reference is an earlier out.json; its centres are compared frame by frame.

import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

import 'package:teenastro_app/services/tetra_solver.dart';

import 'cedar_native.dart';

const _sigma = 10.5;
const _binning = 2;
const _maxStars = 50;
const _pattern = 8;
const _timeoutMs = 800.0;
const _database = 'assets/tetra/phone55.tetra';

void main(List<String> all) {
  final starsArg = all.where((a) => a.startsWith('--stars=')).firstOrNull;
  final maxStars = starsArg == null ? _maxStars : int.parse(starsArg.substring(8));
  final patternArg = all.where((a) => a.startsWith('--pattern=')).firstOrNull;
  final pattern = patternArg == null ? _pattern : int.parse(patternArg.substring(10));
  final args = all.where((a) => !a.startsWith('--')).toList();
  if (args.length < 2) {
    stderr.writeln('Pass the photo and the long-side field in degrees.');
    exitCode = 2;
    return;
  }
  final photoPath = args[0];
  final fovLong = double.parse(args[1]);
  final outPath = args.length > 2 ? args[2] : null;
  final referencePath = args.length > 3 ? args[3] : null;

  final loadWatch = Stopwatch()..start();
  final tetra = TetraSolver.openFile(_database);
  if (tetra == null) {
    stderr.writeln('The Tetra catalog $_database could not be read.');
    exitCode = 1;
    return;
  }
  final loadMs = loadWatch.elapsedMicroseconds / 1000;

  final openWatch = Stopwatch()..start();
  final photo = CedarPhoto.open(photoPath);
  if (photo == null) {
    stderr.writeln('The photo could not be read.');
    exitCode = 1;
    return;
  }
  final openMs = openWatch.elapsedMicroseconds / 1000;
  final shift = (photo.width * 0.10).round();
  final cropW = photo.width - shift;
  final cropH = photo.height;
  final fullLong = math.max(photo.width, photo.height).toDouble();
  final focal = (fullLong / 2) / math.tan(fovLong * math.pi / 360);
  final cropLong = math.max(cropW, cropH).toDouble();
  final cropField = 2 * math.atan((cropLong / 2) / focal) * 180 / math.pi;
  final fovHorizontal = cropField * cropW / cropLong;
  stdout.writeln(
    'Catalog loaded in ${loadMs.toStringAsFixed(0)} ms. '
    'Photo ${photo.width}×${photo.height} decoded in ${openMs.toStringAsFixed(0)} ms. '
    'Crop $cropW×$cropH, ${shift + 1} positions, ${fovHorizontal.toStringAsFixed(2)}° wide.',
  );

  final extractMs = <double>[];
  final solveMs = <double>[];
  final totalMs = <double>[];
  final counts = <int>[];
  final centers = <int, List<double>>{};
  var solved = 0;
  var missed = 0;
  final wall = Stopwatch()..start();
  for (var dx = 0; dx <= shift; dx++) {
    final frame = Stopwatch()..start();
    final crop = photo.extract(
      x0: dx,
      y0: 0,
      cropWidth: cropW,
      cropHeight: cropH,
      sigma: _sigma,
      binning: _binning,
      maxStars: maxStars,
    );
    final extracted = frame.elapsedMicroseconds / 1000;
    extractMs.add(extracted);
    if (crop == null || crop.stars.length < 4) {
      missed++;
      continue;
    }
    counts.add(crop.stars.length);
    final result = tetra.solve(
      stars: [for (final star in crop.stars) (x: star.x, y: star.y)],
      width: cropW.toDouble(),
      height: cropH.toDouble(),
      fovHorizontalDeg: fovHorizontal,
      patternStars: pattern,
      timeoutMs: _timeoutMs,
    );
    final total = frame.elapsedMicroseconds / 1000;
    solveMs.add(total - extracted);
    if (!result.solved) {
      missed++;
      continue;
    }
    solved++;
    totalMs.add(total);
    centers[dx] = [result.raDeg, result.decDeg, result.rollDeg, result.fovDeg];
  }
  final wallS = wall.elapsedMicroseconds / 1e6;
  photo.close();
  tetra.close();

  final summary = <String, Object?>{
    'rule': 'each crop extracted and solved with no previous frame',
    'solver': 'C++ Tetra',
    'positions': shift + 1,
    'crop': '$cropW×$cropH',
    'cedar': {'sigma': _sigma, 'binning': _binning, 'max_stars': maxStars},
    'tetra': {'pattern': pattern, 'timeout_ms': _timeoutMs},
    'catalog_ms': loadMs,
    'decode_ms': openMs,
    'extract_ms': _statsMap(extractMs),
    'stars_per_crop': _statsMap(counts.map((c) => c.toDouble()).toList()),
    'tetra_solved': solved,
    'tetra_missed': missed,
    'tetra_solve_ms': _statsMap(solveMs),
    'extract_plus_solve_ms': _statsMap(totalMs),
    'total_wall_s': wallS,
    'centers': {for (final e in centers.entries) '${e.key}': e.value},
  };

  stdout.writeln(
    'Tetra $solved solved, $missed missed in ${wallS.toStringAsFixed(2)} s. '
    'Cedar ${_stats(extractMs)}. Tetra ${_stats(solveMs)}. '
    'Extract + solve ${_stats(totalMs)}.',
  );

  if (referencePath != null) {
    final reference = jsonDecode(File(referencePath).readAsStringSync()) as Map<String, dynamic>;
    final theirs = (reference['centers'] as Map<String, dynamic>?) ?? const {};
    final offsets = <double>[];
    var onlyHere = 0;
    var onlyThere = 0;
    for (var dx = 0; dx <= shift; dx++) {
      final mine = centers[dx];
      final other = theirs['$dx'] as List?;
      if (mine == null && other == null) continue;
      if (other == null) {
        onlyHere++;
        continue;
      }
      if (mine == null) {
        onlyThere++;
        continue;
      }
      offsets.add(_separationDeg(
        mine[0],
        mine[1],
        (other[0] as num).toDouble(),
        (other[1] as num).toDouble(),
      ) * 3600);
    }
    summary['reference'] = {
      'path': referencePath,
      'both_solved': offsets.length,
      'only_cpp': onlyHere,
      'only_reference': onlyThere,
      'offset_arcsec': _statsMap(offsets),
    };
    final s = _statsMap(offsets);
    stdout.writeln(
      'Against the reference: ${offsets.length} frames solved by both, '
      '$onlyHere only here, $onlyThere only there. Centre offset '
      'median ${s?['median']}″, max ${s?['max']}″.',
    );
  }
  if (outPath != null) {
    File(outPath).writeAsStringSync(const JsonEncoder.withIndent('  ').convert(summary));
  }
}

double _separationDeg(double ra1, double dec1, double ra2, double dec2) {
  const rad = math.pi / 180;
  final a = math.sin((dec2 - dec1) * rad / 2);
  final b = math.sin((ra2 - ra1) * rad / 2);
  final h = a * a + math.cos(dec1 * rad) * math.cos(dec2 * rad) * b * b;
  return 2 * math.asin(math.min(1, math.sqrt(h))) / rad;
}

Map<String, num>? _statsMap(List<double> values) {
  if (values.isEmpty) return null;
  final sorted = [...values]..sort();
  final mean = sorted.reduce((a, b) => a + b) / sorted.length;
  return {
    'n': sorted.length,
    'median': _round(sorted[sorted.length ~/ 2]),
    'mean': _round(mean),
    'min': _round(sorted.first),
    'max': _round(sorted.last),
  };
}

double _round(double value) => (value * 10).round() / 10;

String _stats(List<double> values) {
  final s = _statsMap(values);
  if (s == null) return 'none';
  return 'median ${s['median']} ms, max ${s['max']} ms';
}
