import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../models/lx200_commands.dart';
import '../models/mount_state.dart';
import '../models/catalog_entry.dart';
import '../theme.dart';
import '../widgets/sky_map.dart';

enum AlignStep {
  idle,
  selectStar1,
  confirmStar1,
  slewingStar1,
  recenterStar1,
  selectStar2,
  confirmStar2,
  slewingStar2,
  recenterStar2,
  selectStar3,
  confirmStar3,
  slewingStar3,
  recenterStar3,
  done,
}

class AlignmentScreen extends ConsumerStatefulWidget {
  const AlignmentScreen({super.key});
  @override
  ConsumerState<AlignmentScreen> createState() => _AlignmentScreenState();
}

class _AlignmentScreenState extends ConsumerState<AlignmentScreen> {
  AlignStep _step = AlignStep.idle;
  int _numStars = 2; // 2 or 3 (OnStepX-style multi-star)
  CatalogEntry? _star1;
  CatalogEntry? _star2;
  CatalogEntry? _star3;
  String _error = '';
  String _alignResult = '';
  Timer? _slewPoll;

  @override
  void dispose() {
    _slewPoll?.cancel();
    super.dispose();
  }

  LX200TcpClient get _client => ref.read(lx200ClientProvider);

  // ── Alignment flow ──────────────────────────────────────

  Future<void> _startAlignment() async {
    final ok = await _client.sendBool(LX200.alignStart);
    if (!ok) {
      setState(() => _error = 'Failed to start alignment (:A0#)');
      return;
    }
    setState(() {
      _step = AlignStep.selectStar1;
      _star1 = null;
      _star2 = null;
      _star3 = null;
      _error = '';
      _alignResult = '';
    });
  }

  void _onStarSelected(CatalogEntry star) {
    if (_step == AlignStep.selectStar1) {
      setState(() {
        _star1 = star;
        _step = AlignStep.confirmStar1;
        _error = '';
      });
    } else if (_step == AlignStep.selectStar2) {
      setState(() {
        _star2 = star;
        _step = AlignStep.confirmStar2;
        _error = '';
      });
    } else if (_step == AlignStep.selectStar3) {
      setState(() {
        _star3 = star;
        _step = AlignStep.confirmStar3;
        _error = '';
      });
    }
  }

  void _cancelConfirm(int starNum) {
    setState(() {
      if (starNum == 1) {
        _star1 = null;
        _step = AlignStep.selectStar1;
      } else if (starNum == 2) {
        _star2 = null;
        _step = AlignStep.selectStar2;
      } else {
        _star3 = null;
        _step = AlignStep.selectStar3;
      }
      _error = '';
    });
  }

  void _confirmAndSlew(int starNum) {
    final star = starNum == 1 ? _star1 : starNum == 2 ? _star2 : _star3;
    if (star != null) _gotoStar(star, starNum);
  }

  Future<void> _gotoStar(CatalogEntry star, int starNum) async {
    setState(() {
      _step = starNum == 1
          ? AlignStep.slewingStar1
          : starNum == 2
              ? AlignStep.slewingStar2
              : AlignStep.slewingStar3;
    });

    final raOk =
        await _client.sendBool(LX200.setTargetRa(star.raStr));
    await Future.delayed(const Duration(milliseconds: 60));
    final decOk =
        await _client.sendBool(LX200.setTargetDec(star.decStr.replaceFirst(':', '*')));
    await Future.delayed(const Duration(milliseconds: 60));

    if (!raOk || !decOk) {
      setState(() => _error = 'Failed to set target coordinates');
      return;
    }

    final slewResult = await _client.sendCommand(LX200.gotoTarget);
    if (slewResult != '0' && slewResult != null && slewResult.isNotEmpty) {
      final errIdx = int.tryParse(slewResult) ?? -1;
      final cause = errIdx >= 0 && errIdx < GotoError.values.length
          ? gotoErrorCause(GotoError.values[errIdx])
          : 'Goto rejected ($slewResult)';
      if (mounted) {
        setState(() => _error = cause);
      }
      return;
    }

    _startSlewMonitor(starNum);
  }

  void _startSlewMonitor(int starNum) {
    _slewPoll?.cancel();
    _slewPoll = Timer.periodic(const Duration(seconds: 1), (_) {
      final state = ref.read(mountStateProvider);
      if (!state.isSlewing) {
        _slewPoll?.cancel();
        if (mounted) {
          setState(() {
            _step = starNum == 1
                ? AlignStep.recenterStar1
                : starNum == 2
                    ? AlignStep.recenterStar2
                    : AlignStep.recenterStar3;
          });
        }
      }
    });
  }

