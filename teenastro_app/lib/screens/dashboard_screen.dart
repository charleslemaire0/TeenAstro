import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/mount_state_provider.dart';
import '../services/lx200_tcp_client.dart';
import '../models/mount_state.dart';
import '../theme.dart';
import '../widgets/status_bar.dart';
import '../widgets/coordinate_display.dart';

class DashboardScreen extends ConsumerWidget {
  const DashboardScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(mountStateProvider);

    return RefreshIndicator(
      onRefresh: () => ref.read(mountStateProvider.notifier).refresh(),
      child: ListView(
        physics: const AlwaysScrollableScrollPhysics(),
        padding: const EdgeInsets.all(12),
        children: [
          // Status bar
          MountStatusBar(state: state),
          const SizedBox(height: 12),

          // Position card
          _PositionCard(state: state),
          const SizedBox(height: 8),

          // Time card
          _TimeCard(state: state),
          const SizedBox(height: 8),

          // Target card
          if (state.targetRa.isNotEmpty) ...[
            _TargetCard(state: state),
            const SizedBox(height: 8),
          ],

          // Error card
          if (state.error != MountError.none) ...[
            _ErrorCard(state: state),
            const SizedBox(height: 8),
          ],

          // Firmware info
          _FirmwareCard(state: state),

          // Debug panel (temporary)
          const SizedBox(height: 8),
          const _DebugPanel(),
        ],
      ),
    );
  }
}

// #region agent log
class _DebugPanel extends StatefulWidget {
  const _DebugPanel();
  @override
  State<_DebugPanel> createState() => _DebugPanelState();
}

class _DebugPanelState extends State<_DebugPanel>
    with SingleTickerProviderStateMixin {
  late final Timer _timer;
  late final TabController _tabs;
  int _activeTab = 0;

  @override
  void initState() {
    super.initState();
    _tabs = TabController(length: 2, vsync: this)
      ..addListener(() {
        if (_tabs.indexIsChanging) setState(() => _activeTab = _tabs.index);
      });
    _timer = Timer.periodic(const Duration(seconds: 1), (_) {
      if (mounted) setState(() {});
    });
  }

  @override
  void dispose() {
    _timer.cancel();
    _tabs.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final debugLogs = DebugLog.entries;
    final traceLogs = ConnectTrace.entries;

    return Card(
      color: const Color(0xFF0A0E14),
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // Tab bar + clear button
            Row(
              children: [
                Expanded(
                  child: TabBar(
                    controller: _tabs,
                    labelColor: TAColors.warning,
                    unselectedLabelColor: TAColors.textSecondary,
                    indicatorColor: TAColors.accent,
                    labelStyle: const TextStyle(
                        fontSize: 11, fontWeight: FontWeight.w600),
                    tabs: [
                      Tab(text: 'Debug (${debugLogs.length})'),
                      Tab(text: 'Trace (${traceLogs.length})'),
                    ],
                  ),
                ),
                const SizedBox(width: 8),
                GestureDetector(
                  onTap: () {
                    if (_activeTab == 0) {
                      DebugLog.clear();
                    } else {
                      ConnectTrace.clear();
                    }
                    setState(() {});
                  },
                  child: Text('Clear',
                      style: TextStyle(color: TAColors.accent, fontSize: 12)),
                ),
              ],
            ),
            const SizedBox(height: 8),
            SizedBox(
              height: 220,
              child: TabBarView(
                controller: _tabs,
                children: [
                  // --- Debug log tab ---
                  SingleChildScrollView(
                    reverse: true,
                    child: Text(
                      debugLogs.isEmpty ? '(no logs yet)' : debugLogs.join('\n'),
                      style: const TextStyle(
                        color: Color(0xFF4EC9B0),
                        fontFamily: 'monospace',
                        fontSize: 10,
                        height: 1.4,
                      ),
                    ),
                  ),
                  // --- Connection trace tab ---
                  SingleChildScrollView(
                    reverse: true,
                    child: SelectableText(
                      traceLogs.isEmpty
                          ? '(no trace yet)'
                          : traceLogs.join('\n'),
                      style: const TextStyle(
                        color: Color(0xFFCE9178),
                        fontFamily: 'monospace',
                        fontSize: 10,
                        height: 1.4,
                      ),
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}
// #endregion

class _PositionCard extends StatefulWidget {
  final MountState state;
  const _PositionCard({required this.state});
  @override
  State<_PositionCard> createState() => _PositionCardState();
}

class _PositionCardState extends State<_PositionCard> with SingleTickerProviderStateMixin {
  late TabController _tabs;

  @override
  void initState() {
    super.initState();
    _tabs = TabController(length: 2, vsync: this);
  }

  @override
  void dispose() { _tabs.dispose(); super.dispose(); }

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Column(
        children: [
          TabBar(controller: _tabs, tabs: const [
            Tab(text: 'RA / Dec'),
            Tab(text: 'Alt / Az'),
          ]),
          SizedBox(
            height: 100,
            child: TabBarView(controller: _tabs, children: [
              Padding(
                padding: const EdgeInsets.all(16),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    CoordinateDisplay(label: 'RA', value: widget.state.ra),
                    CoordinateDisplay(label: 'Dec', value: widget.state.dec),
                  ],
                ),
              ),
              Padding(
                padding: const EdgeInsets.all(16),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                  children: [
                    CoordinateDisplay(label: 'Alt', value: widget.state.alt),
                    CoordinateDisplay(label: 'Az', value: widget.state.az),
                  ],
                ),
              ),
            ]),
          ),
        ],
      ),
    );
  }
}

class _TimeCard extends StatelessWidget {
  final MountState state;
  const _TimeCard({required this.state});

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            _InfoItem(label: 'UTC', value: state.utcTime),
            _InfoItem(label: 'Date', value: state.utcDate),
            _InfoItem(label: 'LST', value: state.siderealTime),
          ],
        ),
      ),
    );
  }
}

