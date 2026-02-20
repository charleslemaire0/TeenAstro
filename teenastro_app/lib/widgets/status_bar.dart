import 'package:flutter/material.dart';
import '../models/mount_state.dart';
import '../theme.dart';

class MountStatusBar extends StatelessWidget {
  final MountState state;
  const MountStatusBar({super.key, required this.state});

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
        child: Wrap(
          alignment: WrapAlignment.center,
          spacing: 12,
          runSpacing: 4,
          children: [
            _StatusChip(
              icon: state.isTracking ? Icons.track_changes : Icons.pause_circle,
              label: state.trackingLabel,
              color: state.isTracking ? TAColors.success
                   : state.isSlewing ? TAColors.warning
                   : TAColors.textSecondary,
            ),
            if (state.isTracking)
              _StatusChip(
                icon: _siderealIcon(state.sidereal),
                label: state.siderealLabel,
                color: TAColors.text,
              ),
            _StatusChip(
              icon: state.isParked ? Icons.local_parking : Icons.open_in_new,
              label: state.parkLabel,
              color: state.isParked ? TAColors.warning : TAColors.text,
            ),
            if (state.isGEM)
              _StatusChip(
                icon: Icons.swap_horiz,
                label: 'Pier ${state.pierLabel}',
                color: TAColors.text,
              ),
            _StatusChip(
              icon: Icons.speed,
              label: state.speedLabel,
              color: TAColors.text,
            ),
            if (state.aligned)
              _StatusChip(icon: Icons.check_circle, label: 'Aligned', color: TAColors.success),
            if (state.atHome)
              _StatusChip(icon: Icons.home, label: 'Home', color: TAColors.success),
            if (state.hasGNSS)
              _StatusChip(
                icon: Icons.satellite_alt,
                label: 'GNSS',
                color: state.gnssValid ? TAColors.success : TAColors.textSecondary,
              ),
            if (state.error != MountError.none)
              _StatusChip(icon: Icons.warning, label: state.errorLabel, color: TAColors.error),
          ],
        ),
      ),
    );
  }

  IconData _siderealIcon(SiderealMode mode) => switch (mode) {
    SiderealMode.star => Icons.star,
    SiderealMode.sun => Icons.wb_sunny,
    SiderealMode.moon => Icons.nightlight_round,
    SiderealMode.target => Icons.gps_fixed,
    _ => Icons.help_outline,
  };
}

class _StatusChip extends StatelessWidget {
  final IconData icon;
  final String label;
  final Color color;
  const _StatusChip({required this.icon, required this.label, required this.color});

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Icon(icon, size: 14, color: color),
        const SizedBox(width: 4),
        Text(label, style: TextStyle(color: color, fontSize: 12, fontWeight: FontWeight.w500)),
      ],
    );
  }
}
