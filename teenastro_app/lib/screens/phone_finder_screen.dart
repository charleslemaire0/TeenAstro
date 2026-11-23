import 'dart:async';
import 'dart:io';
import 'dart:isolate';
import 'dart:math' as math;
import 'dart:typed_data';
import 'dart:ui' as ui;

import 'package:flutter/foundation.dart';
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import 'package:teenastro_app/models/catalog_entry.dart';
import 'package:teenastro_app/models/equinox_precession.dart';
import 'package:teenastro_app/models/lx200_commands.dart';
import 'package:teenastro_app/models/mount_config.dart';
import 'package:teenastro_app/models/mount_state.dart';
import 'package:teenastro_app/models/planet_positions.dart';
import 'package:teenastro_app/services/finder_sky.dart';
import 'package:teenastro_app/services/lx200_tcp_client.dart';
import 'package:teenastro_app/services/mount_state_provider.dart';
import 'package:teenastro_app/services/phone_finder.dart';
import 'package:teenastro_app/services/phone_finder_store.dart';
import 'package:teenastro_app/services/stellar_solver.dart';
import 'package:teenastro_app/services/tetra_solver.dart';
import 'package:teenastro_app/providers/night_view_provider.dart';
import 'package:teenastro_app/theme.dart';
import 'package:teenastro_app/widgets/push_to_card.dart';

/// Plate solve from the phone camera. The hardware efinder is not used.
class PhoneFinderScreen extends ConsumerStatefulWidget {
  const PhoneFinderScreen({super.key});

  @override
  ConsumerState<PhoneFinderScreen> createState() => _PhoneFinderScreenState();
}

class _PhoneFinderScreenState extends ConsumerState<PhoneFinderScreen> {
  final _store = PhoneFinderStore();
  static const _cameraChannel = MethodChannel('com.teenastro/camera');
  FinderCones _cones = const FinderCones();
  double _fov = 60;
  bool _auto = false;
  int _limitArcmin = 2;
  bool _recenterNext = false;
  bool _busy = false;
  bool _aligning = false;
  bool _stopAlign = false;
  String _status = '';
  String? _cameraError;
  bool _fovMeasured = false;
  List<StellarIndex>? _indexes;
  PhoneSolver _solver = PhoneSolver.tetra;
  TetraSolver? _tetra;
  String? _cameraId;
  /// Main lens at 1×; centre stars replace digital zoom for solving.
  static const double _zoom = 1.0;
  int _shutterUs = PhoneFinderStore.exposureDefaultUs;
  int _iso = PhoneFinderStore.isoDefault;
  String _exposureNote = '';
  /// Back-camera altitude from gravity at the last capture, or null.
  double? _phoneAltDeg;
  bool _live = false;
  int _liveSeconds = PhoneFinderStore.liveDefaultSeconds;
  DateTime? _liveUntil;
  int _sensorDeg = 90;
  bool _previewOn = false;
  ui.Image? _shotImage;
  Uint8List? _shotRgba;
  int _shotW = 0;
  int _shotH = 0;
  double _gamma = PhoneFinderStore.gammaDefault;
  StarFrame? _shotFrame;
  String _shotNote = '';
  bool _showPhoto = true;

  bool get _hasCamera => !kIsWeb && Platform.isAndroid;
  bool get _previewLive => _previewOn && _cameraId != null;

  @override
  void initState() {
    super.initState();
    _load();
  }

  Future<void> _load() async {
    final cones = await _store.loadCones();
    final fov = await _store.loadFov();
    final auto = await _store.loadAutoSync();
    final limit = await _store.loadRecenterArcmin();
    final solver = await _store.loadSolver();
    final liveSeconds = await _store.loadLiveSeconds();
    final gamma = await _store.loadGamma();
    final shutterUs = await _store.loadExposureUs();
    final iso = await _store.loadIso();
    if (!mounted) return;
    setState(() {
      _liveSeconds = liveSeconds;
      _gamma = gamma;
      _shutterUs = shutterUs;
      _iso = iso;
      _cones = cones;
      _fov = fov;
      _auto = auto;
      _limitArcmin = limit;
      _solver = solver;
    });
    if (_hasCamera) await _openCamera();
  }

  /// Longer-side field of the camera, from its focal length and sensor size.
  Future<double?> _lensField(String cameraId) async {
    try {
      final value = await _cameraChannel.invokeMethod<num>(
          'longerSideFov', {'cameraId': cameraId, 'zoom': _zoom});
      final deg = value?.toDouble();
      if (deg == null || deg < 5 || deg > 170) return null;
      return deg;
    } catch (_) {
      return null;
    }
  }

  Future<void> _openCamera() async {
    try {
      if (mounted) {
        setState(() {
          _cameraError = null;
          _status = 'Asking for the camera…';
        });
      }
      final allowed =
          await _cameraChannel.invokeMethod<bool>('ensureCameraPermission');
      if (allowed != true) {
        if (mounted) {
          setState(() {
            _cameraError =
                'Camera permission is required. Allow the camera, then open Finder again.';
            _status = '';
          });
        }
        return;
      }
      final listed =
          await _cameraChannel.invokeMethod<List<dynamic>>('listBackCameras');
      if (listed == null || listed.isEmpty) {
        if (mounted) setState(() => _cameraError = 'No camera on this phone');
        return;
      }
      final back = Map<String, dynamic>.from(listed.first as Map);
      final id = back['id'] as String;
      final measured = await _lensField(id);
      if (!mounted) return;
      if (measured != null) await _store.saveFov(measured);
      setState(() {
        _cameraId = id;
        _previewOn = true;
        _sensorDeg = (back['sensorOrientation'] as num?)?.toInt() ?? 90;
        _cameraError = null;
        _status = '';
        if (measured != null) {
          _fov = measured;
          _fovMeasured = true;
        }
      });
    } on PlatformException catch (e) {
      if (!mounted) return;
      setState(() {
        _cameraError = e.code == 'permission'
            ? 'Camera permission is required. Allow the camera, then open Finder again.'
            : (e.message ?? 'Camera unavailable');
        _status = '';
      });
    } catch (_) {
      if (mounted) {
        setState(() {
          _cameraError = 'Camera unavailable';
          _status = '';
        });
      }
    }
  }

  Future<void> _stopPreview() async {
    if (mounted) {
      setState(() => _previewOn = false);
    } else {
      _previewOn = false;
    }
    // Let the TextureView PlatformView release the camera.
    await Future<void>.delayed(const Duration(milliseconds: 350));
  }

  @override
  void dispose() {
    _live = false;
    _stopAlign = true;
    _previewOn = false;
    _tetra?.close();
    _shotImage?.dispose();
    super.dispose();
  }

  String _liveLabel(int seconds) {
    if (seconds % 60 == 0) {
      final m = seconds ~/ 60;
      return m == 1 ? '1 min' : '$m min';
    }
    return '$seconds s';
  }

  /// Quarter-turns for still JPEGs (sensor buffer, no TextureView matrix).
  int _uprightTurns(BuildContext context) {
    final deviceDeg = MediaQuery.orientationOf(context) == Orientation.landscape
        ? 90
        : 0;
    return (((_sensorDeg - deviceDeg) + 360) % 360) ~/ 90;
  }

