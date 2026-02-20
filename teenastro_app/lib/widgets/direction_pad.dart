import 'package:flutter/material.dart';
import '../theme.dart';

class DirectionPad extends StatelessWidget {
  final VoidCallback onNorthStart, onNorthStop;
  final VoidCallback onSouthStart, onSouthStop;
  final VoidCallback onEastStart, onEastStop;
  final VoidCallback onWestStart, onWestStop;

  const DirectionPad({
    super.key,
    required this.onNorthStart, required this.onNorthStop,
    required this.onSouthStart, required this.onSouthStop,
    required this.onEastStart, required this.onEastStop,
    required this.onWestStart, required this.onWestStop,
  });

  @override
  Widget build(BuildContext context) {
    return SizedBox(
      width: 240,
      height: 240,
      child: Stack(
        alignment: Alignment.center,
        children: [
          // N
          Positioned(
            top: 0, left: 80, right: 80,
            child: _DirButton(label: 'N', icon: Icons.arrow_upward,
              onStart: onNorthStart, onStop: onNorthStop),
          ),
          // S
          Positioned(
            bottom: 0, left: 80, right: 80,
            child: _DirButton(label: 'S', icon: Icons.arrow_downward,
              onStart: onSouthStart, onStop: onSouthStop),
          ),
          // W
          Positioned(
            left: 0, top: 80, bottom: 80,
            child: _DirButton(label: 'W', icon: Icons.arrow_back,
              onStart: onWestStart, onStop: onWestStop),
          ),
          // E
          Positioned(
            right: 0, top: 80, bottom: 80,
            child: _DirButton(label: 'E', icon: Icons.arrow_forward,
              onStart: onEastStart, onStop: onEastStop),
          ),
          // Center indicator
          Container(
            width: 60, height: 60,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: TAColors.surfaceVariant,
              border: Border.all(color: TAColors.border, width: 2),
            ),
            child: Icon(Icons.control_camera, color: TAColors.textSecondary, size: 28),
          ),
        ],
      ),
    );
  }
}

class _DirButton extends StatefulWidget {
  final String label;
  final IconData icon;
  final VoidCallback onStart;
  final VoidCallback onStop;
  const _DirButton({
    required this.label, required this.icon,
    required this.onStart, required this.onStop,
  });

  @override
  State<_DirButton> createState() => _DirButtonState();
}

class _DirButtonState extends State<_DirButton> {
  bool _pressed = false;

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTapDown: (_) { setState(() => _pressed = true); widget.onStart(); },
      onTapUp: (_) { setState(() => _pressed = false); widget.onStop(); },
      onTapCancel: () { setState(() => _pressed = false); widget.onStop(); },
      child: Container(
        width: 80, height: 80,
        decoration: BoxDecoration(
          color: _pressed ? TAColors.accent : TAColors.surfaceVariant,
          borderRadius: BorderRadius.circular(12),
          border: Border.all(
            color: _pressed ? TAColors.accent : TAColors.border,
            width: 1.5,
          ),
          boxShadow: _pressed ? [
            BoxShadow(color: TAColors.accent.withValues(alpha: 0.3), blurRadius: 12),
          ] : null,
        ),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(widget.icon, color: _pressed ? Colors.white : TAColors.textHigh, size: 28),
            Text(widget.label, style: TextStyle(
              color: _pressed ? Colors.white : TAColors.textSecondary,
              fontSize: 11, fontWeight: FontWeight.w600)),
          ],
        ),
      ),
    );
  }
}