  Future<void> _acceptStar(int starNum) async {
    // OnStepX-style: :A1# first star, :A2# second, :A3# third
    final ok = await _client.sendBool(LX200.alignAddStar(starNum));
    if (!ok) {
      setState(() => _error = 'Failed to accept star $starNum');
      return;
    }

    if (starNum < _numStars) {
      setState(() {
        _step = starNum == 1
            ? AlignStep.selectStar2
            : AlignStep.selectStar3;
        _error = '';
      });
    } else {
      // Alignment complete - fetch error
      await Future.delayed(const Duration(milliseconds: 200));
      final errStr = await _client.sendCommand(LX200.getAlignError);
      setState(() {
        _step = AlignStep.done;
        _alignResult = errStr ?? 'unknown';
      });
    }
  }

  Future<void> _saveAlignment() async {
    final ok = await _client.sendBool(LX200.alignSave);
    setState(() {
      _alignResult += ok ? '\nSaved to EEPROM' : '\nSave failed';
    });
  }

  void _reset() {
    _slewPoll?.cancel();
    setState(() {
      _step = AlignStep.idle;
      _star1 = null;
      _star2 = null;
      _star3 = null;
      _error = '';
      _alignResult = '';
    });
  }

  bool get _isSelectStep =>
      _step == AlignStep.selectStar1 ||
      _step == AlignStep.selectStar2 ||
      _step == AlignStep.selectStar3;

  bool get _isSlewingStep =>
      _step == AlignStep.slewingStar1 ||
      _step == AlignStep.slewingStar2 ||
      _step == AlignStep.slewingStar3;

  // ── UI ──────────────────────────────────────────────────

  String get _overlayMessage => switch (_step) {
        AlignStep.idle => '',
        AlignStep.selectStar1 => 'Tap a bright star for Star 1',
        AlignStep.confirmStar1 =>
          'Slew to ${_star1?.name ?? "star"}?',
        AlignStep.slewingStar1 =>
          'Slewing to ${_star1?.name ?? "star"}...',
        AlignStep.recenterStar1 =>
          'Recenter ${_star1?.name ?? "star"} in eyepiece',
        AlignStep.selectStar2 =>
          'Star 1 done! Tap a star for Star 2',
        AlignStep.confirmStar2 =>
          'Slew to ${_star2?.name ?? "star"}?',
        AlignStep.slewingStar2 =>
          'Slewing to ${_star2?.name ?? "star"}...',
        AlignStep.recenterStar2 =>
          'Recenter ${_star2?.name ?? "star"} in eyepiece',
        AlignStep.selectStar3 =>
          'Star 2 done! Tap a star for Star 3',
        AlignStep.confirmStar3 =>
          'Slew to ${_star3?.name ?? "star"}?',
        AlignStep.slewingStar3 =>
          'Slewing to ${_star3?.name ?? "star"}...',
        AlignStep.recenterStar3 =>
          'Recenter ${_star3?.name ?? "star"} in eyepiece',
        AlignStep.done => 'Alignment complete!',
      };

  CatalogEntry? get _currentHighlight => switch (_step) {
        AlignStep.confirmStar1 ||
        AlignStep.slewingStar1 ||
        AlignStep.recenterStar1 =>
          _star1,
        AlignStep.confirmStar2 ||
        AlignStep.slewingStar2 ||
        AlignStep.recenterStar2 =>
          _star2,
        AlignStep.confirmStar3 ||
        AlignStep.slewingStar3 ||
        AlignStep.recenterStar3 =>
          _star3,
        _ => null,
      };

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(mountStateProvider);
    final overlayMsg = _step == AlignStep.idle && state.aligned
        ? 'Alignment already complete'
        : (_overlayMessage.isNotEmpty ? _overlayMessage : null);

