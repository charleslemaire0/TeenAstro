import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/lx200_commands.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../theme.dart';

/// Card for site selection and site name editing.
class SiteCard extends ConsumerStatefulWidget {
  const SiteCard({super.key});

  @override
  ConsumerState<SiteCard> createState() => _SiteCardState();
}

class _SiteCardState extends ConsumerState<SiteCard> {
  int _currentSite = 0;
  final List<String> _siteNames = ['Site 1', 'Site 2', 'Site 3'];
  bool _loaded = false;
  String _status = '';

  Future<void> _loadSites() async {
    final client = ref.read(lx200ClientProvider);
    if (!client.isConnected) return;

    try {
      final idxStr = await client.sendCommand(LX200.getCurrentSite);
      final idx = int.tryParse(idxStr ?? '') ?? 0;
      final n1 = await client.sendCommand(LX200.getSiteName1);
      await Future.delayed(const Duration(milliseconds: 50));
      final n2 = await client.sendCommand(LX200.getSiteName2);
      await Future.delayed(const Duration(milliseconds: 50));
      final n3 = await client.sendCommand(LX200.getSiteName3);

      if (mounted) {
        setState(() {
          _currentSite = idx.clamp(0, 2);
          _siteNames[0] = _stripHash(n1) ?? 'Site 1';
          _siteNames[1] = _stripHash(n2) ?? 'Site 2';
          _siteNames[2] = _stripHash(n3) ?? 'Site 3';
          _loaded = true;
          _status = '';
        });
      }
    } catch (_) {
      if (mounted) setState(() => _status = 'Failed to load sites');
    }
  }

  String? _stripHash(String? s) {
    if (s == null || s.isEmpty) return null;
    return s.endsWith('#') ? s.substring(0, s.length - 1) : s;
  }

  Future<void> _selectSite(int index) async {
    final client = ref.read(lx200ClientProvider);
    if (!client.isConnected) return;

    setState(() => _status = '');
    await client.send(LX200.setSite(index));
    await Future.delayed(const Duration(milliseconds: 80));
    await _loadSites();
    if (mounted) {
      ref.read(mountStateProvider.notifier).refresh();
      setState(() => _status = 'Site ${index + 1} selected');
    }
  }

  Future<void> _saveSiteName(String name) async {
    final client = ref.read(lx200ClientProvider);
    if (!client.isConnected || name.isEmpty) return;

    setState(() => _status = '');
    final ok = await client.sendBool(LX200.setCurrentSiteName(name));
    if (mounted) {
      setState(() {
        _siteNames[_currentSite] = name;
        _status = ok ? 'Name saved' : 'Save failed';
      });
      if (ok) ref.read(mountStateProvider.notifier).refresh();
    }
  }

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) => _loadSites());
  }

  @override
  Widget build(BuildContext context) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Site', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            DropdownButtonFormField<int>(
              value: _currentSite.clamp(0, 2),
              decoration: const InputDecoration(
                labelText: 'Active site',
                border: OutlineInputBorder(),
              ),
              items: List.generate(3, (i) => DropdownMenuItem(
                value: i,
                child: Text('${i + 1}: ${_siteNames[i]}'),
              )),
              onChanged: _loaded
                  ? (v) {
                      if (v != null && v != _currentSite) _selectSite(v);
                    }
                  : null,
            ),
            const SizedBox(height: 12),
            _SiteNameField(
              name: _siteNames[_currentSite],
              onSave: _saveSiteName,
              enabled: _loaded,
            ),
            if (_status.isNotEmpty) ...[
              const SizedBox(height: 8),
              Text(_status,
                  style: TextStyle(
                      color: _status.contains('Failed') ? TAColors.error : TAColors.success,
                      fontSize: 12)),
            ],
          ],
        ),
      ),
    );
  }
}

class _SiteNameField extends StatefulWidget {
  final String name;
  final Future<void> Function(String) onSave;
  final bool enabled;

  const _SiteNameField({
    required this.name,
    required this.onSave,
    required this.enabled,
  });

  @override
  State<_SiteNameField> createState() => _SiteNameFieldState();
}

class _SiteNameFieldState extends State<_SiteNameField> {
  late TextEditingController _controller;

  @override
  void initState() {
    super.initState();
    _controller = TextEditingController(text: widget.name);
  }

  @override
  void didUpdateWidget(_SiteNameField oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.name != oldWidget.name) {
      _controller.text = widget.name;
    }
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Expanded(
          child: TextField(
            controller: _controller,
            decoration: const InputDecoration(
              labelText: 'Site name',
              border: OutlineInputBorder(),
              hintText: 'e.g. Home, Star Party',
            ),
            maxLength: 14,
            enabled: widget.enabled,
            onChanged: (_) => setState(() {}),
          ),
        ),
        const SizedBox(width: 8),
        FilledButton(
          onPressed: widget.enabled
              ? () => widget.onSave(_controller.text.trim())
              : null,
          child: const Text('Save'),
        ),
      ],
    );
  }
}
