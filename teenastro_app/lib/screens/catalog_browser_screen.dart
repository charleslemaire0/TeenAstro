import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../services/lx200_tcp_client.dart';
import '../services/mount_state_provider.dart';
import '../services/catalog_service.dart';
import '../providers/last_goto_route_provider.dart';
import '../services/catalog_filter_provider.dart';
import '../models/catalog_entry.dart';
import '../models/equinox_precession.dart';
import '../models/lx200_commands.dart';
import '../models/planet_positions.dart';
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
    ref.read(lastGotoTabRouteProvider.notifier).state = '/catalogs';
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

  CatalogType get _currentType =>
      _catalogs.isNotEmpty
          ? _catalogs[ref.read(catalogFilterProvider).selectedCatalog].type
          : CatalogType.dso;

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
        if (alt < filter.filterMinAlt!) return false;
      }
      if (filter.filterHasName && o.name.isEmpty) return false;
      // Double-star separation filters
      if (filter.filterDblMinSep != null && o.separation != null && o.separation! < filter.filterDblMinSep!) return false;
      if (filter.filterDblMaxSep != null && o.separation != null && o.separation! > filter.filterDblMaxSep!) return false;
      // Variable-star period filter
      if (filter.filterVarMaxPer != null && o.period != null && o.period! >= 0 && o.period! > filter.filterVarMaxPer!) return false;
      return true;
    }).toList();
  }

  @override
  Widget build(BuildContext context) {
    final filter = ref.watch(catalogFilterProvider);
    final mountState = ref.watch(mountStateProvider);
    final filterNotifier = ref.read(catalogFilterProvider.notifier);

    if (_loading) {
      return Scaffold(
        appBar: _buildAppBar(),
        body: const Center(child: CircularProgressIndicator()),
      );
    }
    if (_catalogs.isEmpty) {
      return Scaffold(
        appBar: _buildAppBar(),
        body: Center(
          child: Padding(
            padding: const EdgeInsets.all(32),
            child: Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.folder_open, size: 64, color: TA.textSecondary),
                const SizedBox(height: 16),
                Text('No catalogs loaded', style: Theme.of(context).textTheme.titleMedium),
                const SizedBox(height: 8),
                Text('Run tools/extract_catalogs.py to generate catalog JSON files.',
                  style: TextStyle(color: TA.textSecondary), textAlign: TextAlign.center),
              ],
            ),
          ),
        ),
      );
    }

    final catType = _currentType;
    final items = _filtered(filter);

    return Scaffold(
      appBar: _buildAppBar(),
      body: Column(
        children: [
          // Catalog selector chips
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
                  selectedColor: TA.accent,
                  labelStyle: TextStyle(
                    color: i == filter.selectedCatalog ? TA.textHigh : TA.text),
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

          // Filter chips (context-dependent on catalog type)
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
            child: SingleChildScrollView(
              scrollDirection: Axis.horizontal,
              child: Row(
                children: [
                  // Above Horizon
                  _filterChip(
                    label: filter.filterMinAlt != null
                        ? 'Alt ≥ ${filter.filterMinAlt!.toStringAsFixed(0)}°'
                        : 'Horizon',
                    icon: Icons.terrain,
                    active: filter.filterMinAlt != null,
                    onTap: () => _showHorizonFilter(filterNotifier, filter),
                  ),
                  // Magnitude
                  _filterChip(
                    label: filter.filterMaxMag != null
                        ? 'Mag ≤ ${filter.filterMaxMag!.toStringAsFixed(0)}'
                        : 'Magnitude',
                    active: filter.filterMaxMag != null,
                    onTap: () => _showMagFilter(filterNotifier, filter),
                  ),
                  // Constellation
                  _filterChip(
                    label: filter.filterConstellation != null
                        ? constellationAbbr[filter.filterConstellation!]
                        : 'Constellation',
                    active: filter.filterConstellation != null,
                    onTap: () => _showConstellationFilter(filterNotifier, filter),
                  ),
                  // DSO type (only for DSO catalogs)
                  if (catType == CatalogType.dso)
                    _filterChip(
                      label: filter.filterType != null
                          ? dsoTypeNames[filter.filterType!]
                          : 'Type',
                      active: filter.filterType != null,
                      onTap: () => _showTypeFilter(filterNotifier, filter),
                    ),
                  // Double-star: min separation
                  if (catType == CatalogType.doubleStar)
                    _filterChip(
                      label: filter.filterDblMinSep != null
                          ? 'Sep ≥ ${_fmtSep(filter.filterDblMinSep!)}'
                          : 'Min Sep',
                      active: filter.filterDblMinSep != null,
                      onTap: () => _showDblMinSepFilter(filterNotifier, filter),
                    ),
                  // Double-star: max separation
                  if (catType == CatalogType.doubleStar)
                    _filterChip(
                      label: filter.filterDblMaxSep != null
                          ? 'Sep ≤ ${_fmtSep(filter.filterDblMaxSep!)}'
                          : 'Max Sep',
                      active: filter.filterDblMaxSep != null,
                      onTap: () => _showDblMaxSepFilter(filterNotifier, filter),
                    ),
                  // Variable-star: max period
                  if (catType == CatalogType.variableStar)
                    _filterChip(
                      label: filter.filterVarMaxPer != null
                          ? 'Per ≤ ${_fmtPer(filter.filterVarMaxPer!)}d'
                          : 'Max Period',
                      active: filter.filterVarMaxPer != null,
                      onTap: () => _showVarMaxPerFilter(filterNotifier, filter),
                    ),
                  // Named-only toggle
                  _filterChip(
                    label: 'Named',
                    icon: Icons.label_outline,
                    active: filter.filterHasName,
                    onTap: () => filterNotifier.setFilterHasName(!filter.filterHasName),
                  ),
                  // Count badge
                  Padding(
                    padding: const EdgeInsets.only(left: 4),
                    child: Chip(
                      label: Text('${items.length}'),
                      visualDensity: VisualDensity.compact,
                    ),
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
      ),
    );
  }

  AppBar _buildAppBar() => AppBar(
    leading: IconButton(
      icon: const Icon(Icons.arrow_back),
      onPressed: () => context.go('/goto'),
      tooltip: 'Back to Goto',
    ),
    title: const Text('Catalogs'),
  );

  Widget _filterChip({
    required String label,
    IconData? icon,
    required bool active,
    required VoidCallback onTap,
  }) {
    return Padding(
      padding: const EdgeInsets.only(right: 8),
      child: FilterChip(
        avatar: icon != null ? Icon(icon, size: 16) : null,
        label: Text(label),
        selected: active,
        onSelected: (_) => onTap(),
      ),
    );
  }

  static String _fmtSep(double v) => v < 1 ? '${v.toStringAsFixed(1)}″' : '${v.toStringAsFixed(0)}″';
  static String _fmtPer(double v) => v < 10 ? v.toStringAsFixed(1) : v.toStringAsFixed(0);

  // ---------------------------------------------------------------------------
  // Filter pickers (bottom sheets)
  // ---------------------------------------------------------------------------

  void _showHorizonFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
    _showChoiceSheet(
      title: 'Above Horizon',
      options: [null, 0.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0],
      labelOf: (v) => v == null ? 'All' : '≥ ${v.toStringAsFixed(0)}°',
      selected: filter.filterMinAlt,
      onSelected: (v) => notifier.setFilterMinAlt(v),
    );
  }

  void _showMagFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
    _showChoiceSheet(
      title: 'Maximum Magnitude',
      options: [null, 6.0, 8.0, 10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0],
      labelOf: (v) => v == null ? 'All' : '≤ ${v.toStringAsFixed(0)}',
      selected: filter.filterMaxMag,
      onSelected: (v) => notifier.setFilterMaxMag(v),
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
                      onTap: () { notifier.setFilterConstellation(null); Navigator.pop(context); },
                    );
                  }
                  final idx = i - 1;
                  return ListTile(
                    title: Text(constellationAbbr[idx]),
                    selected: filter.filterConstellation == idx,
                    onTap: () { notifier.setFilterConstellation(idx); Navigator.pop(context); },
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
                      onTap: () { notifier.setFilterType(null); Navigator.pop(context); },
                    );
                  }
                  final idx = i - 1;
                  return ListTile(
                    title: Text(dsoTypeNames[idx]),
                    selected: filter.filterType == idx,
                    onTap: () { notifier.setFilterType(idx); Navigator.pop(context); },
                  );
                },
              ),
            ),
          ],
        ),
      ),
    );
  }

  void _showDblMinSepFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
    _showChoiceSheet(
      title: 'Min Separation',
      options: [null, 0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 30.0, 50.0],
      labelOf: (v) => v == null ? 'Off' : '≥ ${_fmtSep(v)}',
      selected: filter.filterDblMinSep,
      onSelected: (v) => notifier.setFilterDblMinSep(v),
    );
  }

  void _showDblMaxSepFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
    _showChoiceSheet(
      title: 'Max Separation',
      options: [null, 0.5, 1.0, 2.0, 5.0, 10.0, 20.0, 50.0, 100.0],
      labelOf: (v) => v == null ? 'Off' : '≤ ${_fmtSep(v)}',
      selected: filter.filterDblMaxSep,
      onSelected: (v) => notifier.setFilterDblMaxSep(v),
    );
  }

  void _showVarMaxPerFilter(CatalogFilterNotifier notifier, CatalogFilterState filter) {
    _showChoiceSheet(
      title: 'Max Period',
      options: [null, 0.5, 1.0, 2.0, 5.0, 10.0, 25.0, 50.0, 100.0],
      labelOf: (v) => v == null ? 'Off' : '≤ ${_fmtPer(v)} d',
      selected: filter.filterVarMaxPer,
      onSelected: (v) => notifier.setFilterVarMaxPer(v),
    );
  }

  void _showChoiceSheet<T>({
    required String title,
    required List<T> options,
    required String Function(T) labelOf,
    required T selected,
    required void Function(T) onSelected,
  }) {
    showModalBottomSheet(
      context: context,
      builder: (_) => Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(title, style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 16),
            Wrap(
              spacing: 8,
              runSpacing: 4,
              children: options.map((v) =>
                ChoiceChip(
                  label: Text(labelOf(v)),
                  selected: selected == v,
                  onSelected: (_) { onSelected(v); Navigator.pop(context); },
                ),
              ).toList(),
            ),
          ],
        ),
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

