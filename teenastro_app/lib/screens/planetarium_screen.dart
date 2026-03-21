import 'dart:convert';
import 'dart:math' as math;
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import '../models/catalog_entry.dart';
import '../models/constellation_lines.dart';
import '../models/equinox_precession.dart';
import '../models/milky_way_data.dart';
import '../models/mount_state.dart';
import '../models/planet_positions.dart';
import '../models/lx200_commands.dart';
import '../services/catalog_service.dart';
import '../services/lx200_tcp_client.dart';
import '../providers/last_goto_route_provider.dart';
import '../providers/planetarium_settings_provider.dart';
import '../services/mount_state_provider.dart';
import '../theme.dart';

// ---------------------------------------------------------------------------
// Selectable sky object (unified for stars, DSOs, planets)
// ---------------------------------------------------------------------------

class _SkyObject {
  final String name;
  final String type;
  final String constellation;
  final double ra;
  final double dec;
  final double? mag;
  final Color? color;
  final String? catalogPrefix;
  final int? catalogId;
  /// True for solar-system objects whose RA/Dec are already apparent (JNow).
  final bool isJNow;

  const _SkyObject({
    required this.name,
    required this.type,
    this.constellation = '',
    required this.ra,
    required this.dec,
    this.mag,
    this.color,
    this.catalogPrefix,
    this.catalogId,
    this.isJNow = false,
  });
}

String _objectSubtitle(_SkyObject obj) {
  final parts = <String>[obj.type];
  if (obj.constellation.isNotEmpty) parts.add(obj.constellation);
  parts.add(obj.mag != null ? 'mag ${obj.mag!.toStringAsFixed(1)}' : 'mag —');
  return parts.join('  •  ');
}

// ---------------------------------------------------------------------------
// PlanetariumScreen
// ---------------------------------------------------------------------------

class PlanetariumScreen extends ConsumerStatefulWidget {
  const PlanetariumScreen({super.key});

  @override
  ConsumerState<PlanetariumScreen> createState() => _PlanetariumScreenState();
}

class _PlanetariumScreenState extends ConsumerState<PlanetariumScreen> {
  // Data
  List<_PlanetariumStar>? _stars;
  List<Constellation>? _constellations;
  List<_ConstellationLabel>? _constNames;
  MilkyWayBand? _milkyWay;
  List<CatalogEntry>? _dsos;
  List<Catalog>? _catalogs;

  // View
  double _zoom = 1.0;
  double _rotation = 0.0;   // radians, horizontal pan rotates the map
  double _panY = 0.0;       // vertical offset (pixels)
  Offset _lastFocalPoint = Offset.zero;
  double _lastScale = 1.0;

  // UI state
  _SkyObject? _selectedObject;
  bool _showSearch = false;
  bool _showLayers = false;
  bool _uiVisible = true;
  final _searchController = TextEditingController();
  List<_SkyObject> _searchResults = [];

  @override
  void initState() {
    super.initState();
    ref.read(lastGotoTabRouteProvider.notifier).state = '/planetarium';
    final saved = ref.read(planetariumSettingsProvider);
    _zoom = saved.lastZoom;
    _rotation = saved.lastRotation;
    _panY = saved.lastPanY;
    SystemChrome.setEnabledSystemUIMode(SystemUiMode.immersiveSticky);
    _loadAllData();
    _scheduleUiHide();
  }

  @override
  void dispose() {
    ref.read(planetariumSettingsProvider.notifier)
        .saveViewState(_zoom, _rotation, _panY);
    SystemChrome.setEnabledSystemUIMode(SystemUiMode.edgeToEdge);
    _searchController.dispose();
    super.dispose();
  }

  void _scheduleUiHide() {
    Future.delayed(const Duration(seconds: 4), () {
      if (mounted && !_showSearch && !_showLayers && _selectedObject == null) {
        setState(() => _uiVisible = false);
      }
    });
  }

  void _showUi() {
    setState(() => _uiVisible = true);
    _scheduleUiHide();
  }

  Future<void> _loadAllData() async {
    final futures = await Future.wait([
      _loadStars(),
      ConstellationData.load(),
      _loadConstellationNames(),
      MilkyWayBand.load(),
      ref.read(catalogServiceProvider).loadCatalogs(),
    ]);

    if (!mounted) return;
    setState(() {
      _stars = futures[0] as List<_PlanetariumStar>;
      _constellations = futures[1] as List<Constellation>;
      _constNames = futures[2] as List<_ConstellationLabel>;
      _milkyWay = futures[3] as MilkyWayBand;
      _catalogs = futures[4] as List<Catalog>;
      _dsos = [];
      for (final cat in _catalogs!) {
        if (cat.type == CatalogType.dso) {
          _dsos!.addAll(cat.objects);
        }
      }
    });
  }

  Future<List<_PlanetariumStar>> _loadStars() async {
    try {
      final jsonStr =
          await rootBundle.loadString('assets/data/stars_mag9.json');
      final json = jsonDecode(jsonStr) as Map<String, dynamic>;
      final list = json['stars'] as List;
      return list.map((s) {
        final m = s as Map<String, dynamic>;
        return _PlanetariumStar(
          ra: (m['ra'] as num).toDouble(),
          dec: (m['dec'] as num).toDouble(),
          mag: (m['mag'] as num).toDouble(),
          bv: (m['bv'] as num?)?.toDouble(),
          name: m['name'] as String? ?? '',
        );
      }).toList();
    } catch (_) {
      return [];
    }
  }

  Future<List<_ConstellationLabel>> _loadConstellationNames() async {
    try {
      final jsonStr =
          await rootBundle.loadString('assets/data/constellation_names.json');
      final list = jsonDecode(jsonStr) as List;
      return list.map((c) {
        final m = c as Map<String, dynamic>;
        return _ConstellationLabel(
          abbr: m['abbr'] as String,
          name: m['name'] as String,
          ra: (m['ra'] as num).toDouble(),
          dec: (m['dec'] as num).toDouble(),
        );
      }).toList();
    } catch (_) {
      return [];
    }
  }

  // -----------------------------------------------------------------------
  // Tap handling
  // -----------------------------------------------------------------------

  void _handleTap(Offset localPos, MountState state) {
    final size = context.size;
    if (size == null) return;

    final settings = ref.read(planetariumSettingsProvider);
    final proj = _Proj(
      lat: state.latitude,
      lst: _parseLST(state.siderealTime),
      size: size,
      zoom: _zoom,
      rotation: _rotation,
      panY: _panY,
    );

    _SkyObject? best;
    double bestDist = 30.0;

    final jd = julianDate(DateTime.now().toUtc());
    // Display always uses JNow (mount and catalog precession).
    (double, double) catCoords(double ra, double dec) =>
        equatorialEquinoxToJNow(ra, dec, 2000, jd);
    (double, double) mountCoords(double ra, double dec) => (ra, dec);

    // Check planets (JNow source)
    for (final body in allBodies(jd)) {
      final (pRa, pDec) = mountCoords(body.ra, body.dec);
      final pt = proj.project(pRa, pDec);
      if (pt == null) continue;
      final d = (pt - localPos).distance;
      if (d < bestDist) {
        bestDist = d;
        best = _SkyObject(
          name: body.name,
          type: 'Solar System',
          ra: body.ra,
          dec: body.dec,
          mag: body.mag,
          color: body.color,
          isJNow: true,
        );
      }
    }

    // Check stars (J2000 catalog)
    if (_stars != null) {
      final starMagLimit = 6.0 + 2.5 * (math.log(_zoom.clamp(0.5, 30)) / math.ln10);
      for (final star in _stars!) {
        if (star.mag > starMagLimit) continue;
        final (sRa, sDec) = catCoords(star.ra, star.dec);
        final pt = proj.project(sRa, sDec);
        if (pt == null) continue;
        final d = (pt - localPos).distance;
        if (d < bestDist) {
          bestDist = d;
          best = _SkyObject(
            name: star.name.isNotEmpty ? star.name : 'HIP star',
            type: 'Star',
            ra: star.ra,
            dec: star.dec,
            mag: star.mag,
          );
        }
      }
    }

    // Check DSOs (J2000 catalog)
    if (_dsos != null && settings.dsoObjects) {
      for (final dso in _dsos!) {
        final (dRa, dDec) = catCoords(dso.ra, dso.dec);
        final pt = proj.project(dRa, dDec);
        if (pt == null) continue;
        final d = (pt - localPos).distance;
        if (d < bestDist) {
          bestDist = d;
          final catName = _catalogs
                  ?.where((c) => c.objects.contains(dso))
                  .map((c) => c.prefix)
                  .firstOrNull ??
              '';
          best = _SkyObject(
            name: dso.name.isNotEmpty
                ? dso.name
                : '$catName${dso.id}',
            type: dso.typeStr,
            constellation: dso.constellationStr,
            ra: dso.ra,
            dec: dso.dec,
            mag: dso.mag,
            catalogPrefix: catName,
            catalogId: dso.id,
          );
        }
      }
    }

    setState(() {
      _selectedObject = best;
      _showSearch = false;
      _showLayers = false;
    });
  }

