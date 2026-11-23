import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'dart:math' as math;
import 'dart:typed_data';

import 'phone_finder.dart';

/// One star Tetra matched to the Bright Star Catalogue.
class TetraStar {
  const TetraStar({
    required this.index,
    required this.catId,
    required this.raDeg,
    required this.decDeg,
    required this.mag,
  });

  /// Position in the star list that was solved.
  final int index;
  final int catId;
  final double raDeg;
  final double decDeg;
  final double mag;
}

class TetraResult {
  const TetraResult._({
    required this.solved,
    required this.timedOut,
    required this.ms,
    this.raDeg = 0,
    this.decDeg = 0,
    this.rollDeg = 0,
    this.fovDeg = 0,
    this.distortion = 0,
    this.rmseArcsec = 0,
    this.prob = 0,
    this.matches = 0,
    this.epoch = 2000,
    this.rotation = const [],
    this.stars = const [],
  });

  final bool solved;
  final bool timedOut;

  /// Time inside the C++ solver, every estimate included.
  final double ms;

  /// J2000 centre of the frame.
  final double raDeg;
  final double decDeg;
  final double rollDeg;

  /// Horizontal field.
  final double fovDeg;
  final double distortion;
  final double rmseArcsec;
  final double prob;
  final int matches;
  final int epoch;

  /// camera = R · sky, row-major. Row 0 is the boresight, row 1 image left,
  /// row 2 image up.
  final List<double> rotation;
  final List<TetraStar> stars;

  /// The same fields the photo window has always read from Tetra.
  Map<String, dynamic> toPayload() => {
        'ok': solved,
        'ra_deg': raDeg,
        'dec_deg': decDeg,
        'roll_deg': rollDeg,
        'fov_deg': fovDeg,
        'distortion': distortion,
        'rmse_arcsec': rmseArcsec,
        'matches': matches,
        'prob': prob,
        'ms': ms,
        'epoch_equinox': epoch,
        'stars': [
          for (final star in stars)
            {
              'index': star.index,
              'ra_deg': star.raDeg,
              'dec_deg': star.decDeg,
              'mag': star.mag,
              'cat': star.catId,
            },
        ],
      };

  /// The phone's solution, with roll measured like StellarSolver's: the
  /// direction of image +x on the sky, from east toward north.
  FinderSolution toFinderSolution() {
    final ra = raDeg * math.pi / 180;
    final dec = decDeg * math.pi / 180;
    final right = [-rotation[3], -rotation[4], -rotation[5]];
    final north = [
      -math.sin(dec) * math.cos(ra),
      -math.sin(dec) * math.sin(ra),
      math.cos(dec),
    ];
    final east = [-math.sin(ra), math.cos(ra), 0.0];
    double dot(List<double> a, List<double> b) =>
        a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    return FinderSolution(
      raH: raDeg / 15,
      decD: decDeg,
      rollRad: math.atan2(dot(right, north), dot(right, east)),
      matches: matches,
      rmsDeg: rmseArcsec / 3600,
    );
  }
}

typedef _AllocC = Pointer<Uint8> Function(Uint64);
typedef _AllocDart = Pointer<Uint8> Function(int);
typedef _FreeC = Void Function(Pointer<Uint8>);
typedef _FreeDart = void Function(Pointer<Uint8>);
typedef _OpenMemoryC = Pointer<Void> Function(Pointer<Uint8>, Uint64);
typedef _OpenMemoryDart = Pointer<Void> Function(Pointer<Uint8>, int);
typedef _OpenFileC = Pointer<Void> Function(Pointer<Uint8>);
typedef _OpenFileDart = Pointer<Void> Function(Pointer<Uint8>);
typedef _CloseC = Void Function(Pointer<Void>);
typedef _CloseDart = void Function(Pointer<Void>);
typedef _MaxFovC = Double Function(Pointer<Void>);
typedef _MaxFovDart = double Function(Pointer<Void>);
typedef _SolveC = Int32 Function(
  Pointer<Void>,
  Pointer<Double>,
  Int32,
  Double,
  Double,
  Double,
  Double,
  Int32,
  Double,
  Double,
  Double,
  Double,
  Double,
  Pointer<Uint8>,
  Pointer<Uint8>,
  Int32,
);
typedef _SolveDart = int Function(
  Pointer<Void>,
  Pointer<Double>,
  int,
  double,
  double,
  double,
  double,
  int,
  double,
  double,
  double,
  double,
  double,
  Pointer<Uint8>,
  Pointer<Uint8>,
  int,
);