class _TargetCard extends StatelessWidget {
  final MountState state;
  const _TargetCard({required this.state});

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Target', style: TextStyle(
              color: TAColors.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
            const SizedBox(height: 8),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                CoordinateDisplay(label: 'RA', value: state.targetRa),
                CoordinateDisplay(label: 'Dec', value: state.targetDec),
              ],
            ),
          ],
        ),
      ),
    );
  }
}

class _ErrorCard extends StatelessWidget {
  final MountState state;
  const _ErrorCard({required this.state});

  @override
  Widget build(BuildContext context) {
    return Card(
      color: TAColors.error.withValues(alpha: 0.1),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Row(
          children: [
            Icon(Icons.warning, color: TAColors.error),
            const SizedBox(width: 12),
            Text(state.errorLabel,
              style: TextStyle(color: TAColors.error, fontWeight: FontWeight.w600)),
          ],
        ),
      ),
    );
  }
}

class _FirmwareCard extends StatelessWidget {
  final MountState state;
  const _FirmwareCard({required this.state});

  @override
  Widget build(BuildContext context) {
    if (state.productName.isEmpty) return const SizedBox.shrink();
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Firmware', style: TextStyle(
              color: TAColors.textSecondary, fontSize: 12, fontWeight: FontWeight.w600)),
            const SizedBox(height: 8),
            Text('${state.productName} v${state.versionNum}',
              style: TextStyle(color: TAColors.textHigh)),
            Text('Board: ${state.boardVersion}  Driver: ${state.driverType}',
              style: TextStyle(color: TAColors.textSecondary, fontSize: 12)),
          ],
        ),
      ),
    );
  }
}

class _InfoItem extends StatelessWidget {
  final String label;
  final String value;
  const _InfoItem({required this.label, required this.value});

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Text(label, style: TextStyle(color: TAColors.textSecondary, fontSize: 11)),
        const SizedBox(height: 4),
        Text(value, style: TextStyle(
          color: TAColors.textHigh, fontSize: 16, fontFamily: 'monospace', fontWeight: FontWeight.w600)),
      ],
    );
  }
}