    return Column(
      children: [
        // Step indicator bar
        _StepBar(
            step: _step,
            numStars: _numStars,
            star1: _star1,
            star2: _star2,
            star3: _star3),

        // Sky map fills remaining space
        Expanded(
          child: Stack(
            children: [
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 4),
                child: LayoutBuilder(
                  builder: (context, constraints) => SkyMapWidget(
                    height: constraints.maxHeight,
                    onStarSelected: _isSelectStep ? _onStarSelected : null,
                    overlayMessage: overlayMsg,
                    highlightedStar: _currentHighlight,
                  ),
                ),
              ),
              // Slewing overlay: show "Slewing" and Abort
              if (_isSlewingStep)
                Positioned.fill(
                  child: Container(
                    color: TAColors.background.withValues(alpha: 0.85),
                    child: Center(
                      child: Column(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const Text(
                            'Slewing',
                            style: TextStyle(
                              color: TAColors.warning,
                              fontSize: 22,
                              fontWeight: FontWeight.w700,
                            ),
                          ),
                          const SizedBox(height: 16),
                          const SizedBox(
                            width: 40,
                            height: 40,
                            child: CircularProgressIndicator(
                              strokeWidth: 3,
                              color: TAColors.warning,
                            ),
                          ),
                          const SizedBox(height: 20),
                          ElevatedButton.icon(
                            onPressed: () async {
                              await _client.send(LX200.stopAll);
                              _slewPoll?.cancel();
                              if (mounted) _reset();
                            },
                            icon: const Icon(Icons.stop, size: 18),
                            label: const Text('Abort'),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: TAColors.error,
                              foregroundColor: Colors.white,
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

        // Error display
        if (_error.isNotEmpty)
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 2),
            child: Text(_error,
                style: TextStyle(color: TAColors.error, fontSize: 11)),
          ),

        // Action bar
        Padding(
          padding: const EdgeInsets.all(6),
          child: _buildActionBar(state),
        ),
      ],
    );
  }

  Widget _buildActionBar(MountState state) {
    switch (_step) {
      case AlignStep.idle:
        if (state.aligned) {
          return Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Row(
                children: [
                  Icon(Icons.check_circle, color: TAColors.success, size: 20),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Text(
                      'Alignment already complete.',
                      style: TextStyle(
                          color: TAColors.success,
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
            ],
          );
        }
        return Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Row(
              children: [
                const Text('Stars:', style: TextStyle(fontSize: 12)),
                const SizedBox(width: 8),
                SegmentedButton<int>(
                  segments: const [
                    ButtonSegment(value: 2, label: Text('2'), icon: Icon(Icons.star, size: 16)),
                    ButtonSegment(value: 3, label: Text('3'), icon: Icon(Icons.star, size: 16)),
                  ],
                  selected: {_numStars},
                  onSelectionChanged: (s) => setState(() => _numStars = s.first),
                ),
              ],
            ),
            const SizedBox(height: 6),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: _startAlignment,
                    icon: const Icon(Icons.play_arrow, size: 18),
                    label: Text('Start $_numStars-Star Alignment'),
                  ),
                ),
              ],
            ),
          ],
        );

      case AlignStep.selectStar1:
      case AlignStep.selectStar2:
      case AlignStep.selectStar3:
        return Row(
          children: [
            Icon(Icons.touch_app, color: TAColors.accent, size: 18),
            const SizedBox(width: 6),
            Expanded(
              child: Text(
                _step == AlignStep.selectStar1
                    ? 'Tap a bright named star on the map'
                    : 'Tap a star (spread across the sky)',
                style: TextStyle(color: TAColors.text, fontSize: 12),
              ),
            ),
            _SmallBtn(Icons.close, 'Cancel alignment', _reset),
          ],
        );

      case AlignStep.confirmStar1:
      case AlignStep.confirmStar2:
      case AlignStep.confirmStar3:
        final starNum = _step == AlignStep.confirmStar1
            ? 1
            : _step == AlignStep.confirmStar2
                ? 2
                : 3;
        final starName = starNum == 1
            ? _star1?.name
            : starNum == 2
                ? _star2?.name
                : _star3?.name;
        return Row(
          children: [
            Icon(Icons.star, color: TAColors.warning, size: 18),
            const SizedBox(width: 6),
            Expanded(
              child: Text(
                'Slew to $starName?',
                style: TextStyle(color: TAColors.text, fontSize: 12),
              ),
            ),
            _SmallBtn(Icons.arrow_back, 'Back', () => _cancelConfirm(starNum)),
            const SizedBox(width: 6),
            ElevatedButton.icon(
              onPressed: () => _confirmAndSlew(starNum),
              icon: const Icon(Icons.navigation, size: 16),
              label: const Text('Slew'),
              style: ElevatedButton.styleFrom(
                backgroundColor: TAColors.accent,
                padding:
                    const EdgeInsets.symmetric(horizontal: 14, vertical: 8),
              ),
            ),
            const SizedBox(width: 6),
            _SmallBtn(Icons.close, 'Cancel alignment', _reset),
          ],
        );

      case AlignStep.slewingStar1:
      case AlignStep.slewingStar2:
      case AlignStep.slewingStar3:
        return Row(
          children: [
            const Text('Slewing',
                style: TextStyle(
                    color: TAColors.warning,
                    fontSize: 12,
                    fontWeight: FontWeight.w600)),
            const SizedBox(width: 8),
            Expanded(child: const SizedBox.shrink()),
            _SmallBtn(Icons.stop, 'Abort', () async {
              await _client.send(LX200.stopAll);
              _slewPoll?.cancel();
              if (mounted) _reset();
            }),
          ],
        );

      case AlignStep.recenterStar1:
      case AlignStep.recenterStar2:
      case AlignStep.recenterStar3:
        final starNum = _step == AlignStep.recenterStar1
            ? 1
            : _step == AlignStep.recenterStar2
                ? 2
                : 3;
        final starName = starNum == 1
            ? _star1?.name
            : starNum == 2
                ? _star2?.name
                : _star3?.name;
        return Row(
          children: [
            Icon(Icons.center_focus_strong,
                color: TAColors.success, size: 18),
            const SizedBox(width: 6),
            Expanded(
              child: Text(
                'Recenter $starName in eyepiece, then accept',
                style: TextStyle(color: TAColors.text, fontSize: 12),
              ),
            ),
            _SmallBtn(Icons.close, 'Cancel alignment', _reset),
            const SizedBox(width: 8),
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                backgroundColor: TAColors.success,
                padding:
                    const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
              ),
              onPressed: () => _acceptStar(starNum),
              child: const Text('Accept',
                  style: TextStyle(fontWeight: FontWeight.w700)),
            ),
          ],
        );

      case AlignStep.done:
        return Column(
          children: [
            Row(
              children: [
                Icon(Icons.check_circle, color: TAColors.success, size: 20),
                const SizedBox(width: 8),
                Expanded(
                  child: Text(
                    'Alignment complete!  Error: $_alignResult',
                    style: TextStyle(
                        color: TAColors.success,
                        fontSize: 12,
                        fontWeight: FontWeight.w600),
                  ),
                ),
              ],
            ),
            const SizedBox(height: 6),
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
}

class _StepBar extends StatelessWidget {
  final AlignStep step;
  final int numStars;
  final CatalogEntry? star1;
  final CatalogEntry? star2;
  final CatalogEntry? star3;
  const _StepBar(
      {required this.step,
      this.numStars = 2,
      this.star1,
      this.star2,
      this.star3});

  @override
  Widget build(BuildContext context) {
    final pastStar2 = step.index >= AlignStep.selectStar2.index;
    final pastStar3 = step.index >= AlignStep.selectStar3.index;
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      color: TAColors.surface,
      child: Row(
        children: [
          _dot(1, step.index >= AlignStep.selectStar1.index, pastStar2),
          _line(pastStar2),
          _dot(2, pastStar2, numStars == 2 ? step == AlignStep.done : pastStar3),
          if (numStars >= 3) ...[
            _line(pastStar3),
            _dot(3, pastStar3, step == AlignStep.done),
          ],
          _line(step == AlignStep.done),
          Icon(
            step == AlignStep.done ? Icons.check_circle : Icons.flag,
            size: 14,
            color: step == AlignStep.done
                ? TAColors.success
                : TAColors.textSecondary,
          ),
          const SizedBox(width: 8),
          if (star1 != null)
            Text('1: ${star1!.name}  ',
                style: const TextStyle(
                    color: TAColors.textHigh,
                    fontSize: 10,
                    fontFamily: 'monospace')),
          if (star2 != null)
            Text('2: ${star2!.name}  ',
                style: const TextStyle(
                    color: TAColors.textHigh,
                    fontSize: 10,
                    fontFamily: 'monospace')),
          if (star3 != null)
            Text('3: ${star3!.name}',
                style: const TextStyle(
                    color: TAColors.textHigh,
                    fontSize: 10,
                    fontFamily: 'monospace')),
        ],
      ),
    );
  }

  Widget _dot(int num, bool active, bool complete) {
    return Container(
      width: 18,
      height: 18,
      decoration: BoxDecoration(
        shape: BoxShape.circle,
        color: complete
            ? TAColors.success
            : active
                ? TAColors.accent
                : TAColors.surfaceVariant,
      ),
      child: Center(
        child: complete
            ? const Icon(Icons.check, size: 12, color: Colors.white)
            : Text('$num',
                style: TextStyle(
                    color: active ? Colors.white : TAColors.textSecondary,
                    fontSize: 10,
                    fontWeight: FontWeight.w700)),
      ),
    );
  }

  Widget _line(bool active) {
    return Container(
      width: 24,
      height: 2,
      color: active ? TAColors.success : TAColors.surfaceVariant,
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
            backgroundColor: TAColors.surfaceVariant,
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