  // -----------------------------------------------------------------------
  // Search
  // -----------------------------------------------------------------------

  void _performSearch(String query) {
    if (query.length < 2) {
      setState(() => _searchResults = []);
      return;
    }
    final q = query.toLowerCase();
    final results = <_SkyObject>[];

    // Planets
    final jd = julianDate(DateTime.now().toUtc());
    for (final body in allBodies(jd)) {
      if (body.name.toLowerCase().contains(q)) {
        results.add(_SkyObject(
          name: body.name,
          type: 'Solar System',
          ra: body.ra,
          dec: body.dec,
          mag: body.mag,
          color: body.color,
          isJNow: true,
        ));
      }
    }

    // Named stars
    if (_stars != null) {
      for (final star in _stars!) {
        if (star.name.isNotEmpty && star.name.toLowerCase().contains(q)) {
          results.add(_SkyObject(
            name: star.name,
            type: 'Star',
            ra: star.ra,
            dec: star.dec,
            mag: star.mag,
          ));
        }
        if (results.length > 50) break;
      }
    }

    // DSO catalogs
    if (_catalogs != null) {
      for (final cat in _catalogs!) {
        for (final obj in cat.objects) {
          if (obj.name.toLowerCase().contains(q) ||
              '${cat.prefix}${obj.id}'.toLowerCase().contains(q)) {
            results.add(_SkyObject(
              name: obj.name.isNotEmpty ? obj.name : '${cat.prefix}${obj.id}',
              type: obj.typeStr,
              constellation: obj.constellationStr,
              ra: obj.ra,
              dec: obj.dec,
              mag: obj.mag,
              catalogPrefix: cat.prefix,
              catalogId: obj.id,
            ));
          }
          if (results.length > 50) break;
        }
        if (results.length > 50) break;
      }
    }

    setState(() => _searchResults = results);
  }

  /// Zoom level when centering on a searched/selected object (object fills view).
  static const double _centerOnObjectZoom = 10.0;

  void _centerOnObject(_SkyObject obj) {
    final state = ref.read(mountStateProvider);
    final jd = julianDate(DateTime.now().toUtc());
    // Display always JNow.
    final double dispRa, dispDec;
    if (obj.isJNow) {
      dispRa = obj.ra;
      dispDec = obj.dec;
    } else {
      (dispRa, dispDec) = equatorialEquinoxToJNow(obj.ra, obj.dec, 2000, jd);
    }
    final lst = _parseLST(state.siderealTime);
    final latRad = state.latitude * math.pi / 180;
    final ha = (lst - dispRa) * 15 * math.pi / 180;
    final dec = dispDec * math.pi / 180;
    final sinAlt = math.sin(dec) * math.sin(latRad) + math.cos(dec) * math.cos(latRad) * math.cos(ha);
    final alt = math.asin(sinAlt.clamp(-1.0, 1.0));
    final cosAlt = math.cos(alt);
    final denom = cosAlt * math.cos(latRad);
    final cosAz = denom.abs() < 1e-10 ? 1.0 : ((math.sin(dec) - sinAlt * math.sin(latRad)) / denom);
    var az = math.acos(cosAz.clamp(-1.0, 1.0));
    if (math.sin(ha) > 0) az = 2 * math.pi - az;

    double panY = 0;
    final size = context.size;
    if (size != null) {
      final diag = math.sqrt(size.width * size.width + size.height * size.height);
      final radius = (diag / 2) * _centerOnObjectZoom;
      panY = radius * cosAlt / (1 + math.sin(alt));
    }

    setState(() {
      _rotation = -az;
      _panY = panY;
      _zoom = _centerOnObjectZoom;
      _selectedObject = obj;
      _showSearch = false;
    });
  }

  void _recenterOnTelescope() {
    final state = ref.read(mountStateProvider);
    final size = context.size;
    if (size == null) return;
    final lst = _parseLST(state.siderealTime);
    final raH = _parseLST(state.ra);
    final decDeg = _parseCoord(state.dec);
    final latRad = state.latitude * math.pi / 180;
    final ha = (lst - raH) * 15 * math.pi / 180;
    final decRad = decDeg * math.pi / 180;

    final sinAlt = math.sin(decRad) * math.sin(latRad) +
        math.cos(decRad) * math.cos(latRad) * math.cos(ha);
    final alt = math.asin(sinAlt.clamp(-1.0, 1.0));
    final cosAlt = math.cos(alt);
    final denom = cosAlt * math.cos(latRad);
    final cosAz = denom.abs() < 1e-10
        ? 1.0
        : ((math.sin(decRad) - sinAlt * math.sin(latRad)) / denom);
    var az = math.acos(cosAz.clamp(-1.0, 1.0));
    if (math.sin(ha) > 0) az = 2 * math.pi - az;

    final diag = math.sqrt(size.width * size.width + size.height * size.height);
    final radius = (diag / 2) * _zoom;
    final r = radius * cosAlt / (1 + math.sin(alt));

    setState(() {
      _rotation = -az;
      _panY = r;
    });
  }

  // -----------------------------------------------------------------------
  // Build
  // -----------------------------------------------------------------------

  String _hudCoordText(MountState state, PlanetariumSettings settings) {
    if (settings.coordDisplay == CoordDisplay.horizontal) {
      final lst = _parseLST(state.siderealTime);
      final raH = _parseLST(state.ra);
      final dec = _parseCoord(state.dec);
      final latRad = state.latitude * math.pi / 180;
      final ha = (lst - raH) * 15 * math.pi / 180;
      final decRad = dec * math.pi / 180;
      final sinAlt = math.sin(decRad) * math.sin(latRad) +
          math.cos(decRad) * math.cos(latRad) * math.cos(ha);
      final alt = math.asin(sinAlt.clamp(-1.0, 1.0));
      final cosAlt = math.cos(alt);
      final denom = cosAlt * math.cos(latRad);
      final cosAz = denom.abs() < 1e-10
          ? 1.0
          : ((math.sin(decRad) - sinAlt * math.sin(latRad)) / denom);
      var az = math.acos(cosAz.clamp(-1.0, 1.0));
      if (math.sin(ha) > 0) az = 2 * math.pi - az;
      final altDeg = alt * 180 / math.pi;
      final azDeg = az * 180 / math.pi;
      return 'Alt ${altDeg.toStringAsFixed(1)}°  Az ${azDeg.toStringAsFixed(1)}°';
    }
    return '${state.ra} / ${state.dec.replaceAll('*', '°')}';
  }

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(mountStateProvider);
    final settings = ref.watch(planetariumSettingsProvider);

    if (_stars == null) {
      return Scaffold(
        backgroundColor: TA.isNight ? const Color(0xFF0A0404) : const Color(0xFF060A10),
        body: const Center(child: CircularProgressIndicator()),
      );
    }

