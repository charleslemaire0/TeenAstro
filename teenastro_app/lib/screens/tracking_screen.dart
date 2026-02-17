import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../models/lx200_commands.dart';
import '../models/mount_state.dart';
import '../theme.dart';
import '../widgets/status_bar.dart';

class TrackingScreen extends ConsumerWidget {
  const TrackingScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final client = ref.read(lx200ClientProvider);
    final state = ref.watch(mountStateProvider);

    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        MountStatusBar(state: state),
        const SizedBox(height: 16),

        // On/Off toggle
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Row(
              mainAxisAlignment: MainAxisAlignment.spaceBetween,
              children: [
                Text('Tracking', style: Theme.of(context).textTheme.titleLarge),
                Switch(
                  value: state.isTracking,
                  activeTrackColor: TAColors.success,
                  onChanged: (on) => client.send(on ? LX200.trackOn : LX200.trackOff),
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // Rate selection
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Tracking Rate', style: TextStyle(
                  color: TAColors.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
                const SizedBox(height: 12),
                Wrap(
                  spacing: 8,
                  children: [
                    _RateChip('Sidereal', Icons.star, state.sidereal == SiderealMode.star,
                      () => client.send(LX200.trackSidereal)),
                    _RateChip('Lunar', Icons.nightlight_round, state.sidereal == SiderealMode.moon,
                      () => client.send(LX200.trackLunar)),
                    _RateChip('Solar', Icons.wb_sunny, state.sidereal == SiderealMode.sun,
                      () => client.send(LX200.trackSolar)),
                    _RateChip('User', Icons.gps_fixed, state.sidereal == SiderealMode.target,
                      () => client.send(LX200.trackUser)),
                  ],
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // Fine rate adjust
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Fine Rate Adjust', style: TextStyle(
                  color: TAColors.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
                const SizedBox(height: 12),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    IconButton.filled(
                      onPressed: () => client.send(LX200.trackDecr),
                      icon: const Icon(Icons.remove),
                    ),
                    const SizedBox(width: 16),
                    ElevatedButton(
                      onPressed: () => client.send(LX200.trackReset),
                      style: ElevatedButton.styleFrom(backgroundColor: TAColors.surfaceVariant),
                      child: const Text('Reset'),
                    ),
                    const SizedBox(width: 16),
                    IconButton.filled(
                      onPressed: () => client.send(LX200.trackIncr),
                      icon: const Icon(Icons.add),
                    ),
                  ],
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // Options
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              children: [
                SwitchListTile(
                  title: const Text('Dual-Axis Tracking'),
                  subtitle: const Text('Apply tracking correction on both axes'),
                  value: false, // would need to track this in state
                  onChanged: (on) => client.send(on ? LX200.trackDualOn : LX200.trackDualOff),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }
}

class _RateChip extends StatelessWidget {
  final String label;
  final IconData icon;
  final bool selected;
  final VoidCallback onTap;
  const _RateChip(this.label, this.icon, this.selected, this.onTap);

  @override
  Widget build(BuildContext context) {
    return ChoiceChip(
      avatar: Icon(icon, size: 16, color: selected ? Colors.white : TAColors.text),
      label: Text(label),
      selected: selected,
      selectedColor: TAColors.accent,
      labelStyle: TextStyle(color: selected ? Colors.white : TAColors.text),
      onSelected: (_) => onTap(),
    );
  }
}
