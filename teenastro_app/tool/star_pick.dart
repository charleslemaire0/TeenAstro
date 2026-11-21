// Click stars on a photo, then run the phone-finder plate solver.
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
      title: 'Star pick',
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

const _lensPref = 'star_pick_lens_distortion';

class StarPickPage extends StatefulWidget {
  const StarPickPage({super.key});

  @override
  State<StarPickPage> createState() => _StarPickPageState();
}

class _StarPickPageState extends State<StarPickPage> {
  final _fov = TextEditingController(text: '70');
  final _ra = TextEditingController();
  final _dec = TextEditingController();
  final _lat = TextEditingController();
  final _lst = TextEditingController();
  final _view = TransformationController();
  final _logScroll = ScrollController();

  List<NamedStar> _bright = const [];
  List<FinderStar> _catalog = const [];
  List<String> _catalogNames = const [];
  List<double> _catalogMags = const [];
  String? _selected;
  String? _photoPath;
  Uint8List? _jpeg;
  double _imageW = 0;
  double _imageH = 0;
  final _placed = <PlacedStar>[];
  final _notInCatalog = <_Pin>[];
  final _missing = <_Pin>[];
  final _matched = <_Pin>[];
  double? _lensK;
  final _log = <String>[
    'Choose a star name, then click it on the photo.',
  ];
  bool _busy = false;

  @override
  void initState() {
    super.initState();
    for (final controller in [_fov, _ra, _dec, _lat, _lst]) {
      controller.addListener(_saveStars);
    }
    _loadCatalog();
    _loadLens();
  }

  @override
  void dispose() {
    _fov.dispose();
    _ra.dispose();
    _dec.dispose();
    _lat.dispose();
    _lst.dispose();
    _view.dispose();
    _logScroll.dispose();
    super.dispose();
  }

  Future<void> _loadCatalog() async {
    final raw = await rootBundle.loadString('assets/catalogs/stars.json');
    final jd = julianDate(DateTime.now().toUtc());
    final json = jsonDecode(raw) as Map<String, dynamic>;
    final bright = <NamedStar>[];
    for (final item in json['objects'] as List<dynamic>) {
      final m = item as Map<String, dynamic>;
      final mag = (m['mag'] as num?)?.toDouble();
      final name = (m['name'] as String?)?.trim() ?? '';
      if (mag == null || mag > 1.5 || name.isEmpty) continue;
      final now = equatorialEquinoxToJNow(
        (m['ra'] as num).toDouble(),
        (m['dec'] as num).toDouble(),
        2000,
        jd,
      );
      bright.add(NamedStar(name, mag, now.$1, now.$2));
    }
    final faint = jsonDecode(await rootBundle.loadString(
      'assets/data/stars_mag9.json',
    )) as Map<String, dynamic>;
    final catalog = <FinderStar>[];
    final catalogNames = <String>[];
    final catalogMags = <double>[];
    for (final item in faint['stars'] as List<dynamic>) {
      final m = item as Map<String, dynamic>;
      final mag = (m['mag'] as num).toDouble();
      if (mag > wideCatalogMag) continue;
      final now = equatorialEquinoxToJNow(
        (m['ra'] as num).toDouble(),
        (m['dec'] as num).toDouble(),
        2000,
        jd,
      );
      final name = ((m['name'] as String?) ?? '').trim();
      catalog.add(FinderStar(now.$1, now.$2));
      catalogNames.add(name.isEmpty ? 'HIP ${m['hip']}' : name);
      catalogMags.add(mag);
    }
    bright.sort((a, b) => a.name.compareTo(b.name));
    if (!mounted) return;
    setState(() {
      _bright = bright;
      _catalog = catalog;
      _catalogNames = catalogNames;
      _catalogMags = catalogMags;
      _selected = bright.any((s) => s.name == 'Vega')
          ? 'Vega'
          : (bright.isEmpty ? null : bright.first.name);
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
      _jpeg = oriented.$1;
      _imageW = oriented.$2.toDouble();
      _imageH = oriented.$3.toDouble();
      _placed.clear();
      _notInCatalog.clear();
      _missing.clear();
      _matched.clear();
      _view.value = Matrix4.identity();
      _busy = false;
    });
    _loadStars(picked);
  }

