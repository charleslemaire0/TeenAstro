import 'dart:math' as math;
import 'package:flutter/gestures.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/catalog_entry.dart';
import '../models/constellation_lines.dart';
import '../models/mount_state.dart';
import '../services/catalog_service.dart';
import '../services/mount_state_provider.dart';
import '../theme.dart';

/// Sky planetarium widget.
/// [onStarSelected] fires when user taps a named star (only bright, named stars).
/// [overlayMessage] shows a prominent message across the map.
/// [highlightedStar] draws a highlight ring around a specific star.
class SkyMapWidget extends ConsumerStatefulWidget {
  final double? height;
  final void Function(CatalogEntry star)? onStarSelected;
  final String? overlayMessage;
  final CatalogEntry? highlightedStar;

  const SkyMapWidget({
    super.key,
    this.height,
    this.onStarSelected,
    this.overlayMessage,
    this.highlightedStar,
  });

  @override
  ConsumerState<SkyMapWidget> createState() => _SkyMapWidgetState();
}

class _SkyMapWidgetState extends ConsumerState<SkyMapWidget> {
  List<CatalogEntry>? _stars;
  List<Constellation>? _constellations;
  double _zoom = 1.0;
  double _rotation = 0.0;
  double _panY = 0.0;
  Offset _lastFocalPoint = Offset.zero;
  double _lastScale = 1.0;
  static const double _radiansPerPixel = 0.008;
  String? _tappedInfo;

  @override
  void initState() {
    super.initState();
    _loadData();
  }

  Future<void> _loadData() async {
    try {
      final catalogs = await ref.read(catalogServiceProvider).loadCatalogs();
      final starCat =
          catalogs.where((c) => c.type == CatalogType.star).toList();
      final consts = await ConstellationData.load();

      List<CatalogEntry> stars = [];
      for (final cat in starCat) {
        stars.addAll(cat.objects.where((o) => o.mag != null && o.mag! <= 5.5));
      }
      if (stars.isEmpty) {
        for (final cat in catalogs) {
          stars.addAll(cat.objects
              .where((o) => o.mag != null && o.mag! <= 4.5)
              .take(200));
        }
      }

      if (mounted) {
        setState(() {
          _stars = stars;
          _constellations = consts;
        });
      }
    } catch (e) {
      // Load failed - leave _stars/_constellations null
    }
  }

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(mountStateProvider);

    Widget mapContent;
    if (_stars == null) {
      mapContent = const Center(
          child: SizedBox(
              width: 24,
              height: 24,
              child: CircularProgressIndicator(strokeWidth: 2)));
    } else {
      mapContent = Listener(
        onPointerSignal: (event) {
          if (event is PointerScrollEvent) {
            setState(() {
              final delta = event.scrollDelta.dy > 0 ? 0.9 : 1.1;
              _zoom = (_zoom * delta).clamp(0.5, 30.0);
            });
          }
        },
        child: GestureDetector(
          onScaleStart: (d) {
            _lastFocalPoint = d.focalPoint;
            _lastScale = 1.0;
          },
          onScaleUpdate: (d) {
            setState(() {
              _zoom = (_zoom * (d.scale / _lastScale)).clamp(0.5, 30.0);
              _lastScale = d.scale;
              final delta = d.focalPoint - _lastFocalPoint;
              _lastFocalPoint = d.focalPoint;
              _rotation += delta.dx * _radiansPerPixel;
              _panY += delta.dy;
            });
          },
          onTapUp: (d) => _handleTap(d.localPosition, state),
          child: Stack(
            fit: StackFit.expand,
            children: [
              CustomPaint(
                size: Size.infinite,
                painter: _SkyPainter(
                  stars: _stars!,
                  constellations: _constellations ?? [],
                  state: state,
                  zoom: _zoom,
                  rotation: _rotation,
                  panY: _panY,
                  highlightedStar: widget.highlightedStar,
                ),
              ),
              // Tapped star tooltip
              if (_tappedInfo != null)
                Positioned(
                  top: 4,
                  left: 4,
                  child: Container(
                    padding:
                        const EdgeInsets.symmetric(horizontal: 8, vertical: 3),
                    decoration: BoxDecoration(
                      color: TAColors.surface.withValues(alpha: 0.9),
                      borderRadius: BorderRadius.circular(4),
                    ),
                    child: Text(_tappedInfo!,
                        style: const TextStyle(
                            color: TAColors.textHigh, fontSize: 11)),
                  ),
                ),
              // Zoom + scope overlay
              Positioned(
                top: 4,
                right: 4,
                child: Container(
                  padding:
                      const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
                  decoration: BoxDecoration(
                    color: Colors.black54,
                    borderRadius: BorderRadius.circular(4),
                  ),
                  child: Text(
                    '${_zoom.toStringAsFixed(1)}x  ${state.ra} / ${state.dec.replaceAll('*', '°')}',
                    style: const TextStyle(
                        color: Colors.white70,
                        fontSize: 10,
                        fontFamily: 'monospace'),
                  ),
                ),
              ),
              // Overlay message (e.g. "Select Star 1")
              if (widget.overlayMessage != null)
                Positioned(
                  bottom: 8,
                  left: 0,
                  right: 0,
                  child: Center(
                    child: Container(
                      padding: const EdgeInsets.symmetric(
                          horizontal: 16, vertical: 6),
                      decoration: BoxDecoration(
                        color: TAColors.accent.withValues(alpha: 0.85),
                        borderRadius: BorderRadius.circular(20),
                      ),
                      child: Text(
                        widget.overlayMessage!,
                        style: const TextStyle(
                            color: Colors.white,
                            fontSize: 13,
                            fontWeight: FontWeight.w600),
                      ),
                    ),
                  ),
                ),
            ],
          ),
        ),
      );
    }

