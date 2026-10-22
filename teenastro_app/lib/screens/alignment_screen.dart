import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../models/lx200_commands.dart';
import '../models/mount_state.dart';
import '../models/catalog_entry.dart';
import '../models/equinox_precession.dart';
import '../models/planet_positions.dart';
import '../providers/planetarium_settings_provider.dart';
import '../theme.dart';
import '../widgets/sky_map.dart';

/// Alignment session offered by the hand controller.
enum AlignKind { two, four, six, mechanical }

enum AlignUi { idle, select, confirm, slewing, recenter, bolt, done }

/// Remembers the session across navigation. GXAS does not carry the session kind.
class AlignSessionMemory {
  static AlignKind? kind;
}

int alignStarCount(AlignKind kind) => switch (kind) {
      AlignKind.two => 2,
      AlignKind.four => 4,
      AlignKind.six => 6,
      AlignKind.mechanical => 2,
    };

String alignKindLabel(AlignKind kind) => switch (kind) {
      AlignKind.two => '2 Stars',
      AlignKind.four => '4 Stars',
      AlignKind.six => '3+3 Stars',
      AlignKind.mechanical => '2 Stars Mech.',
    };

String alignKindBlurb(AlignKind kind) => switch (kind) {
      AlignKind.two =>
        'Measures the pole. Mount error, when on, holds CH and NP.',
      AlignKind.four => 'Four stars on one pier side. Measures NP.',
      AlignKind.six =>
        'Three stars, flip, then three more. Measures CH and NP.',
      AlignKind.mechanical => 'Two stars, then adjust the pole bolts.',
    };

String alignStartCommand(AlignKind kind) => switch (kind) {
      AlignKind.two => LX200.alignStart,
      AlignKind.four => LX200.alignStartRigid(4),
      AlignKind.six => LX200.alignStartRigid(6),
      AlignKind.mechanical => LX200.alignStartMechanicalPole,
    };

bool alignKindOnAltAz(AlignKind kind) =>
    kind == AlignKind.two || kind == AlignKind.four;

String formatSignedArcsec(double arcsec) {
  final n = arcsec.round();
  final sign = n > 0 ? '+' : '';
  return '$sign$n"';
}

String formatRigidRms(double arcsec) {
  if (arcsec >= 60) return "${(arcsec / 60).toStringAsFixed(1)}'";
  return '${arcsec.round()}"';
}

/// Head geometry the way the hand controller prints it. Empty when the fit
/// separated none of CH, NP, and ID.
String formatRigidHead({
  required String mask,
  double? ch,
  double? np,
  double? id,
  int? sideIn,
  int? sideOut,
}) {
  if (!mask.contains('C') && !mask.contains('P') && !mask.contains('I')) {
    return '';
  }
  String term(String letter, String label, double? arcsec) {
    if (mask.contains(letter)) {
      if (arcsec == null) return '$label ?';
      return '$label ${formatSignedArcsec(arcsec)}';
    }
    if (letter == 'C' &&
        sideIn != null &&
        sideOut != null &&
        (sideIn < 3 || sideOut < 3)) {
      return 'CH n/a $sideIn+$sideOut';
    }
    return '$label n/a';
  }

  return '${term('C', 'CH', ch)}\n${term('P', 'NP', np)}\n${term('I', 'ID', id)}';
}

String formatAlignResult({
  double? rmsArcsec,
  String? twoStarError,
  String mask = '',
  double? ch,
  double? np,
  double? id,
  int? sideIn,
  int? sideOut,
  String? ma,
  String? me,
}) {
  final lines = <String>[];
  if (rmsArcsec != null && rmsArcsec > 0) {
    lines.add('RMS ${formatRigidRms(rmsArcsec)}');
  } else if (twoStarError != null && twoStarError.isNotEmpty) {
    lines.add('Error: $twoStarError');
  }
  final head = formatRigidHead(
    mask: mask,
    ch: ch,
    np: np,
    id: id,
    sideIn: sideIn,
    sideOut: sideOut,
  );
  if (head.isNotEmpty) lines.add(head);
  if (rmsArcsec != null &&
      rmsArcsec > 0 &&
      ma != null &&
      ma.isNotEmpty &&
      me != null &&
      me.isNotEmpty) {
    lines.add('MA $ma   ME $me');
  }
  return lines.join('\n');
}

class AlignmentScreen extends ConsumerStatefulWidget {
  const AlignmentScreen({super.key});
  @override
  ConsumerState<AlignmentScreen> createState() => _AlignmentScreenState();
}

