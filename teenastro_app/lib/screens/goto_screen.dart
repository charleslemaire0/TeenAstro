import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../models/lx200_commands.dart';
import '../providers/last_goto_route_provider.dart';
import '../theme.dart';
import '../widgets/status_bar.dart';

class GotoScreen extends ConsumerStatefulWidget {
  const GotoScreen({super.key});
  @override
  ConsumerState<GotoScreen> createState() => _GotoScreenState();
}

class _GotoScreenState extends ConsumerState<GotoScreen> {
  final _raH = TextEditingController();
  final _raM = TextEditingController();
  final _raS = TextEditingController();
  final _decSign = ValueNotifier('+');
  final _decD = TextEditingController();
  final _decM = TextEditingController();
  final _decS = TextEditingController();
  String _result = '';

  Future<void> _gotoRaDec(LX200TcpClient client) async {
    final ra = '${_raH.text.padLeft(2, '0')}:${_raM.text.padLeft(2, '0')}:${_raS.text.padLeft(2, '0')}';
    final dec = '${_decSign.value}${_decD.text.padLeft(2, '0')}:${_decM.text.padLeft(2, '0')}:${_decS.text.padLeft(2, '0')}';

    final setRa = await client.sendBool(LX200.setTargetRa(ra));
    final setDec = await client.sendBool(LX200.setTargetDec(dec));

    if (!setRa || !setDec) {
      setState(() => _result = 'Failed to set target coordinates');
      return;
    }

    final reply = await client.sendCommand(LX200.gotoTarget);
    setState(() {
      if (reply == '0') {
        _result = 'Slewing to target...';
      } else {
        final errIdx = int.tryParse(reply ?? '') ?? -1;
        if (errIdx >= 0 && errIdx < GotoError.values.length) {
          _result = 'Error: ${gotoErrorCause(GotoError.values[errIdx])}';
        } else {
          _result = 'Error: $reply';
        }
      }
    });
  }

  Future<void> _syncRaDec(LX200TcpClient client) async {
    final ra = '${_raH.text.padLeft(2, '0')}:${_raM.text.padLeft(2, '0')}:${_raS.text.padLeft(2, '0')}';
    final dec = '${_decSign.value}${_decD.text.padLeft(2, '0')}:${_decM.text.padLeft(2, '0')}:${_decS.text.padLeft(2, '0')}';

    await client.sendBool(LX200.setTargetRa(ra));
    await client.sendBool(LX200.setTargetDec(dec));
    final reply = await client.sendCommand(LX200.syncTarget);
    setState(() => _result = reply != null ? 'Synced: $reply' : 'Sync failed');
  }

  Future<void> _stopSlew(LX200TcpClient client) async {
    client.sendImmediate(LX200.stopAll);
    setState(() => _result = 'Slew aborted');
  }

  Future<void> _goHome(LX200TcpClient client) async {
    final reply = await client.sendBool(LX200.goHome);
    setState(() {
      _result = reply ? 'Going home...' : 'Go-home failed (mount parked or busy?)';
    });
  }

  Future<void> _park(LX200TcpClient client) async {
    final reply = await client.sendBool(LX200.park);
    setState(() {
      _result = reply ? 'Parking...' : 'Park failed';
    });
  }

  @override
  @override
  void initState() {
    super.initState();
    ref.read(lastGotoTabRouteProvider.notifier).state = '/goto';
  }