    final content = ClipRRect(
      borderRadius: BorderRadius.circular(8),
      child: Container(
        color: const Color(0xFF080C12),
        child: mapContent,
      ),
    );

    return SizedBox(
      height: widget.height ?? 400,
      child: content,
    );
  }

  void _handleTap(Offset localPos, MountState state) {
    if (_stars == null) return;
    final size = context.size;
    if (size == null) return;

    final proj = _Projection(
      lat: state.latitude,
      lst: _parseSiderealTime(state.siderealTime),
      size: size,
      zoom: _zoom,
      rotation: _rotation,
      panY: _panY,
    );

    CatalogEntry? nearest;
    double nearestDist = 25.0;

    for (final star in _stars!) {
      final pt = proj.project(star.ra, star.dec);
      if (pt == null) continue;
      final d = (pt - localPos).distance;
      if (d < nearestDist) {
        nearestDist = d;
        nearest = star;
      }
    }

    if (nearest != null) {
      final label =
          '${nearest.name.isNotEmpty ? nearest.name : "Star ${nearest.id}"} '
          '(${nearest.constellationStr}) '
          'mag ${nearest.mag?.toStringAsFixed(1) ?? "?"}';
      setState(() => _tappedInfo = label);
      Future.delayed(const Duration(seconds: 4), () {
        if (mounted) setState(() => _tappedInfo = null);
      });

      if (widget.onStarSelected != null && nearest.name.isNotEmpty) {
        widget.onStarSelected!(nearest);
      }
    }
  }
}

double _parseSiderealTime(String s) {
  if (s == '?' || s.isEmpty) return 0;
  final parts = s.split(':');
  if (parts.isEmpty) return 0;
  final h = double.tryParse(parts[0]) ?? 0;
  final m = parts.length > 1 ? (double.tryParse(parts[1]) ?? 0) : 0;
  final sec = parts.length > 2 ? (double.tryParse(parts[2]) ?? 0) : 0;
  return h + m / 60 + sec / 3600;
}

double _parseRa(String s) {
  if (s == '?' || s.isEmpty) return 0;
  final parts = s.split(':');
  if (parts.isEmpty) return 0;
  final h = double.tryParse(parts[0]) ?? 0;
  final m = parts.length > 1 ? (double.tryParse(parts[1]) ?? 0) : 0;
  final sec = parts.length > 2 ? (double.tryParse(parts[2]) ?? 0) : 0;
  return h + m / 60 + sec / 3600;
}

