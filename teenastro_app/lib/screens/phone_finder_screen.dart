import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;
import 'dart:typed_data';

import 'package:camera/camera.dart';
import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:teenastro_app/models/catalog_entry.dart';
import 'package:teenastro_app/models/equinox_precession.dart';
import 'package:teenastro_app/models/lx200_commands.dart';
import 'package:teenastro_app/models/mount_config.dart';
import 'package:teenastro_app/models/mount_state.dart';
import 'package:teenastro_app/models/planet_positions.dart';
import 'package:teenastro_app/services/lx200_tcp_client.dart';
import 'package:teenastro_app/services/mount_state_provider.dart';
import 'package:teenastro_app/services/phone_finder.dart';
import 'package:teenastro_app/services/phone_finder_store.dart';
import 'package:teenastro_app/theme.dart';

/// Plate solve from the phone camera. The hardware efinder is not used.
class PhoneFinderScreen extends ConsumerStatefulWidget {
  const PhoneFinderScreen({super.key});

  @override
  ConsumerState<PhoneFinderScreen> createState() => _PhoneFinderScreenState();
}

class _PhoneFinderScreenState extends ConsumerState<PhoneFinderScreen> {
  final _store = PhoneFinderStore();
  CameraController? _cam;
  List<FinderStar> _catalog = const [];
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
  String? _cameraId;
  int? _shutterUs;
  int? _iso;
  String _exposureNote = '';

