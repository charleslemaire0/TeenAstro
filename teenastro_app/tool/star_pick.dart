// Cedar finds the stars for Tetra. StellarSolver solves the photo itself.
//   flutter run -d windows -t tool/star_pick.dart
import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:image/image.dart' as img;
import 'package:shared_preferences/shared_preferences.dart';
import 'package:teenastro_app/models/equinox_precession.dart';
import 'package:teenastro_app/models/planet_positions.dart';
import 'package:teenastro_app/services/phone_finder.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  runApp(const StarPickApp());
}

class StarPickApp extends StatelessWidget {
  const StarPickApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Photo solve',
      debugShowCheckedModeBanner: false,
      theme: ThemeData.dark().copyWith(
        scaffoldBackgroundColor: const Color(0xFF0D1117),
        colorScheme: const ColorScheme.dark(
          primary: Color(0xFFC9453A),
          surface: Color(0xFF161B22),
        ),
      ),
      home: const StarPickPage(),
    );
  }
}

const _magPref = 'star_pick_mag_limit';
const _cedarSigmaPref = 'star_pick_cedar_sigma';
const _cedarBinPref = 'star_pick_cedar_binning';
const _cedarMaxPref = 'star_pick_cedar_max';
const _tetraPatternPref = 'star_pick_tetra_pattern';
const _tetraSecondsPref = 'star_pick_tetra_seconds';
const _solverPref = 'star_pick_solver';
const _stellarIndexPref = 'star_pick_stellar_index';
const _stellarScalePref = 'star_pick_stellar_scale';
const _stellarDownPref = 'star_pick_stellar_downsample';
const _stellarSecondsPref = 'star_pick_stellar_seconds';

enum _Solver { tetra, stellar }

class StarPickPage extends StatefulWidget {
  const StarPickPage({super.key});

  @override
  State<StarPickPage> createState() => _StarPickPageState();
}

class _StarPickPageState extends State<StarPickPage> {
  final _fov = TextEditingController(text: '70');
  final _view = TransformationController();
  final _logScroll = ScrollController();

  List<FinderStar> _identify = const [];
  List<String> _identifyNames = const [];
  List<double> _identifyMags = const [];
  String? _photoPath;
  Uint8List? _jpeg;
  img.Image? _source;
  double _imageW = 0;
  double _imageH = 0;
  final _placed = <PlacedStar>[];
  double _magLimit = 5.5;
  double _cedarSigma = 8;
  int _cedarBinning = 2;
  int _cedarMax = 80;
  int _tetraPattern = 12;
  int _tetraSeconds = 10;
  _Solver _solver = _Solver.tetra;
  String _stellarIndex = '';
  double _stellarScale = 0.20;
  int _stellarDownsample = 2;
  int _stellarSeconds = 30;
  bool _extracting = false;
  final _log = <String>[
    'Open a photo, then press Solve. Tetra uses Cedar. StellarSolver reads the photo itself.',
  ];
  bool _busy = false;

  @override
  void initState() {
    super.initState();
    _fov.addListener(_saveStars);
    _loadCatalog();
    _loadSettings();
  }

  @override
  void dispose() {
    _fov.dispose();
    _view.dispose();
    _logScroll.dispose();
    super.dispose();
  }

  Future<void> _loadCatalog() async {
    final raw = await rootBundle.loadString('assets/data/stars_mag9.json');
    final jd = julianDate(DateTime.now().toUtc());
    final json = jsonDecode(raw) as Map<String, dynamic>;
    final identify = <FinderStar>[];
    final identifyNames = <String>[];
    final identifyMags = <double>[];
    for (final item in json['stars'] as List<dynamic>) {
      final m = item as Map<String, dynamic>;
      final mag = (m['mag'] as num).toDouble();
      if (mag > wideIdentifyMag) continue;
      final now = equatorialEquinoxToJNow(
        (m['ra'] as num).toDouble(),
        (m['dec'] as num).toDouble(),
        2000,
        jd,
      );
      final name = ((m['name'] as String?) ?? '').trim();
      final label = name.isEmpty ? 'HIP ${m['hip']}' : name;
      identify.add(FinderStar(now.$1, now.$2));
      identifyNames.add(label);
      identifyMags.add(mag);
    }
    if (!mounted) return;
    setState(() {
      _identify = identify;
      _identifyNames = identifyNames;
      _identifyMags = identifyMags;
    });
  }

  Future<void> _open() async {
    final picked = await _pickPhoto();
    if (picked == null) return;
    _say('Reading the photo…');
    setState(() => _busy = true);
    final bytes = await File(picked).readAsBytes();
    final oriented = await compute(_orientPhoto, bytes);
    if (!mounted) return;
    if (oriented == null) {
      setState(() => _busy = false);
      _say('The picture could not be read.');
      return;
    }
    setState(() {
      _photoPath = picked;
      _jpeg = oriented.jpeg;
      _source = null;
      _imageW = oriented.width.toDouble();
      _imageH = oriented.height.toDouble();
      _placed.clear();
      _view.value = Matrix4.identity();
      _busy = false;
    });
    _loadStars(picked);
    final fov = oriented.longSideDeg;
    if (fov == null) {
      _say(
          'This photo has no 35 mm equivalent focal length, so the field was left as entered.');
      return;
    }
    _fov.text = fov.toStringAsFixed(1);
    _say(oriented.cameraNote ??
        'Long side ${fov.toStringAsFixed(1)}° from the camera.');
  }

