import 'dart:convert';
import 'dart:io';

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
      setState(() {
        _cam = cam;
        _cameraError = null;
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

  (double, double)? _mountPointing() {
    final state = ref.read(mountStateProvider);
    final ra = parseMountHours(state.ra);
    final dec = parseMountDeg(state.dec);
    if (ra == null || dec == null) return null;
    return (ra, dec);
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
      final shot = await cam.takePicture();
      final bytes = await shot.readAsBytes();
      final frame = readStarFrame(bytes);
      if (frame == null || frame.stars.length < 4) {
        setState(() => _status = 'Not enough stars in the frame');
        return null;
      }
      final solved = solveField(
        stars: frame.stars,
        catalog: _catalog,
        hintRa: hint.$1,
        hintDec: hint.$2,
        width: frame.width,
        height: frame.height,
        fovDeg: _fov,
      );
      if (solved == null) {
        setState(() => _status = 'The field did not match the catalog');
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
        );
      }
      if (!sync) {
        setState(() => _status =
            'Solved ${shown.matches} stars. ${_cones.isZero ? 'Camera axis' : 'Telescope axis'} '
                '${_fmt(shown.raH, shown.decD)}');
        return;
      }
      final err = await _syncMount(shown.raH, shown.decD);
      if (!mounted) return;
      setState(() => _status = err ?? 'Synced ${_fmt(shown.raH, shown.decD)}');
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
    if (raw == null) return (0, 90);
    final cfg = MountConfig.parseBinaryConfig(raw);
    if (!cfg.valid) return (0, 90);
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
          await _cancelAlign(client, true);
          if (mounted) setState(() => _status = 'The field did not match');
          return;
        }
        var axisRa = solved.raH;
        var axisDec = solved.decD;
        if (!_cones.isZero) {
          final axis = telescopeAxis(solved, _cones);
          axisRa = axis.$1;
          axisDec = axis.$2;
        }
        setState(() => _status = '$n/${plan!.places.length}  Recording');
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
              Row(
                children: [
                  Text('Field',
                      style: TextStyle(color: TA.textSecondary, fontSize: 12)),
                  Expanded(
                    child: Slider(
                      value: _fov,
                      min: 15,
                      max: 90,
                      divisions: 75,
                      label: '${_fov.round()}°',
                      onChanged: _busy ? null : (v) => setState(() => _fov = v),
                      onChangeEnd: (v) => _store.saveFov(v),
                    ),
                  ),
                  Text('${_fov.round()}°',
                      style:
                          TextStyle(color: TA.text, fontFamily: 'monospace')),
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