  /// Null when the axis is above the horizon, or when the site is unknown.
  String? _belowHorizon(double raH, double decD) {
    final state = ref.read(skyStateProvider);
    final lst = parseMountHours(state.siderealTime);
    if (lst == null) return null;
    final alt = altitudeDeg(state.latitude, lst, raH, decD);
    if (alt > 0) return null;
    return 'The solution is below the horizon (${alt.toStringAsFixed(0)}°)';
  }

  /// Null when tilt agrees, site/tilt is unknown, or the mismatch is small.
  String? _tiltMismatch(double raH, double decD) {
    final phoneAlt = _phoneAltDeg;
    if (phoneAlt == null) return null;
    final state = ref.read(skyStateProvider);
    final lst = parseMountHours(state.siderealTime);
    if (lst == null) return null;
    final skyAlt = altitudeDeg(state.latitude, lst, raH, decD);
    final miss = tiltMismatchDeg(phoneAlt, skyAlt);
    if (miss == null || miss <= tiltRejectDeg) return null;
    return 'Tilt rejects this solve '
        '(phone ${phoneAlt.toStringAsFixed(0)}°, '
        'sky ${skyAlt.toStringAsFixed(0)}°, '
        'Δ ${miss.toStringAsFixed(0)}°)';
  }

  Future<double?> _readPhoneAltitude() async {
    if (!_hasCamera) return null;
    try {
      final raw =
          await _cameraChannel.invokeMethod<dynamic>('phoneTilt');
      if (raw is num) return raw.toDouble();
    } catch (_) {}
    return null;
  }

  (double, double)? _mountPointing() => _pointingOf(ref.read(mountStateProvider));

  /// StellarSolver's search hint: the mount, or the last finder solve.
  (double, double)? _hintPointing() => _pointingOf(ref.read(skyStateProvider));

  (double, double)? _pointingOf(MountState state) {
    final ra = parseMountHours(state.ra);
    final dec = parseMountDeg(state.dec);
    if (ra == null || dec == null) return null;
    return (ra, dec);
  }

  Uint8List? _jpegBytes(Object? raw) {
    if (raw is Uint8List) return raw;
    if (raw is List) return Uint8List.fromList(raw.cast<int>());
    return null;
  }

  /// The camera picks the gain itself, so only the time is shown.
  String _shutterLabel(int us, int iso) => us >= 1000000
      ? '${(us / 1000000).toStringAsFixed(1)} s'
      : us >= 10000
          ? '${(us / 1000).toStringAsFixed(0)} ms'
          : '${(us / 1000).toStringAsFixed(1)} ms';

  /// Stars on the stacked 16-bit mean (ADU × 256), not the 8-bit JPEG.
  static Future<StarFrame?> _starsFromY16(
    Uint8List y16,
    int width,
    int height,
  ) =>
      Isolate.run(() => readStarFrameU16(y16, width, height, maxStars: 80));

  /// 16-bit grayscale PNG of the stack mean, for post-processing.
  static Future<Uint8List> _pngFromY16(
    Uint8List y16,
    int width,
    int height,
  ) =>
      Isolate.run(() => encodeGray16Png(y16, width, height));

  /// Show a captured photo with its extracted stars.
  Future<void> _showShot(Uint8List jpeg, StarFrame frame, String note) async {
    try {
      final codec = await ui.instantiateImageCodec(
        jpeg,
        targetWidth: math.min(frame.width.round(), 1600),
      );
      final decoded = (await codec.getNextFrame()).image;
      final data = await decoded.toByteData(format: ui.ImageByteFormat.rawRgba);
      _shotW = decoded.width;
      _shotH = decoded.height;
      decoded.dispose();
      if (data == null || !mounted) return;
      _shotRgba = data.buffer.asUint8List();
      _shotFrame = frame;
      _shotNote = note;
      _showPhoto = true;
      await _regammaShot();
    } catch (_) {}
  }

  /// Rebuild the displayed photo with gamma only (no histogram stretch).
  Future<void> _regammaShot() async {
    final src = _shotRgba;
    if (src == null) return;
    final lut = Uint8List(256);
    for (var i = 0; i < 256; i++) {
      lut[i] = (255 * math.pow(i / 255.0, 1 / _gamma)).round().clamp(0, 255);
    }
    final out = Uint8List(src.length);
    for (var i = 0; i < src.length; i += 4) {
      out[i] = lut[src[i]];
      out[i + 1] = lut[src[i + 1]];
      out[i + 2] = lut[src[i + 2]];
      out[i + 3] = 255;
    }
    final done = Completer<ui.Image>();
    ui.decodeImageFromPixels(
        out, _shotW, _shotH, ui.PixelFormat.rgba8888, done.complete);
    final image = await done.future;
    if (!mounted) {
      image.dispose();
      return;
    }
    final old = _shotImage;
    setState(() => _shotImage = image);
    old?.dispose();
  }

  void _setGamma(double g) {
    setState(() => _gamma = g);
    _cameraChannel.invokeMethod<void>('previewGamma', {'gamma': g});
  }

  Future<void> _gammaDone(double g) async {
    await _store.saveGamma(g);
    await _regammaShot();
  }

  /// One still at the user's shutter time and ISO, with a linear tone curve.
  /// The preview is released so the shutter time can be set directly.
  Future<StarFrame?> _captureForStars() async {
    final id = _cameraId;
    if (id == null) return null;
    _phoneAltDeg = await _readPhoneAltitude();
    await _stopPreview();
    await Future<void>.delayed(const Duration(milliseconds: 200));

    Map<dynamic, dynamic>? limits;
    for (var attempt = 0; attempt < 3 && limits == null; attempt++) {
      try {
        final raw = await _cameraChannel.invokeMethod<dynamic>(
          'manualOpen',
          {'cameraId': id, 'zoom': _zoom},
        );
        if (raw is Map) limits = raw;
      } catch (_) {
        await Future<void>.delayed(const Duration(milliseconds: 400));
      }
    }
    if (limits == null) {
      _exposureNote = '';
      if (mounted) {
        setState(() => _status = 'The shutter could not be set');
        await _openCamera();
      }
      return null;
    }

    final minUs = (limits['minUs'] as num).toInt();
    final maxUs = (limits['maxUs'] as num).toInt();
    final minIso = (limits['minIso'] as num).toInt();
    final maxIso = (limits['maxIso'] as num).toInt();
    final us = _shutterUs.clamp(minUs, maxUs);
    final iso = _iso.clamp(minIso, maxIso);

    StarFrame? best;
    var bestNote = '';
    try {
      if (mounted) {
        setState(() => _status = 'Collecting ${_shutterLabel(us, iso)} of sky…');
      }
      Uint8List? jpeg;
      Uint8List? y16;
      var stackW = 0;
      var stackH = 0;
      var delivered = us;
      var frames = 1;
      var aligned = 0;
      var shaken = 0;
      String? saved;
      try {
        final raw = await _cameraChannel.invokeMethod<dynamic>(
          'manualCapture',
          {'exposureUs': us, 'iso': iso},
        );
        if (raw is Map) {
          jpeg = _jpegBytes(raw['jpeg']);
          y16 = _jpegBytes(raw['y16']);
          final w = raw['width'];
          final h = raw['height'];
          if (w is num) stackW = w.toInt();
          if (h is num) stackH = h.toInt();
          final got = raw['exposureUs'];
          if (got is num && got > 0) delivered = got.toInt();
          final count = raw['frames'];
          if (count is num && count > 1) frames = count.toInt();
          final reg = raw['aligned'];
          if (reg is num) aligned = reg.toInt();
          final drop = raw['shaken'];
          if (drop is num) shaken = drop.toInt();
        } else {
          jpeg = _jpegBytes(raw);
        }
      } catch (_) {
        jpeg = null;
        y16 = null;
      }
      bestNote = frames > 1
          ? '$frames × ${_timeLabel(delivered ~/ frames)} = '
              '${_shutterLabel(delivered, iso)}, '
              '${aligned + 1}/$frames aligned'
          : _shutterLabel(delivered, iso);
      if (shaken > 0) bestNote = '$bestNote, $shaken shaky skipped';
      final detected = y16 != null && stackW > 0 && stackH > 0
          ? await _starsFromY16(y16, stackW, stackH)
          : null;
      final frame = detected == null
          ? null
          : StarFrame(
              starsNearestCenter(
                detected.stars,
                detected.width,
                detected.height,
                max: targetStarCount,
              ),
              detected.width,
              detected.height,
            );
      if (y16 != null && stackW > 0 && stackH > 0) {
        try {
          final png = await _pngFromY16(y16, stackW, stackH);
          final name = await _cameraChannel.invokeMethod<String>(
            'manualSave',
            {'bytes': png, 'mime': 'image/png', 'extension': 'png'},
          );
          if (name != null && name.isNotEmpty) saved = name;
        } catch (_) {}
      }
      if (saved != null) bestNote = '$bestNote\nPictures/TeenAstro/$saved';
      if (frame != null && jpeg != null) {
        best = frame;
        _showShot(jpeg, frame, bestNote);
        if (mounted) {
          setState(() =>
              _status = '${frame.stars.length} stars at $bestNote');
        }
      }
    } finally {
      try {
        await _cameraChannel.invokeMethod<void>('manualClose');
      } catch (_) {}
      if (mounted) await _openCamera();
    }
    _exposureNote = bestNote;
    return best;
  }

