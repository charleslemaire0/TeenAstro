import 'package:flutter_riverpod/flutter_riverpod.dart';

class CatalogFilterState {
  final String search;
  final int selectedCatalog;
  final int? filterConstellation;
  final int? filterType;
  final double? filterMaxMag;
  final double? filterMinAlt;

  const CatalogFilterState({
    this.search = '',
    this.selectedCatalog = 0,
    this.filterConstellation,
    this.filterType,
    this.filterMaxMag,
    this.filterMinAlt,
  });

  CatalogFilterState copyWith({
    String? search,
    int? selectedCatalog,
    int? filterConstellation,
    int? filterType,
    double? filterMaxMag,
    double? filterMinAlt,
  }) {
    return CatalogFilterState(
      search: search ?? this.search,
      selectedCatalog: selectedCatalog ?? this.selectedCatalog,
      filterConstellation: filterConstellation ?? this.filterConstellation,
      filterType: filterType ?? this.filterType,
      filterMaxMag: filterMaxMag ?? this.filterMaxMag,
      filterMinAlt: filterMinAlt ?? this.filterMinAlt,
    );
  }
}

class CatalogFilterNotifier extends Notifier<CatalogFilterState> {
  @override
  CatalogFilterState build() => const CatalogFilterState();

  void setSearch(String value) {
    state = state.copyWith(search: value);
  }

  void setSelectedCatalog(int index) {
    state = state.copyWith(selectedCatalog: index);
  }

  void setFilterConstellation(int? value) {
    state = CatalogFilterState(
      search: state.search,
      selectedCatalog: state.selectedCatalog,
      filterConstellation: value,
      filterType: state.filterType,
      filterMaxMag: state.filterMaxMag,
      filterMinAlt: state.filterMinAlt,
    );
  }

  void setFilterType(int? value) {
    state = CatalogFilterState(
      search: state.search,
      selectedCatalog: state.selectedCatalog,
      filterConstellation: state.filterConstellation,
      filterType: value,
      filterMaxMag: state.filterMaxMag,
      filterMinAlt: state.filterMinAlt,
    );
  }

  void setFilterMaxMag(double? value) {
    state = CatalogFilterState(
      search: state.search,
      selectedCatalog: state.selectedCatalog,
      filterConstellation: state.filterConstellation,
      filterType: state.filterType,
      filterMaxMag: value,
      filterMinAlt: state.filterMinAlt,
    );
  }

  void setFilterMinAlt(double? value) {
    state = CatalogFilterState(
      search: state.search,
      selectedCatalog: state.selectedCatalog,
      filterConstellation: state.filterConstellation,
      filterType: state.filterType,
      filterMaxMag: state.filterMaxMag,
      filterMinAlt: value,
    );
  }
}

final catalogFilterProvider =
    NotifierProvider<CatalogFilterNotifier, CatalogFilterState>(
        CatalogFilterNotifier.new);