  @override
  void dispose() {
    _raH.dispose(); _raM.dispose(); _raS.dispose();
    _decD.dispose(); _decM.dispose(); _decS.dispose();
    _decSign.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final client = ref.read(lx200ClientProvider);
    final state = ref.watch(mountStateProvider);
    final slewing = state.isSlewing;

    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        MountStatusBar(state: state),
        const SizedBox(height: 16),

        // Planetarium
        Card(
          child: ListTile(
            leading: Icon(Icons.public, color: TA.accent),
            title: const Text('Planetarium'),
            subtitle: const Text('Interactive sky map with Goto'),
            trailing: const Icon(Icons.chevron_right),
            onTap: () => context.go('/planetarium'),
          ),
        ),
        const SizedBox(height: 8),

        // Catalog button
        Card(
          child: ListTile(
            leading: Icon(Icons.list_alt, color: TA.accent),
            title: const Text('Browse Catalogs'),
            subtitle: const Text('Messier, NGC, Stars, and more'),
            trailing: const Icon(Icons.chevron_right),
            onTap: () => context.go('/catalogs'),
          ),
        ),
        const SizedBox(height: 12),

        // Coordinate entry
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Coordinate Entry (J2000)', style: TextStyle(
                  color: TA.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
                const SizedBox(height: 12),

                // RA
                Text('Right Ascension', style: TextStyle(color: TA.text, fontSize: 13)),
                const SizedBox(height: 8),
                Row(
                  children: [
                    _CoordField(controller: _raH, hint: 'HH', max: 23),
                    const Text(' : ', style: TextStyle(fontSize: 18)),
                    _CoordField(controller: _raM, hint: 'MM', max: 59),
                    const Text(' : ', style: TextStyle(fontSize: 18)),
                    _CoordField(controller: _raS, hint: 'SS', max: 59),
                  ],
                ),
                const SizedBox(height: 16),

                // Dec
                Text('Declination', style: TextStyle(color: TA.text, fontSize: 13)),
                const SizedBox(height: 8),
                Row(
                  children: [
                    ValueListenableBuilder<String>(
                      valueListenable: _decSign,
                      builder: (_, sign, __) => GestureDetector(
                        onTap: () => _decSign.value = sign == '+' ? '-' : '+',
                        child: Container(
                          width: 36, height: 44,
                          decoration: BoxDecoration(
                            color: TA.surfaceVariant,
                            borderRadius: BorderRadius.circular(8),
                            border: Border.all(color: TA.border),
                          ),
                          alignment: Alignment.center,
                          child: Text(sign, style: TextStyle(
                            color: TA.textHigh, fontSize: 20, fontWeight: FontWeight.w700)),
                        ),
                      ),
                    ),
                    const SizedBox(width: 8),
                    _CoordField(controller: _decD, hint: 'DD', max: 90),
                    const Text(' : ', style: TextStyle(fontSize: 18)),
                    _CoordField(controller: _decM, hint: 'MM', max: 59),
                    const Text(' : ', style: TextStyle(fontSize: 18)),
                    _CoordField(controller: _decS, hint: 'SS', max: 59),
                  ],
                ),
                const SizedBox(height: 16),

                // Goto / Stop / Sync buttons
                Row(
                  children: [
                    Expanded(
                      child: slewing
                          ? ElevatedButton.icon(
                              style: ElevatedButton.styleFrom(backgroundColor: TA.error),
                              onPressed: () => _stopSlew(client),
                              icon: const Icon(Icons.stop_circle),
                              label: const Text('Stop'),
                            )
                          : ElevatedButton.icon(
                              onPressed: () => _gotoRaDec(client),
                              icon: const Icon(Icons.my_location),
                              label: const Text('Goto'),
                            ),
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: ElevatedButton.icon(
                        style: ElevatedButton.styleFrom(backgroundColor: TA.surfaceVariant),
                        onPressed: slewing ? null : () => _syncRaDec(client),
                        icon: const Icon(Icons.sync),
                        label: const Text('Sync'),
                      ),
                    ),
                  ],
                ),

                if (_result.isNotEmpty) ...[
                  const SizedBox(height: 12),
                  Text(_result, style: TextStyle(
                    color: _result.startsWith('Error') || _result.contains('failed')
                        ? TA.error
                        : TA.success,
                    fontSize: 13)),
                ],
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // Quick targets
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Quick Targets', style: TextStyle(
                  color: TA.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
                const SizedBox(height: 12),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    if (slewing)
                      ElevatedButton.icon(
                        style: ElevatedButton.styleFrom(backgroundColor: TA.error),
                        onPressed: () => _stopSlew(client),
                        icon: const Icon(Icons.stop_circle),
                        label: const Text('Stop Slew'),
                      )
                    else ...[
                      ElevatedButton.icon(
                        onPressed: () => _goHome(client),
                        icon: const Icon(Icons.home),
                        label: const Text('Home'),
                      ),
                      ElevatedButton.icon(
                        onPressed: () => _park(client),
                        icon: const Icon(Icons.local_parking),
                        label: const Text('Park'),
                      ),
                    ],
                  ],
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }
}

class _CoordField extends StatelessWidget {
  final TextEditingController controller;
  final String hint;
  final int max;
  const _CoordField({required this.controller, required this.hint, required this.max});

  @override
  Widget build(BuildContext context) {
    return Expanded(
      child: TextField(
        controller: controller,
        decoration: InputDecoration(
          hintText: hint,
          contentPadding: const EdgeInsets.symmetric(horizontal: 8, vertical: 12),
        ),
        keyboardType: TextInputType.number,
        textAlign: TextAlign.center,
        style: TextStyle(
          color: TA.textHigh, fontSize: 18, fontFamily: 'monospace', fontWeight: FontWeight.w600),
      ),
    );
  }
}