  Future<List<StellarIndex>?> _loadIndexes() async {
    final cached = _indexes;
    if (cached != null) return cached;
    try {
      final files = <String, Uint8List>{};
      for (final name in [
        'index-4117.fits',
        'index-4118.fits',
        'index-4119.fits'
      ]) {
        final data = await rootBundle.load('tool/stellar/indexes/$name');
        files[name] =
            data.buffer.asUint8List(data.offsetInBytes, data.lengthInBytes);
      }
      final parsed = parseStellarIndexes(files);
      if (parsed.isEmpty) return null;
      _indexes = parsed;
      return parsed;
    } catch (_) {
      return null;
    }
  }

  /// The C++ Tetra catalog, copied once from the app assets.
  Future<TetraSolver?> _loadTetra() async {
    final cached = _tetra;
    if (cached != null) return cached;
    try {
      final data = await rootBundle.load('assets/tetra/phone55.tetra');
      final bytes =
          data.buffer.asUint8List(data.offsetInBytes, data.lengthInBytes);
      final opened = await _openTetraBytes(bytes);
      if (opened == null) return null;
      final tetra = TetraSolver.attach(opened.$1, opened.$2);
      if (!mounted) {
        tetra.close();
        return null;
      }
      _tetra = tetra;
      return tetra;
    } catch (_) {
      return null;
    }
  }

  /// Static so the isolate closure holds only the catalog bytes.
  static Future<(int, double)?> _openTetraBytes(Uint8List bytes) =>
      Isolate.run(() {
        final tetra = TetraSolver.openBytes(bytes);
        return tetra == null ? null : (tetra.address, tetra.maxFovDeg);
      });

  /// Lost-in-space solve of the frame's stars. The result is J2000.
  Future<FinderSolution?> _solveTetra(TetraSolver tetra, StarFrame frame) =>
      _solveTetraOn(tetra, frame, _fov);

  static Future<FinderSolution?> _solveTetraOn(
      TetraSolver tetra, StarFrame frame, double fovLongDeg) {
    final address = tetra.address;
    final catalogFov = tetra.maxFovDeg;
    final stars = [for (final s in frame.stars) (x: s.x, y: s.y)];
    final width = frame.width;
    final height = frame.height;
    final longSide = math.max(width, height);
    final focal = (longSide / 2) / math.tan(fovLongDeg * math.pi / 360);
    final fovHorizontal = 2 * math.atan((width / 2) / focal) * 180 / math.pi;
    return Isolate.run(() {
      final result = TetraSolver.attach(address, catalogFov).solve(
        stars: stars,
        width: width,
        height: height,
        fovHorizontalDeg: fovHorizontal,
      );
      return result.solved ? result.toFinderSolution() : null;
    });
  }

  Future<FinderSolution?> _solve() async {
    if (_cameraId == null) {
      setState(() => _status = 'The phone camera is not ready');
      return null;
    }
    setState(() {
      _busy = true;
      _status = 'Capturing…';
    });
    try {
      final tetra = _solver == PhoneSolver.tetra ? await _loadTetra() : null;
      final indexes = _solver == PhoneSolver.stellar ? await _loadIndexes() : null;
      if (_solver == PhoneSolver.tetra && tetra == null) {
        setState(() => _status = 'The Tetra catalog is not in the app');
        return null;
      }
      if (_solver == PhoneSolver.stellar && indexes == null) {
        setState(
            () => _status = 'The StellarSolver indexes are not in the app');
        return null;
      }
      final frame = await _captureForStars();
      if (frame == null || frame.stars.length < 4) {
        final exposure = _exposureNote.isEmpty ? '' : ' at $_exposureNote';
        setState(() => _status = frame == null
            ? (_exposureNote.isEmpty
                ? 'The picture could not be read'
                : 'The shutter did not fire')
            : 'Not enough stars (${frame.stars.length})$exposure');
        return null;
      }
      final FinderSolution? solved;
      if (tetra != null) {
        if (mounted) setState(() => _status = 'Tetra…');
        solved = await _solveTetra(tetra, frame);
        if (solved == null) {
          if (mounted) {
            setState(() => _status = 'Tetra found no match in this photo');
          }
          return null;
        }
      } else {
        if (mounted) setState(() => _status = 'StellarSolver…');
        await Future<void>.delayed(Duration.zero);
        final pointing = _hintPointing();
        final hint = pointing == null
            ? null
            : equatorialJNowToEquinox(
                pointing.$1,
                pointing.$2,
                2000,
                julianDate(DateTime.now().toUtc()),
              );
        final outcome = solveStellarField(
          stars: frame.stars,
          indexes: indexes!,
          width: frame.width,
          height: frame.height,
          longSideDeg: _fov,
          hintRaH: hint?.$1,
          hintDecD: hint?.$2,
        );
        solved = outcome.solution;
        if (solved == null) {
          setState(() => _status =
              outcome.reason ?? 'StellarSolver found no match in this photo');
          return null;
        }
      }
      final now = equatorialEquinoxToJNow(
        solved.raH,
        solved.decD,
        2000,
        julianDate(DateTime.now().toUtc()),
      );
      return FinderSolution(
        raH: now.$1,
        decD: now.$2,
        rollRad: solved.rollRad,
        matches: solved.matches,
        rmsDeg: solved.rmsDeg,
      );
    } catch (_) {
      if (mounted) setState(() => _status = 'The picture could not be read');
      return null;
    }
  }