class _AlignmentScreenState extends ConsumerState<AlignmentScreen> {
  AlignKind _kind = AlignKind.two;
  AlignUi _ui = AlignUi.idle;
  int _star = 1;
  List<CatalogEntry?> _stars = [];
  String _error = '';
  String _note = '';
  String _alignResult = '';
  Timer? _slewPoll;
  bool _slewReturnsToBolt = false;

  bool _mountErrorOn = false;
  bool _mountErrorBusy = false;
  bool _accepting = false;
  final _chController = TextEditingController();
  final _npController = TextEditingController();

  @override
  void initState() {
    super.initState();
    _restoreFromMountState();
    WidgetsBinding.instance.addPostFrameCallback((_) => _loadMountError());
  }

  @override
  void dispose() {
    _slewPoll?.cancel();
    _chController.dispose();
    _npController.dispose();
    super.dispose();
  }

  void _restoreFromMountState() {
    final state = ref.read(mountStateProvider);
    if (!state.alignmentInProgress) {
      AlignSessionMemory.kind = null;
      return;
    }

    final reported = state.alignStarNum.clamp(1, 9);
    final remembered = AlignSessionMemory.kind;
    final boltPending = remembered == AlignKind.mechanical &&
        state.alignPhase == AlignPhase.recenter &&
        reported >= 3;
    var kind = remembered;
    if (kind == null || (!boltPending && reported > alignStarCount(kind))) {
      kind = reported >= 5
          ? AlignKind.six
          : reported >= 3
              ? AlignKind.four
              : AlignKind.two;
    }
    if (!alignKindOnAltAz(kind) && state.isAltAz) {
      kind = reported > 2 ? AlignKind.four : AlignKind.two;
    }

    final bolt = kind == AlignKind.mechanical &&
        state.alignPhase == AlignPhase.recenter &&
        reported >= 3;
    final AlignUi ui;
    if (bolt) {
      ui = AlignUi.bolt;
    } else if (state.alignPhase == AlignPhase.select) {
      ui = AlignUi.select;
    } else if (state.alignPhase == AlignPhase.slew) {
      ui = AlignUi.slewing;
    } else {
      ui = AlignUi.recenter;
    }

    _kind = kind;
    _ui = ui;
    _star = bolt ? 2 : reported.clamp(1, alignStarCount(kind));
    _stars = List<CatalogEntry?>.filled(alignStarCount(kind), null);
    _note = ui == AlignUi.bolt
        ? 'Adjust the polar bolts, then slew back onto the star.'
        : _noteFor(_star, state.isAltAz);
    AlignSessionMemory.kind = kind;
    if (ui == AlignUi.slewing) {
      _slewReturnsToBolt = false;
      _startSlewMonitor();
    }
  }

  LX200TcpClient get _client => ref.read(lx200ClientProvider);

  AlignKind _effectiveKind(MountState state) {
    if (state.isAltAz && !alignKindOnAltAz(_kind)) return AlignKind.two;
    return _kind;
  }

  String _noteFor(int star, bool altAz) {
    if (altAz) {
      if (_kind == AlignKind.four && star > 1) {
        return 'Stay in the same part of the sky.';
      }
      return '';
    }
    if (_kind == AlignKind.six && star == 4) {
      return 'Flip to the other pier side, then pick a star there.';
    }
    if ((_kind == AlignKind.four || _kind == AlignKind.six) &&
        star > 1 &&
        star <= (_kind == AlignKind.six ? 3 : 4)) {
      return 'Stay on this pier side.';
    }
    if (_kind == AlignKind.six && star > 4) {
      return 'Stay on the pier side you flipped to.';
    }
    return '';
  }

  Future<void> _loadMountError({bool force = false}) async {
    if (!_client.isConnected) return;
    final use = await _client.sendCommand(LX200.getKnownGeomUse);
    final ch = await _client.sendCommand(LX200.getKnownCone);
    final np = await _client.sendCommand(LX200.getKnownPerp);
    if (!mounted) return;
    setState(() {
      if (use == '0' || use == '1') _mountErrorOn = use == '1';
      if (force || _chController.text.isEmpty) {
        final v = double.tryParse(ch ?? '');
        if (v != null) _chController.text = (v / 3600).toStringAsFixed(3);
      }
      if (force || _npController.text.isEmpty) {
        final v = double.tryParse(np ?? '');
        if (v != null) _npController.text = (v / 3600).toStringAsFixed(3);
      }
    });
  }

