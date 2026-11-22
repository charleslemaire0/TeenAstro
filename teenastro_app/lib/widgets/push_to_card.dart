import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../services/finder_sky.dart';
import '../services/push_to.dart';
import '../theme.dart';

/// Where to push the tube: from the last solve to the target.
/// Nothing is shown until there is both a solve and a target.
class PushToCard extends ConsumerWidget {
  const PushToCard({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final sky = ref.watch(finderSkyProvider);
    if (!sky.active || !sky.hasSolve || !sky.hasTarget) {
      return const SizedBox.shrink();
    }
    final snap = sky.snapshot(sky.clock);
    final offset = pushTo(
      fromRa: sky.raH!,
      fromDec: sky.decD!,
      toRa: sky.targetRa!,
      toDec: sky.targetDec!,
      latDeg: sky.hasSite ? sky.latDeg : null,
      lstH: sky.hasSite ? snap.lstH : null,
    );
    final age = sky.solvedAt == null
        ? ''
        : 'solved ${_age(sky.clock.difference(sky.solvedAt!))} ago';
    return Material(
      color: Colors.black.withValues(alpha: 0.7),
      borderRadius: BorderRadius.circular(8),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(10, 4, 0, 4),
        child: Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Icon(Icons.near_me, size: 18, color: TA.success),
            const SizedBox(width: 8),
            Column(
              mainAxisSize: MainAxisSize.min,
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(sky.targetName.isEmpty ? 'Target' : sky.targetName,
                    style: TextStyle(
                        color: TA.textHigh, fontSize: 12, fontWeight: FontWeight.w600)),
                Text(formatPushTo(offset),
                    style: TextStyle(
                        color: TA.success, fontSize: 14, fontFamily: 'monospace')),
                if (age.isNotEmpty)
                  Text(age, style: TextStyle(color: TA.textSecondary, fontSize: 10)),
              ],
            ),
            IconButton(
              tooltip: 'Clear target',
              icon: Icon(Icons.close, size: 18, color: TA.textSecondary),
              onPressed: () => ref.read(finderSkyProvider.notifier).clearTarget(),
            ),
          ],
        ),
      ),
    );
  }

  static String _age(Duration d) {
    if (d.inSeconds < 60) return '${d.inSeconds} s';
    if (d.inMinutes < 60) return '${d.inMinutes} min';
    return '${d.inHours} h';
  }
}