  Future<void> _onSolve({required bool sync, required bool aim}) async {
    if ((aim || sync) && !ref.read(lx200ClientProvider).isConnected) {
      setState(() => _status = 'Mount is not connected');
      return;
    }
    if (aim) {
      final go = await showDialog<bool>(
        context: context,
        builder: (ctx) => AlertDialog(
          title: const Text('Aim'),
          content: const Text(
            'Center a bright star in the telescope and sync. '
            'The next picture measures the Hz cone and the Ve cone.',
          ),
          actions: [
            TextButton(
                onPressed: () => Navigator.pop(ctx, false),
                child: const Text('Cancel')),
            TextButton(
                onPressed: () => Navigator.pop(ctx, true),
                child: const Text('Capture')),
          ],
        ),
      );
      if (go != true) return;
    }
    final solved = await _solve();
    if (!mounted) return;
    try {
      if (solved == null) return;
      var shown = solved;
      if (aim) {
        final here = _mountPointing();
        if (here == null) {
          setState(() => _status = 'The mount position is not available');
          return;
        }
        final cones = measureCones(
          telescopeRa: here.$1,
          telescopeDec: here.$2,
          camera: solved,
        );
        await _store.saveCones(cones);
        setState(() {
          _cones = cones;
          _status = 'Hz Cone ${formatArcmin(cones.hzRad)}   '
              'Ve Cone ${formatArcmin(cones.veRad)}';
        });
        return;
      }
      if (!_cones.isZero) {
        final axis = telescopeAxis(solved, _cones);
        shown = FinderSolution(
          raH: axis.$1,
          decD: axis.$2,
          rollRad: solved.rollRad,
          matches: solved.matches,
          rmsDeg: solved.rmsDeg,
        );
      }
      final horizon = _belowHorizon(shown.raH, shown.decD);
      if (horizon != null) {
        setState(() => _status = horizon);
        return;
      }
      final tilt = _tiltMismatch(shown.raH, shown.decD);
      if (tilt != null) {
        setState(() => _status = tilt);
        return;
      }
      final residual = (shown.rmsDeg * 60).toStringAsFixed(1);
      if (!sync) {
        ref.read(finderSkyProvider.notifier).publishSolve(shown.raH, shown.decD);
        setState(() =>
            _status = 'Solved ${shown.matches} stars, residual $residual\''
                '${_exposureNote.isEmpty ? '' : ', $_exposureNote'}. '
                '${_cones.isZero ? 'Camera axis' : 'Telescope axis'} '
                '${_fmt(shown.raH, shown.decD)}');
        return;
      }
      final err = await _syncMount(shown.raH, shown.decD);
      if (!mounted) return;
      setState(() => _status = err ??
          'Synced ${_fmt(shown.raH, shown.decD)}, residual $residual\''
              '${_exposureNote.isEmpty ? '' : ', $_exposureNote'}');
    } finally {
      if (mounted) setState(() => _busy = false);
    }
  }

  /// Solve again and again until [_liveSeconds] have passed or Live is turned off.
  Future<void> _runLive() async {
    if (!_hasCamera || _cameraId == null) {
      setState(() => _status = 'The phone camera is used on Android');
      return;
    }
    setState(() {
      _live = true;
      _liveUntil = DateTime.now().add(Duration(seconds: _liveSeconds));
    });
    try {
      while (mounted && _live && DateTime.now().isBefore(_liveUntil!)) {
        await _onSolve(sync: false, aim: false);
        if (!mounted || !_live) break;
        await Future.delayed(const Duration(seconds: 1));
      }
    } finally {
      final timedOut = _live;
      if (mounted) {
        setState(() {
          _live = false;
          _liveUntil = null;
          if (timedOut) {
            _status =
                'Live stopped after ${_liveLabel(_liveSeconds)}. $_status';
          }
        });
      }
    }
  }

  Future<String?> _syncMount(double raH, double decD) async {
    final client = ref.read(lx200ClientProvider);
    if (!client.isConnected) return 'Mount is not connected';
    final formatted = formatRaDecLx200(raH, decD);
    if (await client.sendCommand(LX200.setTargetRa(formatted.$1)) != '1') {
      return 'The mount rejected the coordinates';
    }
    if (await client.sendCommand(
            LX200.setTargetDec(formatted.$2.replaceFirst(':', '*'))) !=
        '1') {
      return 'The mount rejected the coordinates';
    }
    final reply = await client.sendCommand(LX200.syncTarget);
    if (reply == null) return 'Sync failed';
    await ref.read(mountStateProvider.notifier).refresh();
    return null;
  }

  Future<void> _afterSlew(double targetRa, double targetDec) async {
    if (_busy || !_auto) return;
    final secondPass = _recenterNext;
    _recenterNext = false;
    _busy = true;
    setState(
        () => _status = secondPass ? 'Syncing after recenter…' : 'Auto sync…');
    try {
      final solved = await _solve();
      if (!mounted || solved == null) return;
      var axisRa = solved.raH;
      var axisDec = solved.decD;
      if (!_cones.isZero) {
        final axis = telescopeAxis(solved, _cones);
        axisRa = axis.$1;
        axisDec = axis.$2;
      }
      final horizon = _belowHorizon(axisRa, axisDec);
      if (horizon != null) {
        setState(() => _status = horizon);
        return;
      }
      final tilt = _tiltMismatch(axisRa, axisDec);
      if (tilt != null) {
        setState(() => _status = tilt);
        return;
      }
      final err = await _syncMount(axisRa, axisDec);
      if (!mounted) return;
      if (err != null) {
        setState(() => _status = err);
        return;
      }
      final miss = separationArcmin(axisRa, axisDec, targetRa, targetDec);
      if (!secondPass && miss > _limitArcmin) {
        final slewErr = await _gotoTarget(targetRa, targetDec);
        if (!mounted) return;
        if (slewErr != null) {
          setState(() => _status = slewErr);
          return;
        }
        _recenterNext = true;
        setState(() =>
            _status = 'Off by ${miss.toStringAsFixed(1)}\'. Recentering.');
        return;
      }
      setState(() =>
          _status = 'Synced. ${miss.toStringAsFixed(1)}\' from the target.');
    } finally {
      _busy = false;
      if (mounted) setState(() {});
    }
  }

  Future<String?> _gotoTarget(double raH, double decD) async {
    final client = ref.read(lx200ClientProvider);
    if (!client.isConnected) return 'Mount is not connected';
    final formatted = formatRaDecLx200(raH, decD);
    if (await client.sendCommand(LX200.setTargetRa(formatted.$1)) != '1') {
      return 'The mount rejected the coordinates';
    }
    if (await client.sendCommand(
            LX200.setTargetDec(formatted.$2.replaceFirst(':', '*'))) !=
        '1') {
      return 'The mount rejected the coordinates';
    }
    final slew = await client.sendCommand(LX200.gotoTarget);
    if (slew == null || slew != '0') {
      final code = gotoErrorFromReply(slew);
      return code != null ? gotoErrorCause(code) : 'Goto rejected';
    }
    return null;
  }

