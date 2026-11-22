// The crop-slide benchmark on an Android phone: Cedar and C++ Tetra, every
// crop extracted and solved from scratch.
//
//   adb push photo.jpg /data/local/tmp/bench.jpg
//   adb shell run-as com.teenastro.teenastro_app cp /data/local/tmp/bench.jpg files/bench.jpg
//   flutter run -d <phone> -t tool/phone_bench.dart --dart-define=FOV=69.4
//
// Results are printed to the flutter run console.

import 'dart:isolate';
import 'dart:math' as math;

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:teenastro_app/services/cedar_native.dart';
import 'package:teenastro_app/services/tetra_solver.dart';

const _photo = '/data/user/0/com.teenastro.teenastro_app/files/bench.jpg';
final _fovLong = double.parse(const String.fromEnvironment('FOV', defaultValue: '69.4'));

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const MaterialApp(home: _Bench()));
}

class _Bench extends StatefulWidget {
  const _Bench();

  @override
  State<_Bench> createState() => _BenchState();
}

class _BenchState extends State<_Bench> {
  String _text = 'Running…';

  @override
  void initState() {
    super.initState();
    _run();
  }

  Future<void> _run() async {
    final data = await rootBundle.load('assets/tetra/phone55.tetra');
    final catalog = data.buffer.asUint8List(data.offsetInBytes, data.lengthInBytes);
    final lines = await _sweepInIsolate(catalog);
    for (final line in lines) {
      debugPrint('BENCH $line');
    }
    if (mounted) setState(() => _text = lines.join('\n\n'));
  }

  @override
  Widget build(BuildContext context) => Scaffold(
        body: SafeArea(child: Padding(padding: const EdgeInsets.all(16), child: Text(_text))),
      );
}

Future<List<String>> _sweepInIsolate(Uint8List catalog) =>
    Isolate.run(() => _sweep(catalog));

List<String> _sweep(List<int> catalog) {
  final load = Stopwatch()..start();
  final tetra = TetraSolver.openBytes(catalog is Uint8List ? catalog : Uint8List.fromList(catalog));
  if (tetra == null) return ['The Tetra catalog could not be read.'];
  final catalogMs = load.elapsedMilliseconds;
  load.reset();
  final photo = CedarPhoto.open(_photo);
  if (photo == null) return ['$_photo could not be read.'];
  final decodeMs = load.elapsedMilliseconds;
  final shift = (photo.width * 0.10).round();
  final cropW = photo.width - shift;
  final cropH = photo.height;
  final fullLong = math.max(photo.width, photo.height).toDouble();
  final focal = (fullLong / 2) / math.tan(_fovLong * math.pi / 360);
  final cropLong = math.max(cropW, cropH).toDouble();
  final fovH = 2 * math.atan((cropLong / 2) / focal) * 180 / math.pi * cropW / cropLong;
  final extract = <double>[];
  final solve = <double>[];
  final total = <double>[];
  var solved = 0;
  final wall = Stopwatch()..start();
  for (var dx = 0; dx <= shift; dx++) {
    final frame = Stopwatch()..start();
    final crop = photo.extract(
        x0: dx, y0: 0, cropWidth: cropW, cropHeight: cropH, sigma: 10.5, binning: 2, maxStars: 50);
    final e = frame.elapsedMicroseconds / 1000;
    extract.add(e);
    if (crop == null || crop.stars.length < 4) continue;
    final result = tetra.solve(
      stars: [for (final s in crop.stars) (x: s.x, y: s.y)],
      width: cropW.toDouble(),
      height: cropH.toDouble(),
      fovHorizontalDeg: fovH,
    );
    final t = frame.elapsedMicroseconds / 1000;
    solve.add(t - e);
    total.add(t);
    if (result.solved) solved++;
  }
  final wallS = wall.elapsedMilliseconds / 1000;
  photo.close();
  tetra.close();
  String stats(List<double> v) {
    if (v.isEmpty) return 'none';
    final s = [...v]..sort();
    return 'median ${s[s.length ~/ 2].toStringAsFixed(1)} ms, max ${s.last.toStringAsFixed(1)} ms';
  }

  return [
    'Catalog $catalogMs ms. Photo ${photo.width}x${photo.height} decoded in $decodeMs ms.',
    'Tetra solved $solved of ${shift + 1} crops in ${wallS.toStringAsFixed(2)} s.',
    'Cedar ${stats(extract)}.',
    'Tetra ${stats(solve)}.',
    'Cedar + Tetra ${stats(total)}.',
  ];
}