    return Scaffold(
      backgroundColor: TA.isNight ? const Color(0xFF0A0404) : const Color(0xFF060A10),
      body: Stack(
        children: [
          // Full-bleed sky renderer
          Listener(
            onPointerSignal: (event) {
              if (event is PointerScrollEvent) {
                _showUi();
                setState(() {
                  final oldZoom = _zoom;
                  final delta = event.scrollDelta.dy > 0 ? 0.9 : 1.1;
                  _zoom = (_zoom * delta).clamp(0.5, 30.0);
                  _panY = _panY * (_zoom / oldZoom);
                });
              }
            },
            child: GestureDetector(
              onScaleStart: (d) {
                _showUi();
                _lastFocalPoint = d.localFocalPoint;
                _lastScale = 1.0;
              },
              onScaleUpdate: (d) {
                setState(() {
                  final oldZoom = _zoom;
                  _zoom = (_zoom * (d.scale / _lastScale)).clamp(0.5, 30.0);
                  _lastScale = d.scale;
                  _panY = _panY * (_zoom / oldZoom);

                  final delta = d.localFocalPoint - _lastFocalPoint;
                  _lastFocalPoint = d.localFocalPoint;

                  final size = context.size;
                  if (size != null) {
                    final cx = size.width / 2;
                    final cy = size.height / 2 + _panY;
                    final fDy = cy - d.localFocalPoint.dy;
                    final fDx = cx - d.localFocalPoint.dx;
                    // Damped Jacobian inversion: avoids singularity when finger
                    // is on the horizontal equator of the projection (fDy ≈ 0).
                    const eps2 = 30.0 * 30.0;
                    final denom = fDy * fDy + eps2;
                    _rotation -= delta.dx * fDy / denom;
                    _panY += delta.dy + delta.dx * fDx * fDy / denom;
                  } else {
                    _panY += delta.dy;
                  }
                });
              },
              onTapUp: (d) {
                if (!_uiVisible && _selectedObject == null) {
                  _showUi();
                } else {
                  _showUi();
                  _handleTap(d.localPosition, state);
                }
              },
              child: RepaintBoundary(
                child: CustomPaint(
                  size: Size.infinite,
                  painter: _SkyRenderer(
                    stars: _stars!,
                    constellations: _constellations ?? [],
                    constNames: _constNames ?? [],
                    milkyWay: _milkyWay,
                    dsos: settings.dsoObjects ? _dsos : null,
                    state: state,
                    zoom: _zoom,
                    rotation: _rotation,
                    panY: _panY,
                    settings: settings,
                    selectedObject: _selectedObject,
                    nightMode: TA.isNight,
                  ),
                ),
              ),
            ),
          ),

          // Auto-hiding HUD overlays
          AnimatedOpacity(
            opacity: _uiVisible ? 1.0 : 0.0,
            duration: const Duration(milliseconds: 400),
            child: IgnorePointer(
              ignoring: !_uiVisible,
              child: Stack(
                children: [
                  // Top-left: back button + zoom/coord info
                  Positioned(
                    top: MediaQuery.of(context).padding.top + 4,
                    left: 4,
                    child: Row(
                      children: [
                        IconButton(
                          icon: Icon(Icons.arrow_back, color: TA.text, size: 20),
                          onPressed: () => context.go('/goto'),
                          style: IconButton.styleFrom(
                            backgroundColor: Colors.black45,
                            minimumSize: const Size(36, 36),
                          ),
                        ),
                        const SizedBox(width: 4),
                        Container(
                          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                          decoration: BoxDecoration(
                            color: Colors.black54,
                            borderRadius: BorderRadius.circular(4),
                          ),
                          child: Text(
                            '${_zoom.toStringAsFixed(1)}x  ${_hudCoordText(state, settings)}',
                            style: TextStyle(
                              color: TA.text,
                              fontSize: 10,
                              fontFamily: 'monospace',
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),

                  // Top-right: layer toggle
                  Positioned(
                    top: MediaQuery.of(context).padding.top + 4,
                    right: 4,
                    child: IconButton(
                      icon: Icon(Icons.layers, color: TA.text, size: 20),
                      onPressed: () => setState(() {
                        _showLayers = !_showLayers;
                        _showSearch = false;
                      }),
                      style: IconButton.styleFrom(
                        backgroundColor: Colors.black45,
                        minimumSize: const Size(36, 36),
                      ),
                    ),
                  ),

                  // Layer toggle panel
                  if (_showLayers)
                    Positioned(
                      top: MediaQuery.of(context).padding.top + 44,
                      right: 4,
                      child: _LayerPanel(
                        settings: settings,
                        notifier: ref.read(planetariumSettingsProvider.notifier),
                        onChanged: () => setState(() {}),
                      ),
                    ),

                  // Recenter on telescope FAB
                  if (!_showSearch && _selectedObject == null)
                    Positioned(
                      bottom: MediaQuery.of(context).padding.bottom + 16,
                      left: 16,
                      child: FloatingActionButton.small(
                        backgroundColor: TA.surfaceVariant,
                        onPressed: _recenterOnTelescope,
                        tooltip: 'Recenter on telescope',
                        child: Icon(Icons.center_focus_strong, color: TA.textHigh),
                      ),
                    ),

                  // Search FAB
                  if (!_showSearch && _selectedObject == null)
                    Positioned(
                      bottom: MediaQuery.of(context).padding.bottom + 16,
                      right: 16,
                      child: FloatingActionButton.small(
                        backgroundColor: TA.accent,
                        onPressed: () => setState(() {
                          _showSearch = true;
                          _showLayers = false;
                        }),
                        child: Icon(Icons.search, color: TA.textHigh),
                      ),
                    ),
                ],
              ),
            ),
          ),

          // Search overlay (always interactive)
          if (_showSearch)
            Positioned.fill(
              child: _SearchPanel(
                controller: _searchController,
                results: _searchResults,
                onSearch: _performSearch,
                onSelect: _centerOnObject,
                onClose: () => setState(() => _showSearch = false),
              ),
            ),

          // Object info panel (always interactive)
          if (_selectedObject != null)
            Positioned(
              bottom: 0,
              left: 0,
              right: 0,
              child: _ObjectInfoPanel(
                object: _selectedObject!,
                onClose: () => setState(() {
                  _selectedObject = null;
                  _scheduleUiHide();
                }),
              ),
            ),
        ],
      ),
    );
  }
}

// ---------------------------------------------------------------------------
// Data types
// ---------------------------------------------------------------------------

class _PlanetariumStar {
  final double ra, dec, mag;
  final double? bv;
  final String name;
  const _PlanetariumStar({
    required this.ra,
    required this.dec,
    required this.mag,
    this.bv,
    this.name = '',
  });
}

class _ConstellationLabel {
  final String abbr, name;
  final double ra, dec;
  const _ConstellationLabel({
    required this.abbr,
    required this.name,
    required this.ra,
    required this.dec,
  });
}

// ---------------------------------------------------------------------------
// Projection (stereographic, same as sky_map.dart)
// ---------------------------------------------------------------------------

double _parseLST(String s) {
  if (s == '?' || s.isEmpty) return 0;
  final parts = s.split(':');
  if (parts.isEmpty) return 0;
  final h = double.tryParse(parts[0]) ?? 0;
  final m = parts.length > 1 ? (double.tryParse(parts[1]) ?? 0) : 0;
  final sec = parts.length > 2 ? (double.tryParse(parts[2]) ?? 0) : 0;
  return h + m / 60 + sec / 3600;
}

double _parseCoord(String s) {
  if (s == '?' || s.isEmpty) return 0;
  final sign = s.startsWith('-') ? -1.0 : 1.0;
  final cleaned = s.replaceAll('*', ':').replaceAll(RegExp(r'[^0-9.:]'), '');
  final parts = cleaned.split(':');
  if (parts.isEmpty) return 0;
  final d = double.tryParse(parts[0]) ?? 0;
  final m = parts.length > 1 ? (double.tryParse(parts[1]) ?? 0) : 0;
  final sec = parts.length > 2 ? (double.tryParse(parts[2]) ?? 0) : 0;
  return sign * (d + m / 60 + sec / 3600);
}

class _Proj {
  final double lat, lst;
  final Size size;
  final double zoom;
  final double rotation;
  final double panY;
  late final double _latRad, _cx, _cy, _radius;

  _Proj({
    required this.lat,
    required this.lst,
    required this.size,
    required this.zoom,
    required this.rotation,
    required this.panY,
  }) {
    _latRad = lat * math.pi / 180;
    _cx = size.width / 2;
    _cy = size.height / 2 + panY;
    _radius = (math.sqrt(size.width * size.width + size.height * size.height) / 2) * zoom;
  }

  Offset? project(double raH, double decDeg, {bool clipHorizon = true}) {
    final ha = (lst - raH) * 15 * math.pi / 180;
    final dec = decDeg * math.pi / 180;
    final sinAlt = math.sin(dec) * math.sin(_latRad) +
        math.cos(dec) * math.cos(_latRad) * math.cos(ha);
    final alt = math.asin(sinAlt.clamp(-1.0, 1.0));
    if (clipHorizon && alt < 0) return null;
    final cosAlt = math.cos(alt);
    final denom = cosAlt * math.cos(_latRad);
    final cosAz = denom.abs() < 1e-10
        ? 1.0
        : ((math.sin(dec) - sinAlt * math.sin(_latRad)) / denom);
    var az = math.acos(cosAz.clamp(-1.0, 1.0));
    if (math.sin(ha) > 0) az = 2 * math.pi - az;
    final azRot = az + rotation;
    final r = _radius * cosAlt / (1 + math.sin(alt));
    return Offset(_cx - r * math.sin(azRot), _cy - r * math.cos(azRot));
  }

  Offset? projectAltAz(double alt, double az, {bool clipHorizon = true}) {
    if (clipHorizon && alt < 0) return null;
    final azRot = az + rotation;
    final r = _radius * math.cos(alt) / (1 + math.sin(alt));
    return Offset(_cx - r * math.sin(azRot), _cy - r * math.cos(azRot));
  }

  double get cx => _cx;
  double get cy => _cy;
  double get radius => _radius;
}

// ---------------------------------------------------------------------------
// Sky renderer (CustomPainter)
// ---------------------------------------------------------------------------

class _SkyRenderer extends CustomPainter {
  final List<_PlanetariumStar> stars;
  final List<Constellation> constellations;
  final List<_ConstellationLabel> constNames;
  final MilkyWayBand? milkyWay;
  final List<CatalogEntry>? dsos;
  final MountState state;
  final double zoom;
  final double rotation;
  final double panY;
  final PlanetariumSettings settings;
  final _SkyObject? selectedObject;
  final bool nightMode;

  _SkyRenderer({
    required this.stars,
    required this.constellations,
    required this.constNames,
    this.milkyWay,
    this.dsos,
    required this.state,
    required this.zoom,
    required this.rotation,
    required this.panY,
    required this.settings,
    this.selectedObject,
    this.nightMode = false,
  });

  // Display always JNow (no epoch toggle).
  static const bool _useJNow = true;

  // Sky colors: normal (cool blue) vs night vision (warm red)
  Color get _skyBg          => nightMode ? const Color(0xFF120606) : const Color(0xFF0C1520);
  Color get _groundColor    => nightMode ? const Color(0xFF100808) : const Color(0xFF0A140A);
  Color get _horizonColor   => nightMode ? const Color(0xFF3A1818) : const Color(0xFF2A4020);
  Color get _gridColor      => nightMode ? const Color(0xFF2A1010) : const Color(0xFF1A3020);
  Color get _constLineColor => nightMode ? const Color(0xFF662222) : const Color(0xFF4466AA);
  Color get _constNameColor => nightMode ? const Color(0xFF884444) : const Color(0xFF5577AA);
  Color get _starLabelColor => nightMode ? const Color(0xFF884444) : const Color(0xFFAABBDD);
  Color get _cardinalColor  => nightMode ? const Color(0xFF884444) : const Color(0xFF5577AA);
  Color get _milkyWayColor  => nightMode ? const Color(0xFF1A0C0C) : const Color(0xFF1A2840);
  Color get _mwOutlineColor => nightMode ? const Color(0xFF2A1616) : const Color(0xFF1A3050);
  Color get _eqGridColor    => nightMode ? const Color(0xFF201010) : const Color(0xFF1A3020);

  /// Shift any color to a red-only tone for night vision.
  static Color _toNightRed(Color c) {
    final lum = (c.red * 0.299 + c.green * 0.587 + c.blue * 0.114).round().clamp(0, 255);
    return Color.fromARGB(c.alpha.toInt(), lum, (lum * 0.3).round(), (lum * 0.2).round());
  }

  /// Convert J2000 catalog coords to display epoch.
  (double, double) _catCoords(double raH, double decDeg, double jd) {
    if (_useJNow) return equatorialEquinoxToJNow(raH, decDeg, 2000, jd);
    return (raH, decDeg);
  }

  /// Convert JNow mount/planet coords to display epoch.
  (double, double) _mountCoords(double raH, double decDeg, double jd) {
    if (!_useJNow) return equatorialJNowToEquinox(raH, decDeg, 2000, jd);
    return (raH, decDeg);
  }

  @override
  void paint(Canvas canvas, Size size) {
    final proj = _Proj(
      lat: state.latitude,
      lst: _parseLST(state.siderealTime),
      size: size,
      zoom: zoom,
      rotation: rotation,
      panY: panY,
    );
    final jd = julianDate(DateTime.now().toUtc());

    // 1. Sky background
    canvas.drawRect(
      Rect.fromLTWH(0, 0, size.width, size.height),
      Paint()..color = _skyBg,
    );

    // Ground below horizon
    final groundAll = Path()..addRect(Rect.fromLTWH(0, 0, size.width, size.height));
    final skyCircle = Path()..addOval(
      Rect.fromCircle(center: Offset(proj.cx, proj.cy), radius: proj.radius));
    final groundPath = Path.combine(PathOperation.difference, groundAll, skyCircle);
    canvas.drawPath(groundPath, Paint()..color = _groundColor);

    // Horizon line
    canvas.drawCircle(
      Offset(proj.cx, proj.cy),
      proj.radius,
      Paint()
        ..color = _horizonColor
        ..style = PaintingStyle.stroke
        ..strokeWidth = 1.5,
    );

    // 2. Milky Way band
    if (settings.milkyWay && milkyWay != null) {
      _drawMilkyWay(canvas, proj, jd);
    }

    // 3. Alt-Az grid
    if (settings.altAzGrid) {
      _drawAltAzGrid(canvas, proj);
    }

    // 4. Equatorial grid
    if (settings.eqGrid) {
      _drawEqGrid(canvas, proj);
    }

    // 5. Constellation lines
    if (settings.constellationLines) {
      _drawConstellationLines(canvas, proj, jd);
    }

    // 6. Constellation names
    if (settings.constellationNames) {
      _drawConstellationNames(canvas, proj, jd);
    }

    // 7. Stars (drawn first so DSO symbols appear on top, Stellarium-style)
    _drawStars(canvas, proj, jd);

    // 8. DSO objects (symbols in front of star field)
    if (settings.dsoObjects && dsos != null) {
      _drawDSOs(canvas, proj, jd);
    }

    // 9. Planets
    _drawPlanets(canvas, proj);

    // 10. Telescope crosshair
    _drawCrosshair(canvas, proj);

    // 11. Target marker
    _drawTarget(canvas, proj);

    // Cardinal labels
    _drawCardinal(canvas, proj, 'N', 0, rotation, 12);
    _drawCardinal(canvas, proj, 'NE', math.pi / 4, rotation, 9);
    _drawCardinal(canvas, proj, 'E', math.pi / 2, rotation, 12);
    _drawCardinal(canvas, proj, 'SE', 3 * math.pi / 4, rotation, 9);
    _drawCardinal(canvas, proj, 'S', math.pi, rotation, 12);
    _drawCardinal(canvas, proj, 'SW', 5 * math.pi / 4, rotation, 9);
    _drawCardinal(canvas, proj, 'W', 3 * math.pi / 2, rotation, 12);
    _drawCardinal(canvas, proj, 'NW', 7 * math.pi / 4, rotation, 9);

    // Selection ring
    if (selectedObject != null) {
      final double dispRa, dispDec;
      if (selectedObject!.isJNow) {
        (dispRa, dispDec) = _mountCoords(selectedObject!.ra, selectedObject!.dec, jd);
      } else {
        (dispRa, dispDec) = _catCoords(selectedObject!.ra, selectedObject!.dec, jd);
      }
      final pt = proj.project(dispRa, dispDec, clipHorizon: false);
      if (pt != null) {
        canvas.drawCircle(
          pt,
          12,
          Paint()
            ..color = TA.warning
            ..style = PaintingStyle.stroke
            ..strokeWidth = 2.0,
        );
      }
    }
  }

  void _drawMilkyWay(Canvas canvas, _Proj proj, double jd) {
    final mw = milkyWay!;
    final path = Path();
    bool first = true;

    for (final p in mw.center) {
      final (raNow, decNow) = _catCoords(p.$1, p.$2, jd);
      final pt = proj.project(raNow, decNow, clipHorizon: false);
      if (pt == null) continue;
      if (first) {
        path.moveTo(pt.dx, pt.dy);
        first = false;
      } else {
        path.lineTo(pt.dx, pt.dy);
      }
    }
    for (int i = mw.width.length - 1; i >= 0; i--) {
      final w = mw.width[i];
      final (raNow, decNow) = _catCoords(w.$1, w.$2, jd);
      final pt = proj.project(raNow, decNow, clipHorizon: false);
      if (pt == null) continue;
      path.lineTo(pt.dx, pt.dy);
    }
    path.close();

    canvas.save();
    canvas.clipPath(Path()
      ..addOval(Rect.fromCircle(
          center: Offset(proj.cx, proj.cy), radius: proj.radius)));
    canvas.drawPath(
      path,
      Paint()..color = nightMode ? const Color(0x0D884444) : const Color(0x0D6688AA),
    );
    canvas.restore();
  }

  void _drawAltAzGrid(Canvas canvas, _Proj proj) {
    final paint = Paint()
      ..color = _milkyWayColor
      ..style = PaintingStyle.stroke
      ..strokeWidth = 0.5;

    for (int alt = 10; alt <= 80; alt += 10) {
      final altRad = alt * math.pi / 180;
      final r = proj.radius * math.cos(altRad) / (1 + math.sin(altRad));
      canvas.drawCircle(Offset(proj.cx, proj.cy), r, paint);
    }

    for (int az = 0; az < 360; az += 30) {
      final a = az * math.pi / 180 + rotation;
      final p1 = Offset(proj.cx, proj.cy);
      final p2 = Offset(
        proj.cx - proj.radius * math.sin(a),
        proj.cy - proj.radius * math.cos(a),
      );
      canvas.drawLine(p1, p2, paint);
    }
  }

  void _drawEqGrid(Canvas canvas, _Proj proj) {
    final paint = Paint()
      ..color = _eqGridColor
      ..style = PaintingStyle.stroke
      ..strokeWidth = 0.5;

    // RA hour lines
    for (int h = 0; h < 24; h += 2) {
      final path = Path();
      bool first = true;
      for (int d = -80; d <= 80; d += 2) {
        final pt = proj.project(h.toDouble(), d.toDouble());
        if (pt == null) continue;
        if (first) {
          path.moveTo(pt.dx, pt.dy);
          first = false;
        } else {
          path.lineTo(pt.dx, pt.dy);
        }
      }
      canvas.drawPath(path, paint);
    }

    // Dec circles
    for (int d = -75; d <= 75; d += 15) {
      if (d == 0) continue;
      final path = Path();
      bool first = true;
      for (double ra = 0; ra < 24; ra += 0.2) {
        final pt = proj.project(ra, d.toDouble());
        if (pt == null) continue;
        if (first) {
          path.moveTo(pt.dx, pt.dy);
          first = false;
        } else {
          path.lineTo(pt.dx, pt.dy);
        }
      }
      canvas.drawPath(path, paint);
    }
  }

  void _drawConstellationLines(Canvas canvas, _Proj proj, double jd) {
    final paint = Paint()
      ..color = _constLineColor.withValues(alpha: 0.55)
      ..strokeWidth = 1.2
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round
      ..isAntiAlias = true;

    for (final c in constellations) {
      for (final line in c.lines) {
        final (ra1, dec1) = _catCoords(line.ra1, line.dec1, jd);
        final (ra2, dec2) = _catCoords(line.ra2, line.dec2, jd);
        final p1 = proj.project(ra1, dec1);
        final p2 = proj.project(ra2, dec2);
        if (p1 != null && p2 != null) {
          canvas.drawLine(p1, p2, paint);
        }
      }
    }
  }

  void _drawConstellationNames(Canvas canvas, _Proj proj, double jd) {
    for (final cn in constNames) {
      final (raNow, decNow) = _catCoords(cn.ra, cn.dec, jd);
      final pt = proj.project(raNow, decNow);
      if (pt == null) continue;
      final fontSize = zoom > 4 ? 13.0 : (zoom > 2 ? 11.0 : 10.0);
      final tp = TextPainter(
        text: TextSpan(
          text: cn.abbr,
          style: TextStyle(
            color: _constNameColor.withValues(alpha: 0.65),
            fontSize: fontSize,
            fontWeight: FontWeight.w500,
            letterSpacing: 1.0,
          ),
        ),
        textDirection: TextDirection.ltr,
      )..layout();
      tp.paint(canvas, Offset(pt.dx - tp.width / 2, pt.dy - tp.height / 2));
    }
  }

  void _drawDSOs(Canvas canvas, _Proj proj, double jd) {
    final magLimit = (8.0 + 2.5 * (math.log(zoom.clamp(0.5, 30)) / math.ln10)).clamp(8.0, 25.0);

    for (final dso in dsos!) {
      if (dso.mag != null && dso.mag! > magLimit) continue;
      final (raNow, decNow) = _catCoords(dso.ra, dso.dec, jd);
      final pt = proj.project(raNow, decNow);
      if (pt == null) continue;

      final mag = dso.mag ?? 10.0;
      // Higher opacity so DSOs stand out in front of sky (Stellarium-like)
      final alpha = ((magLimit - mag) / magLimit * 0.5 + 0.5).clamp(0.5, 0.95);
      final type = dso.objType ?? 0;

      final isGalaxy = (type == 0 || type == 5 || type == 6 || type == 7);
      final isPlanNeb = (type == 9);
      final isOpenCluster = (type == 1);
      final isGlobularCluster = (type == 8);
      final isClusterNebula = (type == 12);
      final isNebula = (type == 10 || type == 11 || type == 14 || type == 16);

      final scale = math.sqrt(zoom) * settings.objectScale;

      final Color galaxyC   = nightMode ? const Color(0xFF993322) : const Color(0xFFCC8866);
      final Color nebulaC   = nightMode ? const Color(0xFF884422) : const Color(0xFF44CCAA);
      final Color clusterC  = nightMode ? const Color(0xFF886633) : const Color(0xFFCCCC66);
      final Color openClrC  = nightMode ? const Color(0xFF776644) : const Color(0xFFAAAAAA);
      final Color diffNebuC = nightMode ? const Color(0xFF884433) : const Color(0xFF6699CC);
      final Color defaultC  = nightMode ? const Color(0xFF993322) : const Color(0xFFCC6644);

      if (isGalaxy) {
        final r = ((6.0 - mag * 0.35).clamp(2.5, 8.0)) * scale;
        final paint = Paint()
          ..color = galaxyC.withValues(alpha: alpha)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.2;
        canvas.drawOval(
          Rect.fromCenter(center: pt, width: r * 2.2, height: r * 1.1),
          paint,
        );
      } else if (isPlanNeb) {
        final r = ((6.0 - mag * 0.35).clamp(2.5, 8.0)) * scale;
        final paint = Paint()
          ..color = nebulaC.withValues(alpha: alpha)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.0;
        canvas.drawCircle(pt, r, paint);
        final spoke = r * 0.5;
        for (int i = 0; i < 4; i++) {
          final a = i * math.pi / 2;
          canvas.drawLine(
            Offset(pt.dx + r * math.cos(a), pt.dy + r * math.sin(a)),
            Offset(pt.dx + (r + spoke) * math.cos(a), pt.dy + (r + spoke) * math.sin(a)),
            paint,
          );
        }
        canvas.drawCircle(pt, 1.5 * scale,
          Paint()..color = nebulaC.withValues(alpha: alpha * 0.9));
      } else if (isOpenCluster || isClusterNebula) {
        final r = ((5.5 - mag * 0.3).clamp(3.0, 7.0)) * scale;
        final gray = isOpenCluster ? openClrC : clusterC;
        final dashPaint = Paint()
          ..color = gray.withValues(alpha: alpha)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 0.8;
        const nDashes = 8;
        const dashFrac = 0.6;
        final segAngle = 2 * math.pi / nDashes;
        final dashAngle = segAngle * dashFrac;
        final rect = Rect.fromCircle(center: pt, radius: r);
        for (int i = 0; i < nDashes; i++) {
          final start = i * segAngle - math.pi / 2;
          canvas.drawArc(rect, start, dashAngle, false, dashPaint);
        }
      } else if (isGlobularCluster) {
        final r = ((5.5 - mag * 0.3).clamp(3.0, 7.0)) * scale;
        final paint = Paint()
          ..color = clusterC.withValues(alpha: alpha)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.0;
        canvas.drawCircle(pt, r, paint);
        canvas.drawLine(Offset(pt.dx - r, pt.dy), Offset(pt.dx + r, pt.dy), paint);
        canvas.drawLine(Offset(pt.dx, pt.dy - r), Offset(pt.dx, pt.dy + r), paint);
      } else if (isNebula) {
        final r = ((6.0 - mag * 0.35).clamp(2.5, 7.0)) * scale;
        final paint = Paint()
          ..color = diffNebuC.withValues(alpha: alpha)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.0;
        canvas.drawRRect(
          RRect.fromRectAndRadius(
            Rect.fromCenter(center: pt, width: r * 2, height: r * 2),
            const Radius.circular(2),
          ),
          paint,
        );
      } else {
        final r = ((4.0 - mag * 0.25).clamp(2.0, 5.0)) * scale;
        canvas.drawCircle(
          pt, r,
          Paint()
            ..color = defaultC.withValues(alpha: alpha)
            ..style = PaintingStyle.stroke
            ..strokeWidth = 1.0,
        );
      }
    }
  }

  void _drawStars(Canvas canvas, _Proj proj, double jd) {
    final magLimit = 6.0 + 2.5 * (math.log(zoom.clamp(0.5, 30)) / math.ln10);

    for (final star in stars) {
      if (star.mag > magLimit) continue;
      final (raNow, decNow) = _catCoords(star.ra, star.dec, jd);
      final pt = proj.project(raNow, decNow);
      if (pt == null) continue;

      final mag = star.mag;
      // Softer magnitude curve so faint stars stay visible at reasonable scale
      final baseR = 2.5 * math.pow(10, -0.14 * mag) * 1.25;
      final zoomScale = math.pow(zoom.clamp(0.5, 30), 0.45);
      final r = (baseR * zoomScale * settings.starScale).clamp(0.3, 40.0);
      if (r < 0.3) continue;
      final brightness = math.pow(10, -0.4 * (mag - magLimit)).clamp(0.2, 1.0).toDouble();

      final baseColor = _bvToColor(star.bv);
      final cr = (baseColor.r * 255).round();
      final cg = (baseColor.g * 255).round();
      final cb = (baseColor.b * 255).round();

      if (r >= 1.8) {
        // Radial gradient: bright center, slightly darker/transparent edge
        final centerColor = Color.fromRGBO(
          (cr * brightness).round().clamp(0, 255),
          (cg * brightness).round().clamp(0, 255),
          (cb * brightness).round().clamp(0, 255),
          brightness,
        );
        final edgeColor = Color.fromRGBO(
          (cr * brightness * 0.5).round().clamp(0, 255),
          (cg * brightness * 0.5).round().clamp(0, 255),
          (cb * brightness * 0.5).round().clamp(0, 255),
          brightness * 0.3,
        );
        final gradient = RadialGradient(colors: [centerColor, edgeColor]);
        final starPaint = Paint()
          ..shader = gradient.createShader(
            Rect.fromCircle(center: pt, radius: r),
          );
        canvas.drawCircle(pt, r, starPaint);
      } else {
        // Tiny stars: plain dot (gradient not visible at this size)
        final starPaint = Paint()
          ..color = Color.fromRGBO(
            (cr * brightness).round().clamp(0, 255),
            (cg * brightness).round().clamp(0, 255),
            (cb * brightness).round().clamp(0, 255),
            brightness,
          );
        canvas.drawCircle(pt, r, starPaint);
      }

      // Labels for bright named stars
      if (settings.starLabels &&
          star.name.isNotEmpty &&
          mag <= 3.5 &&
          zoom >= 0.7) {
        final labelFontSize = zoom > 4 ? 12.0 : (zoom > 2 ? 10.0 : 9.0);
        final tp = TextPainter(
          text: TextSpan(
            text: star.name,
            style: TextStyle(
              color: _starLabelColor.withValues(alpha: 0.85),
              fontSize: labelFontSize,
            ),
          ),
          textDirection: TextDirection.ltr,
        )..layout();
        tp.paint(canvas, pt + Offset(r + 3, -tp.height / 2));
      }
    }
  }

  /// SKY2000 spectral color palette: maps B-V color index to star color.
  Color _bvToColor(double? bv) {
    if (nightMode) return const Color(0xFFCC6644);
    if (bv == null) return const Color(0xFFE0E8FF);
    if (bv < -0.30) return const Color(0xFF9BB0FF); // O  blue
    if (bv < -0.02) return const Color(0xFFAABFFF); // B  blue-white
    if (bv <  0.30) return const Color(0xFFCAD7FF); // A  white
    if (bv <  0.58) return const Color(0xFFF8F7FF); // F  yellow-white
    if (bv <  0.81) return const Color(0xFFFFF4EA); // G  yellow (Sun-like)
    if (bv <  1.40) return const Color(0xFFFFD2A1); // K  orange
    return const Color(0xFFFFCC6F);                  // M  red-orange
  }

  void _drawPlanets(Canvas canvas, _Proj proj) {
    final jd = julianDate(DateTime.now().toUtc());
    final bodies = allBodies(jd);

    for (final body in bodies) {
      final (pRa, pDec) = _mountCoords(body.ra, body.dec, jd);
      final pt = proj.project(pRa, pDec);
      if (pt == null) continue;

      final pColor = nightMode ? _toNightRed(body.color) : body.color;
      final r = body.radius * math.sqrt(zoom) * settings.objectScale;
      canvas.drawCircle(pt, r, Paint()..color = pColor);
      canvas.drawCircle(
        pt,
        r + 1,
        Paint()
          ..color = pColor.withValues(alpha: 0.4)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.5,
      );

      if (settings.planetLabels) {
        final tp = TextPainter(
          text: TextSpan(
            text: body.name,
            style: TextStyle(
              color: pColor,
              fontSize: zoom > 2 ? 11 : 9,
              fontWeight: FontWeight.w600,
            ),
          ),
          textDirection: TextDirection.ltr,
        )..layout();
        tp.paint(canvas, Offset(pt.dx + r + 4, pt.dy - tp.height / 2));
      }
    }
  }

  void _drawCrosshair(Canvas canvas, _Proj proj) {
    final jd = julianDate(DateTime.now().toUtc());
    final (raH, scopeDec) = _mountCoords(_parseLST(state.ra), _parseCoord(state.dec), jd);

    final scopePt = proj.project(raH, scopeDec, clipHorizon: false);
    if (scopePt == null) return;

    final paint = Paint()
      ..color = TA.accent
      ..strokeWidth = 1.5
      ..style = PaintingStyle.stroke;

    const arm = 12.0;
    canvas.drawLine(
      scopePt - const Offset(arm, 0),
      scopePt + const Offset(arm, 0),
      paint,
    );
    canvas.drawLine(
      scopePt - const Offset(0, arm),
      scopePt + const Offset(0, arm),
      paint,
    );
    canvas.drawCircle(scopePt, 7, paint);
    canvas.drawCircle(scopePt, 2, Paint()..color = TA.accent);
  }

  void _drawTarget(Canvas canvas, _Proj proj) {
    if (state.targetRa.isEmpty || state.targetDec.isEmpty) return;
    if (state.targetRa == '00:00:00' && state.targetDec.contains('00:00:00')) {
      return;
    }

    final jd = julianDate(DateTime.now().toUtc());
    final (raH, tDec) = _mountCoords(
        _parseLST(state.targetRa), _parseCoord(state.targetDec), jd);

    final pt = proj.project(raH, tDec, clipHorizon: false);
    if (pt == null) return;

    final paint = Paint()
      ..color = TA.success
      ..strokeWidth = 1.0
      ..style = PaintingStyle.stroke;

    canvas.drawCircle(pt, 8, paint);
    final path = Path()
      ..moveTo(pt.dx, pt.dy - 6)
      ..lineTo(pt.dx + 6, pt.dy)
      ..lineTo(pt.dx, pt.dy + 6)
      ..lineTo(pt.dx - 6, pt.dy)
      ..close();
    canvas.drawPath(path, paint);
  }

  void _drawCardinal(Canvas canvas, _Proj proj, String label, double azRad, double rotation, double fontSize) {
    final a = azRad + rotation;
    final x = proj.cx - (proj.radius - 16) * math.sin(a);
    final y = proj.cy - (proj.radius - 16) * math.cos(a);
    final tp = TextPainter(
      text: TextSpan(
        text: label,
        style: TextStyle(
          color: _cardinalColor,
          fontSize: fontSize,
          fontWeight: FontWeight.w700,
        ),
      ),
      textDirection: TextDirection.ltr,
    )..layout();
    tp.paint(canvas, Offset(x - tp.width / 2, y - tp.height / 2));
  }

  @override
  bool shouldRepaint(_SkyRenderer old) => true;
}

// ---------------------------------------------------------------------------
// Layer toggle panel
// ---------------------------------------------------------------------------

class _LayerPanel extends StatelessWidget {
  final PlanetariumSettings settings;
  final PlanetariumSettingsNotifier notifier;
  final VoidCallback onChanged;

  const _LayerPanel({
    required this.settings,
    required this.notifier,
    required this.onChanged,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      width: 220,
      padding: const EdgeInsets.all(8),
      decoration: BoxDecoration(
        color: TA.surface.withValues(alpha: 0.95),
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: TA.border),
      ),
      child: SingleChildScrollView(
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            // --- Layers ---
            const _SectionHeader('Layers'),
            _toggle('Constellation lines', settings.constellationLines,
                (v) => notifier.setLayer('constellationLines', v)),
            _toggle('Constellation names', settings.constellationNames,
                (v) => notifier.setLayer('constellationNames', v)),
            _toggle('Alt-Az grid', settings.altAzGrid,
                (v) => notifier.setLayer('altAzGrid', v)),
            _toggle('Equatorial grid', settings.eqGrid,
                (v) => notifier.setLayer('eqGrid', v)),
            _toggle('Milky Way', settings.milkyWay,
                (v) => notifier.setLayer('milkyWay', v)),
            _toggle('DSO objects', settings.dsoObjects,
                (v) => notifier.setLayer('dsoObjects', v)),
            _toggle('Planet labels', settings.planetLabels,
                (v) => notifier.setLayer('planetLabels', v)),
            _toggle('Star labels', settings.starLabels,
                (v) => notifier.setLayer('starLabels', v)),
            Divider(color: TA.border, height: 12),

            // --- Display ---
            const _SectionHeader('Display'),
            _slider('Star size', settings.starScale, (v) {
              notifier.setStarScale(v);
              onChanged();
            }, min: 1.0, max: 10.0),
            _slider('Object size', settings.objectScale, (v) {
              notifier.setObjectScale(v);
              onChanged();
            }),
            Divider(color: TA.border, height: 12),

            // --- Coordinates ---
            const _SectionHeader('Coordinates'),
            _coordSelector(),
            Divider(color: TA.border, height: 12),

            // --- Reset ---
            Center(
              child: TextButton.icon(
                onPressed: () {
                  notifier.resetToDefaults();
                  onChanged();
                },
                icon: Icon(Icons.restore, size: 14, color: TA.textSecondary),
                label: Text('Reset to defaults',
                    style: TextStyle(color: TA.textSecondary, fontSize: 11)),
                style: TextButton.styleFrom(
                  minimumSize: const Size(0, 28),
                  padding: const EdgeInsets.symmetric(horizontal: 8),
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _toggle(String label, bool value, void Function(bool) setter) {
    return SizedBox(
      height: 32,
      child: Row(
        children: [
          SizedBox(
            width: 28,
            child: Checkbox(
              value: value,
              onChanged: (v) {
                setter(v ?? false);
                onChanged();
              },
              materialTapTargetSize: MaterialTapTargetSize.shrinkWrap,
              visualDensity: VisualDensity.compact,
            ),
          ),
          Expanded(
            child: Text(label,
                style: TextStyle(color: TA.text, fontSize: 12)),
          ),
        ],
      ),
    );
  }

  Widget _slider(String label, double value, ValueChanged<double> onChanged,
      {double min = 0.3, double max = 3.0, int? divisions}) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 2),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Expanded(
                child: Text(label,
                    style: TextStyle(color: TA.text, fontSize: 12)),
              ),
              Text(value.toStringAsFixed(1),
                  style: TextStyle(
                      color: TA.textSecondary, fontSize: 11, fontFamily: 'monospace')),
            ],
          ),
          SizedBox(
            height: 24,
            child: Slider(
              value: value.clamp(min, max),
              min: min,
              max: max,
              divisions: divisions ?? ((max - min) * 10).round(),
              onChanged: onChanged,
            ),
          ),
        ],
      ),
    );
  }

  Widget _coordSelector() {
    return Row(
      children: [
        _coordChip('RA / Dec', CoordDisplay.equatorial),
        const SizedBox(width: 4),
        _coordChip('Alt / Az', CoordDisplay.horizontal),
      ],
    );
  }

  Widget _coordChip(String label, CoordDisplay mode) {
    final selected = settings.coordDisplay == mode;
    return Expanded(
      child: GestureDetector(
        onTap: () {
          notifier.setCoordDisplay(mode);
          onChanged();
        },
        child: Container(
          padding: const EdgeInsets.symmetric(vertical: 6),
          decoration: BoxDecoration(
            color: selected ? TA.accent.withValues(alpha: 0.25) : Colors.transparent,
            borderRadius: BorderRadius.circular(4),
            border: Border.all(
              color: selected ? TA.accent : TA.border,
              width: selected ? 1.5 : 1.0,
            ),
          ),
          alignment: Alignment.center,
          child: Text(
            label,
            style: TextStyle(
              color: selected ? TA.accent : TA.textSecondary,
              fontSize: 11,
              fontWeight: selected ? FontWeight.w600 : FontWeight.normal,
            ),
          ),
        ),
      ),
    );
  }

}

class _SectionHeader extends StatelessWidget {
  final String text;
  const _SectionHeader(this.text);
  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.only(bottom: 2, top: 2),
      child: Text(text,
          style: TextStyle(
            color: TA.textSecondary,
            fontSize: 10,
            fontWeight: FontWeight.w600,
            letterSpacing: 0.8,
          )),
    );
  }
}