class _Tetra {
  _Tetra(DynamicLibrary lib)
      : alloc = lib.lookupFunction<_AllocC, _AllocDart>('tetra_alloc'),
        free = lib.lookupFunction<_FreeC, _FreeDart>('tetra_free'),
        openMemory =
            lib.lookupFunction<_OpenMemoryC, _OpenMemoryDart>('tetra_open_memory'),
        openFile = lib.lookupFunction<_OpenFileC, _OpenFileDart>('tetra_open_file'),
        close = lib.lookupFunction<_CloseC, _CloseDart>('tetra_close'),
        maxFov = lib.lookupFunction<_MaxFovC, _MaxFovDart>('tetra_max_fov'),
        solve = lib.lookupFunction<_SolveC, _SolveDart>('tetra_solve');

  final _AllocDart alloc;
  final _FreeDart free;
  final _OpenMemoryDart openMemory;
  final _OpenFileDart openFile;
  final _CloseDart close;
  final _MaxFovDart maxFov;
  final _SolveDart solve;

  static _Tetra? _loaded;

  static _Tetra load() => _loaded ??= _Tetra(_openLibrary());

  static DynamicLibrary _openLibrary() {
    if (Platform.isAndroid) return DynamicLibrary.open('libtetra.so');
    if (Platform.isWindows) {
      final local = File('tool/tetra/tetra.dll');
      if (local.existsSync()) return DynamicLibrary.open(local.absolute.path);
      return DynamicLibrary.open('tetra.dll');
    }
    if (Platform.isMacOS || Platform.isIOS) return DynamicLibrary.process();
    return DynamicLibrary.open('libtetra.so');
  }
}

// TetraSolution in tetra.h: 4 int32, 8 double, 9 double.
const _solutionBytes = 16 + 8 * 8 + 9 * 8;
// TetraMatch: 2 int32, 3 double.
const _matchBytes = 8 + 3 * 8;
const _matchCapacity = 64;

/// Tetra3 in C++, through dart:ffi. The catalog is loaded once.
class TetraSolver {
  TetraSolver._(this.address, this.maxFovDeg);

  /// The native catalog, so another isolate can [attach] to it.
  final int address;

  /// Horizontal field the catalog was built for.
  final double maxFovDeg;

  static TetraSolver? openBytes(Uint8List bytes) {
    final tetra = _Tetra.load();
    final buffer = tetra.alloc(bytes.length);
    if (buffer == nullptr) return null;
    try {
      buffer.asTypedList(bytes.length).setAll(0, bytes);
      final handle = tetra.openMemory(buffer, bytes.length);
      if (handle == nullptr) return null;
      return TetraSolver._(handle.address, tetra.maxFov(handle));
    } finally {
      tetra.free(buffer);
    }
  }

  static TetraSolver? openFile(String path) {
    final tetra = _Tetra.load();
    final text = utf8.encode(path);
    final buffer = tetra.alloc(text.length + 1);
    if (buffer == nullptr) return null;
    try {
      buffer.asTypedList(text.length + 1)
        ..setAll(0, text)
        ..[text.length] = 0;
      final handle = tetra.openFile(buffer);
      if (handle == nullptr) return null;
      return TetraSolver._(handle.address, tetra.maxFov(handle));
    } finally {
      tetra.free(buffer);
    }
  }

  static TetraSolver attach(int address, double maxFovDeg) =>
      TetraSolver._(address, maxFovDeg);

  void close() => _Tetra.load().close(Pointer<Void>.fromAddress(address));