double _parseDec(String s) {
  if (s == '?' || s.isEmpty) return 0;
  final sign = s.startsWith('-') ? -1.0 : 1.0;
  final withColons = s.replaceAll('*', ':');
  final cleaned = withColons.replaceAll(RegExp(r'[^0-9.:]'), '');
  final parts = cleaned.split(':');
  if (parts.isEmpty) return 0;
  final d = double.tryParse(parts[0]) ?? 0;
  final m = parts.length > 1 ? (double.tryParse(parts[1]) ?? 0) : 0;
  final sec = parts.length > 2 ? (double.tryParse(parts[2]) ?? 0) : 0;
  return sign * (d + m / 60 + sec / 3600);
}

class _Projection {
  final double lat, lst;
  final Size size;
  final double zoom;
  final double rotation;
  final double panY;
  late final double _latRad, _cx, _cy, _radius;

  _Projection({
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

  double get cx => _cx;
  double get cy => _cy;
  double get radius => _radius;
}

class _SkyPainter extends CustomPainter {
  final List<CatalogEntry> stars;
  final List<Constellation> constellations;
  final MountState state;
  final double zoom;
  final double rotation;
  final double panY;
  final CatalogEntry? highlightedStar;

  _SkyPainter({
    required this.stars,
    required this.constellations,
    required this.state,
    required this.zoom,
    required this.rotation,
    required this.panY,
    this.highlightedStar,
  });

  @override
  void paint(Canvas canvas, Size size) {
    final proj = _Projection(
      lat: state.latitude,
      lst: _parseSiderealTime(state.siderealTime),
      size: size,
      zoom: zoom,
      rotation: rotation,
      panY: panY,
    );

    // Sky background
    canvas.drawRect(Rect.fromLTWH(0, 0, size.width, size.height),
        Paint()..color = const Color(0xFF0C1520));

    // Ground below horizon
    final groundAll = Path()..addRect(Rect.fromLTWH(0, 0, size.width, size.height));
    final skyCircle = Path()..addOval(
      Rect.fromCircle(center: Offset(proj.cx, proj.cy), radius: proj.radius));
    final groundPath = Path.combine(PathOperation.difference, groundAll, skyCircle);
    canvas.drawPath(groundPath, Paint()..color = const Color(0xFF0A140A));

    // Horizon ring
    canvas.drawCircle(
        Offset(proj.cx, proj.cy),
        proj.radius,
        Paint()
          ..color = const Color(0xFF2A4020)
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.5);

    // Cardinal labels on horizon (rotate with map)
    _drawCardinal(canvas, proj, 'N', 0, rotation);
    _drawCardinal(canvas, proj, 'E', math.pi / 2, rotation);
    _drawCardinal(canvas, proj, 'S', math.pi, rotation);
    _drawCardinal(canvas, proj, 'W', 3 * math.pi / 2, rotation);

    // Constellation lines (Stellarium-style)
    final linePaint = Paint()
      ..color = const Color(0xFF4466AA).withValues(alpha: 0.55)
      ..strokeWidth = 1.2
      ..style = PaintingStyle.stroke
      ..strokeCap = StrokeCap.round
      ..strokeJoin = StrokeJoin.round
      ..isAntiAlias = true;

    for (final c in constellations) {
      for (final line in c.lines) {
        final p1 = proj.project(line.ra1, line.dec1, clipHorizon: true);
        final p2 = proj.project(line.ra2, line.dec2, clipHorizon: true);
        if (p1 != null && p2 != null) {
          canvas.drawLine(p1, p2, linePaint);
        }
      }
    }

    // Stars (same formula as planetarium: magLimit, baseR, brightness)
    final magLimit = 6.0 + 2.5 * (math.log(zoom.clamp(0.5, 30)) / math.ln10);
    const neutralColor = Color(0xFFE0E8FF); // no B-V in catalog
    for (final star in stars) {
      final mag = star.mag ?? 5.0;
      if (mag > magLimit) continue;
      final pt = proj.project(star.ra, star.dec);
      if (pt == null) continue;

      final baseR = 2.5 * math.pow(10, -0.2 * mag);
      final r = baseR.clamp(0.5, 12.0);
      if (r < 0.3) continue;
      final brightness = math.pow(10, -0.4 * (mag - magLimit)).clamp(0.2, 1.0).toDouble();
      final cr = neutralColor.red;
      final cg = neutralColor.green;
      final cb = neutralColor.blue;

      if (r >= 1.8) {
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
        canvas.drawCircle(pt, r, Paint()
          ..shader = gradient.createShader(
              Rect.fromCircle(center: pt, radius: r)));
      } else {
        canvas.drawCircle(pt, r, Paint()
          ..color = Color.fromRGBO(
              (cr * brightness).round().clamp(0, 255),
              (cg * brightness).round().clamp(0, 255),
              (cb * brightness).round().clamp(0, 255),
              brightness));
      }

      if (highlightedStar != null &&
          star.ra == highlightedStar!.ra &&
          star.dec == highlightedStar!.dec) {
        canvas.drawCircle(
            pt,
            r + 6,
            Paint()
              ..color = TAColors.warning
              ..style = PaintingStyle.stroke
              ..strokeWidth = 2.0);
      }

      if (mag <= 2.5 && star.name.isNotEmpty && zoom >= 0.8) {
        final labelFontSize = zoom > 4 ? 12.0 : (zoom > 2 ? 10.0 : 9.0);
        final tp = TextPainter(
          text: TextSpan(
            text: star.name,
            style: TextStyle(
              color: const Color(0xFFAABBDD).withValues(alpha: 0.85),
              fontSize: labelFontSize,
            ),
          ),
          textDirection: TextDirection.ltr,
        )..layout();
        tp.paint(canvas, pt + Offset(r + 3, -tp.height / 2));
      }
    }

    // Telescope crosshair
    final scopeRa = _parseRa(state.ra);
    final scopeDec = _parseDec(state.dec);
    final scopePt = proj.project(scopeRa, scopeDec, clipHorizon: false);

    if (scopePt != null) {
      final scopePaint = Paint()
        ..color = TAColors.accent
        ..strokeWidth = 1.5
        ..style = PaintingStyle.stroke;

      const arm = 10.0;
      canvas.drawLine(scopePt - const Offset(arm, 0),
          scopePt + const Offset(arm, 0), scopePaint);
      canvas.drawLine(scopePt - const Offset(0, arm),
          scopePt + const Offset(0, arm), scopePaint);
      canvas.drawCircle(scopePt, 6, scopePaint);
      canvas.drawCircle(scopePt, 2, Paint()..color = TAColors.accent);
    }

    // Target diamond
    if (state.targetRa.isNotEmpty &&
        state.targetDec.isNotEmpty &&
        state.targetRa != '00:00:00') {
      final tRa = _parseRa(state.targetRa);
      final tDec = _parseDec(state.targetDec);
      final tPt = proj.project(tRa, tDec, clipHorizon: false);
      if (tPt != null) {
        final targetPaint = Paint()
          ..color = TAColors.success
          ..strokeWidth = 1.0
          ..style = PaintingStyle.stroke;
        canvas.drawCircle(tPt, 8, targetPaint);
        final path = Path()
          ..moveTo(tPt.dx, tPt.dy - 5)
          ..lineTo(tPt.dx + 5, tPt.dy)
          ..lineTo(tPt.dx, tPt.dy + 5)
          ..lineTo(tPt.dx - 5, tPt.dy)
          ..close();
        canvas.drawPath(path, targetPaint);
      }
    }
  }

  void _drawCardinal(
      Canvas canvas, _Projection proj, String label, double azRad, double rotation) {
    final a = azRad + rotation;
    final x = proj.cx - (proj.radius - 16) * math.sin(a);
    final y = proj.cy - (proj.radius - 16) * math.cos(a);
    final tp = TextPainter(
      text: TextSpan(
        text: label,
        style: const TextStyle(
            color: Color(0xFF5577AA),
            fontSize: 11,
            fontWeight: FontWeight.w700),
      ),
      textDirection: TextDirection.ltr,
    )..layout();
    tp.paint(canvas, Offset(x - tp.width / 2, y - tp.height / 2));
  }

  @override
  bool shouldRepaint(_SkyPainter old) => true;
}