// ---------------------------------------------------------------------------
// Object info panel
// ---------------------------------------------------------------------------

class _ObjectInfoPanel extends ConsumerWidget {
  final _SkyObject object;
  final VoidCallback onClose;

  const _ObjectInfoPanel({required this.object, required this.onClose});

  String _formatRa(double ra) {
    var h = ra.floor();
    var m = ((ra - h) * 60).floor();
    var s = (((ra - h) * 60 - m) * 60).round();
    if (s >= 60) { s -= 60; m += 1; }
    if (m >= 60) { m -= 60; h += 1; }
    if (h >= 24) h -= 24;
    return '${h.toString().padLeft(2, '0')}:${m.toString().padLeft(2, '0')}:${s.toString().padLeft(2, '0')}';
  }

  String _formatDec(double dec) {
    final sign = dec >= 0 ? '+' : '-';
    final abs = dec.abs();
    var d = abs.floor();
    var m = ((abs - d) * 60).floor();
    var s = (((abs - d) * 60 - m) * 60).round();
    if (s >= 60) { s -= 60; m += 1; }
    if (m >= 60) { m -= 60; d += 1; }
    return '$sign${d.toString().padLeft(2, '0')}:${m.toString().padLeft(2, '0')}:${s.toString().padLeft(2, '0')}';
  }