  Future<void> _setMountError(bool on) async {
    setState(() => _mountErrorBusy = true);
    final ok = await _client.sendBool(LX200.setKnownGeomUse(on));
    if (!mounted) return;
    setState(() {
      _mountErrorBusy = false;
      if (ok) {
        _mountErrorOn = on;
        _error = '';
      } else {
        _error = 'Could not set mount error';
      }
    });
  }

  Future<void> _uploadMountError() async {
    final ch = double.tryParse(_chController.text.trim().replaceAll(',', '.'));
    final np = double.tryParse(_npController.text.trim().replaceAll(',', '.'));
    if (ch == null || np == null || ch.abs() > 5 || np.abs() > 5) {
      setState(() => _error = 'CH and NP are degrees, from -5 to +5');
      return;
    }
    setState(() {
      _mountErrorBusy = true;
      _error = '';
    });
    final chOk = await _client.sendBool(LX200.setKnownConeDeg(ch));
    final npOk = await _client.sendBool(LX200.setKnownPerpDeg(np));
    if (!mounted) return;
    setState(() => _mountErrorBusy = false);
    if (!mounted) return;
    ScaffoldMessenger.of(context).showSnackBar(SnackBar(
      content: Text(chOk && npOk ? 'Mount error stored' : 'Mount error rejected'),
    ));
    if (!(chOk && npOk)) {
      setState(() => _error = 'Mount rejected CH or NP');
    }
  }

  Future<void> _startAlignment() async {
    final kind = _effectiveKind(ref.read(mountStateProvider));
    final ok = await _client.sendBool(alignStartCommand(kind));
    if (!ok) {
      setState(() => _error = 'Failed to start alignment (${alignStartCommand(kind)})');
      return;
    }
    AlignSessionMemory.kind = kind;
    setState(() {
      _kind = kind;
      _ui = AlignUi.select;
      _star = 1;
      _stars = List<CatalogEntry?>.filled(alignStarCount(kind), null);
      _error = '';
      _note = 'The mount must be at the home position.';
      _alignResult = '';
    });
  }

  void _onStarSelected(CatalogEntry star) {
    if (_ui != AlignUi.select) return;
    setState(() {
      _ensureStars();
      _stars[_star - 1] = star;
      _ui = AlignUi.confirm;
      _error = '';
    });
  }

  void _ensureStars() {
    final n = alignStarCount(_kind);
    if (_stars.length != n) {
      _stars = List<CatalogEntry?>.filled(n, null);
    }
  }

  void _cancelConfirm() {
    setState(() {
      _ensureStars();
      _stars[_star - 1] = null;
      _ui = AlignUi.select;
      _error = '';
    });
  }

  CatalogEntry? get _currentStar {
    if (_star < 1 || _star > _stars.length) return null;
    return _stars[_star - 1];
  }

  Future<void> _gotoStar(CatalogEntry star) async {
    setState(() => _ui = AlignUi.slewing);

    if (star.name.isNotEmpty) {
      await _client.sendBool(LX200.setAlignStarName(star.name));
      await Future.delayed(const Duration(milliseconds: 60));
    }

    final jd = julianDate(DateTime.now().toUtc());
    final (raStr, decStr) = j2000ToJNowLx200(star.ra, star.dec, jd);
    final raOk = await _client.sendBool(LX200.setTargetRa(raStr));
    await Future.delayed(const Duration(milliseconds: 60));
    final decOk = await _client.sendBool(
        LX200.setTargetDec(decStr.replaceFirst(':', '*')));
    await Future.delayed(const Duration(milliseconds: 60));

    if (!raOk || !decOk) {
      if (mounted) {
        setState(() {
          _error = 'Failed to set target coordinates';
          _ui = AlignUi.confirm;
        });
      }
      return;
    }

    final slewResult = await _client.sendCommand(LX200.gotoTarget);
    if (slewResult == null || slewResult.isEmpty || slewResult != '0') {
      final errIdx = int.tryParse(slewResult ?? '') ?? -1;
      final cause = errIdx >= 0 && errIdx < GotoError.values.length
          ? gotoErrorCause(GotoError.values[errIdx])
          : 'Goto rejected (${slewResult ?? 'no reply'})';
      if (mounted) {
        setState(() {
          _error = cause;
          _ui = AlignUi.confirm;
        });
      }
      return;
    }

    _slewReturnsToBolt = false;
    _startSlewMonitor();
  }

