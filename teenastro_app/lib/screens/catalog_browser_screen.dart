import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../services/catalog_service.dart';
import '../services/catalog_filter_provider.dart';
import '../models/catalog_entry.dart';
import '../models/lx200_commands.dart';
import '../models/mount_state.dart';
import '../theme.dart';

class CatalogBrowserScreen extends ConsumerStatefulWidget {
  const CatalogBrowserScreen({super.key});
  @override
  ConsumerState<CatalogBrowserScreen> createState() => _CatalogBrowserScreenState();
}

class _CatalogBrowserScreenState extends ConsumerState<CatalogBrowserScreen> {
  List<Catalog> _catalogs = [];
  bool _loading = true;
  late final TextEditingController _searchController;

  @override
  void initState() {
    super.initState();
    _searchController = TextEditingController(
      text: ref.read(catalogFilterProvider).search,
    );
    _searchController.addListener(_onSearchChanged);
    _loadCatalogs();
  }

  void _onSearchChanged() {
    ref.read(catalogFilterProvider.notifier).setSearch(_searchController.text);
  }

  @override
  void dispose() {
    _searchController.removeListener(_onSearchChanged);
    _searchController.dispose();
    super.dispose();
  }

  Future<void> _loadCatalogs() async {
    final svc = ref.read(catalogServiceProvider);
    final cats = await svc.loadCatalogs();
    if (mounted) setState(() { _catalogs = cats; _loading = false; });
  }

  List<CatalogEntry> _filtered(CatalogFilterState filter) {
    if (_catalogs.isEmpty) return [];
    final cat = _catalogs[filter.selectedCatalog];
    final mountState = ref.read(mountStateProvider);
    final lst = parseSiderealTime(mountState.siderealTime);
    final lat = mountState.latitude;

    return cat.objects.where((o) {
      if (filter.search.isNotEmpty) {
        final q = filter.search.toLowerCase();
        if (!o.name.toLowerCase().contains(q) &&
            !'${cat.prefix}${o.id}'.toLowerCase().contains(q)) {
          return false;
        }
      }
      if (filter.filterConstellation != null && o.constellation != filter.filterConstellation) return false;
      if (filter.filterType != null && o.objType != filter.filterType) return false;
      if (filter.filterMaxMag != null && o.mag != null && o.mag! > filter.filterMaxMag!) return false;
      if (filter.filterMinAlt != null && lst != null) {
        final alt = objectAltitude(lat, lst, o.ra, o.dec);
        if (alt < filter.filterMinAlt!) {
          return false;
        }
      }
      return true;
    }).toList();
  }