  /// Lost-in-space solve of stars in pixels (nearest the centre preferred).
  ///
  /// [fovHorizontalDeg] is the expected horizontal field. When it is more
  /// than 5° from the catalog's field, the catalog field is tried as well.
  /// Phone lenses need a few percent of distortion, so a ±5% range is searched.
  TetraResult solve({
    required List<({double x, double y})> stars,
    required double width,
    required double height,
    required double fovHorizontalDeg,
    int patternStars = 8,
    double timeoutMs = 800,
  }) {
    final started = Stopwatch()..start();
    if (stars.length < 4 || width < 2 || height < 2 || fovHorizontalDeg <= 0) {
      return const TetraResult._(solved: false, timedOut: false, ms: 0);
    }
    final estimates = [fovHorizontalDeg];
    if ((fovHorizontalDeg - maxFovDeg).abs() > 5) estimates.add(maxFovDeg);
    final budget = math.min(800.0, math.max(250.0, timeoutMs));
    final slice = budget / estimates.length;
    final checking = math.min(stars.length, math.max(4, patternStars));
    for (final estimate in estimates) {
      final remaining = budget - started.elapsedMicroseconds / 1000;
      final allowed = math.min(remaining, slice);
      if (allowed < 40) break;
      final result = solveOnce(
        stars: stars,
        width: width,
        height: height,
        fovEstimateDeg: estimate,
        fovMaxErrorDeg: 20,
        patternCheckingStars: checking,
        timeoutMs: allowed,
        distortionMin: -0.05,
        distortionMax: 0.05,
      );
      if (result.solved) {
        return TetraResult._(
          solved: true,
          timedOut: false,
          ms: started.elapsedMicroseconds / 1000,
          raDeg: result.raDeg,
          decDeg: result.decDeg,
          rollDeg: result.rollDeg,
          fovDeg: result.fovDeg,
          distortion: result.distortion,
          rmseArcsec: result.rmseArcsec,
          prob: result.prob,
          matches: result.matches,
          epoch: result.epoch,
          rotation: result.rotation,
          stars: result.stars,
        );
      }
    }
    final ms = started.elapsedMicroseconds / 1000;
    return TetraResult._(solved: false, timedOut: ms >= budget - 30, ms: ms);
  }

  /// One call of Tetra3's solve_from_centroids.
  TetraResult solveOnce({
    required List<({double x, double y})> stars,
    required double width,
    required double height,
    required double fovEstimateDeg,
    double fovMaxErrorDeg = 20,
    int patternCheckingStars = 8,
    double matchRadius = 0.03,
    double matchThreshold = 1e-3,
    double timeoutMs = 800,
    double distortionMin = -0.05,
    double distortionMax = 0.05,
  }) {
    final tetra = _Tetra.load();
    final count = stars.length;
    final centroidBytes = count * 16;
    final size = centroidBytes + _solutionBytes + _matchCapacity * _matchBytes;
    final buffer = tetra.alloc(size);
    if (buffer == nullptr) {
      return const TetraResult._(solved: false, timedOut: false, ms: 0);
    }
    try {
      final centroids = buffer.cast<Double>().asTypedList(count * 2);
      for (var i = 0; i < count; i++) {
        centroids[i * 2] = stars[i].y;
        centroids[i * 2 + 1] = stars[i].x;
      }
      final solution = buffer + centroidBytes;
      final matches = solution + _solutionBytes;
      final code = tetra.solve(
        Pointer<Void>.fromAddress(address),
        buffer.cast<Double>(),
        count,
        height,
        width,
        fovEstimateDeg,
        fovMaxErrorDeg,
        patternCheckingStars,
        matchRadius,
        matchThreshold,
        timeoutMs,
        distortionMin,
        distortionMax,
        solution,
        matches,
        _matchCapacity,
      );
      final head = solution.cast<Int32>().asTypedList(4);
      final values = (solution + 16).cast<Double>().asTypedList(17);
      if (code != 1 || head[0] != 1) {
        return TetraResult._(solved: false, timedOut: head[1] == 1, ms: values[7]);
      }
      final matched = math.min(head[2], _matchCapacity);
      final found = <TetraStar>[];
      for (var m = 0; m < matched; m++) {
        final row = matches + m * _matchBytes;
        final ints = row.cast<Int32>().asTypedList(2);
        final doubles = (row + 8).cast<Double>().asTypedList(3);
        found.add(TetraStar(
          index: ints[0],
          catId: ints[1],
          raDeg: doubles[0],
          decDeg: doubles[1],
          mag: doubles[2],
        ));
      }
      return TetraResult._(
        solved: true,
        timedOut: false,
        ms: values[7],
        raDeg: values[0],
        decDeg: values[1],
        rollDeg: values[2],
        fovDeg: values[3],
        distortion: values[4],
        rmseArcsec: values[5],
        prob: values[6],
        matches: head[2],
        epoch: head[3],
        rotation: List<double>.of(values.sublist(8, 17)),
        stars: found,
      );
    } finally {
      tetra.free(buffer);
    }
  }
}