  bool get _hasCamera => !kIsWeb && Platform.isAndroid;

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
    final raw = await rootBundle.loadString('assets/catalogs/stars.json');
    final jd = julianDate(DateTime.now().toUtc());
    final catalog = _catalogFromJson(raw, jd);
    if (!mounted) return;
    setState(() {
      _cones = cones;
      _fov = fov;
      _auto = auto;
      _limitArcmin = limit;
      _catalog = catalog;
    });
    if (_hasCamera) await _openCamera();
  }

  List<FinderStar> _catalogFromJson(String raw, double jd) {
    final json = jsonDecode(raw) as Map<String, dynamic>;
    final objects = json['objects'] as List<dynamic>;
    final stars = <FinderStar>[];
    for (final item in objects) {
      final m = item as Map<String, dynamic>;
      final mag = (m['mag'] as num?)?.toDouble();
      if (mag == null || mag > 4.5) continue;
      final ra = (m['ra'] as num).toDouble();
      final dec = (m['dec'] as num).toDouble();
      final now = equatorialEquinoxToJNow(ra, dec, 2000, jd);
      stars.add(FinderStar(now.$1, now.$2));
    }
    return stars;
  }

  /// Longer-side field of the camera, from its focal length and sensor size.
  Future<double?> _lensField(String cameraId) async {
    try {
      final value = await const MethodChannel('com.teenastro/camera')
          .invokeMethod<num>('longerSideFov', {'cameraId': cameraId});
      final deg = value?.toDouble();
      if (deg == null || deg < 5 || deg > 170) return null;
      return deg;
    } catch (_) {
      return null;
    }
  }

  Future<void> _openCamera() async {
    try {
      final cameras = await availableCameras();
      if (cameras.isEmpty) {
        if (mounted) setState(() => _cameraError = 'No camera on this phone');
        return;
      }
      final back = cameras.cast<CameraDescription?>().firstWhere(
            (c) => c!.lensDirection == CameraLensDirection.back,
            orElse: () => cameras.first,
          )!;
      final measured = await _lensField(back.name);
      final cam = CameraController(
        back,
        ResolutionPreset.medium,
        enableAudio: false,
      );
      await cam.initialize();
      if (!mounted) {
        await cam.dispose();
        return;
      }
      try {
        await cam.setExposureMode(ExposureMode.auto);
        await cam.setFlashMode(FlashMode.off);
      } catch (_) {}
      if (measured != null) await _store.saveFov(measured);
      setState(() {
        _cam = cam;
        _cameraId = back.name;
        _cameraError = null;
        if (measured != null) {
          _fov = measured;
          _fovMeasured = true;
        }
      });
    } catch (e) {
      if (mounted) setState(() => _cameraError = 'Camera unavailable');
    }
  }

  @override
  void dispose() {
    _stopAlign = true;
    _cam?.dispose();
    super.dispose();
  }

  /// Null when the axis is above the horizon, or when the site is unknown.
  String? _belowHorizon(double raH, double decD) {
    final state = ref.read(mountStateProvider);
    final lst = parseMountHours(state.siderealTime);
    if (lst == null) return null;
    final alt = altitudeDeg(state.latitude, lst, raH, decD);
    if (alt > 0) return null;
    return 'The solution is below the horizon (${alt.toStringAsFixed(0)}°)';
  }

  (double, double)? _mountPointing() {
    final state = ref.read(mountStateProvider);
    final ra = parseMountHours(state.ra);
    final dec = parseMountDeg(state.dec);
    if (ra == null || dec == null) return null;
    return (ra, dec);
  }

  static const _cameraChannel = MethodChannel('com.teenastro/camera');

  Uint8List? _jpegBytes(Object? raw) {
    if (raw is Uint8List) return raw;
    if (raw is List) return Uint8List.fromList(raw.cast<int>());
    return null;
  }

  String _shutterLabel(int us, int iso) {
    final time = us >= 1000000
        ? '${(us / 1000000).toStringAsFixed(1)} s'
        : us >= 10000
            ? '${(us / 1000).toStringAsFixed(0)} ms'
            : '${(us / 1000).toStringAsFixed(1)} ms';
    return '$time · ISO $iso';
  }

  /// Lengthen the shutter, then the ISO, until the frame holds about 30 stars.
  /// The preview is released so the shutter time can be set directly.
  Future<StarFrame?> _captureForStars() async {
    final id = _cameraId;
    if (id == null) return null;
    final preview = _cam;
    if (mounted) setState(() => _cam = null);
    await preview?.dispose();
    await Future<void>.delayed(const Duration(milliseconds: 350));

    Map<dynamic, dynamic>? limits;
    for (var attempt = 0; attempt < 3 && limits == null; attempt++) {
      try {
        final raw = await _cameraChannel.invokeMethod<dynamic>(
          'manualOpen',
          {'cameraId': id},
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
    var maxUs = (limits['maxUs'] as num).toInt();
    final minIso = (limits['minIso'] as num).toInt();
    final maxIso = (limits['maxIso'] as num).toInt();
    var us = openingShutter(minUs: minUs, maxUs: maxUs);
    var iso = (_iso ?? math.min(maxIso, math.max(minIso, 1600))).clamp(minIso, maxIso);

    StarFrame? best;
    var bestGap = 1 << 30;
    var bestNote = '';
    try {
      for (var attempt = 0; attempt < 8; attempt++) {
        final note = _shutterLabel(us, iso);
        if (mounted) setState(() => _status = 'Shutter $note');
        Uint8List? bytes;
        var delivered = us;
        try {
          final raw = await _cameraChannel.invokeMethod<dynamic>(
            'manualCapture',
            {'exposureUs': us, 'iso': iso},
          );
          if (raw is Map) {
            bytes = _jpegBytes(raw['jpeg']);
            final got = raw['exposureUs'];
            if (got is num && got > 0) delivered = got.toInt();
          } else {
            bytes = _jpegBytes(raw);
          }
        } catch (_) {
          bytes = null;
        }
        final applied = deliveredShutter(
          requestedUs: us,
          deliveredUs: delivered,
          minUs: minUs,
          maxUs: maxUs,
        );
        us = applied.us;
        maxUs = applied.maxUs;
        final shown = _shutterLabel(us, iso);
        final frame = bytes == null ? null : readStarFrame(bytes, maxStars: 80);
        if (frame == null) continue;
        final count = frame.stars.length;
        final gap = (count - targetStarCount).abs();
        if (gap < bestGap) {
          bestGap = gap;
          best = frame;
          bestNote = shown;
          _shutterUs = us;
          _iso = iso;
        }
        if (mounted) setState(() => _status = '$count stars at $shown');
        if (gap <= 6) break;
        final next = nextShutter(
          starCount: count,
          us: us,
          iso: iso,
          minUs: minUs,
          maxUs: maxUs,
          minIso: minIso,
          maxIso: maxIso,
        );
        if (next == null) break;
        us = next.us;
        iso = next.iso;
      }
    } finally {
      try {
        await _cameraChannel.invokeMethod<void>('manualClose');
      } catch (_) {}
      if (mounted) await _openCamera();
    }
    _exposureNote = bestNote;
    if (best == null) return null;
    final kept = best.stars.length <= targetStarCount
        ? best.stars
        : best.stars.sublist(0, targetStarCount);
    return StarFrame(kept, best.width, best.height);
  }

  Future<FinderSolution?> _solve() async {
    final cam = _cam;
    if (cam == null || !cam.value.isInitialized) {
      setState(() => _status = 'The phone camera is not ready');
      return null;
    }
    final hint = _mountPointing();
    if (hint == null) {
      setState(
          () => _status = 'Connect the mount so the solve knows where to look');
      return null;
    }
    setState(() {
      _busy = true;
      _status = 'Capturing…';
    });
    try {
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
      final state = ref.read(mountStateProvider);
      final lst = parseMountHours(state.siderealTime);
      final outcome = solveField(
        stars: frame.stars,
        catalog: _catalog,
        hintRa: hint.$1,
        hintDec: hint.$2,
        width: frame.width,
        height: frame.height,
        fovDeg: _fov,
        latDeg: lst == null ? null : state.latitude,
        lstH: lst,
      );
      final solved = outcome.solution;
      if (solved == null) {
        setState(() => _status =
            outcome.reason ?? 'The field did not match the catalog');
        return null;
      }
      return solved;
    } catch (_) {
      if (mounted) setState(() => _status = 'The picture could not be read');
      return null;
    }
  }

  Future<void> _onSolve({required bool sync, required bool aim}) async {
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
      final residual = (shown.rmsDeg * 60).toStringAsFixed(1);
      if (!sync) {
        setState(() => _status =
            'Solved ${shown.matches} stars, residual $residual\''
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
    setState(() => _status = secondPass ? 'Syncing after recenter…' : 'Auto sync…');
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
    final six =
        mount == MountType.gem || mount == MountType.undefined;
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
              onPressed: () => Navigator.pop(ctx),
              child: const Text('Cancel')),
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
    final reply = await client.sendCommand(':M?${formatted.$1}${formatted.$2}#');
    final side = ref.read(mountStateProvider).pierSide;
    if (side == PierSide.unknown) return reply == 'E' || reply == 'W';
    final want = side == PierSide.west ? 'W' : 'E';
    return reply == want;
  }

  Future<bool> _waitStill({Duration limit = const Duration(seconds: 180)}) async {
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
    if (!_hasCamera || _cam == null) {
      setState(() => _status = 'The phone camera is used on Android');
      return;
    }
    if (_catalog.isEmpty) {
      setState(() => _status = 'The star catalog is not ready');
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
            setState(() => _status = _stopAlign
                ? 'Alignment stopped'
                : 'The slew did not finish');
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
            setState(() => _status =
                why.isEmpty ? 'The field did not match' : why);
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
        final residual = (solved.rmsDeg * 60).toStringAsFixed(1);
        setState(() => _status =
            '$n/${plan!.places.length}  $residual\'  Recording');
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
    final preview = _cam;
    return Column(
      children: [
        Expanded(
          child: preview != null && preview.value.isInitialized
              ? CameraPreview(preview)
              : Center(
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
                ),
        ),
        Padding(
          padding: const EdgeInsets.fromLTRB(12, 8, 12, 8),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              if (_fovMeasured)
                Text(
                  'Field ${_fov.round()}°',
                  style: TextStyle(color: TA.text, fontFamily: 'monospace'),
                )
              else
                Row(
                  children: [
                    Text('Field',
                        style:
                            TextStyle(color: TA.textSecondary, fontSize: 12)),
                    Expanded(
                      child: Slider(
                        value: _fov.roundToDouble().clamp(15, 90),
                        min: 15,
                        max: 90,
                        divisions: 75,
                        label: '${_fov.round()}°',
                        onChanged:
                            _busy ? null : (v) => setState(() => _fov = v),
                        onChangeEnd: (v) => _store.saveFov(v),
                      ),
                    ),
                    Text('${_fov.round()}°',
                        style: TextStyle(
                            color: TA.text, fontFamily: 'monospace')),
                  ],
                ),
              Text(
                _cones.isZero
                    ? 'Hz Cone 0.0\'   Ve Cone 0.0\''
                    : 'Hz Cone ${formatArcmin(_cones.hzRad)}   '
                        'Ve Cone ${formatArcmin(_cones.veRad)}',
                style: TextStyle(color: TA.textHigh, fontFamily: 'monospace'),
              ),
              Row(
                children: [
                  Expanded(
                    child: Text(
                      'Auto sync',
                      style: TextStyle(color: TA.text),
                    ),
                  ),
                  Switch(
                    value: _auto,
                    onChanged: _busy
                        ? null
                        : (on) {
                            setState(() => _auto = on);
                            _store.saveAutoSync(on);
                          },
                  ),
                ],
              ),
              if (_auto)
                Row(
                  children: [
                    Text('Recenter',
                        style:
                            TextStyle(color: TA.textSecondary, fontSize: 12)),
                    Expanded(
                      child: Slider(
                        value: _limitArcmin.toDouble(),
                        min: 2,
                        max: 60,
                        divisions: 58,
                        label: "$_limitArcmin'",
                        onChanged: _busy
                            ? null
                            : (v) => setState(() => _limitArcmin = v.round()),
                        onChangeEnd: (v) =>
                            _store.saveRecenterArcmin(v.round()),
                      ),
                    ),
                    Text("$_limitArcmin'",
                        style:
                            TextStyle(color: TA.text, fontFamily: 'monospace')),
                  ],
                ),
              if (_status.isNotEmpty)
                Padding(
                  padding: const EdgeInsets.only(top: 4),
                  child: Text(_status,
                      style: TextStyle(color: TA.textSecondary, fontSize: 12)),
                ),
              const SizedBox(height: 8),
              _aligning
                  ? OutlinedButton(
                      onPressed: () {
                        _stopAlign = true;
                        ref.read(lx200ClientProvider).sendCommand(LX200.stopAll);
                      },
                      child: const Text('Stop'),
                    )
                  : OutlinedButton(
                      onPressed: _busy
                          ? null
                          : () async {
                              final stars = await _askAlignStars();
                              if (stars == null || !mounted) return;
                              await _runAlign(stars);
                            },
                      child: const Text('Align'),
                    ),
              const SizedBox(height: 6),
              Row(
                children: [
                  Expanded(
                    child: OutlinedButton(
                      onPressed: _busy
                          ? null
                          : () => _onSolve(sync: false, aim: false),
                      child: const Text('Solve'),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: ElevatedButton(
                      onPressed:
                          _busy ? null : () => _onSolve(sync: true, aim: false),
                      child: const Text('Solve & Sync'),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 6),
              Row(
                children: [
                  Expanded(
                    child: OutlinedButton(
                      onPressed:
                          _busy ? null : () => _onSolve(sync: false, aim: true),
                      child: const Text('Aim'),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: TextButton(
                      onPressed: _busy
                          ? null
                          : () async {
                              await _store.clearCones();
                              setState(() {
                                _cones = const FinderCones();
                                _status = 'Cones cleared';
                              });
                            },
                      child: const Text('Clear cones'),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
      ],
    );
  }
}