  @override
  Widget build(BuildContext context) {
    final filter = ref.watch(catalogFilterProvider);
    final mountState = ref.watch(mountStateProvider);
    final filterNotifier = ref.read(catalogFilterProvider.notifier);

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

    final items = _filtered(filter);

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
                selected: i == filter.selectedCatalog,
                selectedColor: TAColors.accent,
                labelStyle: TextStyle(
                  color: i == filter.selectedCatalog ? Colors.white : TAColors.text),
                onSelected: (_) => filterNotifier.setSelectedCatalog(i),
              ),
            ),
          ),
        ),

        // Search bar
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
          child: TextField(
            controller: _searchController,
            decoration: InputDecoration(
              hintText: 'Search ${_catalogs[filter.selectedCatalog].title}...',
              prefixIcon: const Icon(Icons.search),
              isDense: true,
            ),
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
                  avatar: const Icon(Icons.terrain, size: 16),
                  label: Text(filter.filterMinAlt != null
                      ? 'Alt ≥ ${filter.filterMinAlt!.toStringAsFixed(0)}°'
                      : 'Horizon'),
                  selected: filter.filterMinAlt != null,
                  onSelected: (_) => _showHorizonFilter(filterNotifier, filter),
                ),
                const SizedBox(width: 8),
                FilterChip(
                  label: Text(filter.filterMaxMag != null ? 'Mag ≤ ${filter.filterMaxMag!.toStringAsFixed(0)}' : 'Magnitude'),
                  selected: filter.filterMaxMag != null,
                  onSelected: (_) => _showMagFilter(filterNotifier, filter),
                ),
                const SizedBox(width: 8),
                FilterChip(
                  label: Text(filter.filterConstellation != null
                      ? constellationAbbr[filter.filterConstellation!]
                      : 'Constellation'),
                  selected: filter.filterConstellation != null,
                  onSelected: (_) => _showConstellationFilter(filterNotifier, filter),
                ),
                const SizedBox(width: 8),
                if (_catalogs[filter.selectedCatalog].type == CatalogType.dso)
                  Padding(
                    padding: const EdgeInsets.only(right: 8),
                    child: FilterChip(
                      label: Text(filter.filterType != null
                          ? dsoTypeNames[filter.filterType!]
                          : 'Type'),
                      selected: filter.filterType != null,
                      onSelected: (_) => _showTypeFilter(filterNotifier, filter),
                    ),
                  ),
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
              catalog: _catalogs[filter.selectedCatalog],
              entry: items[i],
              client: ref.read(lx200ClientProvider),
              mountState: mountState,
            ),
          ),
        ),
      ],
    );
  }

  void _showHorizonFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
    showModalBottomSheet(
      context: context,
      builder: (_) => Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text('Above Horizon', style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 16),
            Wrap(
              spacing: 8,
              children: [null, 0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0].map((v) =>
                ChoiceChip(
                  label: Text(v == null ? 'All' : '≥ ${v.toStringAsFixed(0)}°'),
                  selected: filter.filterMinAlt == v,
                  onSelected: (_) {
                    notifier.setFilterMinAlt(v);
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

  void _showMagFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
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
                  selected: filter.filterMaxMag == v,
                  onSelected: (_) {
                    notifier.setFilterMaxMag(v);
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

  void _showConstellationFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      builder: (_) => DraggableScrollableSheet(
        expand: false,
        initialChildSize: 0.6,
        maxChildSize: 0.9,
        builder: (ctx, scrollCtrl) => Column(
          children: [
            Padding(
              padding: const EdgeInsets.all(16),
              child: Text('Constellation', style: Theme.of(ctx).textTheme.titleMedium),
            ),
            Expanded(
              child: ListView.builder(
                controller: scrollCtrl,
                itemCount: constellationAbbr.length + 1,
                itemBuilder: (_, i) {
                  if (i == 0) {
                    return ListTile(
                      title: const Text('All'),
                      selected: filter.filterConstellation == null,
                      onTap: () {
                        notifier.setFilterConstellation(null);
                        Navigator.pop(context);
                      },
                    );
                  }
                  final idx = i - 1;
                  return ListTile(
                    title: Text(constellationAbbr[idx]),
                    selected: filter.filterConstellation == idx,
                    onTap: () {
                      notifier.setFilterConstellation(idx);
                      Navigator.pop(context);
                    },
                  );
                },
              ),
            ),
          ],
        ),
      ),
    );
  }

  void _showTypeFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
    showModalBottomSheet(
      context: context,
      isScrollControlled: true,
      builder: (_) => DraggableScrollableSheet(
        expand: false,
        initialChildSize: 0.5,
        maxChildSize: 0.8,
        builder: (ctx, scrollCtrl) => Column(
          children: [
            Padding(
              padding: const EdgeInsets.all(16),
              child: Text('Object Type', style: Theme.of(ctx).textTheme.titleMedium),
            ),
            Expanded(
              child: ListView.builder(
                controller: scrollCtrl,
                itemCount: dsoTypeNames.length + 1,
                itemBuilder: (_, i) {
                  if (i == 0) {
                    return ListTile(
                      title: const Text('All'),
                      selected: filter.filterType == null,
                      onTap: () {
                        notifier.setFilterType(null);
                        Navigator.pop(context);
                      },
                    );
                  }
                  final idx = i - 1;
                  return ListTile(
                    title: Text(dsoTypeNames[idx]),
                    selected: filter.filterType == idx,
                    onTap: () {
                      notifier.setFilterType(idx);
                      Navigator.pop(context);
                    },
                  );
                },
              ),
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
  final MountState mountState;
  const _ObjectTile({
    required this.catalog, required this.entry,
    required this.client, required this.mountState,
  });

  @override
  Widget build(BuildContext context) {
    final title = entry.name.isNotEmpty
        ? '${catalog.prefix}${entry.id} - ${entry.name}'
        : '${catalog.prefix}${entry.id}';
    final subtitle = StringBuffer()
      ..write(entry.constellationStr)
      ..write('  ')
      ..write(entry.typeStr);
    if (entry.mag != null) subtitle.write('  mag ${entry.mag!.toStringAsFixed(1)}');

    // Altitude badge
    final lst = parseSiderealTime(mountState.siderealTime);
    String? altStr;
    bool belowHorizon = false;
    if (lst != null) {
      final alt = objectAltitude(mountState.latitude, lst, entry.ra, entry.dec);
      altStr = '${alt.toStringAsFixed(0)}°';
      belowHorizon = alt < 0;
    }

    return ListTile(
      title: Text(title, style: TextStyle(
        color: belowHorizon ? TAColors.textSecondary : TAColors.textHigh,
        fontWeight: FontWeight.w500)),
      subtitle: Row(
        children: [
          Expanded(child: Text(subtitle.toString(),
            style: TextStyle(color: TAColors.textSecondary, fontSize: 12))),
          if (altStr != null)
            Text(altStr, style: TextStyle(
              color: belowHorizon ? TAColors.error : TAColors.success,
              fontSize: 11, fontWeight: FontWeight.w600)),
        ],
      ),
      trailing: Text('${entry.raStr}\n${entry.decStr}',
        textAlign: TextAlign.right,
        style: TextStyle(color: TAColors.text, fontFamily: 'monospace', fontSize: 11)),
      onTap: () => _showActions(context),
    );
  }

  void _showActions(BuildContext context) {
    showModalBottomSheet(
      context: context,
      builder: (_) => _ObjectActionSheet(
        catalog: catalog,
        entry: entry,
        client: client,
        mountState: mountState,
      ),
    );
  }
}

class _ObjectActionSheet extends ConsumerWidget {
  final Catalog catalog;
  final CatalogEntry entry;
  final LX200TcpClient client;
  final MountState mountState;

  const _ObjectActionSheet({
    required this.catalog, required this.entry,
    required this.client, required this.mountState,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final state = ref.watch(mountStateProvider);
    final slewing = state.isSlewing;

    return Padding(
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
                child: slewing
                    ? ElevatedButton.icon(
                        style: ElevatedButton.styleFrom(backgroundColor: TAColors.error),
                        onPressed: () {
                          client.sendImmediate(LX200.stopAll);
                          if (context.mounted) Navigator.pop(context);
                        },
                        icon: const Icon(Icons.stop_circle),
                        label: const Text('Stop'),
                      )
                    : ElevatedButton.icon(
                        onPressed: () async {
                          final okRa = await client.sendBool(LX200.setTargetRa(entry.raStr));
                          final okDec = await client.sendBool(LX200.setTargetDec(entry.decStr));
                          if (!okRa || !okDec) {
                            if (context.mounted) {
                              ScaffoldMessenger.of(context).showSnackBar(const SnackBar(
                                content: Text('Failed to set target coordinates'),
                                backgroundColor: TAColors.error,
                              ));
                              Navigator.pop(context);
                            }
                            return;
                          }
                          final reply = await client.sendCommand(LX200.gotoTarget);
                          if (context.mounted) {
                            if (reply != '0') {
                              final errIdx = int.tryParse(reply ?? '') ?? -1;
                              final msg = errIdx >= 0 && errIdx < GotoError.values.length
                                  ? gotoErrorCause(GotoError.values[errIdx])
                                  : 'Goto failed: $reply';
                              ScaffoldMessenger.of(context).showSnackBar(SnackBar(
                                content: Text(msg),
                                backgroundColor: TAColors.error,
                              ));
                            }
                            Navigator.pop(context);
                          }
                        },
                        icon: const Icon(Icons.my_location),
                        label: const Text('Goto'),
                      ),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(backgroundColor: TAColors.surfaceVariant),
                  onPressed: slewing ? null : () async {
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
    );
  }
}
