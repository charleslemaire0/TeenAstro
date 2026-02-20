import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/lx200_tcp_client.dart';
import '../services/catalog_service.dart';
import '../models/catalog_entry.dart';
import '../models/lx200_commands.dart';
import '../theme.dart';

class CatalogBrowserScreen extends ConsumerStatefulWidget {
  const CatalogBrowserScreen({super.key});
  @override
  ConsumerState<CatalogBrowserScreen> createState() => _CatalogBrowserScreenState();
}

class _CatalogBrowserScreenState extends ConsumerState<CatalogBrowserScreen> {
  List<Catalog> _catalogs = [];
  int _selectedCatalog = 0;
  String _search = '';
  int? _filterConstellation;
  int? _filterType;
  double? _filterMaxMag;
  bool _loading = true;

  @override
  void initState() {
    super.initState();
    _loadCatalogs();
  }

  Future<void> _loadCatalogs() async {
    final svc = ref.read(catalogServiceProvider);
    final cats = await svc.loadCatalogs();
    if (mounted) setState(() { _catalogs = cats; _loading = false; });
  }

  List<CatalogEntry> get _filtered {
    if (_catalogs.isEmpty) return [];
    final cat = _catalogs[_selectedCatalog];
    return cat.objects.where((o) {
      if (_search.isNotEmpty) {
        final q = _search.toLowerCase();
        if (!o.name.toLowerCase().contains(q) &&
            !'${cat.prefix}${o.id}'.toLowerCase().contains(q)) return false;
      }
      if (_filterConstellation != null && o.constellation != _filterConstellation) return false;
      if (_filterType != null && o.objType != _filterType) return false;
      if (_filterMaxMag != null && o.mag != null && o.mag! > _filterMaxMag!) return false;
      return true;
    }).toList();
  }

  @override
  Widget build(BuildContext context) {
    if (_loading) return const Center(child: CircularProgressIndicator());
    if (_catalogs.isEmpty) {
      return Center(
        child: Padding(
          padding: const EdgeInsets.all(32),
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.folder_open, size: 64, color: TAColors.textSecondary),
              const SizedBox(height: 16),
              Text('No catalogs loaded', style: Theme.of(context).textTheme.titleMedium),
              const SizedBox(height: 8),
              Text('Run tools/extract_catalogs.py to generate catalog JSON files.',
                style: TextStyle(color: TAColors.textSecondary), textAlign: TextAlign.center),
            ],
          ),
        ),
      );
    }

    final items = _filtered;

    return Column(
      children: [
        // Catalog selector
        SizedBox(
          height: 48,
          child: ListView.builder(
            scrollDirection: Axis.horizontal,
            padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
            itemCount: _catalogs.length,
            itemBuilder: (_, i) => Padding(
              padding: const EdgeInsets.symmetric(horizontal: 4),
              child: ChoiceChip(
                label: Text(_catalogs[i].title),
                selected: i == _selectedCatalog,
                selectedColor: TAColors.accent,
                labelStyle: TextStyle(
                  color: i == _selectedCatalog ? Colors.white : TAColors.text),
                onSelected: (_) => setState(() => _selectedCatalog = i),
              ),
            ),
          ),
        ),

        // Search bar
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
          child: TextField(
            decoration: InputDecoration(
              hintText: 'Search ${_catalogs[_selectedCatalog].title}...',
              prefixIcon: const Icon(Icons.search),
              isDense: true,
            ),
            onChanged: (v) => setState(() => _search = v),
          ),
        ),

        // Filter chips
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
          child: SingleChildScrollView(
            scrollDirection: Axis.horizontal,
            child: Row(
              children: [
                FilterChip(
                  label: Text(_filterMaxMag != null ? 'Mag ≤ ${_filterMaxMag!.toStringAsFixed(0)}' : 'Magnitude'),
                  selected: _filterMaxMag != null,
                  onSelected: (_) => _showMagFilter(),
                ),
                const SizedBox(width: 8),
                FilterChip(
                  label: Text('${items.length} objects'),
                  selected: false,
                  onSelected: null,
                ),
              ],
            ),
          ),
        ),

        // Object list
        Expanded(
          child: ListView.builder(
            itemCount: items.length,
            itemBuilder: (_, i) => _ObjectTile(
              catalog: _catalogs[_selectedCatalog],
              entry: items[i],
              client: ref.read(lx200ClientProvider),
            ),
          ),
        ),
      ],
    );
  }

  void _showMagFilter() {
    showModalBottomSheet(
      context: context,
      builder: (_) => Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text('Maximum Magnitude', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 16),
            Wrap(
              spacing: 8,
              children: [null, 6.0, 8.0, 10.0, 12.0, 14.0].map((v) =>
                ChoiceChip(
                  label: Text(v == null ? 'All' : '≤ ${v.toStringAsFixed(0)}'),
                  selected: _filterMaxMag == v,
                  onSelected: (_) {
                    setState(() => _filterMaxMag = v);
                    Navigator.pop(context);
                  },
                ),
              ).toList(),
            ),
          ],
        ),
      ),
    );
  }
}

class _ObjectTile extends StatelessWidget {
  final Catalog catalog;
  final CatalogEntry entry;
  final LX200TcpClient client;
  const _ObjectTile({required this.catalog, required this.entry, required this.client});

  @override
  Widget build(BuildContext context) {
    final title = entry.name.isNotEmpty
        ? '${catalog.prefix}${entry.id} - ${entry.name}'
        : '${catalog.prefix}${entry.id}';
    final subtitle = '${entry.constellationStr}  ${entry.typeStr}'
        '${entry.mag != null ? '  mag ${entry.mag!.toStringAsFixed(1)}' : ''}';

    return ListTile(
      title: Text(title, style: TextStyle(color: TAColors.textHigh, fontWeight: FontWeight.w500)),
      subtitle: Text(subtitle, style: TextStyle(color: TAColors.textSecondary, fontSize: 12)),
      trailing: Text('${entry.raStr}\n${entry.decStr}',
        textAlign: TextAlign.right,
        style: TextStyle(color: TAColors.text, fontFamily: 'monospace', fontSize: 11)),
      onTap: () => _showActions(context),
    );
  }

  void _showActions(BuildContext context) {
    showModalBottomSheet(
      context: context,
      builder: (_) => Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(entry.name.isNotEmpty ? entry.name : '${catalog.prefix}${entry.id}',
              style: Theme.of(context).textTheme.titleLarge),
            const SizedBox(height: 4),
            Text('RA: ${entry.raStr}  Dec: ${entry.decStr}',
              style: TextStyle(fontFamily: 'monospace', color: TAColors.textSecondary)),
            const SizedBox(height: 24),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () async {
                      await client.sendBool(LX200.setTargetRa(entry.raStr));
                      await client.sendBool(LX200.setTargetDec(entry.decStr));
                      await client.sendCommand(LX200.gotoTarget);
                      if (context.mounted) Navigator.pop(context);
                    },
                    icon: const Icon(Icons.my_location),
                    label: const Text('Goto'),
                  ),
                ),
                const SizedBox(width: 8),
                Expanded(
                  child: ElevatedButton.icon(
                    style: ElevatedButton.styleFrom(backgroundColor: TAColors.surfaceVariant),
                    onPressed: () async {
                      await client.sendBool(LX200.setTargetRa(entry.raStr));
                      await client.sendBool(LX200.setTargetDec(entry.decStr));
                      await client.sendCommand(LX200.syncTarget);
                      if (context.mounted) Navigator.pop(context);
                    },
                    icon: const Icon(Icons.sync),
                    label: const Text('Sync'),
                  ),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
