import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../models/lx200_commands.dart';
import '../models/mount_state.dart';
import '../theme.dart';
import '../widgets/direction_pad.dart';
import '../widgets/status_bar.dart';

class ControlScreen extends ConsumerWidget {
  const ControlScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final client = ref.read(lx200ClientProvider);
    final state = ref.watch(mountStateProvider);

    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        MountStatusBar(state: state),
        const SizedBox(height: 16),

        // Speed selector
        _SpeedSelector(
          current: state.speed,
          onChanged: (level) => client.send(LX200.setSpeed(level)),
        ),
        const SizedBox(height: 16),

        // Direction pad
        Center(
          child: DirectionPad(
            onNorthStart: () => client.send(LX200.moveNorth),
            onNorthStop:  () => client.send(LX200.stopNorth),
            onSouthStart: () => client.send(LX200.moveSouth),
            onSouthStop:  () => client.send(LX200.stopSouth),
            onEastStart:  () => client.send(LX200.moveEast),
            onEastStop:   () => client.send(LX200.stopEast),
            onWestStart:  () => client.send(LX200.moveWest),
            onWestStop:   () => client.send(LX200.stopWest),
          ),
        ),
        const SizedBox(height: 24),

        // Quick actions
        Wrap(
          alignment: WrapAlignment.center,
          spacing: 8,
          runSpacing: 8,
          children: [
            _ActionButton(
              icon: Icons.stop_circle,
              label: 'STOP',
              color: TAColors.error,
              onPressed: () => client.send(LX200.stopAll),
            ),
            _ActionButton(
              icon: Icons.local_parking,
              label: state.isParked ? 'Unpark' : 'Park',
              onPressed: () => client.send(
                state.isParked ? LX200.unpark : LX200.park),
            ),
            _ActionButton(
              icon: Icons.home,
              label: 'Home',
              onPressed: () => client.send(LX200.goHome),
            ),
            if (state.isGEM)
              _ActionButton(
                icon: Icons.swap_horiz,
                label: 'Flip',
                onPressed: () => client.send(LX200.meridianFlip),
              ),
          ],
        ),

        // Focuser (if connected)
        if (state.focuserConnected) ...[
          const SizedBox(height: 24),
          _FocuserPanel(client: client),
        ],
      ],
    );
  }
}

class _SpeedSelector extends StatelessWidget {
  final SpeedLevel current;
  final ValueChanged<int> onChanged;
  const _SpeedSelector({required this.current, required this.onChanged});

  @override
  Widget build(BuildContext context) {
    final labels = ['Guide', 'Slow', 'Med', 'Fast', 'Max'];
    return Row(
      mainAxisAlignment: MainAxisAlignment.center,
      children: List.generate(5, (i) {
        final selected = current.index == i;
        return Padding(
          padding: const EdgeInsets.symmetric(horizontal: 4),
          child: ChoiceChip(
            label: Text(labels[i]),
            selected: selected,
            selectedColor: TAColors.accent,
            labelStyle: TextStyle(
              color: selected ? Colors.white : TAColors.text,
              fontWeight: selected ? FontWeight.w600 : FontWeight.normal,
            ),
            onSelected: (_) => onChanged(i),
          ),
        );
      }),
    );
  }
}

class _ActionButton extends StatelessWidget {
  final IconData icon;
  final String label;
  final Color? color;
  final VoidCallback onPressed;
  const _ActionButton({
    required this.icon, required this.label,
    this.color, required this.onPressed,
  });

  @override
  Widget build(BuildContext context) {
    return ElevatedButton.icon(
      style: ElevatedButton.styleFrom(
        backgroundColor: color ?? TAColors.surfaceVariant,
        foregroundColor: color != null ? Colors.white : TAColors.text,
        minimumSize: const Size(100, 48),
      ),
      onPressed: onPressed,
      icon: Icon(icon, size: 18),
      label: Text(label),
    );
  }
}

class _FocuserPanel extends StatelessWidget {
  final LX200TcpClient client;
  const _FocuserPanel({required this.client});

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Focuser', style: TextStyle(
              color: TAColors.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
            const SizedBox(height: 12),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                _HoldButton(label: 'In', onStart: () => client.send(LX200.focuserIn),
                  onStop: () => client.send(LX200.focuserStop)),
                const SizedBox(width: 16),
                ElevatedButton(
                  style: ElevatedButton.styleFrom(backgroundColor: TAColors.error),
                  onPressed: () => client.send(LX200.focuserStop),
                  child: const Text('Stop'),
                ),
                const SizedBox(width: 16),
                _HoldButton(label: 'Out', onStart: () => client.send(LX200.focuserOut),
                  onStop: () => client.send(LX200.focuserStop)),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _HoldButton extends StatelessWidget {
  final String label;
  final VoidCallback onStart;
  final VoidCallback onStop;
  const _HoldButton({required this.label, required this.onStart, required this.onStop});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: (_) => onStart(),
      onTapUp: (_) => onStop(),
      onTapCancel: onStop,
      child: Container(
        width: 64, height: 48,
        decoration: BoxDecoration(
          color: TAColors.surfaceVariant,
          borderRadius: BorderRadius.circular(8),
          border: Border.all(color: TAColors.border),
        ),
        alignment: Alignment.center,
        child: Text(label, style: TextStyle(color: TAColors.text, fontWeight: FontWeight.w600)),
      ),
    );
  }
}
