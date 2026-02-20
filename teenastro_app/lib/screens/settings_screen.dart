import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../theme.dart';

class SettingsScreen extends ConsumerWidget {
  const SettingsScreen({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(mountStateProvider);
    final client = ref.read(lx200ClientProvider);

    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        // Mount info
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Mount Information', style: Theme.of(context).textTheme.titleMedium),
                const SizedBox(height: 12),
                _InfoRow('Product', state.productName),
                _InfoRow('Firmware', state.versionNum),
                _InfoRow('Board', state.boardVersion),
                _InfoRow('Driver', state.driverType),
                _InfoRow('Mount Type', state.mountLabel),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // Alignment
        Card(
          child: ListTile(
            leading: Icon(Icons.architecture, color: TAColors.accent),
            title: const Text('Alignment'),
            subtitle: Text(state.aligned ? 'Aligned' : 'Not aligned'),
            trailing: const Icon(Icons.chevron_right),
            onTap: () => context.go('/alignment'),
          ),
        ),
        const SizedBox(height: 12),

        // Connection
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('Connection', style: Theme.of(context).textTheme.titleMedium),
                const SizedBox(height: 12),
                Row(
                  children: [
                    Icon(Icons.circle, size: 12,
                      color: client.isConnected ? TAColors.success : TAColors.error),
                    const SizedBox(width: 8),
                    Text(client.isConnected ? 'Connected' : 'Disconnected'),
                  ],
                ),
                const SizedBox(height: 12),
                SizedBox(
                  width: double.infinity,
                  child: ElevatedButton.icon(
                    style: ElevatedButton.styleFrom(backgroundColor: TAColors.surfaceVariant),
                    onPressed: () async {
                      ref.read(mountStateProvider.notifier).stopPolling();
                      await client.disconnect();
                      if (context.mounted) context.go('/connect');
                    },
                    icon: const Icon(Icons.link_off),
                    label: const Text('Disconnect'),
                  ),
                ),
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),

        // App info
        Card(
          child: Padding(
            padding: const EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('TeenAstro Controller', style: Theme.of(context).textTheme.titleMedium),
                const SizedBox(height: 4),
                Text('Version 1.0.0', style: TextStyle(color: TAColors.textSecondary)),
                const SizedBox(height: 4),
                Text('Flutter app for TeenAstro telescope mounts',
                  style: TextStyle(color: TAColors.textSecondary, fontSize: 12)),
              ],
            ),
          ),
        ),
      ],
    );
  }
}

class _InfoRow extends StatelessWidget {
  final String label;
  final String value;
  const _InfoRow(this.label, this.value);

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 4),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Text(label, style: TextStyle(color: TAColors.textSecondary)),
          Text(value.isNotEmpty ? value : '—',
            style: TextStyle(color: TAColors.textHigh, fontWeight: FontWeight.w500)),
        ],
      ),
    );
  }
}
