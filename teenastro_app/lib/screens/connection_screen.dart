import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../theme.dart';

class ConnectionScreen extends ConsumerStatefulWidget {
  const ConnectionScreen({super.key});
  @override
  ConsumerState<ConnectionScreen> createState() => _ConnectionScreenState();
}

class _ConnectionScreenState extends ConsumerState<ConnectionScreen> {
  final _ipController = TextEditingController(text: '192.168.0.1');
  final _portController = TextEditingController(text: '9999');
  bool _connecting = false;
  String _error = '';

  @override
  void initState() {
    super.initState();
    _loadSavedIp();
  }

  Future<void> _loadSavedIp() async {
    final prefs = await SharedPreferences.getInstance();
    final saved = prefs.getString('last_ip');
    if (saved != null && saved.isNotEmpty) {
      _ipController.text = saved;
    }
    final port = prefs.getString('last_port');
    if (port != null && port.isNotEmpty) {
      _portController.text = port;
    }
  }

  Future<void> _connect() async {
    setState(() { _connecting = true; _error = ''; });

    final client = ref.read(lx200ClientProvider);
    final ip = _ipController.text.trim();
    final port = int.tryParse(_portController.text.trim()) ?? 9999;

    final ok = await client.connect(ip, port);

    if (ok && mounted) {
      // Save for next time
      final prefs = await SharedPreferences.getInstance();
      await prefs.setString('last_ip', ip);
      await prefs.setString('last_port', port.toString());

      // Start polling
      ref.read(mountStateProvider.notifier).startPolling();

      if (mounted) context.go('/dashboard');
    } else if (mounted) {
      setState(() {
        _connecting = false;
        _error = client.lastError.isNotEmpty ? client.lastError : 'Connection failed';
      });
    }
  }

  Color get _statusColor {
    if (_error.isNotEmpty) return TAColors.error;
    if (_connecting) return TAColors.warning;
    final client = ref.read(lx200ClientProvider);
    return client.isConnected ? TAColors.success : TAColors.textSecondary;
  }

  String get _statusLabel {
    if (_error.isNotEmpty) return 'Error';
    if (_connecting) return 'Connecting...';
    final client = ref.read(lx200ClientProvider);
    return client.isConnected ? 'Connected' : 'Disconnected';
  }

  @override
  void dispose() {
    _ipController.dispose();
    _portController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Center(
        child: SingleChildScrollView(
          padding: const EdgeInsets.all(32),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.explore, size: 80, color: TAColors.accent),
              const SizedBox(height: 16),
              Text('TeenAstro Controller',
                style: Theme.of(context).textTheme.headlineMedium),
              const SizedBox(height: 8),
              Text('Connect to your telescope mount',
                style: Theme.of(context).textTheme.bodySmall),
              const SizedBox(height: 24),
              // Connection status indicator
              Container(
                padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                decoration: BoxDecoration(
                  color: _statusColor.withValues(alpha: 0.15),
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: _statusColor.withValues(alpha: 0.4)),
                ),
                child: Row(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Container(
                      width: 10,
                      height: 10,
                      decoration: BoxDecoration(
                        shape: BoxShape.circle,
                        color: _statusColor,
                        boxShadow: [
                          BoxShadow(
                            color: _statusColor.withValues(alpha: 0.5),
                            blurRadius: 4,
                          ),
                        ],
                      ),
                    ),
                    const SizedBox(width: 8),
                    Text(_statusLabel,
                      style: TextStyle(
                        color: _statusColor,
                        fontWeight: FontWeight.w600,
                        fontSize: 13,
                      )),
                  ],
                ),
              ),
              const SizedBox(height: 32),
              TextField(
                key: const Key('connect_ip'),
                controller: _ipController,
                decoration: const InputDecoration(
                  labelText: 'IP Address',
                  prefixIcon: Icon(Icons.wifi),
                ),
                keyboardType: TextInputType.number,
              ),
              const SizedBox(height: 16),
              TextField(
                key: const Key('connect_port'),
                controller: _portController,
                decoration: const InputDecoration(
                  labelText: 'Port',
                  prefixIcon: Icon(Icons.numbers),
                ),
                keyboardType: TextInputType.number,
              ),
              const SizedBox(height: 24),
              SizedBox(
                width: double.infinity,
                height: 52,
                child: ElevatedButton.icon(
                  key: const Key('connect_btn'),
                  onPressed: _connecting ? null : _connect,
                  icon: _connecting
                      ? const SizedBox(width: 20, height: 20,
                          child: CircularProgressIndicator(strokeWidth: 2, color: Colors.white))
                      : const Icon(Icons.link),
                  label: Text(_connecting ? 'Connecting...' : 'Connect'),
                ),
              ),
              if (_error.isNotEmpty) ...[
                const SizedBox(height: 16),
                Container(
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: TAColors.error.withValues(alpha: 0.1),
                    borderRadius: BorderRadius.circular(8),
                    border: Border.all(color: TAColors.error.withValues(alpha: 0.3)),
                  ),
                  child: Row(
                    children: [
                      Icon(Icons.error_outline, color: TAColors.error, size: 20),
                      const SizedBox(width: 8),
                      Expanded(child: Text(_error,
                        style: TextStyle(color: TAColors.error, fontSize: 13))),
                    ],
                  ),
                ),
              ],
            ],
          ),
        ),
      ),
    );
  }
}
