import 'package:flutter/material.dart';
import '../theme.dart';

class CoordinateDisplay extends StatelessWidget {
  final String label;
  final String value;
  const CoordinateDisplay({super.key, required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        Text(label, style: TextStyle(
          color: TAColors.textSecondary, fontSize: 11, fontWeight: FontWeight.w600)),
        const SizedBox(height: 4),
        Text(value, style: TextStyle(
          color: TAColors.textHigh,
          fontSize: 20,
          fontFamily: 'monospace',
          fontWeight: FontWeight.w700,
          letterSpacing: 1,
        )),
      ],
    );
  }
}