  Future<int?> _askAlignStars() {
    final mount = ref.read(mountStateProvider).mountType;
    final six = mount == MountType.gem || mount == MountType.undefined;
    return showDialog<int>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Align'),
        content: const Text(
          'The telescope must be at home. The phone slews to each place and plate-solves. Nothing is centered by hand.',
        ),
        actions: [
          TextButton(
              onPressed: () => Navigator.pop(ctx, 2),
              child: const Text('2 Stars')),
          TextButton(
              onPressed: () => Navigator.pop(ctx, 4),
              child: const Text('4 Stars')),
          if (six)
            TextButton(
                onPressed: () => Navigator.pop(ctx, 6),
                child: const Text('3+3 Stars')),
          TextButton(
              onPressed: () => Navigator.pop(ctx), child: const Text('Cancel')),
        ],
      ),
    );
  }

  Future<(double, double)> _altLimits() async {
    final raw =
        await ref.read(lx200ClientProvider).sendCommand(LX200.getAllConfig);
    if (raw == null) return (0.0, 90.0);
    final cfg = MountConfig.parseBinaryConfig(raw);
    if (!cfg.valid) return (0.0, 90.0);
    return (cfg.minAlt.toDouble(), cfg.maxAlt.toDouble());
  }

  Future<bool> _onThisPier(PhoneAlignPlace place) async {
    final client = ref.read(lx200ClientProvider);
    final formatted = formatRaDecLx200(place.raH, place.decD);
    final reply =
        await client.sendCommand(':M?${formatted.$1}${formatted.$2}#');
    final side = ref.read(mountStateProvider).pierSide;
    if (side == PierSide.unknown) return reply == 'E' || reply == 'W';
    final want = side == PierSide.west ? 'W' : 'E';
    return reply == want;
  }

  Future<bool> _waitStill(
      {Duration limit = const Duration(seconds: 180)}) async {
    final notifier = ref.read(mountStateProvider.notifier);
    final started = DateTime.now();
    await Future.delayed(const Duration(milliseconds: 1500));
    while (DateTime.now().difference(started) < limit) {
      if (_stopAlign || !mounted) return false;
      await notifier.refresh();
      if (!ref.read(mountStateProvider).isSlewing) return true;
      await Future.delayed(const Duration(milliseconds: 400));
    }
    return false;
  }

  Future<void> _cancelAlign(LX200TcpClient client, bool started) async {
    await client.sendCommand(LX200.stopAll);
    if (started) await client.sendBool(LX200.alignAbort);
  }

  Future<String?> _recordSolved(int star, double raH, double decD) async {
    final client = ref.read(lx200ClientProvider);
    final formatted = formatRaDecLx200(raH, decD);
    if (await client.sendCommand(LX200.setTargetRa(formatted.$1)) != '1') {
      return 'The mount rejected the solved position';
    }
    if (await client.sendCommand(
            LX200.setTargetDec(formatted.$2.replaceFirst(':', '*'))) !=
        '1') {
      return 'The mount rejected the solved position';
    }
    final reply = await client.sendCommand(
      LX200.alignSelectStar(star),
      timeout: star >= 3 ? const Duration(seconds: 20) : null,
    );
    if (reply != '1') return 'The mount rejected star $star';
    return null;
  }

  Future<void> _runAlign(int stars) async {
    if (!_hasCamera || _cameraId == null) {
      setState(() => _status = 'The phone camera is used on Android');
      return;
    }
    final client = ref.read(lx200ClientProvider);
    if (!client.isConnected) {
      setState(() => _status = 'Mount is not connected');
      return;
    }
    final notifier = ref.read(mountStateProvider.notifier);
    await notifier.refresh();
    var state = ref.read(mountStateProvider);
    if (state.isParked) {
      setState(() => _status = 'The mount is parked');
      return;
    }
    if (state.isSlewing) {
      setState(() => _status = 'The mount is slewing');
      return;
    }
    if (!state.atHome) {
      setState(() => _status = 'The telescope must be at home');
      return;
    }
    final lst = parseSiderealTime(state.siderealTime);
    if (lst == null) {
      setState(() => _status = 'Sidereal time is not available');
      return;
    }
    final limits = await _altLimits();
    if (!mounted) return;
    final altAz = state.mountType == MountType.altAz ||
        state.mountType == MountType.forkAlt;
    final plans = phoneAlignPlans(
      stars: stars,
      lstH: lst,
      latDeg: state.latitude,
      altAz: altAz,
      minAlt: limits.$1,
      maxAlt: limits.$2,
    );
    PhoneAlignPlan? plan;
    for (final candidate in plans) {
      final probe = candidate.flipAfterThird ? 3 : candidate.places.length;
      var ok = true;
      for (var i = 0; i < probe; i++) {
        if (!await _onThisPier(candidate.places[i])) {
          ok = false;
          break;
        }
      }
      if (ok) {
        plan = candidate;
        break;
      }
    }
    if (plan == null) {
      if (mounted) {
        setState(() => _status = 'These places are not reachable from home');
      }
      return;
    }
    _stopAlign = false;
    setState(() {
      _busy = true;
      _aligning = true;
      _status = 'Starting alignment…';
    });
    var started = false;
    try {
      final startCmd =
          stars == 2 ? LX200.alignStart : LX200.alignStartRigid(stars);
      if (!await client.sendBool(startCmd)) {
        setState(() => _status = 'The mount rejected the alignment');
        return;
      }
      started = true;
      for (var i = 0; i < plan.places.length; i++) {
        if (_stopAlign || !mounted) {
          await _cancelAlign(client, true);
          if (mounted) setState(() => _status = 'Alignment stopped');
          return;
        }
        final n = i + 1;
        if (plan.flipAfterThird && i == 3) {
          setState(() => _status = '$n/${plan!.places.length}  Flipping');
          final flip = await client.sendCommand(LX200.meridianFlip);
          if (flip != '0') {
            await _cancelAlign(client, true);
            if (mounted) setState(() => _status = 'The flip was rejected');
            return;
          }
          if (!await _waitStill()) {
            await _cancelAlign(client, true);
            if (mounted) {
              setState(() => _status =
                  _stopAlign ? 'Alignment stopped' : 'The flip did not finish');
            }
            return;
          }
        }
        final place = plan.places[i];
        setState(() => _status = '$n/${plan!.places.length}  Slewing');
        final slewErr = await _gotoTarget(place.raH, place.decD);
        if (slewErr != null) {
          await _cancelAlign(client, true);
          if (mounted) setState(() => _status = slewErr);
          return;
        }
        if (!await _waitStill()) {
          await _cancelAlign(client, true);
          if (mounted) {
            setState(() => _status =
                _stopAlign ? 'Alignment stopped' : 'The slew did not finish');
          }
          return;
        }
        await Future.delayed(const Duration(seconds: 1));
        FinderSolution? solved;
        for (var attempt = 0; attempt < 3 && solved == null; attempt++) {
          if (_stopAlign || !mounted) break;
          setState(() => _status = '$n/${plan!.places.length}  Solving');
          solved = await _solve();
        }
        if (_stopAlign || !mounted) {
          await _cancelAlign(client, true);
          if (mounted) setState(() => _status = 'Alignment stopped');
          return;
        }
        if (solved == null) {
          final why = _status;
          await _cancelAlign(client, true);
          if (mounted) {
            setState(
                () => _status = why.isEmpty ? 'The field did not match' : why);
          }
          return;
        }
        var axisRa = solved.raH;
        var axisDec = solved.decD;
        if (!_cones.isZero) {
          final axis = telescopeAxis(solved, _cones);
          axisRa = axis.$1;
          axisDec = axis.$2;
        }
        final horizon = _belowHorizon(axisRa, axisDec);
        if (horizon != null) {
          await _cancelAlign(client, true);
          if (mounted) setState(() => _status = horizon);
          return;
        }
        final tilt = _tiltMismatch(axisRa, axisDec);
        if (tilt != null) {
          await _cancelAlign(client, true);
          if (mounted) setState(() => _status = tilt);
          return;
        }
        final residual = (solved.rmsDeg * 60).toStringAsFixed(1);
        setState(() =>
            _status = '$n/${plan!.places.length}  $residual\'  Recording');
        final err = await _recordSolved(n, axisRa, axisDec);
        if (err != null) {
          await _cancelAlign(client, true);
          if (mounted) setState(() => _status = err);
          return;
        }
      }
      if (!await client.sendBool(LX200.alignSave)) {
        if (mounted) setState(() => _status = 'The model was not saved');
        return;
      }
      await notifier.refresh();
      if (mounted) setState(() => _status = 'Alignment saved');
    } finally {
      if (mounted) {
        setState(() {
          _busy = false;
          _aligning = false;
        });
      }
    }
  }

  String _fmt(double raH, double decD) {
    final pair = formatRaDecLx200(raH, decD);
    return '${pair.$1}  ${pair.$2}';
  }

  @override
  Widget build(BuildContext context) {
    ref.listen(mountStateProvider, (prev, next) {
      if (!_auto || _busy || prev == null) return;
      if (!prev.isSlewing || next.isSlewing) return;
      final ra = parseMountHours(next.targetRa);
      final dec = parseMountDeg(next.targetDec);
      if (ra == null || dec == null) return;
      _afterSlew(ra, dec);
    });
    final sky = ref.watch(finderSkyProvider);
    final mount = ref.read(lx200ClientProvider).isConnected;
    return LayoutBuilder(builder: (context, box) {
      if (box.maxWidth > box.maxHeight * 1.2) {
        return _landscape(sky, mount, box);
      }
      return Column(
        children: [
          _header(sky),
          Expanded(child: _view()),
          _actions(sky, mount),
        ],
      );
    });
  }

  /// Square view on the left, everything else in a column on the right.
  Widget _landscape(FinderSky sky, bool mount, BoxConstraints box) {
    final panel = math.max(260.0, box.maxWidth - box.maxHeight);
    final live = _previewLive;
    final photo = _shotImage != null &&
        _shotFrame != null &&
        (_showPhoto || !live);
    return Row(
      children: [
        Expanded(child: _view(side: true)),
        SizedBox(
          width: math.min(panel, 380),
          child: ColoredBox(
            color: TA.surface,
            child: Column(
              children: [
                _header(sky, compact: true),
                Expanded(
                  child: SingleChildScrollView(
                    padding: const EdgeInsets.symmetric(horizontal: 8),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        const PushToCard(),
                        if (live || photo) ...[
                          const SizedBox(height: 6),
                          _toneControls(live: live, photo: photo),
                        ],
                        if (_status.isNotEmpty) ...[
                          const SizedBox(height: 6),
                          Text(_status, style: _overlayStyle),
                        ],
                      ],
                    ),
                  ),
                ),
                _actions(sky, mount),
              ],
            ),
          ),
        ),
      ],
    );
  }

  TextStyle get _overlayStyle =>
      TextStyle(color: TA.textHigh, fontSize: 12, fontFamily: 'monospace');

  Widget _toneControls({required bool live, required bool photo}) {
    final overlay = _overlayStyle;
    return Container(
      padding: const EdgeInsets.only(left: 10),
      decoration: BoxDecoration(
          color: Colors.black54, borderRadius: BorderRadius.circular(6)),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          _stepSlider(
            icon: Icons.shutter_speed,
            steps: PhoneFinderStore.exposureSteps,
            value: _shutterUs,
            label: _timeLabel(_shutterUs),
            enabled: !_busy && !_live,
            onChanged: (v) => setState(() => _shutterUs = v),
            onDone: _store.saveExposureUs,
          ),
          Row(
            children: [
              Icon(Icons.brightness_6, size: 18, color: TA.textHigh),
              Expanded(
                child: Slider(
                  min: PhoneFinderStore.gammaMin,
                  max: PhoneFinderStore.gammaMax,
                  divisions: 40,
                  value: _gamma,
                  onChanged: _setGamma,
                  onChangeEnd: _gammaDone,
                ),
              ),
              SizedBox(
                width: 52,
                child: Text('γ ${_gamma.toStringAsFixed(1)}', style: overlay),
              ),
            ],
          ),
        ],
      ),
    );
  }

  String _timeLabel(int us) => us >= 1000000
      ? '${(us / 1000000).toStringAsFixed(us % 1000000 == 0 ? 0 : 1)} s'
      : '${(us / 1000).round()} ms';

  /// A slider that snaps to [steps]; [value] need not be one of them.
  Widget _stepSlider({
    required IconData icon,
    required List<int> steps,
    required int value,
    required String label,
    required bool enabled,
    required ValueChanged<int> onChanged,
    required ValueChanged<int> onDone,
  }) {
    var index = 0;
    for (var i = 0; i < steps.length; i++) {
      if ((steps[i] - value).abs() < (steps[index] - value).abs()) index = i;
    }
    return Row(
      children: [
        Icon(icon, size: 18, color: TA.textHigh),
        Expanded(
          child: Slider(
            min: 0,
            max: (steps.length - 1).toDouble(),
            divisions: steps.length - 1,
            value: index.toDouble(),
            onChanged: enabled ? (v) => onChanged(steps[v.round()]) : null,
            onChangeEnd: enabled ? (v) => onDone(steps[v.round()]) : null,
          ),
        ),
        SizedBox(width: 52, child: Text(label, style: _overlayStyle)),
      ],
    );
  }

  Widget _header(FinderSky sky, {bool compact = false}) {
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.fromLTRB(12, 0, 0, 0),
      color: TA.surface,
      child: Row(
        children: [
          if (!compact) ...[
            Icon(Icons.photo_camera, size: 16, color: TA.accent),
            const SizedBox(width: 8),
            Text('Phone finder',
                style: TextStyle(
                    color: TA.textHigh,
                    fontSize: 14,
                    fontWeight: FontWeight.w600)),
            const SizedBox(width: 12),
          ],
          Expanded(
            child: sky.active
                ? Text(
                    sky.serverError ??
                        'SkySafari ${sky.serverAddress ?? 'starting…'}',
                    overflow: TextOverflow.ellipsis,
                    style: TextStyle(
                        color: sky.serverError == null ? TA.textSecondary : TA.error,
                        fontSize: 11,
                        fontFamily: 'monospace'),
                  )
                : const SizedBox.shrink(),
          ),
          if (sky.active && !sky.hasSite)
            Tooltip(
              message: 'Site missing',
              child: Icon(Icons.location_off, size: 18, color: TA.warning),
            ),
          IconButton(
            tooltip: ref.watch(nightViewProvider) ? 'Day view' : 'Night view',
            icon: Icon(
              ref.watch(nightViewProvider)
                  ? Icons.wb_sunny_outlined
                  : Icons.nightlight_round,
              color: TA.textHigh,
            ),
            onPressed: () => ref.read(nightViewProvider.notifier).toggle(),
          ),
          IconButton(
            tooltip: 'Parameters',
            icon: Icon(Icons.tune, color: TA.textHigh),
            onPressed: _openSettings,
          ),
          if (sky.active)
            IconButton(
              tooltip: 'Exit finder',
              icon: Icon(Icons.logout, color: TA.textHigh),
              onPressed: () async {
                setState(() => _live = false);
                await ref.read(finderSkyProvider.notifier).stop();
                if (mounted) context.go('/connect');
              },
            ),
        ],
      ),
    );
  }

  /// The camera, or the last photo with the stars Cedar found in it.
  /// With [side], the push-to card, sliders and status live in the side panel.
  Widget _view({bool side = false}) {
    final shot = _shotImage;
    final frame = _shotFrame;
    final live = _previewLive;
    final photo = shot != null && frame != null && (_showPhoto || !live);
    final Widget base;
    final turns = _uprightTurns(context);
    if (photo) {
      base = CustomPaint(
        size: Size.infinite,
        painter: _ShotPainter(
          image: shot,
          frame: frame,
          quarterTurns: turns,
          ring: TA.success,
        ),
      );
    } else if (live) {
      // Hybrid composition so TextureView.setTransform is honored.
      const viewType = 'teenastro/infinity_preview';
      final params = <String, dynamic>{
        'cameraId': _cameraId,
        'gamma': _gamma,
        'zoom': _zoom,
      };
      base = PlatformViewLink(
        viewType: viewType,
        surfaceFactory: (context, controller) {
          return AndroidViewSurface(
            controller: controller as AndroidViewController,
            gestureRecognizers: const <Factory<OneSequenceGestureRecognizer>>{},
            hitTestBehavior: PlatformViewHitTestBehavior.opaque,
          );
        },
        onCreatePlatformView: (params_) {
          return PlatformViewsService.initSurfaceAndroidView(
            id: params_.id,
            viewType: viewType,
            layoutDirection: TextDirection.ltr,
            creationParams: params,
            creationParamsCodec: const StandardMessageCodec(),
            onFocus: () => params_.onFocusChanged(true),
          )
            ..addOnPlatformViewCreatedListener(params_.onPlatformViewCreated)
            ..create();
        },
      );
    } else {
      base = Center(
        child: Padding(
          padding: const EdgeInsets.all(24),
          child: Text(
            _cameraError ??
                (_hasCamera
                    ? 'Opening the camera…'
                    : 'The phone camera is used on Android.'),
            textAlign: TextAlign.center,
            style: TextStyle(color: TA.textSecondary),
          ),
        ),
      );
    }
    final overlay = _overlayStyle;
    return Stack(
      fit: StackFit.expand,
      children: [
        ColoredBox(color: Colors.black, child: base),
        if (photo)
          Positioned(
            top: 8,
            left: 8,
            child: Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
              decoration: BoxDecoration(
                  color: Colors.black54, borderRadius: BorderRadius.circular(4)),
              child: Text('${frame.stars.length} stars  $_shotNote',
                  style: overlay),
            ),
          ),
        if (shot != null && live)
          Positioned(
            top: 4,
            right: 4,
            child: IconButton(
              tooltip: _showPhoto ? 'Camera' : 'Last photo',
              style: IconButton.styleFrom(backgroundColor: Colors.black54),
              icon: Icon(_showPhoto ? Icons.videocam : Icons.photo,
                  color: TA.textHigh),
              onPressed: () => setState(() => _showPhoto = !_showPhoto),
            ),
          ),
        if (!side)
          const Positioned(
            top: 48,
            left: 0,
            right: 0,
            child: Center(child: PushToCard()),
          ),
        if (!side && (live || photo))
          Positioned(
            left: 8,
            right: 8,
            bottom: _status.isNotEmpty ? 30 : 4,
            child: _toneControls(live: live, photo: photo),
          ),
        if (!side && _status.isNotEmpty)
          Positioned(
            left: 0,
            right: 0,
            bottom: 0,
            child: Container(
              color: Colors.black54,
              padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 6),
              child: Text(_status, style: overlay),
            ),
          ),
      ],
    );
  }

  Widget _actions(FinderSky sky, bool mount) {
    final idle = !_busy && !_live;
    final solve = OutlinedButton(
      onPressed: idle ? () => _onSolve(sync: false, aim: false) : null,
      child: const Text('Solve'),
    );
    final List<Widget> rest;
    if (sky.active) {
      final left = _liveUntil == null
          ? _liveSeconds
          : math.max(0, _liveUntil!.difference(DateTime.now()).inSeconds);
      rest = [
        Expanded(
          child: _live
              ? FilledButton.icon(
                  style: FilledButton.styleFrom(backgroundColor: TA.error),
                  onPressed: () => setState(() => _live = false),
                  icon: const Icon(Icons.stop),
                  label: Text('Stop live (${_liveLabel(left)})'),
                )
              : FilledButton.icon(
                  onPressed: _busy ? null : _runLive,
                  icon: const Icon(Icons.play_arrow),
                  label: Text('Live ${_liveLabel(_liveSeconds)}'),
                ),
        ),
      ];
    } else {
      rest = [
        Expanded(
          child: ElevatedButton(
            onPressed: idle && mount ? () => _onSolve(sync: true, aim: false) : null,
            child: const Text('Solve & Sync'),
          ),
        ),
        const SizedBox(width: 8),
        Expanded(
          child: _aligning
              ? OutlinedButton(
                  onPressed: () {
                    _stopAlign = true;
                    ref.read(lx200ClientProvider).sendCommand(LX200.stopAll);
                  },
                  child: const Text('Stop'),
                )
              : OutlinedButton(
                  onPressed: idle && mount
                      ? () async {
                          final stars = await _askAlignStars();
                          if (stars == null || !mounted) return;
                          await _runAlign(stars);
                        }
                      : null,
                  child: const Text('Align'),
                ),
        ),
      ];
    }
    return Container(
      color: TA.surface,
      padding: const EdgeInsets.fromLTRB(8, 6, 8, 6),
      child: Row(children: [
        Expanded(child: solve),
        const SizedBox(width: 8),
        ...rest,
      ]),
    );
  }

  Future<void> _openSettings() async {
    await showModalBottomSheet<void>(
      context: context,
      isScrollControlled: true,
      backgroundColor: TA.surface,
      builder: (_) => Consumer(
        builder: (ctx, ref, _) {
          final sky = ref.watch(finderSkyProvider);
          final mount = ref.read(lx200ClientProvider).isConnected;
          return StatefulBuilder(
            builder: (ctx, setSheet) {
              void update(VoidCallback change) {
                setState(change);
                setSheet(() {});
              }

              return SafeArea(
                child: ConstrainedBox(
                  constraints: BoxConstraints(
                      maxHeight: MediaQuery.sizeOf(ctx).height * 0.8),
                  child: SingleChildScrollView(
                    padding: const EdgeInsets.fromLTRB(16, 12, 16, 16),
                    child: _settings(ctx, sky, mount, update),
                  ),
                ),
              );
            },
          );
        },
      ),
    );
  }

  Widget _settings(BuildContext ctx, FinderSky sky, bool mount,
      void Function(VoidCallback) update) {
    final label = TextStyle(color: TA.textSecondary, fontSize: 12);
    final value = TextStyle(color: TA.text, fontFamily: 'monospace');
    Widget section(String title) => Padding(
          padding: const EdgeInsets.only(top: 12, bottom: 4),
          child: Text(title,
              style: TextStyle(
                  color: TA.accent, fontSize: 13, fontWeight: FontWeight.w600)),
        );
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      mainAxisSize: MainAxisSize.min,
      children: [
        Text('Parameters',
            style: TextStyle(
                color: TA.textHigh, fontSize: 16, fontWeight: FontWeight.w600)),
        section('Display'),
        Row(children: [
          Expanded(
            child: Text('Night view', style: TextStyle(color: TA.text)),
          ),
          Switch(
            value: ref.watch(nightViewProvider),
            onChanged: (on) =>
                ref.read(nightViewProvider.notifier).setEnabled(on),
          ),
        ]),
        section('Solver'),
        SegmentedButton<PhoneSolver>(
          segments: const [
            ButtonSegment(value: PhoneSolver.tetra, label: Text('Tetra')),
            ButtonSegment(
                value: PhoneSolver.stellar, label: Text('StellarSolver')),
          ],
          selected: {_solver},
          showSelectedIcon: false,
          onSelectionChanged: _busy
              ? null
              : (picked) {
                  final solver = picked.first;
                  update(() => _solver = solver);
                  _store.saveSolver(solver);
                },
        ),
        section('Field'),
        if (_fovMeasured)
          Text('${_fov.round()}° (measured from the lens)', style: value)
        else
          Row(children: [
            Expanded(
              child: Slider(
                value: _fov.roundToDouble().clamp(15, 90),
                min: 15,
                max: 90,
                divisions: 75,
                label: '${_fov.round()}°',
                onChanged: _busy ? null : (v) => update(() => _fov = v),
                onChangeEnd: (v) => _store.saveFov(v),
              ),
            ),
            Text('${_fov.round()}°', style: value),
          ]),
        if (sky.active) ...[
          section('Live'),
          Row(children: [
            Text('Run for', style: label),
            Expanded(
              child: Slider(
                value: _liveSeconds.toDouble(),
                min: PhoneFinderStore.liveMinSeconds.toDouble(),
                max: PhoneFinderStore.liveMaxSeconds.toDouble(),
                divisions: (PhoneFinderStore.liveMaxSeconds -
                        PhoneFinderStore.liveMinSeconds) ~/
                    10,
                label: _liveLabel(_liveSeconds),
                onChanged: _live
                    ? null
                    : (v) => update(() => _liveSeconds = (v / 10).round() * 10),
                onChangeEnd: (v) =>
                    _store.saveLiveSeconds((v / 10).round() * 10),
              ),
            ),
            Text(_liveLabel(_liveSeconds), style: value),
          ]),
          section('Site'),
          Row(children: [
            Expanded(child: Text(_siteText(sky), style: value)),
            TextButton(
              onPressed: sky.locating
                  ? null
                  : () => ref.read(finderSkyProvider.notifier).locate(),
              child: const Text('Locate'),
            ),
          ]),
          section('SkySafari'),
          Text(
            sky.serverError ??
                'Meade LX200 Classic over Wi-Fi\n${sky.serverAddress ?? 'starting…'}',
            style: value,
          ),
        ],
        section('Telescope cones'),
        Text(
          _cones.isZero
              ? 'Hz Cone 0.0\'   Ve Cone 0.0\''
              : 'Hz Cone ${formatArcmin(_cones.hzRad)}   '
                  'Ve Cone ${formatArcmin(_cones.veRad)}',
          style: value,
        ),
        Row(children: [
          Expanded(
            child: OutlinedButton(
              onPressed: !_busy && !_live && mount
                  ? () {
                      Navigator.pop(ctx);
                      _onSolve(sync: false, aim: true);
                    }
                  : null,
              child: const Text('Aim'),
            ),
          ),
          const SizedBox(width: 8),
          Expanded(
            child: TextButton(
              onPressed: _busy || _live
                  ? null
                  : () async {
                      await _store.clearCones();
                      update(() {
                        _cones = const FinderCones();
                        _status = 'Cones cleared';
                      });
                    },
              child: const Text('Clear cones'),
            ),
          ),
        ]),
        if (mount) ...[
          section('Auto sync'),
          Row(children: [
            Expanded(
                child: Text('Solve and sync after each goto',
                    style: TextStyle(color: TA.text))),
            Switch(
              value: _auto,
              onChanged: _busy
                  ? null
                  : (on) {
                      update(() => _auto = on);
                      _store.saveAutoSync(on);
                    },
            ),
          ]),
          if (_auto)
            Row(children: [
              Text('Recenter', style: label),
              Expanded(
                child: Slider(
                  value: _limitArcmin.toDouble(),
                  min: 2,
                  max: 60,
                  divisions: 58,
                  label: "$_limitArcmin'",
                  onChanged: _busy
                      ? null
                      : (v) => update(() => _limitArcmin = v.round()),
                  onChangeEnd: (v) => _store.saveRecenterArcmin(v.round()),
                ),
              ),
              Text("$_limitArcmin'", style: value),
            ]),
        ],
      ],
    );
  }

  String _siteText(FinderSky sky) {
    if (!sky.hasSite) return sky.locating ? 'Locating…' : 'Site missing';
    final lat = sky.latDeg!;
    final lon = sky.lonEastDeg!;
    final source = switch (sky.siteSource) {
      'gps' => 'GPS',
      'ip' => 'network',
      _ => 'saved',
    };
    return '${lat.abs().toStringAsFixed(2)}°${lat >= 0 ? 'N' : 'S'} '
        '${lon.abs().toStringAsFixed(2)}°${lon >= 0 ? 'E' : 'W'} ($source)';
  }
}

