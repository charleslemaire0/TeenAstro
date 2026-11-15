import 'package:flutter_riverpod/flutter_riverpod.dart';

/// Catalog filters for this app session. Nothing is written to disk.
///
/// Horizon, zenith, star magnitude, deep-sky magnitude, type, separation,
/// and variable period are the kept filters. Constellation is separate:
/// it is not part of that set.
class CatalogFilterState {
  final int selectedCatalog;
  final String search;
  final int? filterConstellation;
  final int? filterType;
  final double? filterStarMaxMag;
  final double? filterDsoMaxMag;
  /// null = All (no horizon filter), 0.0 = above horizon, 10.0 = ≥10°, etc.
  final double? filterMinAlt;
  /// null = no zenith filter. Ceilings are 85, 80, 75, 70, 65.
  final double? filterMaxAlt;
  final bool filterHasName;
  final double? filterDblMinSep;
  final double? filterDblMaxSep;
  final double? filterVarMaxPer;

  const CatalogFilterState({
    this.selectedCatalog = 0,
    this.search = '',
    this.filterConstellation,
    this.filterType,
    this.filterStarMaxMag,
    this.filterDsoMaxMag,
    this.filterMinAlt = 0.0,
    this.filterMaxAlt,
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
    double? filterStarMaxMag,
    double? filterDsoMaxMag,
    double? filterMinAlt,
    double? filterMaxAlt,
    bool? filterHasName,
    double? filterDblMinSep,
    double? filterDblMaxSep,
    double? filterVarMaxPer,
    bool clearFilterMinAlt = false,
    bool clearFilterMaxAlt = false,
    bool clearConstellation = false,
    bool clearType = false,
    bool clearStarMaxMag = false,
    bool clearDsoMaxMag = false,
    bool clearDblMinSep = false,
    bool clearDblMaxSep = false,
    bool clearVarMaxPer = false,
  }) {
    return CatalogFilterState(
      selectedCatalog: selectedCatalog ?? this.selectedCatalog,
      search: search ?? this.search,
      filterConstellation: clearConstellation ? null : (filterConstellation ?? this.filterConstellation),
      filterType: clearType ? null : (filterType ?? this.filterType),
      filterStarMaxMag: clearStarMaxMag ? null : (filterStarMaxMag ?? this.filterStarMaxMag),
      filterDsoMaxMag: clearDsoMaxMag ? null : (filterDsoMaxMag ?? this.filterDsoMaxMag),
      filterMinAlt: clearFilterMinAlt ? null : (filterMinAlt ?? this.filterMinAlt),
      filterMaxAlt: clearFilterMaxAlt ? null : (filterMaxAlt ?? this.filterMaxAlt),
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

  void setFilterStarMaxMag(double? v) {
    state = v == null
        ? state.copyWith(clearStarMaxMag: true)
        : state.copyWith(filterStarMaxMag: v);
  }

  void setFilterDsoMaxMag(double? v) {
    state = v == null
        ? state.copyWith(clearDsoMaxMag: true)
        : state.copyWith(filterDsoMaxMag: v);
  }

  void setFilterMinAlt(double? v) {
    state = v == null
        ? state.copyWith(clearFilterMinAlt: true)
        : state.copyWith(filterMinAlt: v);
  }

  void setFilterMaxAlt(double? v) {
    state = v == null
        ? state.copyWith(clearFilterMaxAlt: true)
        : state.copyWith(filterMaxAlt: v);
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