  Future<void> _slewToCurrent() async {
    setState(() => _ui = AlignUi.slewing);
    final slewResult = await _client.sendCommand(LX200.gotoPolarAlignCurrent);
    if (slewResult == null || slewResult.isEmpty || slewResult != '0') {
      if (!mounted) return;
      setState(() {
        _error = 'Goto current rejected: ${slewResult ?? 'no reply'}';
        _ui = AlignUi.bolt;
      });
      return;
    }
    _slewReturnsToBolt = true;
    _startSlewMonitor();
  }

  void _startSlewMonitor() {
    _slewPoll?.cancel();
    final started = DateTime.now();
    final backToBolt = _slewReturnsToBolt;
    _slewPoll = Timer.periodic(const Duration(milliseconds: 500), (_) {
      if (DateTime.now().difference(started) < const Duration(milliseconds: 1500)) {
        return;
      }
      final state = ref.read(mountStateProvider);
      if (!state.isSlewing) {
        _slewPoll?.cancel();
        if (mounted) {
          setState(() => _ui = backToBolt ? AlignUi.bolt : AlignUi.recenter);
        }
      }
    });
  }

  Future<void> _acceptStar() async {
    if (_accepting) return;
    final bolt = _ui == AlignUi.bolt;
    final cmd = bolt ? LX200.alignPolarFinalize : LX200.alignSelectStar(_star);
    final slow = !bolt &&
        _star >= 3 &&
        (_kind == AlignKind.four || _kind == AlignKind.six);
    setState(() => _accepting = true);
    try {
      final reply = await _client.sendCommand(
        cmd,
        timeout: slow ? const Duration(seconds: 20) : null,
      );
      if (!mounted) return;
      if (reply != '1') {
        setState(() => _error = bolt
            ? 'Failed to finish the pole pass'
            : 'Failed to accept star $_star');
        return;
      }

      if (_kind == AlignKind.mechanical) {
        if (bolt) {
          setState(() {
            _ui = AlignUi.done;
            _alignResult = 'Mechanical pole done. Soft model cleared, '
                'EEPROM alignment cleared.';
            _error = '';
            _note = '';
          });
          AlignSessionMemory.kind = null;
          return;
        }
        if (_star < 2) {
          setState(() {
            _star = 2;
            _ui = AlignUi.select;
            _error = '';
            _note = '';
          });
          return;
        }
        setState(() {
          _ui = AlignUi.bolt;
          _error = '';
          _note = 'Adjust the polar bolts, then slew back onto the star.';
        });
        return;
      }

      if (_star < alignStarCount(_kind)) {
        final next = _star + 1;
        setState(() {
          _star = next;
          _ui = AlignUi.select;
          _error = '';
          _note = _noteFor(next, ref.read(mountStateProvider).isAltAz);
        });
        return;
      }

      final result = await _readAlignResult();
      if (!mounted) return;
      setState(() {
        _ui = AlignUi.done;
        _alignResult = result;
        _error = '';
        _note = '';
      });
      AlignSessionMemory.kind = null;
    } finally {
      if (mounted) setState(() => _accepting = false);
    }
  }

  Future<String> _readAlignResult() async {
    if (_kind == AlignKind.two) {
      final err = await _client.sendCommand(LX200.getAlignError);
      return 'Error: ${err ?? 'unknown'}';
    }
    final rms = double.tryParse(
        (await _client.sendCommand(LX200.getAlignRigidRms) ?? '').trim());
    final mask = (await _client.sendCommand(LX200.getAlignFittedTerms) ?? '').trim();
    double? ch;
    double? np;
    double? id;
    int? sideIn;
    int? sideOut;
    if (mask.contains(RegExp('[CPI]'))) {
      ch = double.tryParse(
          (await _client.sendCommand(LX200.getAlignHeadCone) ?? '').trim());
      np = double.tryParse(
          (await _client.sendCommand(LX200.getAlignHeadPerp) ?? '').trim());
      id = double.tryParse(
          (await _client.sendCommand(LX200.getAlignHeadIndex) ?? '').trim());
      final sides = (await _client.sendCommand(LX200.getAlignPierSides) ?? '').trim();
      final parts = sides.split(',');
      if (parts.length == 2) {
        sideIn = int.tryParse(parts[0]);
        sideOut = int.tryParse(parts[1]);
      }
    }
    String? twoStar;
    if (rms == null || rms <= 0) {
      twoStar = await _client.sendCommand(LX200.getAlignError);
    }
    final ma = rms != null && rms > 0
        ? await _client.sendCommand(LX200.getAlignErrorAz)
        : null;
    final me = rms != null && rms > 0
        ? await _client.sendCommand(LX200.getAlignErrorAlt)
        : null;
    final text = formatAlignResult(
      rmsArcsec: rms,
      twoStarError: twoStar,
      mask: mask,
      ch: ch,
      np: np,
      id: id,
      sideIn: sideIn,
      sideOut: sideOut,
      ma: ma,
      me: me,
    );
    return text.isEmpty ? 'Alignment complete' : text;
  }

