import 'dart:convert';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/catalog_entry.dart';

class CatalogService {
  List<Catalog>? _catalogs;

  Future<List<Catalog>> loadCatalogs() async {
    if (_catalogs != null) return _catalogs!;

    final catalogFiles = [
      'messier', 'caldwell', 'herschel', 'ngc', 'ic',
      'stars', 'stf', 'stt', 'gcvs',
    ];

    _catalogs = [];
    for (final name in catalogFiles) {
      try {
        final jsonStr =
            await rootBundle.loadString('assets/catalogs/$name.json');
        final json = jsonDecode(jsonStr) as Map<String, dynamic>;
        _catalogs!.add(Catalog.fromJson(json));
      } catch (_) {
        // Catalog file not yet generated - skip
      }
    }
    return _catalogs!;
  }

  /// Search across all catalogs
  Future<List<MapEntry<Catalog, CatalogEntry>>> search(String query) async {
    final catalogs = await loadCatalogs();
    final results = <MapEntry<Catalog, CatalogEntry>>[];
    final q = query.toLowerCase();

    for (final cat in catalogs) {
      for (final obj in cat.objects) {
        if (obj.name.toLowerCase().contains(q) ||
            '${cat.prefix}${obj.id}'.toLowerCase().contains(q)) {
          results.add(MapEntry(cat, obj));
        }
      }
    }
    return results;
  }
}

final catalogServiceProvider =
    Provider<CatalogService>((ref) => CatalogService());