/// The photo, turned upright, with a ring on each star Cedar extracted.
class _ShotPainter extends CustomPainter {
  final ui.Image image;
  final StarFrame frame;
  final int quarterTurns;
  final Color ring;

  _ShotPainter({
    required this.image,
    required this.frame,
    required this.quarterTurns,
    required this.ring,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final turned = quarterTurns.isOdd;
    final w = frame.width;
    final h = frame.height;
    final scale = math.min(size.width / (turned ? h : w), size.height / (turned ? w : h));
    canvas.save();
    canvas.translate(size.width / 2, size.height / 2);
    canvas.rotate(quarterTurns * math.pi / 2);
    canvas.scale(scale);
    canvas.drawImageRect(
      image,
      Rect.fromLTWH(0, 0, image.width.toDouble(), image.height.toDouble()),
      Rect.fromCenter(center: Offset.zero, width: w, height: h),
      Paint()..filterQuality = FilterQuality.medium,
    );
    final paint = Paint()
      ..style = PaintingStyle.stroke
      ..strokeWidth = 1.5 / scale
      ..color = ring;
    for (final s in frame.stars) {
      canvas.drawCircle(Offset(s.x - w / 2, s.y - h / 2), 9 / scale, paint);
    }
    canvas.restore();
  }

  @override
  bool shouldRepaint(_ShotPainter old) =>
      old.image != image || old.frame != frame || old.quarterTurns != quarterTurns;
}