  void _loadStars(String photoPath) {
    final side = _starsPath(photoPath);
    final file = File(side);
    if (!file.existsSync()) return;
    try {
      final json = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
      final savedW = (json['width'] as num?)?.toDouble() ?? _imageW;
      final savedH = (json['height'] as num?)?.toDouble() ?? _imageH;
      final sx = savedW == 0 ? 1.0 : _imageW / savedW;
      final sy = savedH == 0 ? 1.0 : _imageH / savedH;
      final marks = json['stars'] as List<dynamic>? ?? const [];
      final placed = <PlacedStar>[];
      for (final item in marks) {
        final m = item as Map<String, dynamic>;
        final name = m['name'] as String?;
        final x = (m['x'] as num).toDouble() * sx;
        final y = (m['y'] as num).toDouble() * sy;
        final refined = m['refined'] == true;
        if (m['named'] == true && name != null && name.isNotEmpty) {
          placed.add(PlacedStar(
            name: name,
            mag: (m['mag'] as num?)?.toDouble() ?? 0,
            raH: (m['ra'] as num?)?.toDouble() ?? 0,
            decD: (m['dec'] as num?)?.toDouble() ?? 0,
            x: x,
            y: y,
            refined: refined,
          ));
        } else {
          final used = placed.where((s) => !s.named).length + 1;
          final label = name == null || name.isEmpty ? '$used' : name;
          placed.add(PlacedStar(
            name: label,
            mag: 0,
            raH: 0,
            decD: 0,
            x: x,
            y: y,
            named: false,
            refined: refined,
          ));
        }
      }
      setState(() {
        _placed
          ..clear()
          ..addAll(placed);
        _text(_fov, json['fov']);
      });
      _say(
          'Restored ${placed.length} stars from ${file.uri.pathSegments.last}.');
      if (placed.any((s) => !s.refined)) _refineUncentered();
    } catch (error) {
      _say('The saved stars could not be read ($error).');
    }
  }

  void _saveStars() {
    final path = _photoPath;
    if (path == null || _imageW < 2) return;
    final payload = {
      'photo': path,
      'width': _imageW,
      'height': _imageH,
      'fov': _fov.text,
      'stars': [
        for (final s in _placed)
          {
            'name': s.name,
            'named': s.named,
            'mag': s.mag,
            'ra': s.raH,
            'dec': s.decD,
            'x': s.x,
            'y': s.y,
            'refined': s.refined,
          },
      ],
    };
    try {
      File(_starsPath(path)).writeAsStringSync(
        const JsonEncoder.withIndent('  ').convert(payload),
      );
    } catch (error) {
      _log.add('The stars could not be saved ($error).');
    }
  }

  void _text(TextEditingController controller, Object? value) {
    if (value == null) return;
    controller.text = value.toString();
  }

  Future<void> _loadSettings() async {
    final prefs = await SharedPreferences.getInstance();
    for (final key in const [
      'star_pick_lens_distortion',
      'star_pick_lens_k2',
      'star_pick_lens_k3',
      'star_pick_lens_ox',
      'star_pick_lens_oy',
      'star_pick_lens_field',
    ]) {
      await prefs.remove(key);
    }
    if (!mounted) return;
    final mag = prefs.getDouble(_magPref);
    final sigma = prefs.getDouble(_cedarSigmaPref);
    final binning = prefs.getInt(_cedarBinPref);
    final maxStars = prefs.getInt(_cedarMaxPref);
    final pattern = prefs.getInt(_tetraPatternPref);
    final seconds = prefs.getInt(_tetraSecondsPref);
    final which = prefs.getString(_solverPref);
    final stellarIndex = prefs.getString(_stellarIndexPref);
    final stellarScale = prefs.getDouble(_stellarScalePref);
    final stellarDown = prefs.getInt(_stellarDownPref);
    final stellarSeconds = prefs.getInt(_stellarSecondsPref);
    setState(() {
      if (mag != null) {
        _magLimit = mag.clamp(1.0, wideIdentifyMag).toDouble();
      }
      if (sigma != null) _cedarSigma = sigma.clamp(4.0, 15.0).toDouble();
      if (binning == 1 || binning == 2 || binning == 4) {
        _cedarBinning = binning!;
      }
      if (maxStars != null) _cedarMax = maxStars.clamp(20, 100);
      if (pattern != null) _tetraPattern = pattern.clamp(4, 30);
      if (seconds != null) _tetraSeconds = seconds.clamp(1, 30);
      if (which == 'stellar') _solver = _Solver.stellar;
      if (stellarIndex != null) _stellarIndex = stellarIndex;
      if (stellarScale != null) {
        _stellarScale = stellarScale.clamp(0.05, 0.50).toDouble();
      }
      if (stellarDown == 1 || stellarDown == 2 || stellarDown == 4) {
        _stellarDownsample = stellarDown!;
      }
      if (stellarSeconds != null) {
        _stellarSeconds = stellarSeconds.clamp(1, 120);
      }
    });
  }