  Future<void> _saveAlignment() async {
    final ok = await _client.sendBool(LX200.alignSave);
    if (!mounted) return;
    setState(() {
      _alignResult += ok ? '\nSaved to EEPROM' : '\nSave failed';
    });
    if (ok) await _loadMountError(force: true);
  }

  void _reset() {
    _slewPoll?.cancel();
    AlignSessionMemory.kind = null;
    setState(() {
      _ui = AlignUi.idle;
      _star = 1;
      _stars = [];
      _error = '';
      _note = '';
      _alignResult = '';
    });
  }

  Future<void> _abortAlignment() async {
    if (_ui == AlignUi.slewing) await _client.send(LX200.stopAll);
    await _client.sendBool(LX200.alignAbort);
    _slewPoll?.cancel();
    if (mounted) _reset();
  }

  String get _overlayMessage {
    final name = _currentStar?.name ?? 'star';
    final extra = _note.isEmpty ? '' : '\n$_note';
    return switch (_ui) {
      AlignUi.idle => '',
      AlignUi.select => 'Tap a bright star for star $_star$extra',
      AlignUi.confirm => 'Slew to $name?',
      AlignUi.slewing => 'Slewing to $name...',
      AlignUi.recenter => 'Recenter $name in the eyepiece',
      AlignUi.bolt => _note,
      AlignUi.done => 'Alignment complete!',
    };
  }

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(mountStateProvider);
    final overlayMsg = _ui == AlignUi.idle && state.aligned
        ? 'Alignment already complete'
        : (_overlayMessage.isNotEmpty ? _overlayMessage : null);