  Widget _coordLines(String altStr) {
    final jd = julianDate(DateTime.now().toUtc());
    final double raJ2000, decJ2000, raJNow, decJNow;
    if (object.isJNow) {
      raJNow = object.ra;
      decJNow = object.dec;
      (raJ2000, decJ2000) = equatorialJNowToEquinox(object.ra, object.dec, 2000, jd);
    } else {
      raJ2000 = object.ra;
      decJ2000 = object.dec;
      (raJNow, decJNow) = equatorialEquinoxToJNow(object.ra, object.dec, 2000, jd);
    }
    final mono = TextStyle(
      color: TA.text,
      fontSize: 10,
      fontFamily: 'monospace',
    );
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('J2000  RA ${_formatRa(raJ2000)}   Dec ${_formatDec(decJ2000)}', style: mono),
        const SizedBox(height: 1),
        Text('JNow   RA ${_formatRa(raJNow)}   Dec ${_formatDec(decJNow)}   Alt $altStr', style: mono),
      ],
    );
  }

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final mountState = ref.watch(mountStateProvider);
    final client = ref.read(lx200ClientProvider);
    final slewing = mountState.isSlewing;

    // Compute altitude
    final lst = parseSiderealTime(mountState.siderealTime);
    String altStr = '?';
    if (lst != null) {
      final alt = objectAltitude(mountState.latitude, lst, object.ra, object.dec);
      altStr = '${alt.toStringAsFixed(1)}°';
    }

    return Container(
      padding: const EdgeInsets.fromLTRB(12, 8, 12, 10),
      decoration: BoxDecoration(
        color: TA.surface.withValues(alpha: 0.95),
        borderRadius: const BorderRadius.vertical(top: Radius.circular(12)),
        border: Border.all(color: TA.border),
      ),
      child: Column(
        mainAxisSize: MainAxisSize.min,
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              if (object.color != null)
                Container(
                  width: 8,
                  height: 8,
                  margin: const EdgeInsets.only(right: 6),
                  decoration: BoxDecoration(
                    color: object.color,
                    shape: BoxShape.circle,
                  ),
                ),
              Expanded(
                child: Text(
                  object.name,
                  style: TextStyle(
                    color: TA.textHigh,
                    fontSize: 14,
                    fontWeight: FontWeight.w600,
                  ),
                  maxLines: 2,
                  overflow: TextOverflow.ellipsis,
                ),
              ),
              IconButton(
                icon: Icon(Icons.close, size: 18, color: TA.textSecondary),
                onPressed: onClose,
                constraints: const BoxConstraints(minWidth: 24, minHeight: 24),
                padding: EdgeInsets.zero,
                style: IconButton.styleFrom(
                  tapTargetSize: MaterialTapTargetSize.shrinkWrap,
                ),
              ),
            ],
          ),
          const SizedBox(height: 2),
          Text(
            _objectSubtitle(object),
            style: TextStyle(color: TA.textSecondary, fontSize: 10),
            maxLines: 1,
            overflow: TextOverflow.ellipsis,
          ),
          const SizedBox(height: 2),
          _coordLines(altStr),
          const SizedBox(height: 8),
          Row(
            children: [
              Expanded(
                child: slewing
                    ? ElevatedButton.icon(
                        style: ElevatedButton.styleFrom(
                          backgroundColor: TA.error,
                          minimumSize: const Size(0, 34),
                        ),
                        onPressed: () {
                          client.sendImmediate(LX200.stopAll);
                        },
                        icon: const Icon(Icons.stop_circle, size: 14),
                        label: const Text('Stop'),
                      )
                    : ElevatedButton.icon(
                        style: ElevatedButton.styleFrom(
                          minimumSize: const Size(0, 34),
                        ),
                        icon: const Icon(Icons.my_location, size: 14),
                        onPressed: () async {
                          String raStr;
                          String decStr;
                          if (object.isJNow) {
                            raStr = _formatRa(object.ra);
                            decStr = _formatDec(object.dec);
                          } else {
                            final jd = julianDate(DateTime.now().toUtc());
                            (raStr, decStr) = j2000ToJNowLx200(object.ra, object.dec, jd);
                          }
                          await client.sendBool(LX200.setTargetRa(raStr));
                          await client.sendBool(LX200.setTargetDec(decStr));
                          final reply =
                              await client.sendCommand(LX200.gotoTarget);
                          if (context.mounted && reply != '0') {
                            final errIdx = int.tryParse(reply ?? '') ?? -1;
                            final msg =
                                errIdx >= 0 && errIdx < GotoError.values.length
                                    ? gotoErrorCause(GotoError.values[errIdx])
                                    : 'Goto failed: $reply';
                            ScaffoldMessenger.of(context).showSnackBar(
                              SnackBar(
                                content: Text(msg),
                                backgroundColor: TA.error,
                              ),
                            );
                          }
                        },
                        label: const Text('Goto'),
                      ),
              ),
              const SizedBox(width: 6),
              Expanded(
                child: ElevatedButton.icon(
                  style: ElevatedButton.styleFrom(
                    backgroundColor: TA.surfaceVariant,
                    minimumSize: const Size(0, 34),
                  ),
                  icon: const Icon(Icons.sync, size: 14),
                  onPressed: slewing
                      ? null
                      : () async {
                          String raStr;
                          String decStr;
                          if (object.isJNow) {
                            raStr = _formatRa(object.ra);
                            decStr = _formatDec(object.dec);
                          } else {
                            final jd = julianDate(DateTime.now().toUtc());
                            (raStr, decStr) = j2000ToJNowLx200(object.ra, object.dec, jd);
                          }
                          await client.sendBool(LX200.setTargetRa(raStr));
                          await client.sendBool(LX200.setTargetDec(decStr));
                          await client.sendCommand(LX200.syncTarget);
                        },
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

// ---------------------------------------------------------------------------
// Search panel
// ---------------------------------------------------------------------------

class _SearchPanel extends StatelessWidget {
  final TextEditingController controller;
  final List<_SkyObject> results;
  final void Function(String) onSearch;
  final void Function(_SkyObject) onSelect;
  final VoidCallback onClose;

  const _SearchPanel({
    required this.controller,
    required this.results,
    required this.onSearch,
    required this.onSelect,
    required this.onClose,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      color: TA.background.withValues(alpha: 0.92),
      child: SafeArea(
        child: Column(
          children: [
            Padding(
              padding: const EdgeInsets.all(8),
              child: Row(
                children: [
                  IconButton(
                    icon: Icon(Icons.close, color: TA.text),
                    onPressed: onClose,
                  ),
                  Expanded(
                    child: TextField(
                      controller: controller,
                      autofocus: true,
                      decoration: const InputDecoration(
                        hintText: 'Search stars, DSOs, planets...',
                        prefixIcon: Icon(Icons.search),
                        isDense: true,
                      ),
                      onChanged: onSearch,
                    ),
                  ),
                ],
              ),
            ),
            Expanded(
              child: ListView.builder(
                itemCount: results.length,
                itemBuilder: (_, i) {
                  final obj = results[i];
                  return ListTile(
                    dense: true,
                    title: Text(obj.name,
                        style: TextStyle(
                            color: TA.textHigh, fontSize: 14)),
                    subtitle: Text(
                      _objectSubtitle(obj),
                      style: TextStyle(
                          color: TA.textSecondary, fontSize: 11),
                    ),
                    onTap: () => onSelect(obj),
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