String _periodText(double? period) {
  if (period == null) return '';
  if ((period + 1.0).abs() < 0.1) return 'Unknown';
  if ((period + 2.0).abs() < 0.1) return 'Irregular';
  if (period < 10) return '${period.toStringAsFixed(2)} d';
  return '${period.toStringAsFixed(1)} d';
}

// ---------------------------------------------------------------------------
// Object list tile — shows type-specific secondary info
// ---------------------------------------------------------------------------

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
        ? '${catalog.prefix}${entry.id}${entry.subId ?? ''} – ${entry.name}'
        : '${catalog.prefix}${entry.id}${entry.subId ?? ''}';

    final sub = StringBuffer();
    sub.write(entry.constellationStr);

    switch (catalog.type) {
      case CatalogType.dso:
        if (entry.typeStr.isNotEmpty) sub.write('  ${entry.typeStr}');
        if (entry.mag != null) sub.write('  mag ${entry.mag!.toStringAsFixed(1)}');
      case CatalogType.star:
        if (entry.mag != null) sub.write('  mag ${entry.mag!.toStringAsFixed(1)}');
      case CatalogType.doubleStar:
        if (entry.mag != null) sub.write('  ${entry.mag!.toStringAsFixed(1)}');
        if (entry.mag2 != null) sub.write('+${entry.mag2!.toStringAsFixed(1)}');
        if (entry.separation != null) sub.write('  sep ${entry.separation!.toStringAsFixed(1)}″');
        if (entry.positionAngle != null) sub.write('  PA ${entry.positionAngle}°');
      case CatalogType.variableStar:
        if (entry.mag != null) sub.write('  ${entry.mag!.toStringAsFixed(1)}');
        if (entry.magMin != null) sub.write('–${entry.magMin!.toStringAsFixed(1)}');
        if (entry.period != null) sub.write('  per ${_periodText(entry.period)}');
    }

    final lst = parseSiderealTime(mountState.siderealTime);
    String? altStr;
    bool belowHorizon = false;
    if (lst != null) {
      final alt = objectAltitude(mountState.latitude, lst, entry.ra, entry.dec);
      altStr = '${alt.toStringAsFixed(0)}°';
      belowHorizon = alt < 0;
    }

    return ListTile(
      dense: true,
      title: Text(title, style: TextStyle(
        color: belowHorizon ? TA.textSecondary : TA.textHigh,
        fontWeight: FontWeight.w500, fontSize: 13)),
      subtitle: Row(
        children: [
          Expanded(child: Text(sub.toString(),
            style: TextStyle(color: TA.textSecondary, fontSize: 11),
            maxLines: 1, overflow: TextOverflow.ellipsis)),
          if (altStr != null)
            Text(altStr, style: TextStyle(
              color: belowHorizon ? TA.error : TA.success,
              fontSize: 11, fontWeight: FontWeight.w600)),
        ],
      ),
      trailing: Text('${entry.raStr}\n${entry.decStr}',
        textAlign: TextAlign.right,
        style: TextStyle(color: TA.text, fontFamily: 'monospace', fontSize: 10)),
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

// ---------------------------------------------------------------------------
// Action sheet — type-specific detail + Goto/Sync
// ---------------------------------------------------------------------------

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

    // Compute JNow coordinates
    final jd = julianDate(DateTime.now().toUtc());
    final (raJNow, decJNow) = formatRaDecLx200(
      equatorialEquinoxToJNow(entry.ra, entry.dec, 2000, jd).$1,
      equatorialEquinoxToJNow(entry.ra, entry.dec, 2000, jd).$2,
    );

    final lst = parseSiderealTime(mountState.siderealTime);
    String altStr = '—';
    if (lst != null) {
      final alt = objectAltitude(mountState.latitude, lst, entry.ra, entry.dec);
      altStr = '${alt.toStringAsFixed(1)}°';
    }

    return Padding(
      padding: const EdgeInsets.fromLTRB(20, 16, 20, 20),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          // Title
          Text(
            entry.name.isNotEmpty
                ? '${catalog.prefix}${entry.id}${entry.subId ?? ''} – ${entry.name}'
                : '${catalog.prefix}${entry.id}${entry.subId ?? ''}',
            style: Theme.of(context).textTheme.titleMedium,
          ),
          const SizedBox(height: 8),

          // Type-specific info rows
          ..._infoRows(),

          const SizedBox(height: 8),

          // Coordinates
          _coordRow('J2000', entry.raStr, entry.decStr),
          const SizedBox(height: 2),
          _coordRow('JNow', raJNow, decJNow),
          const SizedBox(height: 2),
          Text('Alt $altStr', style: TextStyle(color: TA.textSecondary, fontSize: 11, fontFamily: 'monospace')),

          const SizedBox(height: 16),

          // Action buttons
          Row(
            children: [
              Expanded(
                child: slewing
                    ? ElevatedButton.icon(
                        style: ElevatedButton.styleFrom(backgroundColor: TA.error),
                        onPressed: () { client.sendImmediate(LX200.stopAll); if (context.mounted) Navigator.pop(context); },
                        icon: const Icon(Icons.stop_circle, size: 16),
                        label: const Text('Stop'),
                      )
                    : ElevatedButton.icon(
                        onPressed: () async {
                          final jd = julianDate(DateTime.now().toUtc());
                          final (raStr, decStr) = j2000ToJNowLx200(entry.ra, entry.dec, jd);
                          final okRa = await client.sendBool(LX200.setTargetRa(raStr));
                          final okDec = await client.sendBool(LX200.setTargetDec(decStr));
                          if (!okRa || !okDec) {
                            if (context.mounted) {
                              ScaffoldMessenger.of(context).showSnackBar(SnackBar(
                                content: const Text('Failed to set target coordinates'),
                                backgroundColor: TA.error,
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
                                content: Text(msg), backgroundColor: TA.error,
                              ));
                            }
                            Navigator.pop(context);
                          }
                        },
                        icon: const Icon(Icons.my_location, size: 16),
                        label: const Text('Goto'),
                      ),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(backgroundColor: TA.surfaceVariant),
                  onPressed: slewing ? null : () async {
                    final jd = julianDate(DateTime.now().toUtc());
                    final (raStr, decStr) = j2000ToJNowLx200(entry.ra, entry.dec, jd);
                    await client.sendBool(LX200.setTargetRa(raStr));
                    await client.sendBool(LX200.setTargetDec(decStr));
                    await client.sendCommand(LX200.syncTarget);
                    if (context.mounted) Navigator.pop(context);
                  },
                  icon: const Icon(Icons.sync, size: 16),
                  label: const Text('Sync'),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  List<Widget> _infoRows() {
    final style = TextStyle(color: TA.textSecondary, fontSize: 12);
    final rows = <Widget>[];

    switch (catalog.type) {
      case CatalogType.dso:
        final parts = <String>[];
        if (entry.typeStr.isNotEmpty) parts.add(entry.typeStr);
        parts.add(entry.constellationStr);
        if (entry.mag != null) parts.add('mag ${entry.mag!.toStringAsFixed(1)}');
        else parts.add('mag —');
        rows.add(Text(parts.join('  •  '), style: style));

      case CatalogType.star:
        final parts = <String>[entry.constellationStr];
        if (entry.mag != null) parts.add('mag ${entry.mag!.toStringAsFixed(1)}');
        rows.add(Text(parts.join('  •  '), style: style));

      case CatalogType.doubleStar:
        final line1 = <String>[entry.constellationStr];
        if (entry.mag != null) {
          String m = 'mag ${entry.mag!.toStringAsFixed(1)}';
          if (entry.mag2 != null) m += ' + ${entry.mag2!.toStringAsFixed(1)}';
          line1.add(m);
        }
        rows.add(Text(line1.join('  •  '), style: style));
        final line2 = <String>[];
        if (entry.separation != null) line2.add('Sep ${entry.separation!.toStringAsFixed(1)}″');
        if (entry.positionAngle != null) line2.add('PA ${entry.positionAngle}°');
        if (line2.isNotEmpty) rows.add(Text(line2.join('   '), style: style));

      case CatalogType.variableStar:
        final line1 = <String>[entry.constellationStr];
        if (entry.mag != null) {
          String m = '${entry.mag!.toStringAsFixed(1)}';
          if (entry.magMin != null) m += ' – ${entry.magMin!.toStringAsFixed(1)}';
          line1.add('mag $m');
        }
        rows.add(Text(line1.join('  •  '), style: style));
        if (entry.period != null) {
          rows.add(Text('Period ${_periodText(entry.period)}', style: style));
        }
    }

    return rows;
  }

  Widget _coordRow(String label, String ra, String dec) {
    return Text(
      '$label  RA $ra   Dec $dec',
      style: TextStyle(color: TA.text, fontSize: 11, fontFamily: 'monospace'),
    );
  }
}
