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
  final bool filterHasName;
  // Double-star filters (arcsec)
  final double? filterDblMinSep;
  final double? filterDblMaxSep;
  // Variable-star filter (days)
  final double? filterVarMaxPer;

  const CatalogFilterState({
    this.selectedCatalog = 0,
    this.search = '',
    this.filterConstellation,
    this.filterType,
    this.filterMaxMag,
    this.filterMinAlt = 0.0,
    this.filterHasName = false,
    this.filterDblMinSep,
    this.filterDblMaxSep,
    this.filterVarMaxPer,
  });

  CatalogFilterState copyWith({
    int? selectedCatalog,
    String? search,
    int? filterConstellation,
    int? filterType,
    double? filterMaxMag,
    double? filterMinAlt,
    bool? filterHasName,
    double? filterDblMinSep,
    double? filterDblMaxSep,
    double? filterVarMaxPer,
    bool clearFilterMinAlt = false,
    bool clearConstellation = false,
    bool clearType = false,
    bool clearMaxMag = false,
    bool clearDblMinSep = false,
    bool clearDblMaxSep = false,
    bool clearVarMaxPer = false,
  }) {
    return CatalogFilterState(
      selectedCatalog: selectedCatalog ?? this.selectedCatalog,
      search: search ?? this.search,
      filterConstellation: clearConstellation ? null : (filterConstellation ?? this.filterConstellation),
      filterType: clearType ? null : (filterType ?? this.filterType),
      filterMaxMag: clearMaxMag ? null : (filterMaxMag ?? this.filterMaxMag),
      filterMinAlt: clearFilterMinAlt ? null : (filterMinAlt ?? this.filterMinAlt),
      filterHasName: filterHasName ?? this.filterHasName,
      filterDblMinSep: clearDblMinSep ? null : (filterDblMinSep ?? this.filterDblMinSep),
      filterDblMaxSep: clearDblMaxSep ? null : (filterDblMaxSep ?? this.filterDblMaxSep),
      filterVarMaxPer: clearVarMaxPer ? null : (filterVarMaxPer ?? this.filterVarMaxPer),
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
    state = v == null
        ? state.copyWith(clearConstellation: true)
        : state.copyWith(filterConstellation: v);
  }

  void setFilterType(int? v) {
    state = v == null
        ? state.copyWith(clearType: true)
        : state.copyWith(filterType: v);
  }

  void setFilterMaxMag(double? v) {
    state = v == null
        ? state.copyWith(clearMaxMag: true)
        : state.copyWith(filterMaxMag: v);
  }

  void setFilterMinAlt(double? v) {
    state = v == null
        ? state.copyWith(clearFilterMinAlt: true)
        : state.copyWith(filterMinAlt: v);
  }

  void setFilterHasName(bool v) {
    state = state.copyWith(filterHasName: v);
  }

  void setFilterDblMinSep(double? v) {
    state = v == null
        ? state.copyWith(clearDblMinSep: true)
        : state.copyWith(filterDblMinSep: v);
  }

  void setFilterDblMaxSep(double? v) {
    state = v == null
        ? state.copyWith(clearDblMaxSep: true)
        : state.copyWith(filterDblMaxSep: v);
  }

  void setFilterVarMaxPer(double? v) {
    state = v == null
        ? state.copyWith(clearVarMaxPer: true)
        : state.copyWith(filterVarMaxPer: v);
  }
}

final catalogFilterProvider =
    StateNotifierProvider<CatalogFilterNotifier, CatalogFilterState>(
        (ref) => CatalogFilterNotifier());