  void _loadStars(String photoPath) {
    final side = _starsPath(photoPath);
    final file = File(side);
    if (!file.existsSync()) {
      _say('Choose a star name, then click it on the photo.');
      return;
    }
    try {
      final json = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
      final savedW = (json['width'] as num?)?.toDouble() ?? _imageW;
      final savedH = (json['height'] as num?)?.toDouble() ?? _imageH;
      final sx = savedW == 0 ? 1.0 : _imageW / savedW;
      final sy = savedH == 0 ? 1.0 : _imageH / savedH;
      final marks = json['stars'] as List<dynamic>? ?? const [];
      final known = {for (final s in _bright) s.name: s};
      final placed = <PlacedStar>[];
      for (final item in marks) {
        final m = item as Map<String, dynamic>;
        final name = m['name'] as String?;
        final star = name == null ? null : known[name];
        final x = (m['x'] as num).toDouble() * sx;
        final y = (m['y'] as num).toDouble() * sy;
        if (star != null && m['named'] != false) {
          placed.add(PlacedStar(
            name: star.name,
            mag: star.mag,
            raH: star.raH,
            decD: star.decD,
            x: x,
            y: y,
          ));
        } else {
          final used = placed.where((s) => !s.named).length + 1;
          final label = name == null || name.isEmpty || known.containsKey(name)
              ? '$used'
              : name;
          placed.add(PlacedStar(
            name: label,
            mag: 0,
            raH: 0,
            decD: 0,
            x: x,
            y: y,
            named: false,
          ));
        }
      }
      setState(() {
        _placed
          ..clear()
          ..addAll(placed);
        _text(_lat, json['lat']);
        _text(_lst, json['lst']);
        if (placed.length < 2) {
          _text(_fov, json['fov']);
          _text(_ra, json['ra']);
          _text(_dec, json['dec']);
        }
      });
      _say(
          'Restored ${placed.length} stars from ${file.uri.pathSegments.last}.');
      _estimateField();
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
      'ra': _ra.text,
      'dec': _dec.text,
      'lat': _lat.text,
      'lst': _lst.text,
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

  int _nextUnnamed() {
    var n = 0;
    for (final star in _placed) {
      if (star.named) continue;
      final value = int.tryParse(star.name) ?? 0;
      if (value > n) n = value;
    }
    return n + 1;
  }

  NamedStar? _named(String? name) {
    for (final star in _bright) {
      if (star.name == name) return star;
    }
    return null;
  }

  void _estimateField() {
    final named = [
      for (final s in _placed)
        if (s.named) s
    ];
    if (named.length < 2 || _imageW < 2) return;
    final fov = longSideFieldFromStars(
      stars: [
        for (final s in named) (raH: s.raH, decD: s.decD, x: s.x, y: s.y),
      ],
      width: _imageW,
      height: _imageH,
    );
    if (fov == null) {
      _say('These stars are too close to estimate a field.');
      return;
    }
    final long = math.max(_imageW, _imageH);
    final lines = <String>[];
    for (var i = 0; i < named.length; i++) {
      for (var j = i + 1; j < named.length; j++) {
        final a = named[i];
        final b = named[j];
        final pixels =
            math.sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
        final sky = sphereSeparationDeg(a.raH, a.decD, b.raH, b.decD);
        if (pixels < 30 || sky < 0.5) continue;
        final pair = sky / pixels * long;
        lines.add(
            '${a.name} to ${b.name}: ${sky.toStringAsFixed(1)}° over ${pixels.toStringAsFixed(0)} px, field ${pair.toStringAsFixed(1)}°');
      }
    }
    _fov.text = fov.toStringAsFixed(1);
    final center = named.length >= 3 ? null : _meanSky(named);
    if (center != null) {
      _ra.text = _clockHours(center.$1);
      _dec.text = _signedDegrees(center.$2);
    }
    final pose = named.length < 3 ? null : _poseSentence(named, fov);
    setState(() {
      _log.addAll(lines);
      if (pose != null) {
        _log.add(pose);
      } else {
        _log.add(
            'Long side ${fov.toStringAsFixed(1)}°. Search center ${_ra.text}, ${_dec.text}. A third named star gives the pose.');
      }
    });
    _scrollLog();
  }

  String? _poseSentence(List<PlacedStar> named, double fov) {
    final camera = widePoseFromNamedStars(
      stars: [
        for (final s in named) (raH: s.raH, decD: s.decD, x: s.x, y: s.y),
      ],
      width: _imageW,
      height: _imageH,
      fovDeg: fov,
      distortion: _lensK,
    );
    if (camera == null) {
      return 'The named stars do not form one camera pose.';
    }
    final pose = camera.solution;
    _fov.text = camera.fovDeg.toStringAsFixed(1);
    final poseRa = _clockHours(pose.raH);
    final poseDec = _signedDegrees(pose.decD);
    var roll = pose.rollRad * 180 / math.pi;
    roll %= 360;
    if (roll < 0) roll += 360;
    final residual = (pose.rmsDeg * 60).toStringAsFixed(1);
    var compared = 'residual $residual\'.';
    if (_lensK != null) {
      final plain = widePoseFromNamedStars(
        stars: [
          for (final s in named) (raH: s.raH, decD: s.decD, x: s.x, y: s.y),
        ],
        width: _imageW,
        height: _imageH,
        fovDeg: fov,
        distortion: 0,
      );
      final without = plain == null
          ? 'the frame does not close'
          : '${(plain.solution.rmsDeg * 60).toStringAsFixed(1)}\'';
      compared =
          'residual $residual\' with the stored lens, $without without it.';
    }
    return 'Pose $poseRa, $poseDec, roll ${roll.toStringAsFixed(1)}°, '
        '$compared '
        'Long side ${camera.fovDeg.toStringAsFixed(1)}°.';
  }

  (double, double) _meanSky(List<PlacedStar> stars) {
    var east = 0.0;
    var north = 0.0;
    var dec = 0.0;
    for (final s in stars) {
      final angle = s.raH * math.pi / 12;
      east += math.cos(angle);
      north += math.sin(angle);
      dec += s.decD;
    }
    return (
      wrapHours(math.atan2(north, east) * 12 / math.pi),
      dec / stars.length
    );
  }

  Future<void> _loadLens() async {
    final prefs = await SharedPreferences.getInstance();
    final value = prefs.getDouble(_lensPref);
    if (!mounted) return;
    setState(() => _lensK = value);
  }

  Future<void> _calibrateLens() async {
    if (_jpeg == null) {
      _say('Open a photo first.');
      return;
    }
    final named = [for (final s in _placed) if (s.named) s];
    if (named.length < 3) {
      _say('Place 3 named stars before calibrating the lens.');
      return;
    }
    final fov = double.tryParse(_fov.text.trim().replaceAll(',', '.')) ?? 70;
    final camera = widePoseFromNamedStars(
      stars: [
        for (final s in named) (raH: s.raH, decD: s.decD, x: s.x, y: s.y),
      ],
      width: _imageW,
      height: _imageH,
      fovDeg: fov,
      fitDistortion: true,
    );
    if (camera == null) {
      _say('The named stars do not agree, so the lens was not calibrated.');
      return;
    }
    final plain = widePoseFromNamedStars(
      stars: [
        for (final s in named) (raH: s.raH, decD: s.decD, x: s.x, y: s.y),
      ],
      width: _imageW,
      height: _imageH,
      fovDeg: fov,
      distortion: 0,
    );
    final prefs = await SharedPreferences.getInstance();
    await prefs.setDouble(_lensPref, camera.distortion);
    if (!mounted) return;
    setState(() => _lensK = camera.distortion);
    final withLens = (camera.solution.rmsDeg * 60).toStringAsFixed(1);
    final without = plain == null
        ? 'the frame does not close'
        : '${(plain.solution.rmsDeg * 60).toStringAsFixed(1)}\'';
    _say(
      'Lens distortion ${camera.distortion.toStringAsFixed(3)} stored. '
      'Named-star residual $withLens\' with this lens, $without without it.',
    );
  }

  Future<void> _clearLens() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove(_lensPref);
    if (!mounted) return;
    setState(() => _lensK = null);
    _say('Lens calibration cleared. The next solve uses no radial distortion.');
  }

  Future<void> _openParameters() async {
    await showDialog<void>(
      context: context,
      builder: (context) {
        return StatefulBuilder(
          builder: (context, setDialog) {
            final k = _lensK;
            return AlertDialog(
              backgroundColor: const Color(0xFF161B22),
              title: const Text('Parameters'),
              content: SizedBox(
                width: 360,
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Text(
                      k == null
                          ? 'No lens calibration is stored. Solves use no radial distortion.'
                          : 'Radial distortion k = ${k.toStringAsFixed(3)}. Solves reuse this value, and the residual is lower than with no distortion.',
                      style: const TextStyle(color: Color(0xFFF0F6FC)),
                    ),
                    const SizedBox(height: 16),
                    FilledButton(
                      onPressed: () async {
                        await _calibrateLens();
                        setDialog(() {});
                      },
                      child: const Text('Calibrate lens'),
                    ),
                    const SizedBox(height: 8),
                    OutlinedButton(
                      onPressed: k == null
                          ? null
                          : () async {
                              await _clearLens();
                              setDialog(() {});
                            },
                      child: const Text('Clear lens calibration'),
                    ),
                  ],
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
  }

  void _say(String line) {
    setState(() => _log.add(line));
    _scrollLog();
  }

  void _scrollLog() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!_logScroll.hasClients) return;
      _logScroll.jumpTo(_logScroll.position.maxScrollExtent);
    });
  }

  Future<void> _solve() async {
    if (_jpeg == null) {
      _say('Open a photo first.');
      return;
    }
    final namedCount = _placed.where((s) => s.named).length;
    if (namedCount < 3 && _placed.length < 4) {
      _say('Place 3 named stars for a pose, or mark at least 4 stars.');
      return;
    }
    final fov = double.tryParse(_fov.text.trim().replaceAll(',', '.'));
    if (fov == null || fov <= 0) {
      _say('Enter the field of the long side, in degrees.');
      return;
    }
    if (namedCount >= 3) {
      final sentence = _poseSentence(
        [
          for (final s in _placed)
            if (s.named) s
        ],
        fov,
      );
      if (sentence != null) _say(sentence);
    }
    final hintRa = parseMountHours(_ra.text.trim());
    final hintDec = parseMountDeg(_dec.text.trim());
    if (_placed.length < 4) return;
    if (hintRa == null || hintDec == null) {
      _say('Enter the right ascension and declination to search around.');
      return;
    }
    final lat = _lat.text.trim().isEmpty
        ? null
        : double.tryParse(_lat.text.trim().replaceAll(',', '.'));
    final lst =
        _lst.text.trim().isEmpty ? null : parseMountHours(_lst.text.trim());
    if ((lat == null) != (lst == null)) {
      _say(
          'Latitude and sidereal time are used together. One of them is missing, so the horizon check is skipped.');
    }
    if (_catalog.isEmpty) {
      _say('The star catalog is not loaded.');
      return;
    }
    _saveStars();
    setState(() {
      _busy = true;
      _notInCatalog.clear();
      _missing.clear();
      _matched.clear();
      _log.add('Solving ${_placed.length} stars…');
      for (final s in _placed) {
        _log.add(
            '  ${s.named ? s.name : 'Unnamed ${s.name}'}  x ${s.x.toStringAsFixed(0)}  y ${s.y.toStringAsFixed(0)}');
      }
    });
    _scrollLog();
    await Future<void>.delayed(Duration.zero);
    try {
      final result = await compute(runPickedSolve, <String, Object?>{
        'xs': [for (final s in _placed) s.x],
        'ys': [for (final s in _placed) s.y],
        'markNames': [for (final s in _placed) s.name],
        'markNamed': [for (final s in _placed) s.named],
        'markRa': [for (final s in _placed) s.raH],
        'markDec': [for (final s in _placed) s.decD],
        'ra': [for (final s in _catalog) s.raH],
        'dec': [for (final s in _catalog) s.decD],
        'catalogNames': _catalogNames,
        'catalogMags': _catalogMags,
        'hintRa': hintRa,
        'hintDec': hintDec,
        'width': _imageW,
        'height': _imageH,
        'fov': fov,
        'distortion': _lensK,
        'lat': lat,
        'lst': lst,
      });
      final lines = [
        for (final line in result['trace'] as List) line.toString(),
      ];
      if (!mounted) return;
      final solvedRa = result['raH'];
      final solvedDec = result['decD'];
      final raText = solvedRa is num ? _clockHours(solvedRa.toDouble()) : null;
      final decText =
          solvedDec is num ? _signedDegrees(solvedDec.toDouble()) : null;
      if (raText != null && decText != null) {
        _ra.text = raText;
        _dec.text = decText;
        _saveStars();
      }
      setState(() {
        _busy = false;
        _notInCatalog
          ..clear()
          ..addAll(_pins(result['notInCatalog']));
        _missing
          ..clear()
          ..addAll(_pins(result['missing']));
        _matched
          ..clear()
          ..addAll(_pins(result['matched']));
        _log.addAll(lines);
        if (raText != null && decText != null) {
          _log.add('Search center $raText, $decText.');
        }
      });
    } catch (error) {
      if (!mounted) return;
      setState(() {
        _busy = false;
        _log.add('The solve stopped: $error');
      });
    }
    _scrollLog();
  }

  void _onTap(Offset local, double viewW, double viewH) {
    if (_jpeg == null || viewW < 2 || viewH < 2) return;
    final image = Offset(
      local.dx / viewW * _imageW,
      local.dy / viewH * _imageH,
    );
    if (image.dx < 0 ||
        image.dy < 0 ||
        image.dx > _imageW ||
        image.dy > _imageH) {
      return;
    }
    final unnamed = _selected == 'Unnamed';
    final chosen = unnamed ? null : _named(_selected);
    final hit = 16 * _imageW / viewW;
    var nearest = -1;
    var best = hit * hit;
    for (var i = 0; i < _placed.length; i++) {
      final d = (Offset(_placed[i].x, _placed[i].y) - image).distanceSquared;
      if (d <= best) {
        best = d;
        nearest = i;
      }
    }
    setState(() {
      _notInCatalog.clear();
      _missing.clear();
      _matched.clear();
      if (nearest >= 0) {
        final gone = _placed.removeAt(nearest);
        final label = gone.named ? gone.name : 'Unnamed ${gone.name}';
        _log.add('$label removed. ${_placed.length} stars placed.');
      } else if (unnamed) {
        final n = _nextUnnamed();
        _placed.add(PlacedStar(
          name: '$n',
          mag: 0,
          raH: 0,
          decD: 0,
          x: image.dx,
          y: image.dy,
          named: false,
        ));
        _log.add('Unnamed star $n placed. ${_placed.length} stars placed.');
      } else if (chosen == null) {
        _log.add('Choose a star name, or Unnamed.');
      } else {
        _placed.removeWhere((s) => s.named && s.name == chosen.name);
        _placed.add(PlacedStar(
          name: chosen.name,
          mag: chosen.mag,
          raH: chosen.raH,
          decD: chosen.decD,
          x: image.dx,
          y: image.dy,
        ));
        _log.add('${chosen.name} placed. ${_placed.length} stars placed.');
      }
    });
    _estimateField();
    _saveStars();
    _scrollLog();
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
      child: GestureDetector(
        onTapUp: (d) => _onTap(d.localPosition, viewW, viewH),
        child: SizedBox(
          width: viewW,
          height: viewH,
          child: Stack(
            children: [
              Image.memory(jpeg, width: viewW, height: viewH, fit: BoxFit.fill),
              for (final star in _missing) _skyMark(star, viewW, viewH),
              for (var i = 0; i < _placed.length; i++) _mark(i, viewW, viewH),
            ],
          ),
        ),
      ),
    );
  }

  String? _alignedName(PlacedStar star) {
    for (final pin in _matched) {
      if ((pin.x - star.x).abs() < 1 && (pin.y - star.y).abs() < 1) {
        return pin.name;
      }
    }
    return null;
  }

  bool _missedCatalog(PlacedStar star) {
    for (final pin in _notInCatalog) {
      if ((pin.x - star.x).abs() < 1 && (pin.y - star.y).abs() < 1) {
        return true;
      }
    }
    return false;
  }

  Widget _mark(int index, double viewW, double viewH) {
    final star = _placed[index];
    final missed = _missedCatalog(star);
    final aligned = _alignedName(star);
    final color = missed
        ? const Color(0xFFFF5A5A)
        : aligned != null
            ? const Color(0xFF3DDC84)
            : const Color(0xFFF0C040);
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
                border: Border.all(color: color, width: 1.5),
              ),
              child: star.named
                  ? null
                  : Text(
                      star.name,
                      style: TextStyle(color: color, fontSize: 9),
                    ),
            ),
            if (star.named || missed || aligned != null) const SizedBox(width: 4),
            if (star.named || missed || aligned != null)
              DecoratedBox(
                decoration: BoxDecoration(
                  color: const Color(0xCC0D1117),
                  borderRadius: BorderRadius.circular(3),
                ),
                child: Padding(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 4, vertical: 1),
                  child: Text(
                    missed
                        ? '${star.named ? star.name : 'Unnamed ${star.name}'}  not in catalog'
                        : (aligned ?? star.name),
                    style: TextStyle(color: color, fontSize: 12),
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }

  Widget _skyMark(_Pin star, double viewW, double viewH) {
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
              decoration: BoxDecoration(
                shape: BoxShape.circle,
                border: Border.all(color: const Color(0xFF6CB6FF), width: 1.5),
              ),
            ),
            const SizedBox(width: 4),
            DecoratedBox(
              decoration: BoxDecoration(
                color: const Color(0xCC0D1117),
                borderRadius: BorderRadius.circular(3),
              ),
              child: Padding(
                padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 1),
                child: Text(
                  star.name,
                  style: const TextStyle(color: Color(0xFF6CB6FF), fontSize: 12),
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
              'Star pick',
              style: TextStyle(fontSize: 18, color: Color(0xFFF0F6FC)),
            ),
            const SizedBox(height: 8),
            FilledButton(
                onPressed: _busy ? null : _open,
                child: const Text('Open photo')),
            Row(
              children: [
                TextButton(
                  onPressed: _placed.isEmpty
                      ? null
                      : () {
                          setState(() {
                            _notInCatalog.clear();
                            _missing.clear();
                            _matched.clear();
                            final gone = _placed.removeLast();
                            final label =
                                gone.named ? gone.name : 'Unnamed ${gone.name}';
                            _log.add('$label removed.');
                          });
                          _estimateField();
                          _saveStars();
                        },
                  child: const Text('Undo'),
                ),
                TextButton(
                  onPressed: _placed.isEmpty
                      ? null
                      : () {
                          setState(() {
                            _placed.clear();
                            _notInCatalog.clear();
                            _missing.clear();
                            _matched.clear();
                            _log.add('Marks cleared.');
                          });
                          _saveStars();
                        },
                  child: const Text('Clear'),
                ),
              ],
            ),
            const SizedBox(height: 8),
            DropdownButton<String>(
              value: _selected == 'Unnamed' ||
                      _bright.any((s) => s.name == _selected)
                  ? _selected
                  : null,
              isExpanded: true,
              hint: const Text('Star to place'),
              dropdownColor: const Color(0xFF161B22),
              style: const TextStyle(color: Color(0xFFF0F6FC), fontSize: 14),
              items: [
                const DropdownMenuItem(
                  value: 'Unnamed',
                  child: Text('Unnamed'),
                ),
                for (final s in _bright)
                  DropdownMenuItem(
                    value: s.name,
                    child: Text('${s.name}   ${s.mag.toStringAsFixed(1)}'),
                  ),
              ],
              onChanged: (name) => setState(() => _selected = name),
            ),
            _field(_fov, 'Field of the long side (°)', '70'),
            _field(_ra, 'Search around right ascension', '18:36:56'),
            _field(_dec, 'Search around declination', '+38:47'),
            _field(_lat, 'Latitude, optional', '47.3'),
            _field(_lst, 'Sidereal time, optional', '22:34:45'),
            const SizedBox(height: 8),
            FilledButton(
              onPressed: _busy ? null : _solve,
              child: Text(_busy ? 'Solving…' : 'Solve'),
            ),
            const SizedBox(height: 6),
            OutlinedButton(
              onPressed: _busy ? null : _openParameters,
              child: const Text('Parameters'),
            ),
            const SizedBox(height: 6),
            const Text(
              'Green marks matched a catalog star. Red marks were clicked and are absent from the catalog. Blue marks are catalog stars in the frame that were not clicked.',
              style: TextStyle(color: Color(0xFF8B949E), fontSize: 12),
            ),
            const SizedBox(height: 10),
            const Text(
              'Solver',
              style: TextStyle(color: Color(0xFFF0F6FC)),
            ),
            const SizedBox(height: 6),
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

class _Pin {
  final String name;
  final double x;
  final double y;
  const _Pin(this.name, this.x, this.y);
}

List<_Pin> _pins(Object? raw) {
  if (raw is! List) return const [];
  return [
    for (final item in raw)
      if (item is Map)
        _Pin(
          (item['name'] ?? '').toString(),
          (item['x'] as num).toDouble(),
          (item['y'] as num).toDouble(),
        ),
  ];
}

class NamedStar {
  final String name;
  final double mag;
  final double raH;
  final double decD;
  const NamedStar(this.name, this.mag, this.raH, this.decD);
}

class PlacedStar {
  final String name;
  final double mag;
  final double raH;
  final double decD;
  final double x;
  final double y;
  final bool named;
  const PlacedStar({
    required this.name,
    required this.mag,
    required this.raH,
    required this.decD,
    required this.x,
    required this.y,
    this.named = true,
  });
}

String _starsPath(String photoPath) {
  final slash =
      math.max(photoPath.lastIndexOf('\\'), photoPath.lastIndexOf('/'));
  final dot = photoPath.lastIndexOf('.');
  final base = dot > slash ? photoPath.substring(0, dot) : photoPath;
  return '$base.stars.json';
}

Map<String, Object?> runPickedSolve(Map<String, Object?> raw) {
  double numOf(Object? value) => (value as num).toDouble();
  double? opt(Object? value) => value == null ? null : numOf(value);
  List<double> nums(Object? value) =>
      [for (final item in value as List) (item as num).toDouble()];
  List<String> words(Object? value) =>
      [for (final item in value as List) item.toString()];
  final xs = nums(raw['xs']);
  final ys = nums(raw['ys']);
  final ras = nums(raw['ra']);
  final decs = nums(raw['dec']);
  final trace = <String>[];
  final lat = opt(raw['lat']);
  final lst = opt(raw['lst']);
  final markNamed = raw['markNamed'];
  final namedCount = markNamed is List
      ? markNamed.where((item) => item == true).length
      : 0;
  if (namedCount >= 3) {
    final markNames = words(raw['markNames']);
    final markRa = nums(raw['markRa']);
    final markDec = nums(raw['markDec']);
    final report = solveWideField(
      marks: [
        for (var i = 0; i < xs.length; i++)
          (
            name: markNames[i],
            named: markNamed is List && markNamed[i] == true,
            raH: markRa[i],
            decD: markDec[i],
            x: xs[i],
            y: ys[i],
          ),
      ],
      catalog: [
        for (var i = 0; i < ras.length; i++) FinderStar(ras[i], decs[i]),
      ],
      catalogNames: words(raw['catalogNames']),
      catalogMags: nums(raw['catalogMags']),
      width: numOf(raw['width']),
      height: numOf(raw['height']),
      fovDeg: numOf(raw['fov']),
      distortion: opt(raw['distortion']),
      trace: trace,
    );
    final solution = report.solution;
    if (solution != null && xs.length >= 4) {
      var namedRoll = solution.rollRad * 180 / math.pi;
      namedRoll %= 360;
      if (namedRoll < 0) namedRoll += 360;
      trace.add(
        'Named stars stay on the picture and are ignored for the roll check. '
        'The search uses the solved position. '
        'Named roll ${namedRoll.toStringAsFixed(1)}°.',
      );
      final rollCheck = solveField(
        stars: [
          for (var i = 0; i < xs.length; i++) PixelStar(xs[i], ys[i]),
        ],
        catalog: [
          for (var i = 0; i < ras.length; i++) FinderStar(ras[i], decs[i]),
        ],
        hintRa: solution.raH,
        hintDec: solution.decD,
        width: numOf(raw['width']),
        height: numOf(raw['height']),
        fovDeg: numOf(raw['fov']),
        latDeg: lat != null && lst != null ? lat : null,
        lstH: lat != null && lst != null ? lst : null,
        trace: trace,
      );
      final found = rollCheck.solution;
      if (found == null) {
        trace.add(
          rollCheck.reason ??
              'The unnamed stars did not recover a roll around the solved position.',
        );
      } else {
        var roll = found.rollRad * 180 / math.pi;
        roll %= 360;
        if (roll < 0) roll += 360;
        var delta = roll - namedRoll;
        if (delta > 180) delta -= 360;
        if (delta < -180) delta += 360;
        trace.add(
          'Unnamed roll ${roll.toStringAsFixed(1)}°, '
          '${delta.toStringAsFixed(1)}° from the named roll. '
          '${found.matches} stars, residual ${(found.rmsDeg * 60).toStringAsFixed(1)}\'.',
        );
      }
    }
    return {
      'trace': trace,
      if (solution != null) 'raH': solution.raH,
      if (solution != null) 'decD': solution.decD,
      'notInCatalog': [
        for (final s in report.notInCatalog)
          {'name': s.label, 'x': s.x, 'y': s.y},
      ],
      'missing': [
        for (final s in report.missing) {'name': s.name, 'x': s.x, 'y': s.y},
      ],
      'matched': [
        for (final s in report.matched) {'name': s.name, 'x': s.x, 'y': s.y},
      ],
    };
  }
  final catalog = [
    for (var i = 0; i < ras.length; i++) FinderStar(ras[i], decs[i]),
  ];
  final pixels = [
    for (var i = 0; i < xs.length; i++) PixelStar(xs[i], ys[i]),
  ];
  final fov = numOf(raw['fov']);
  final width = numOf(raw['width']);
  final height = numOf(raw['height']);
  final identified = <({double raH, double decD, double x, double y})>[];
  var solved = solveField(
    stars: pixels,
    catalog: catalog,
    hintRa: numOf(raw['hintRa']),
    hintDec: numOf(raw['hintDec']),
    width: width,
    height: height,
    fovDeg: fov,
    latDeg: lat != null && lst != null ? lat : null,
    lstH: lat != null && lst != null ? lst : null,
    trace: trace,
    identified: identified,
  );
  if (solved.solution != null) {
    trace.add(
      'Solving again around the first solution. '
      'Search radius stays ${(fov * 0.65).toStringAsFixed(1)}°.',
    );
    final second = <({double raH, double decD, double x, double y})>[];
    final again = solveField(
      stars: pixels,
      catalog: catalog,
      hintRa: solved.solution!.raH,
      hintDec: solved.solution!.decD,
      width: width,
      height: height,
      fovDeg: fov,
      latDeg: lat != null && lst != null ? lat : null,
      lstH: lat != null && lst != null ? lst : null,
      trace: trace,
      identified: second,
    );
    if (again.solution != null) {
      solved = again;
      identified
        ..clear()
        ..addAll(second);
    }
  }
  if (identified.length >= 3) {
    final known = <int>{};
    for (final star in identified) {
      for (var i = 0; i < xs.length; i++) {
        if ((xs[i] - star.x).abs() < 0.5 && (ys[i] - star.y).abs() < 0.5) {
          known.add(i);
        }
      }
    }
    final marks =
        <({String name, bool named, double raH, double decD, double x, double y})>[];
    for (var i = 0; i < xs.length; i++) {
      ({double raH, double decD, double x, double y})? hit;
      if (known.contains(i)) {
        hit = identified.firstWhere(
          (s) => (s.x - xs[i]).abs() < 0.5 && (s.y - ys[i]).abs() < 0.5,
        );
      } else {
        hit = null;
      }
      marks.add((
        name: hit == null ? '$i' : 'matched',
        named: hit != null,
        raH: hit?.raH ?? 0,
        decD: hit?.decD ?? 0,
        x: xs[i],
        y: ys[i],
      ));
    }
    final report = solveWideField(
      marks: marks,
      catalog: catalog,
      catalogNames: words(raw['catalogNames']),
      catalogMags: nums(raw['catalogMags']),
      width: width,
      height: height,
      fovDeg: fov,
      distortion: opt(raw['distortion']),
      trace: trace,
    );
    final solution = report.solution ?? solved.solution;
    return {
      'trace': trace,
      if (solution != null) 'raH': solution.raH,
      if (solution != null) 'decD': solution.decD,
      'notInCatalog': [
        for (final s in report.notInCatalog)
          {'name': s.label, 'x': s.x, 'y': s.y},
      ],
      'missing': [
        for (final s in report.missing) {'name': s.name, 'x': s.x, 'y': s.y},
      ],
      'matched': [
        for (final s in report.matched) {'name': s.name, 'x': s.x, 'y': s.y},
      ],
    };
  }
  final solution = solved.solution;
  return {
    'trace': trace,
    if (solution != null) 'raH': solution.raH,
    if (solution != null) 'decD': solution.decD,
  };
}

/// Upright JPEG plus its pixel size. Coordinates match the picture on screen.
(Uint8List, int, int)? _orientPhoto(Uint8List bytes) {
  final decoded = img.decodeImage(bytes);
  if (decoded == null) return null;
  final frame = img.bakeOrientation(decoded);
  return (
    Uint8List.fromList(img.encodeJpg(frame, quality: 85)),
    frame.width,
    frame.height,
  );
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