  Future<void> _storeMagLimit(double value) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setDouble(_magPref, value);
  }

  Future<void> _storeTetra() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setInt(_tetraPatternPref, _tetraPattern);
    await prefs.setInt(_tetraSecondsPref, _tetraSeconds);
  }

  Future<void> _storeCedar() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setDouble(_cedarSigmaPref, _cedarSigma);
    await prefs.setInt(_cedarBinPref, _cedarBinning);
    await prefs.setInt(_cedarMaxPref, _cedarMax);
  }

  Future<void> _storeSolver() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(
      _solverPref,
      _solver == _Solver.stellar ? 'stellar' : 'tetra',
    );
  }

  Future<void> _storeStellar() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_stellarIndexPref, _stellarIndex);
    await prefs.setDouble(_stellarScalePref, _stellarScale);
    await prefs.setInt(_stellarDownPref, _stellarDownsample);
    await prefs.setInt(_stellarSecondsPref, _stellarSeconds);
  }

  void _chooseSolver(_Solver solver) {
    setState(() => _solver = solver);
    _storeSolver();
  }

  /// Image-width field, in degrees, from the long-side field.
  ///
  /// StellarSolver's degree scale is the width of the picture.
  double? _widthFieldDeg() {
    final long = double.tryParse(_fov.text.trim().replaceAll(',', '.'));
    if (long == null || long <= 0 || _imageW < 2 || _imageH < 2) return null;
    return long * _imageW / math.max(_imageW, _imageH);
  }

  String _scaleBand() {
    final widthDeg = _widthFieldDeg();
    if (widthDeg == null) {
      return 'A band around the long-side field. It is the image width once a photo is open.';
    }
    final low = widthDeg * (1 - _stellarScale);
    final high = widthDeg * (1 + _stellarScale);
    return 'Image width ${widthDeg.toStringAsFixed(1)}° from the long side. '
        'StellarSolver searches ${low.toStringAsFixed(1)}° to ${high.toStringAsFixed(1)}°.';
  }

  Future<void> _openParameters() async {
    final index = TextEditingController(text: _stellarIndex);
    try {
      await showDialog<void>(
        context: context,
        builder: (context) {
          return StatefulBuilder(
            builder: (context, setDialog) {
              return AlertDialog(
                backgroundColor: const Color(0xFF161B22),
                title: const Text('Parameters'),
                content: SizedBox(
                  width: 400,
                  child: SingleChildScrollView(
                    child: Column(
                      mainAxisSize: MainAxisSize.min,
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        Text(
                          _solver == _Solver.stellar
                              ? 'Solve uses StellarSolver.'
                              : 'Solve uses Tetra.',
                          style: const TextStyle(
                            color: Color(0xFF8B949E),
                            fontSize: 13,
                          ),
                        ),
                        const SizedBox(height: 14),
                        const Text(
                          'Tetra',
                          style:
                              TextStyle(color: Color(0xFFF0F6FC), fontSize: 16),
                        ),
                        const SizedBox(height: 8),
                        Text(
                          'Magnitude limit ${_magLimit.toStringAsFixed(1)}',
                          style: const TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        Slider(
                          min: 1,
                          max: wideIdentifyMag,
                          divisions: ((wideIdentifyMag - 1) * 10).round(),
                          value:
                              _magLimit.clamp(1.0, wideIdentifyMag).toDouble(),
                          label: _magLimit.toStringAsFixed(1),
                          onChanged: (value) {
                            setDialog(() => _magLimit = value);
                            _storeMagLimit(value);
                          },
                        ),
                        const Text(
                          'Tetra leaves a fainter star unnamed. The mark stays on the photo.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                        const SizedBox(height: 16),
                        Text(
                          'Pattern stars $_tetraPattern',
                          style: const TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        Slider(
                          min: 4,
                          max: 30,
                          divisions: 26,
                          value: _tetraPattern.clamp(4, 30).toDouble(),
                          label: '$_tetraPattern',
                          onChanged: (value) {
                            setDialog(() => _tetraPattern = value.round());
                            _storeTetra();
                          },
                        ),
                        const Text(
                          'Tetra builds its pattern from this many of the brightest detections. A higher number tries more combinations.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                        const SizedBox(height: 16),
                        Text(
                          'Solve time $_tetraSeconds s',
                          style: const TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        Slider(
                          min: 1,
                          max: 30,
                          divisions: 29,
                          value: _tetraSeconds.clamp(1, 30).toDouble(),
                          label: '$_tetraSeconds s',
                          onChanged: (value) {
                            setDialog(() => _tetraSeconds = value.round());
                            _storeTetra();
                          },
                        ),
                        const Text(
                          'Tetra stops searching when this time is up. A pattern found earlier is kept.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                        const SizedBox(height: 16),
                        Text(
                          'Cedar sigma ${_cedarSigma.toStringAsFixed(1)}',
                          style: const TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        Slider(
                          min: 4,
                          max: 15,
                          divisions: 22,
                          value: _cedarSigma.clamp(4.0, 15.0).toDouble(),
                          label: _cedarSigma.toStringAsFixed(1),
                          onChanged: (value) {
                            setDialog(() => _cedarSigma = value);
                            _storeCedar();
                          },
                        ),
                        const Text(
                          'A higher sigma keeps only the stars that stand further above the noise.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                        const SizedBox(height: 12),
                        const Text(
                          'Cedar binning',
                          style: TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        const SizedBox(height: 6),
                        Row(
                          children: [
                            for (final bin in const [1, 2, 4])
                              Padding(
                                padding: const EdgeInsets.only(right: 8),
                                child: ChoiceChip(
                                  label: Text(bin == 1 ? 'Full' : '$bin×$bin'),
                                  selected: _cedarBinning == bin,
                                  onSelected: (_) {
                                    setDialog(() => _cedarBinning = bin);
                                    _storeCedar();
                                  },
                                ),
                              ),
                          ],
                        ),
                        const SizedBox(height: 4),
                        const Text(
                          'Binning finds broader stars. The mark is still placed on the full photo.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                        const SizedBox(height: 12),
                        Text(
                          'Stars to keep $_cedarMax',
                          style: const TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        Slider(
                          min: 20,
                          max: 100,
                          divisions: 8,
                          value: _cedarMax.clamp(20, 100).toDouble(),
                          label: '$_cedarMax',
                          onChanged: (value) {
                            setDialog(() => _cedarMax = value.round());
                            _storeCedar();
                          },
                        ),
                        const Text(
                          'Cedar returns the brightest stars first. Only this many are placed.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                        const SizedBox(height: 18),
                        const Text(
                          'StellarSolver',
                          style:
                              TextStyle(color: Color(0xFFF0F6FC), fontSize: 16),
                        ),
                        const SizedBox(height: 8),
                        TextField(
                          controller: index,
                          style: const TextStyle(
                            color: Color(0xFFF0F6FC),
                            fontSize: 13,
                          ),
                          decoration: const InputDecoration(
                            labelText: 'Index folder',
                            isDense: true,
                            labelStyle: TextStyle(
                              color: Color(0xFF8B949E),
                              fontSize: 12,
                            ),
                          ),
                          onChanged: (value) {
                            _stellarIndex = value;
                            _storeStellar();
                          },
                        ),
                        Align(
                          alignment: Alignment.centerLeft,
                          child: TextButton(
                            onPressed: () async {
                              final picked = await _pickFolder();
                              if (picked == null) return;
                              index.text = picked;
                              setDialog(() => _stellarIndex = picked);
                              _storeStellar();
                            },
                            child: const Text('Browse'),
                          ),
                        ),
                        const Text(
                          'The folder of Astrometry.net index files. StellarSolver reads the photo with its own star extractor.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                        const SizedBox(height: 12),
                        Text(
                          'Scale tolerance ${(_stellarScale * 100).round()}%',
                          style: const TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        Slider(
                          min: 0.05,
                          max: 0.50,
                          divisions: 45,
                          value: _stellarScale.clamp(0.05, 0.50).toDouble(),
                          label: '${(_stellarScale * 100).round()}%',
                          onChanged: (value) {
                            setDialog(() => _stellarScale = value);
                            _storeStellar();
                          },
                        ),
                        Text(
                          _scaleBand(),
                          style: const TextStyle(
                            color: Color(0xFF8B949E),
                            fontSize: 12,
                          ),
                        ),
                        const SizedBox(height: 12),
                        const Text(
                          'Downsample',
                          style: TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        const SizedBox(height: 6),
                        Row(
                          children: [
                            for (final factor in const [1, 2, 4])
                              Padding(
                                padding: const EdgeInsets.only(right: 8),
                                child: ChoiceChip(
                                  label:
                                      Text(factor == 1 ? 'Full' : '$factor×'),
                                  selected: _stellarDownsample == factor,
                                  onSelected: (_) {
                                    setDialog(
                                        () => _stellarDownsample = factor);
                                    _storeStellar();
                                  },
                                ),
                              ),
                          ],
                        ),
                        const SizedBox(height: 4),
                        const Text(
                          'The photo is reduced by this factor while solving. The marks are placed on the full photo.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                        const SizedBox(height: 12),
                        Text(
                          'Solve time $_stellarSeconds s',
                          style: const TextStyle(color: Color(0xFFF0F6FC)),
                        ),
                        Slider(
                          min: 1,
                          max: 120,
                          divisions: 119,
                          value: _stellarSeconds.clamp(1, 120).toDouble(),
                          label: '$_stellarSeconds s',
                          onChanged: (value) {
                            setDialog(() => _stellarSeconds = value.round());
                            _storeStellar();
                          },
                        ),
                        const Text(
                          'StellarSolver stops when this time is up. Names use the Tetra magnitude limit. A fainter match stays numbered.',
                          style:
                              TextStyle(color: Color(0xFF8B949E), fontSize: 12),
                        ),
                      ],
                    ),
                  ),
                ),
                actions: [
                  TextButton(
                    onPressed: () => Navigator.pop(context),
                    child: const Text('Close'),
                  ),
                ],
              );
            },
          );
        },
      );
    } finally {
      index.dispose();
    }
  }

  void _say(String line) {
    setState(() => _log.add(line));
    _scrollLog();
  }

  /// Center, roll, residual, and solve time, in the same shape for both solvers.
  String _solutionTail({
    required double? raDeg,
    required double? decDeg,
    required int epoch,
    required double jd,
    required double? rollDeg,
    required double? rmseArcsec,
    required double? elapsedMs,
  }) {
    if (raDeg == null || decDeg == null) return '';
    final now = equatorialEquinoxToJNow(raDeg / 15.0, decDeg, epoch, jd);
    final rollText =
        rollDeg == null ? '' : ', roll ${rollDeg.toStringAsFixed(1)}°';
    final residual = rmseArcsec == null
        ? ''
        : ', residual ${(rmseArcsec / 60).toStringAsFixed(1)}\'';
    final timeText =
        elapsedMs == null ? '' : ', ${elapsedMs.toStringAsFixed(0)} ms';
    return ' Center ${_clockHours(now.$1)}, ${_signedDegrees(now.$2)}$rollText$residual$timeText.';
  }

  void _scrollLog() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!_logScroll.hasClients) return;
      _logScroll.jumpTo(_logScroll.position.maxScrollExtent);
    });
  }

  /// Tetra solves the Cedar marks and names the stars it matched.
  Future<void> _identifyWithTetra() async {
    final photo = _photoPath;
    if (photo == null) return;
    final script = File('tool/tetra_solve.py');
    if (!script.existsSync()) {
      _say('Tetra is not next to the star picker.');
      return;
    }
    ProcessResult result;
    try {
      result = await Process.run(
        'python',
        [
          script.absolute.path,
          _starsPath(photo),
          '$_tetraPattern',
          '$_tetraSeconds',
        ],
        stdoutEncoding: utf8,
        stderrEncoding: utf8,
      );
    } catch (error) {
      _say('Tetra could not be started ($error).');
      return;
    }
    final raw = (result.stdout as String).trim();
    Map<String, dynamic> payload;
    try {
      payload = jsonDecode(raw) as Map<String, dynamic>;
    } catch (_) {
      final err = (result.stderr as String).trim();
      final detail = err.isEmpty ? raw : err;
      _say(
          'Tetra did not return a solution.${detail.isEmpty ? '' : ' $detail'}');
      return;
    }
    if (payload['ok'] != true) {
      final reason = payload['reason']?.toString() ?? '';
      _say(
          'Tetra did not match these clicks${reason.isEmpty ? '' : ' ($reason)'}.');
      return;
    }
    final epoch = (payload['epoch_equinox'] as num?)?.round() ?? 2000;
    final jd = julianDate(DateTime.now().toUtc());
    final rows = payload['stars'];
    if (rows is! List) return;
    final next = List<PlacedStar>.from(_placed);
    final used = <int>{};
    final names = <String>[];
    var accepted = 0;
    var fainter = 0;
    for (final item in rows) {
      if (item is! Map) continue;
      final index = (item['index'] as num?)?.toInt();
      final raDeg = (item['ra_deg'] as num?)?.toDouble();
      final decDeg = (item['dec_deg'] as num?)?.toDouble();
      if (index == null ||
          index < 0 ||
          index >= next.length ||
          next[index].named ||
          raDeg == null ||
          decDeg == null) {
        continue;
      }
      final now = equatorialEquinoxToJNow(raDeg / 15.0, decDeg, epoch, jd);
      var best = -1;
      var bestSep = 0.3;
      for (var i = 0; i < _identify.length; i++) {
        if (used.contains(i)) continue;
        final sep = sphereSeparationDeg(
          now.$1,
          now.$2,
          _identify[i].raH,
          _identify[i].decD,
        );
        if (sep < bestSep) {
          bestSep = sep;
          best = i;
        }
      }
      final old = next[index];
      final mag = best >= 0
          ? _identifyMags[best]
          : (item['mag'] as num?)?.toDouble() ?? old.mag;
      accepted++;
      if (mag > _magLimit) {
        fainter++;
        continue;
      }
      if (best >= 0) {
        used.add(best);
        next[index] = PlacedStar(
          name: _identifyNames[best],
          mag: _identifyMags[best],
          raH: _identify[best].raH,
          decD: _identify[best].decD,
          x: old.x,
          y: old.y,
          named: true,
          refined: old.refined,
        );
      } else {
        final cat = item['cat'];
        next[index] = PlacedStar(
          name: cat == null ? 'Tetra' : 'HR $cat',
          mag: mag,
          raH: now.$1,
          decD: now.$2,
          x: old.x,
          y: old.y,
          named: true,
          refined: old.refined,
        );
      }
      names.add(next[index].name);
    }
    final center = _solutionTail(
      raDeg: (payload['ra_deg'] as num?)?.toDouble(),
      decDeg: (payload['dec_deg'] as num?)?.toDouble(),
      epoch: epoch,
      jd: jd,
      rollDeg: (payload['roll_deg'] as num?)?.toDouble(),
      rmseArcsec: (payload['rmse_arcsec'] as num?)?.toDouble(),
      elapsedMs: (payload['ms'] as num?)?.toDouble(),
    );
    if (accepted < 3) {
      _say(
          'Tetra matched ${payload['matches']} stars, and $accepted of them are in the catalog.$center');
      return;
    }
    setState(() => _placed
      ..clear()
      ..addAll(next));
    _saveStars();
    final hidden = fainter == 0
        ? ''
        : ' Magnitude ${_magLimit.toStringAsFixed(1)} left $fainter fainter stars unnamed.';
    _say('Tetra named ${names.join(', ')}.$center$hidden');
  }

  /// StellarSolver extracts the stars itself and names catalog matches.
  Future<void> _solveWithStellar() async {
    final jpeg = _jpeg;
    if (jpeg == null) {
      _say('Open a photo first.');
      return;
    }
    final index = _stellarIndex.trim();
    if (index.isEmpty) {
      _say('Choose the folder of Astrometry.net index files in Parameters.');
      return;
    }
    if (!Directory(index).existsSync()) {
      _say('The index folder is not there.');
      return;
    }
    final widthDeg = _widthFieldDeg();
    if (widthDeg == null || widthDeg <= 0) {
      _say('Enter the field of the long side, in degrees.');
      return;
    }
    final script = File('tool/stellar_solve.py');
    if (!script.existsSync()) {
      _say('StellarSolver is not next to the star picker.');
      return;
    }
    final low = widthDeg * (1 - _stellarScale);
    final high = widthDeg * (1 + _stellarScale);
    final temp = File(
      '${Directory.systemTemp.path}${Platform.pathSeparator}teenastro-stellar-${DateTime.now().microsecondsSinceEpoch}.jpg',
    );
    setState(() {
      _log.add(
        'StellarSolver is extracting stars, downsample $_stellarDownsample, '
        'width ${low.toStringAsFixed(1)}° to ${high.toStringAsFixed(1)}°, limit $_stellarSeconds s.',
      );
    });
    _scrollLog();
    try {
      await temp.writeAsBytes(jpeg, flush: true);
    } catch (error) {
      _say('The photo could not be prepared for StellarSolver ($error).');
      return;
    }
    ProcessResult result;
    try {
      result = await Process.run(
        'python',
        [
          script.absolute.path,
          temp.path,
          index,
          '$low',
          '$high',
          '$_stellarDownsample',
          '$_stellarSeconds',
        ],
        stdoutEncoding: utf8,
        stderrEncoding: utf8,
      );
    } catch (error) {
      _say('StellarSolver could not be started ($error).');
      return;
    } finally {
      try {
        if (temp.existsSync()) temp.deleteSync();
      } catch (_) {}
    }
    if (!mounted) return;
    final raw = (result.stdout as String).trim();
    Map<String, dynamic> payload;
    try {
      payload = jsonDecode(raw) as Map<String, dynamic>;
    } catch (_) {
      final err = (result.stderr as String).trim();
      final detail = err.isEmpty ? raw : err;
      _say(
          'StellarSolver did not return a solution.${detail.isEmpty ? '' : ' $detail'}');
      return;
    }
    if (payload['ok'] != true) {
      final reason = payload['reason']?.toString() ?? '';
      _say(
          'StellarSolver did not solve this photo${reason.isEmpty ? '' : ' ($reason)'}.');
      return;
    }
    _applyStellar(payload);
  }

  void _applyStellar(Map<String, dynamic> payload) {
    final epoch = (payload['epoch_equinox'] as num?)?.round() ?? 2000;
    final jd = julianDate(DateTime.now().toUtc());
    final rows = payload['stars'];
    final stars = rows is List ? rows : const <dynamic>[];
    final next = <PlacedStar>[];
    final used = <int>{};
    final names = <String>[];
    final seps = <double>[];
    var fainter = 0;
    var number = 0;
    for (final item in stars) {
      if (item is! Map) continue;
      final x = (item['x'] as num?)?.toDouble();
      final y = (item['y'] as num?)?.toDouble();
      if (x == null || y == null) continue;
      number++;
      var placed = PlacedStar(
        name: '$number',
        mag: 0,
        raH: 0,
        decD: 0,
        x: x,
        y: y,
        named: false,
        refined: true,
      );
      final raDeg = (item['ra_deg'] as num?)?.toDouble();
      final decDeg = (item['dec_deg'] as num?)?.toDouble();
      if (raDeg != null && decDeg != null && _identify.isNotEmpty) {
        final now = equatorialEquinoxToJNow(raDeg / 15.0, decDeg, epoch, jd);
        var best = -1;
        var bestSep = 0.3;
        for (var i = 0; i < _identify.length; i++) {
          if (used.contains(i)) continue;
          final sep = sphereSeparationDeg(
            now.$1,
            now.$2,
            _identify[i].raH,
            _identify[i].decD,
          );
          if (sep < bestSep) {
            bestSep = sep;
            best = i;
          }
        }
        if (best >= 0) {
          seps.add(bestSep);
          final mag = _identifyMags[best];
          if (mag > _magLimit) {
            fainter++;
          } else {
            used.add(best);
            placed = PlacedStar(
              name: _identifyNames[best],
              mag: mag,
              raH: _identify[best].raH,
              decD: _identify[best].decD,
              x: x,
              y: y,
              named: true,
              refined: true,
            );
            names.add(placed.name);
          }
        }
      }
      next.add(placed);
    }
    var sum = 0.0;
    for (final sep in seps) {
      final arcsec = sep * 3600;
      sum += arcsec * arcsec;
    }
    final computed = seps.isEmpty ? null : math.sqrt(sum / seps.length);
    final reported = (payload['rmse_arcsec'] as num?)?.toDouble();
    final center = _solutionTail(
      raDeg: (payload['ra_deg'] as num?)?.toDouble(),
      decDeg: (payload['dec_deg'] as num?)?.toDouble(),
      epoch: epoch,
      jd: jd,
      rollDeg: (payload['roll_deg'] as num?)?.toDouble(),
      rmseArcsec: reported != null && reported > 0 ? reported : computed,
      elapsedMs: (payload['ms'] as num?)?.toDouble(),
    );
    setState(() => _placed
      ..clear()
      ..addAll(next));
    _saveStars();
    final hidden = fainter == 0
        ? ''
        : ' Magnitude ${_magLimit.toStringAsFixed(1)} left $fainter fainter stars unnamed.';
    final lead = names.isEmpty
        ? 'StellarSolver placed ${next.length} stars.'
        : 'StellarSolver named ${names.join(', ')}.';
    _say('$lead$center$hidden');
  }

  Future<void> _solve() async {
    if (_jpeg == null) {
      _say('Open a photo first.');
      return;
    }
    final typedFov = double.tryParse(_fov.text.trim().replaceAll(',', '.'));
    if (typedFov == null || typedFov <= 0) {
      _say('Enter the field of the long side, in degrees.');
      return;
    }
    if (_solver == _Solver.stellar) {
      setState(() {
        _busy = true;
        _extracting = true;
      });
      await _solveWithStellar();
      if (!mounted) return;
      setState(() => _busy = false);
      return;
    }
    setState(() {
      _busy = true;
      _extracting = true;
    });
    final extracted = await _extractStars();
    if (!mounted || !extracted) return;
    if (_placed.length < 4) {
      setState(() {
        _busy = false;
        _extracting = false;
      });
      _say('Cedar placed ${_placed.length} stars. Tetra needs at least 4.');
      return;
    }
    setState(() {
      _busy = true;
      _extracting = false;
      _log.add(
          'Tetra is solving from the $_tetraPattern brightest, limit $_tetraSeconds s…');
    });
    _scrollLog();
    await _identifyWithTetra();
    if (!mounted) return;
    setState(() => _busy = false);
  }

  img.Image? _sourceImage() {
    final cached = _source;
    if (cached != null) return cached;
    final bytes = _jpeg;
    if (bytes == null) return null;
    final decoded = img.decodeImage(bytes);
    _source = decoded;
    return decoded;
  }

  PlacedStar? _refinedStar(PlacedStar star, img.Image src) {
    final mid = starMiddle(src, star.x, star.y);
    if (mid == null) return null;
    return PlacedStar(
      name: star.name,
      mag: star.mag,
      raH: star.raH,
      decD: star.decD,
      x: mid.x,
      y: mid.y,
      named: star.named,
      refined: true,
    );
  }

  void _refineUncentered() {
    final src = _sourceImage();
    if (src == null) {
      _say('The picture could not be read, so the unrefined stars were left.');
      return;
    }
    var pending = 0;
    var done = 0;
    var missed = 0;
    setState(() {
      for (var i = 0; i < _placed.length; i++) {
        final star = _placed[i];
        if (star.refined) continue;
        pending++;
        final next = _refinedStar(star, src);
        if (next == null) {
          missed++;
          continue;
        }
        _placed[i] = next;
        done++;
      }
      _log.add(
        'Centered $done of $pending stars that were not refined. '
        '$missed had no star nearby.',
      );
    });
    _saveStars();
    _scrollLog();
  }

  /// Cedar finds the stars. The marks replace whatever was clicked before.
  Future<bool> _extractStars() async {
    final limit = _cedarMax;
    final photo = _photoPath;
    if (photo == null || _jpeg == null) {
      _say('Open a photo first.');
      return false;
    }
    final script = File('tool/cedar_extract.py');
    if (!script.existsSync()) {
      if (mounted) {
        setState(() {
          _busy = false;
          _extracting = false;
        });
      }
      _say('Cedar is not next to the star picker.');
      return false;
    }
    _saveStars();
    setState(() {
      _log.add(
        'Cedar is extracting stars, sigma ${_cedarSigma.toStringAsFixed(1)}, '
        '${_cedarBinning == 1 ? 'full image' : '$_cedarBinning×$_cedarBinning binning'}.',
      );
    });
    _scrollLog();
    ProcessResult result;
    try {
      result = await Process.run(
        'python',
        [
          script.absolute.path,
          _starsPath(photo),
          _cedarSigma.toString(),
          '$_cedarBinning',
          '$limit',
        ],
        stdoutEncoding: utf8,
        stderrEncoding: utf8,
      );
    } catch (error) {
      if (mounted) {
        setState(() {
          _busy = false;
          _extracting = false;
        });
      }
      _say('Cedar could not be started ($error).');
      return false;
    }
    if (!mounted) return false;
    final raw = (result.stdout as String).trim();
    Map<String, dynamic> payload;
    try {
      payload = jsonDecode(raw) as Map<String, dynamic>;
    } catch (_) {
      final err = (result.stderr as String).trim();
      final detail = err.isEmpty ? raw : err;
      setState(() {
        _busy = false;
        _extracting = false;
      });
      _say('Cedar did not return stars.${detail.isEmpty ? '' : ' $detail'}');
      return false;
    }
    if (payload['ok'] != true) {
      setState(() {
        _busy = false;
        _extracting = false;
      });
      final reason = payload['reason']?.toString() ?? '';
      _say(reason.isEmpty ? 'Cedar found no stars.' : reason);
      return false;
    }
    final stars = payload['stars'];
    if (stars is! List || stars.isEmpty) {
      setState(() {
        _busy = false;
        _extracting = false;
      });
      _say('Cedar found no stars.');
      return false;
    }
    final placed = <PlacedStar>[];
    for (var i = 0; i < stars.length; i++) {
      final star = stars[i];
      if (star is! Map) continue;
      final x = (star['x'] as num?)?.toDouble();
      final y = (star['y'] as num?)?.toDouble();
      if (x == null || y == null) continue;
      placed.add(PlacedStar(
        name: '${i + 1}',
        mag: 0,
        raH: 0,
        decD: 0,
        x: x,
        y: y,
        named: false,
        refined: true,
      ));
    }
    final found = (payload['found'] as num?)?.toInt() ?? placed.length;
    final noise = (payload['noise'] as num?)?.toDouble();
    setState(() {
      _placed
        ..clear()
        ..addAll(placed);
      _extracting = false;
      _log.add(
        'Cedar placed ${placed.length} of $found stars'
        '${noise == null ? '' : ', noise ${noise.toStringAsFixed(1)}'}.',
      );
    });
    _saveStars();
    _scrollLog();
    return true;
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Row(
        children: [
          Expanded(child: _photo()),
          SizedBox(width: 420, child: _panel()),
        ],
      ),
    );
  }

  Widget _photo() {
    final jpeg = _jpeg;
    if (jpeg == null) {
      return Center(
        child: Text(
          _log.isEmpty ? '' : _log.last,
          style: const TextStyle(color: Color(0xFF8B949E)),
        ),
      );
    }
    final long = math.max(_imageW, _imageH);
    final scale = 900 / long;
    final viewW = _imageW * scale;
    final viewH = _imageH * scale;
    return InteractiveViewer(
      transformationController: _view,
      minScale: 0.4,
      maxScale: 8,
      constrained: false,
      boundaryMargin: const EdgeInsets.all(200),
      child: SizedBox(
        width: viewW,
        height: viewH,
        child: Stack(
          children: [
            Image.memory(jpeg, width: viewW, height: viewH, fit: BoxFit.fill),
            for (var i = 0; i < _placed.length; i++) _mark(i, viewW, viewH),
          ],
        ),
      ),
    );
  }

  Widget _mark(int index, double viewW, double viewH) {
    final star = _placed[index];
    const color = Color(0xFFF0C040);
    const namedColor = Color(0xFF3DDC84);
    final ink = star.named ? namedColor : color;
    final x = star.x / _imageW * viewW;
    final y = star.y / _imageH * viewH;
    return Positioned(
      left: x - 8,
      top: y - 8,
      child: IgnorePointer(
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Container(
              width: 16,
              height: 16,
              alignment: Alignment.center,
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                border: Border.all(color: ink, width: 1.5),
              ),
              child: star.named
                  ? null
                  : Text(
                      star.name,
                      style: TextStyle(color: ink, fontSize: 9),
                    ),
            ),
            if (star.named) const SizedBox(width: 4),
            if (star.named)
              DecoratedBox(
                decoration: BoxDecoration(
                  color: const Color(0xCC0D1117),
                  borderRadius: BorderRadius.circular(3),
                ),
                child: Padding(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 4, vertical: 1),
                  child: Text(
                    star.name,
                    style: TextStyle(color: ink, fontSize: 12),
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }

  Widget _panel() {
    return Material(
      color: const Color(0xFF161B22),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(16, 12, 16, 12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            const Text(
              'Photo solve',
              style: TextStyle(fontSize: 18, color: Color(0xFFF0F6FC)),
            ),
            const SizedBox(height: 8),
            FilledButton(
                onPressed: _busy ? null : _open,
                child: const Text('Open photo')),
            Align(
              alignment: Alignment.centerLeft,
              child: TextButton(
                onPressed: _placed.isEmpty
                    ? null
                    : () {
                        setState(() {
                          _placed.clear();
                          _log.add('Marks cleared.');
                        });
                        _saveStars();
                      },
                child: const Text('Clear'),
              ),
            ),
            _field(_fov, 'Field of the long side (°)', '70'),
            const Text(
              'Solver',
              style: TextStyle(color: Color(0xFFF0F6FC)),
            ),
            const SizedBox(height: 6),
            Row(
              children: [
                for (final solver in _Solver.values)
                  Padding(
                    padding: const EdgeInsets.only(right: 8),
                    child: ChoiceChip(
                      label: Text(
                        solver == _Solver.tetra ? 'Tetra' : 'StellarSolver',
                      ),
                      selected: _solver == solver,
                      onSelected: _busy ? null : (_) => _chooseSolver(solver),
                    ),
                  ),
              ],
            ),
            const SizedBox(height: 8),
            FilledButton(
              onPressed: _busy ? null : _solve,
              child: Text(
                  _busy ? (_extracting ? 'Extracting…' : 'Solving…') : 'Solve'),
            ),
            const SizedBox(height: 6),
            OutlinedButton(
              onPressed: _busy ? null : _openParameters,
              child: const Text('Parameters'),
            ),
            const SizedBox(height: 8),
            Text(
              _solver == _Solver.stellar
                  ? 'StellarSolver reads the photo and names catalog matches. A numbered mark was left unmatched.'
                  : 'Cedar places the stars. Tetra names the ones it matches. A numbered mark was left unmatched.',
              style: const TextStyle(color: Color(0xFF8B949E), fontSize: 12),
            ),
            const SizedBox(height: 10),
            Expanded(
              child: DecoratedBox(
                decoration: BoxDecoration(
                  color: const Color(0xFF0D1117),
                  border: Border.all(color: const Color(0xFF30363D)),
                  borderRadius: BorderRadius.circular(6),
                ),
                child: Scrollbar(
                  controller: _logScroll,
                  thumbVisibility: true,
                  child: ListView.builder(
                    controller: _logScroll,
                    padding: const EdgeInsets.all(8),
                    itemCount: _log.length,
                    itemBuilder: (context, index) => SelectableText(
                      _log[index],
                      style: const TextStyle(
                        fontFamily: 'Consolas',
                        fontSize: 12,
                        height: 1.35,
                        color: Color(0xFFC9D1D9),
                      ),
                    ),
                  ),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _field(TextEditingController controller, String label, String hint) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 10),
      child: TextField(
        controller: controller,
        style: const TextStyle(color: Color(0xFFF0F6FC)),
        decoration: InputDecoration(
          labelText: label,
          hintText: hint,
          isDense: true,
          labelStyle: const TextStyle(color: Color(0xFF8B949E), fontSize: 12),
        ),
      ),
    );
  }
}

String _clockHours(double hours) {
  var total = (wrapHours(hours) * 3600).round();
  final h = total ~/ 3600;
  total -= h * 3600;
  final m = total ~/ 60;
  final s = total - m * 60;
  return '${h.toString().padLeft(2, '0')}:'
      '${m.toString().padLeft(2, '0')}:'
      '${s.toString().padLeft(2, '0')}';
}

String _signedDegrees(double degrees) {
  final sign = degrees < 0 ? '-' : '+';
  var total = (degrees.abs() * 60).round();
  final d = total ~/ 60;
  final m = total - d * 60;
  return '$sign${d.toString().padLeft(2, '0')}*${m.toString().padLeft(2, '0')}';
}

class PlacedStar {
  final String name;
  final double mag;
  final double raH;
  final double decD;
  final double x;
  final double y;
  final bool named;
  final bool refined;
  const PlacedStar({
    required this.name,
    required this.mag,
    required this.raH,
    required this.decD,
    required this.x,
    required this.y,
    this.named = true,
    this.refined = false,
  });
}

String _starsPath(String photoPath) {
  final slash =
      math.max(photoPath.lastIndexOf('\\'), photoPath.lastIndexOf('/'));
  final dot = photoPath.lastIndexOf('.');
  final base = dot > slash ? photoPath.substring(0, dot) : photoPath;
  return '$base.stars.json';
}

/// Upright JPEG plus its pixel size. Coordinates match the picture on screen.
({
  Uint8List jpeg,
  int width,
  int height,
  double? longSideDeg,
  String? cameraNote,
})? _orientPhoto(Uint8List bytes) {
  final decoded = img.decodeImage(bytes);
  if (decoded == null) return null;
  final camera = _cameraField(decoded);
  final frame = img.bakeOrientation(decoded);
  return (
    jpeg: Uint8List.fromList(img.encodeJpg(frame, quality: 85)),
    width: frame.width,
    height: frame.height,
    longSideDeg: camera?.fov,
    cameraNote: camera?.line,
  );
}

/// Long-side field from the 35 mm equivalent focal length.
///
/// The equivalent length matches the diagonal of a 36×24 mm frame to the
/// diagonal of this photo, so a 4:3 phone frame is not treated as 3:2 film.
({double fov, String line})? _cameraField(img.Image image) {
  final focal35 = image.exif.exifIfd['FocalLengthIn35mmFilm']?.toDouble() ?? 0;
  if (focal35 <= 0) return null;
  final zoom = image.exif.exifIfd['DigitalZoomRatio']?.toDouble() ?? 1;
  final focal = zoom > 1 ? focal35 * zoom : focal35;
  final long = math.max(image.width, image.height).toDouble();
  final short = math.min(image.width, image.height).toDouble();
  final diag = math.sqrt(long * long + short * short);
  const filmDiag = 43.26661530556787;
  final halfLong = filmDiag / 2 * (long / diag);
  final fov = 2 * math.atan(halfLong / focal) * 180 / math.pi;
  final lens = image.exif.exifIfd['FocalLength']?.toDouble() ?? 0;
  final make = image.exif.imageIfd['Make']?.toString().trim() ?? '';
  final model = image.exif.imageIfd['Model']?.toString().trim() ?? '';
  final camera = [make, model].where((part) => part.isNotEmpty).join(' ');
  final lensText = lens <= 0
      ? '${focal35.toStringAsFixed(0)} mm equivalent'
      : '${lens.toStringAsFixed(2)} mm, ${focal35.toStringAsFixed(0)} mm equivalent';
  final who = camera.isEmpty ? lensText : '$camera, $lensText';
  final zoomText = zoom > 1 ? ', digital zoom ${zoom.toStringAsFixed(1)}×' : '';
  return (
    fov: fov,
    line: '$who$zoomText. Long side ${fov.toStringAsFixed(1)}°.'
  );
}

Future<String?> _pickFolder() async {
  const command = r'''
Add-Type -AssemblyName System.Windows.Forms
$dialog = New-Object System.Windows.Forms.FolderBrowserDialog
$dialog.Description = "Astrometry.net index files"
if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
  [Console]::Out.WriteLine($dialog.SelectedPath)
}
''';
  final result = await Process.run(
    'powershell',
    ['-NoProfile', '-STA', '-Command', command],
  );
  final path = (result.stdout as String).trim();
  if (path.isEmpty || !Directory(path).existsSync()) return null;
  return path;
}

Future<String?> _pickPhoto() async {
  const command = r'''
Add-Type -AssemblyName System.Windows.Forms
$dialog = New-Object System.Windows.Forms.OpenFileDialog
$dialog.Filter = "Photos (*.jpg;*.jpeg;*.png)|*.jpg;*.jpeg;*.png"
$dialog.Title = "Open a night photo"
if ($dialog.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
  [Console]::Out.WriteLine($dialog.FileName)
}
''';
  final result = await Process.run(
    'powershell',
    ['-NoProfile', '-STA', '-Command', command],
  );
  final path = (result.stdout as String).trim();
  if (path.isEmpty || !File(path).existsSync()) return null;
  return path;
}
