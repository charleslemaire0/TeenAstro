import 'package:flutter_riverpod/flutter_riverpod.dart';

/// Persisted catalog filter state. Survives navigation away from the catalog screen.
class CatalogFilterState {
  final int selectedCatalog;
  final String search;
  final int? filterConstellation;
  final int? filterType;
  final double? filterMaxMag;
  /// null = All (no horizon filter), 0.0 = default (above horizon), 10.0 = ≥10°, etc.
  final double? filterMinAlt;

  const CatalogFilterState({
    this.selectedCatalog = 0,
    this.search = '',
    this.filterConstellation,
    this.filterType,
    this.filterMaxMag,
    this.filterMinAlt = 0.0,
  });

  CatalogFilterState copyWith({
    int? selectedCatalog,
    String? search,
    int? filterConstellation,
    int? filterType,
    double? filterMaxMag,
    double? filterMinAlt,
    bool clearFilterMinAlt = false,
  }) {
    return CatalogFilterState(
      selectedCatalog: selectedCatalog ?? this.selectedCatalog,
      search: search ?? this.search,
      filterConstellation: filterConstellation ?? this.filterConstellation,
      filterType: filterType ?? this.filterType,
      filterMaxMag: filterMaxMag ?? this.filterMaxMag,
      filterMinAlt: clearFilterMinAlt ? null : (filterMinAlt ?? this.filterMinAlt),
    );
  }
}

class CatalogFilterNotifier extends StateNotifier<CatalogFilterState> {
  CatalogFilterNotifier() : super(const CatalogFilterState());

  void setSelectedCatalog(int i) {
    state = state.copyWith(selectedCatalog: i);
  }

  void setSearch(String s) {
    state = state.copyWith(search: s);
  }

  void setFilterConstellation(int? v) {
    state = state.copyWith(filterConstellation: v);
  }

  void setFilterType(int? v) {
    state = state.copyWith(filterType: v);
  }

  void setFilterMaxMag(double? v) {
    state = state.copyWith(filterMaxMag: v);
  }

  void setFilterMinAlt(double? v) {
    state = v == null
        ? state.copyWith(clearFilterMinAlt: true)
        : state.copyWith(filterMinAlt: v);
  }
}

final catalogFilterProvider =
    StateNotifierProvider<CatalogFilterNotifier, CatalogFilterState>(
        (ref) => CatalogFilterNotifier());
