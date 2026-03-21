import 'dart:convert';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';

const _prefsKey = 'planetarium_settings';

enum CoordDisplay { equatorial, horizontal }
enum EpochMode { jNow, j2000 }

class PlanetariumSettings {
  // Rendering scales
  final double starScale;
  final double objectScale;

  // Coordinate display
  final CoordDisplay coordDisplay;

  // Epoch: JNow (precess catalog stars to current date) or J2000 (convert mount coords to J2000)
  final EpochMode epochMode;

  // Layer visibility
  final bool constellationLines;
  final bool constellationNames;
  final bool altAzGrid;
  final bool eqGrid;
  final bool milkyWay;
  final bool dsoObjects;
  final bool planetLabels;
  final bool starLabels;

  // Persisted view state (restored when app restarts)
  final double lastZoom;
  final double lastRotation;
  final double lastPanY;

  const PlanetariumSettings({
    this.starScale = 3.0,
    this.objectScale = 1.0,
    this.coordDisplay = CoordDisplay.equatorial,
    this.epochMode = EpochMode.jNow,
    this.constellationLines = true,
    this.constellationNames = true,
    this.altAzGrid = false,
    this.eqGrid = false,
    this.milkyWay = true,
    this.dsoObjects = true,
    this.planetLabels = true,
    this.starLabels = true,
    this.lastZoom = 1.0,
    this.lastRotation = 0.0,
    this.lastPanY = 0.0,
  });

  PlanetariumSettings copyWith({
    double? starScale,
    double? objectScale,
    CoordDisplay? coordDisplay,
    EpochMode? epochMode,
    bool? constellationLines,
    bool? constellationNames,
    bool? altAzGrid,
    bool? eqGrid,
    bool? milkyWay,
    bool? dsoObjects,
    bool? planetLabels,
    bool? starLabels,
    double? lastZoom,
    double? lastRotation,
    double? lastPanY,
  }) {
    return PlanetariumSettings(
      starScale: starScale ?? this.starScale,
      objectScale: objectScale ?? this.objectScale,
      coordDisplay: coordDisplay ?? this.coordDisplay,
      epochMode: epochMode ?? this.epochMode,
      constellationLines: constellationLines ?? this.constellationLines,
      constellationNames: constellationNames ?? this.constellationNames,
      altAzGrid: altAzGrid ?? this.altAzGrid,
      eqGrid: eqGrid ?? this.eqGrid,
      milkyWay: milkyWay ?? this.milkyWay,
      dsoObjects: dsoObjects ?? this.dsoObjects,
      planetLabels: planetLabels ?? this.planetLabels,
      starLabels: starLabels ?? this.starLabels,
      lastZoom: lastZoom ?? this.lastZoom,
      lastRotation: lastRotation ?? this.lastRotation,
      lastPanY: lastPanY ?? this.lastPanY,
    );
  }

  Map<String, dynamic> toJson() => {
        'starScale': starScale,
        'objectScale': objectScale,
        'coordDisplay': coordDisplay.name,
        'epochMode': epochMode.name,
        'constellationLines': constellationLines,
        'constellationNames': constellationNames,
        'altAzGrid': altAzGrid,
        'eqGrid': eqGrid,
        'milkyWay': milkyWay,
        'dsoObjects': dsoObjects,
        'planetLabels': planetLabels,
        'starLabels': starLabels,
        'lastZoom': lastZoom,
        'lastRotation': lastRotation,
        'lastPanY': lastPanY,
      };

  factory PlanetariumSettings.fromJson(Map<String, dynamic> j) {
    return PlanetariumSettings(
      starScale: (j['starScale'] as num?)?.toDouble() ?? 3.0,
      objectScale: (j['objectScale'] as num?)?.toDouble() ?? 1.0,
      coordDisplay: j['coordDisplay'] == 'horizontal'
          ? CoordDisplay.horizontal
          : CoordDisplay.equatorial,
      epochMode: j['epochMode'] == 'j2000'
          ? EpochMode.j2000
          : EpochMode.jNow,
      constellationLines: j['constellationLines'] as bool? ?? true,
      constellationNames: j['constellationNames'] as bool? ?? true,
      altAzGrid: j['altAzGrid'] as bool? ?? false,
      eqGrid: j['eqGrid'] as bool? ?? false,
      milkyWay: j['milkyWay'] as bool? ?? true,
      dsoObjects: j['dsoObjects'] as bool? ?? true,
      planetLabels: j['planetLabels'] as bool? ?? true,
      starLabels: j['starLabels'] as bool? ?? true,
      lastZoom: (j['lastZoom'] as num?)?.toDouble() ?? 1.0,
      lastRotation: (j['lastRotation'] as num?)?.toDouble() ?? 0.0,
      lastPanY: (j['lastPanY'] as num?)?.toDouble() ?? 0.0,
    );
  }
}

class PlanetariumSettingsNotifier extends StateNotifier<PlanetariumSettings> {
  PlanetariumSettingsNotifier() : super(const PlanetariumSettings()) {
    _load();
  }

  Future<void> _load() async {
    final prefs = await SharedPreferences.getInstance();
    final raw = prefs.getString(_prefsKey);
    if (raw != null) {
      try {
        state = PlanetariumSettings.fromJson(
            jsonDecode(raw) as Map<String, dynamic>);
      } catch (_) {}
    }
  }

  Future<void> _save() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_prefsKey, jsonEncode(state.toJson()));
  }

  void setStarScale(double v) {
    state = state.copyWith(starScale: v.clamp(1.0, 10.0));
    _save();
  }

  void setObjectScale(double v) {
    state = state.copyWith(objectScale: v.clamp(0.3, 3.0));
    _save();
  }

  void setCoordDisplay(CoordDisplay v) {
    state = state.copyWith(coordDisplay: v);
    _save();
  }

  void setEpochMode(EpochMode v) {
    state = state.copyWith(epochMode: v);
    _save();
  }

  void setLayer(String name, bool value) {
    switch (name) {
      case 'constellationLines':
        state = state.copyWith(constellationLines: value);
      case 'constellationNames':
        state = state.copyWith(constellationNames: value);
      case 'altAzGrid':
        state = state.copyWith(altAzGrid: value);
      case 'eqGrid':
        state = state.copyWith(eqGrid: value);
      case 'milkyWay':
        state = state.copyWith(milkyWay: value);
      case 'dsoObjects':
        state = state.copyWith(dsoObjects: value);
      case 'planetLabels':
        state = state.copyWith(planetLabels: value);
      case 'starLabels':
        state = state.copyWith(starLabels: value);
    }
    _save();
  }

  void saveViewState(double zoom, double rotation, double panY) {
    state = state.copyWith(
        lastZoom: zoom, lastRotation: rotation, lastPanY: panY);
    _save();
  }

  void resetToDefaults() {
    state = const PlanetariumSettings();
    _save();
  }
}

final planetariumSettingsProvider =
    StateNotifierProvider<PlanetariumSettingsNotifier, PlanetariumSettings>(
  (ref) => PlanetariumSettingsNotifier(),
);