    return Column(
      children: [
        _StepBar(ui: _ui, kind: _kind, star: _star, stars: _stars),
        if (state.alignmentInProgress)
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
            color: TA.surfaceVariant,
            child: Row(
              children: [
                Icon(Icons.settings_input_antenna, size: 14, color: TA.accent),
                const SizedBox(width: 6),
                Text('Mount: ${state.alignPhaseLabel}',
                    style: TextStyle(color: TA.text, fontSize: 11)),
              ],
            ),
          ),
        Consumer(
          builder: (_, ref, __) {
            final settings = ref.watch(planetariumSettingsProvider);
            final notifier = ref.read(planetariumSettingsProvider.notifier);
            final starScale = settings.starScale.clamp(1.0, 4.0);
            return Container(
              padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
              color: TA.surface.withValues(alpha: 0.6),
              child: Row(
                children: [
                  Text('Star size',
                      style: TextStyle(color: TA.textSecondary, fontSize: 11)),
                  const SizedBox(width: 6),
                  SizedBox(
                    width: 80,
                    height: 24,
                    child: Slider(
                      value: starScale,
                      min: 1.0,
                      max: 4.0,
                      divisions: 30,
                      onChanged: (v) => notifier.setStarScale(v),
                    ),
                  ),
                  Text(starScale.toStringAsFixed(1),
                      style: TextStyle(
                          color: TA.text, fontSize: 11, fontFamily: 'monospace')),
                ],
              ),
            );
          },
        ),
        Expanded(
          child: Stack(
            children: [
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 4),
                child: LayoutBuilder(
                  builder: (context, constraints) => SkyMapWidget(
                    height: constraints.maxHeight,
                    onStarSelected: _ui == AlignUi.select ? _onStarSelected : null,
                    overlayMessage: overlayMsg,
                    highlightedStar: _ui == AlignUi.select ? null : _currentStar,
                    maxStarScale: 4.0,
                  ),
                ),
              ),
              if (_ui == AlignUi.slewing)
                Positioned.fill(
                  child: Container(
                    color: TA.background.withValues(alpha: 0.85),
                    child: Center(
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Text(
                            'Slewing',
                            style: TextStyle(
                              color: TA.warning,
                              fontSize: 22,
                              fontWeight: FontWeight.w700,
                            ),
                          ),
                          const SizedBox(height: 16),
                          SizedBox(
                            width: 40,
                            height: 40,
                            child: CircularProgressIndicator(
                              strokeWidth: 3,
                              color: TA.warning,
                            ),
                          ),
                          const SizedBox(height: 20),
                          ElevatedButton.icon(
                            onPressed: _abortAlignment,
                            icon: const Icon(Icons.stop, size: 18),
                            label: const Text('Abort'),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: TA.error,
                              foregroundColor: TA.textHigh,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ),
            ],
          ),
        ),
        if (_error.isNotEmpty)
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
            child: Text(_error, style: TextStyle(color: TA.error, fontSize: 11)),
          ),
        Padding(
          padding: const EdgeInsets.all(6),
          child: _buildActionBar(state),
        ),
      ],
    );
  }

  Widget _buildActionBar(MountState state) {
    switch (_ui) {
      case AlignUi.idle:
        return Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            if (state.aligned) ...[
              Row(
                children: [
                  Icon(Icons.check_circle, color: TA.success, size: 20),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      'Alignment already complete.',
                      style: TextStyle(
                          color: TA.success,
                          fontSize: 13,
                          fontWeight: FontWeight.w600),
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              Row(
                children: [
                  _SmallBtn(Icons.save, 'Save to EEPROM', () async {
                    final ok = await _client.sendBool(LX200.alignSave);
                    if (ok) await _loadMountError(force: true);
                    if (mounted) {
                      ScaffoldMessenger.of(context).showSnackBar(SnackBar(
                          content: Text(ok ? 'Saved' : 'Save failed')));
                    }
                  }),
                  const SizedBox(width: 8),
                  _SmallBtn(Icons.delete_outline, 'Remove alignment', () async {
                    await _client.sendBool(LX200.alignClear);
                    if (mounted) _reset();
                  }),
                ],
              ),
            ] else ...[
              _modePicker(state),
              const SizedBox(height: 6),
              ElevatedButton.icon(
                onPressed: _startAlignment,
                icon: const Icon(Icons.play_arrow, size: 18),
                label: Text('Start ${alignKindLabel(_effectiveKind(state))}'),
              ),
            ],
            const SizedBox(height: 8),
            _mountErrorEditor(),
          ],
        );

      case AlignUi.select:
        return Row(
          children: [
            Icon(Icons.touch_app, color: TA.accent, size: 18),
            const SizedBox(width: 6),
            Expanded(
              child: Text(
                _note.isEmpty
                    ? 'Tap a bright named star on the map'
                    : _note,
                style: TextStyle(color: TA.text, fontSize: 12),
              ),
            ),
            _SmallBtn(Icons.close, 'Abort alignment', _abortAlignment),
          ],
        );

      case AlignUi.confirm:
        return Row(
          children: [
            Icon(Icons.star, color: TA.warning, size: 18),
            const SizedBox(width: 6),
            Expanded(
              child: Text(
                'Slew to ${_currentStar?.name ?? 'star'}?',
                style: TextStyle(color: TA.text, fontSize: 12),
              ),
            ),
            _SmallBtn(Icons.arrow_back, 'Back', _cancelConfirm),
            const SizedBox(width: 6),
            ElevatedButton.icon(
              onPressed: () {
                final star = _currentStar;
                if (star != null) _gotoStar(star);
              },
              icon: const Icon(Icons.navigation, size: 16),
              label: const Text('Slew'),
              style: ElevatedButton.styleFrom(
                backgroundColor: TA.accent,
                padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
              ),
            ),
            const SizedBox(width: 6),
            _SmallBtn(Icons.close, 'Abort alignment', _abortAlignment),
          ],
        );

      case AlignUi.slewing:
        return Row(
          children: [
            Text('Slewing',
                style: TextStyle(
                    color: TA.warning, fontSize: 12, fontWeight: FontWeight.w600)),
            const Spacer(),
            _SmallBtn(Icons.stop, 'Abort', _abortAlignment),
          ],
        );

      case AlignUi.recenter:
        return Row(
          children: [
            Icon(Icons.center_focus_strong, color: TA.success, size: 18),
            const SizedBox(width: 6),
            Expanded(
              child: Text(
                'Recenter ${_currentStar?.name ?? 'the star'} in the eyepiece, then accept',
                style: TextStyle(color: TA.text, fontSize: 12),
              ),
            ),
            _SmallBtn(Icons.close, 'Abort alignment', _abortAlignment),
            const SizedBox(width: 8),
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                backgroundColor: TA.success,
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
              ),
              onPressed: _accepting ? null : _acceptStar,
              child: Text(
                _accepting &&
                        _star >= 3 &&
                        (_kind == AlignKind.four || _kind == AlignKind.six)
                    ? 'Fitting…'
                    : 'Accept',
                style: const TextStyle(fontWeight: FontWeight.w700),
              ),
            ),
          ],
        );

      case AlignUi.bolt:
        return Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Text(
              'Adjust the polar bolts, slew back onto the star, then finish.',
              style: TextStyle(color: TA.text, fontSize: 12),
            ),
            const SizedBox(height: 6),
            Row(
              children: [
                _SmallBtn(Icons.my_location, 'Slew to star', _slewToCurrent),
                const SizedBox(width: 8),
                ElevatedButton(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: TA.success,
                    padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                  ),
                  onPressed: _accepting ? null : _acceptStar,
                  child: const Text('Finish',
                      style: TextStyle(fontWeight: FontWeight.w700)),
                ),
                const Spacer(),
                _SmallBtn(Icons.close, 'Abort alignment', _abortAlignment),
              ],
            ),
          ],
        );

      case AlignUi.done:
        final headline = _kind == AlignKind.mechanical
            ? _alignResult
            : 'Alignment complete!\n$_alignResult';
        return Column(
          children: [
            Row(
              children: [
                Icon(Icons.check_circle, color: TA.success, size: 20),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    headline,
                    style: TextStyle(
                        color: TA.success, fontSize: 12, fontWeight: FontWeight.w600),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 6),
            if (_kind == AlignKind.mechanical)
              ElevatedButton.icon(
                onPressed: _reset,
                icon: const Icon(Icons.check, size: 16),
                label: const Text('Done'),
              )
            else
              Row(
                children: [
                  Expanded(
                    child: ElevatedButton.icon(
                      onPressed: _saveAlignment,
                      icon: const Icon(Icons.save, size: 16),
                      label: const Text('Save to EEPROM'),
                    ),
                  ),
                  const SizedBox(width: 8),
                  _SmallBtn(Icons.delete_outline, 'Remove alignment', () async {
                    await _client.sendBool(LX200.alignClear);
                    if (mounted) _reset();
                  }),
                ],
              ),
          ],
        );
    }
  }

  Widget _modePicker(MountState state) {
    final kinds = AlignKind.values
        .where((k) => !state.isAltAz || alignKindOnAltAz(k))
        .toList();
    final selected = _effectiveKind(state);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Wrap(
          spacing: 6,
          runSpacing: 4,
          children: [
            for (final kind in kinds)
              ChoiceChip(
                label: Text(alignKindLabel(kind),
                    style: const TextStyle(fontSize: 12)),
                selected: selected == kind,
                visualDensity: VisualDensity.compact,
                onSelected: (_) => setState(() => _kind = kind),
              ),
          ],
        ),
        const SizedBox(height: 4),
        Text(alignKindBlurb(selected),
            style: TextStyle(color: TA.textSecondary, fontSize: 11)),
      ],
    );
  }

  Widget _mountErrorEditor() {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 6),
      decoration: BoxDecoration(
        color: TA.surface,
        borderRadius: BorderRadius.circular(8),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Wrap(
            crossAxisAlignment: WrapCrossAlignment.center,
            spacing: 8,
            runSpacing: 4,
            children: [
              Text('Mount error',
                  style: TextStyle(
                      color: TA.text, fontSize: 13, fontWeight: FontWeight.w600)),
              SegmentedButton<bool>(
                segments: const [
                  ButtonSegment(value: false, label: Text('Off')),
                  ButtonSegment(value: true, label: Text('On')),
                ],
                selected: {_mountErrorOn},
                onSelectionChanged: _mountErrorBusy
                    ? null
                    : (s) => _setMountError(s.first),
              ),
            ],
          ),
          const SizedBox(height: 2),
          Text(
            'Hold CH and NP during a 2-star alignment. Signs are Wallace\'s. '
            '4 Stars and 3+3 measure these and do not read them.',
            style: TextStyle(color: TA.textSecondary, fontSize: 11),
          ),
          if (_mountErrorOn) ...[
            const SizedBox(height: 6),
            Wrap(
              spacing: 8,
              runSpacing: 6,
              crossAxisAlignment: WrapCrossAlignment.center,
              children: [
                _degField('CH', _chController),
                _degField('NP', _npController),
                _SmallBtn(
                  Icons.upload,
                  'Upload',
                  _mountErrorBusy ? () {} : _uploadMountError,
                ),
              ],
            ),
          ],
        ],
      ),
    );
  }

  Widget _degField(String label, TextEditingController controller) {
    return SizedBox(
      width: 108,
      child: TextField(
        controller: controller,
        enabled: !_mountErrorBusy,
        keyboardType:
            const TextInputType.numberWithOptions(decimal: true, signed: true),
        style: TextStyle(color: TA.text, fontSize: 13, fontFamily: 'monospace'),
        decoration: InputDecoration(
          isDense: true,
          labelText: '$label °',
          labelStyle: TextStyle(color: TA.textSecondary, fontSize: 12),
        ),
      ),
    );
  }
}

