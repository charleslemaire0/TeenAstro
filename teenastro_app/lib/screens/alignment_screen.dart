import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../models/lx200_commands.dart';
import '../models/mount_state.dart';
import '../theme.dart';
import '../widgets/status_bar.dart';

class AlignmentScreen extends ConsumerStatefulWidget {
  const AlignmentScreen({super.key});
  @override
  ConsumerState<AlignmentScreen> createState() => _AlignmentScreenState();
}

class _AlignmentScreenState extends ConsumerState<AlignmentScreen> {
  String _status = 'Not started';
  String _alignError = '';

  @override
  Widget build(BuildContext context) {
    final client = ref.read(lx200ClientProvider);
    final state = ref.watch(mountStateProvider);

    final pierSegment = switch (state.pierSide) {
      PierSide.east => 'E',
      PierSide.west => 'W',
      PierSide.unknown => 'N',
    };

    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        MountStatusBar(state: state),
        const SizedBox(height: 16),

        // Alignment status
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Alignment', style: Theme.of(context).textTheme.titleLarge),
                const SizedBox(height: 8),
                Row(
                  children: [
                    Icon(
                      state.aligned ? Icons.check_circle : Icons.radio_button_unchecked,
                      color: state.aligned ? TAColors.success : TAColors.textSecondary,
                    ),
                    const SizedBox(width: 8),
                    Text(state.aligned ? 'Aligned' : 'Not aligned',
                      style: TextStyle(color: state.aligned ? TAColors.success : TAColors.text)),
                  ],
                ),
                const SizedBox(height: 8),
                Text('Status: $_status', style: TextStyle(color: TAColors.textSecondary)),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // Actions
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Actions', style: TextStyle(
                  color: TAColors.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
                const SizedBox(height: 12),
                Wrap(
                  spacing: 8,
                  runSpacing: 8,
                  children: [
                    ElevatedButton.icon(
                      onPressed: () async {
                        final ok = await client.sendBool(LX200.alignStart);
                        setState(() => _status = ok ? 'Started - slew to first star' : 'Failed to start');
                      },
                      icon: const Icon(Icons.play_arrow),
                      label: const Text('Start'),
                    ),
                    ElevatedButton.icon(
                      onPressed: () async {
                        final ok = await client.sendBool(LX200.alignAcceptStar);
                        setState(() => _status = ok ? 'Star accepted' : 'Failed');
                      },
                      icon: const Icon(Icons.check),
                      label: const Text('Accept Star'),
                    ),
                    ElevatedButton.icon(
                      onPressed: () async {
                        final ok = await client.sendBool(LX200.alignAtHome);
                        setState(() => _status = ok ? 'Aligned at home' : 'Failed');
                      },
                      icon: const Icon(Icons.home),
                      label: const Text('At Home'),
                    ),
                    ElevatedButton.icon(
                      onPressed: () async {
                        final ok = await client.sendBool(LX200.alignSave);
                        setState(() => _status = ok ? 'Alignment saved' : 'Failed');
                      },
                      icon: const Icon(Icons.save),
                      label: const Text('Save'),
                    ),
                    ElevatedButton.icon(
                      style: ElevatedButton.styleFrom(backgroundColor: TAColors.surfaceVariant),
                      onPressed: () async {
                        final ok = await client.sendBool(LX200.alignClear);
                        setState(() => _status = ok ? 'Alignment cleared' : 'Failed');
                      },
                      icon: const Icon(Icons.clear),
                      label: const Text('Clear'),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // Pier side
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Pier Side', style: TextStyle(
                  color: TAColors.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
                const SizedBox(height: 12),
                SegmentedButton<String>(
                  segments: const [
                    ButtonSegment(value: 'E', label: Text('East')),
                    ButtonSegment(value: 'W', label: Text('West')),
                    ButtonSegment(value: 'N', label: Text('None')),
                  ],
                  selected: {pierSegment},
                  onSelectionChanged: (s) {
                    final cmd = switch (s.first) {
                      'E' => LX200.pierEast,
                      'W' => LX200.pierWest,
                      _ => LX200.pierNone,
                    };
                    client.send(cmd);
                  },
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // Alignment error
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text('Alignment Error', style: TextStyle(
                      color: TAColors.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
                    TextButton(
                      onPressed: () async {
                        final err = await client.sendCommand(LX200.getAlignError);
                        setState(() => _alignError = err ?? 'N/A');
                      },
                      child: const Text('Refresh'),
                    ),
                  ],
                ),
                if (_alignError.isNotEmpty)
                  Text(_alignError, style: TextStyle(
                    color: TAColors.textHigh, fontFamily: 'monospace', fontSize: 14)),
              ],
            ),
          ),
        ),
      ],
    );
  }
}