class _StepBar extends StatelessWidget {
  final AlignUi ui;
  final AlignKind kind;
  final int star;
  final List<CatalogEntry?> stars;
  const _StepBar({
    required this.ui,
    required this.kind,
    required this.star,
    required this.stars,
  });

  @override
  Widget build(BuildContext context) {
    final total = alignStarCount(kind);
    final done = ui == AlignUi.done;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      color: TA.surface,
      child: Row(
        children: [
          for (var n = 1; n <= total; n++) ...[
            _dot(n, _active(n), _complete(n, total)),
            if (n < total) _line(_complete(n, total)),
          ],
          if (kind == AlignKind.mechanical) ...[
            _line(ui == AlignUi.bolt || done),
            Icon(
              Icons.build,
              size: 14,
              color: ui == AlignUi.bolt || done ? TA.accent : TA.textSecondary,
            ),
          ],
          _line(done),
          Icon(
            done ? Icons.check_circle : Icons.flag,
            size: 14,
            color: done ? TA.success : TA.textSecondary,
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              _caption(total),
              overflow: TextOverflow.ellipsis,
              style: TextStyle(
                  color: TA.textHigh, fontSize: 10, fontFamily: 'monospace'),
            ),
          ),
        ],
      ),
    );
  }

  bool _active(int n) =>
      ui != AlignUi.idle &&
      ui != AlignUi.done &&
      ui != AlignUi.bolt &&
      n == star;

  bool _complete(int n, int total) {
    if (ui == AlignUi.done) return true;
    if (ui == AlignUi.bolt) return n <= total;
    if (ui == AlignUi.idle) return false;
    return n < star;
  }

  String _caption(int total) {
    if (ui == AlignUi.idle) return '';
    if (ui == AlignUi.done) return 'Done';
    if (ui == AlignUi.bolt) return 'Pole bolts';
    final name = star >= 1 && star <= stars.length ? stars[star - 1]?.name : null;
    final who = name == null || name.isEmpty ? '' : '  $name';
    return 'Star $star/$total$who';
  }

  Widget _dot(int num, bool active, bool complete) {
    return Container(
      width: 18,
      height: 18,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: complete
            ? TA.success
            : active
                ? TA.accent
                : TA.surfaceVariant,
      ),
      child: Center(
        child: complete
            ? Icon(Icons.check, size: 12, color: TA.textHigh)
            : Text('$num',
                style: TextStyle(
                    color: active ? TA.textHigh : TA.textSecondary,
                    fontSize: 10,
                    fontWeight: FontWeight.w700)),
      ),
    );
  }

  Widget _line(bool active) {
    return Container(
      width: 12,
      height: 2,
      color: active ? TA.success : TA.surfaceVariant,
    );
  }
}

class _SmallBtn extends StatelessWidget {
  final IconData icon;
  final String label;
  final VoidCallback onTap;
  const _SmallBtn(this.icon, this.label, this.onTap);

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(left: 4),
      child: SizedBox(
        height: 32,
        child: ElevatedButton(
          style: ElevatedButton.styleFrom(
            padding: const EdgeInsets.symmetric(horizontal: 10),
            visualDensity: VisualDensity.compact,
            backgroundColor: TA.surfaceVariant,
            textStyle: const TextStyle(fontSize: 11),
          ),
          onPressed: onTap,
          child: Row(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(icon, size: 14),
              const SizedBox(width: 3),
              Text(label),
            ],
          ),
        ),
      ),
    );
  }
}
