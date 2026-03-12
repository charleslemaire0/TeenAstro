import 'dart:convert';
import 'package:flutter/services.dart';

class ConstellationLine {
  final double ra1, dec1, ra2, dec2;
  const ConstellationLine(this.ra1, this.dec1, this.ra2, this.dec2);
}

class Constellation {
  final String abbr;
  final List<ConstellationLine> lines;
  const Constellation({required this.abbr, required this.lines});
}

class ConstellationData {
  static List<Constellation>? _cache;

  static Future<List<Constellation>> load() async {
    if (_cache != null) return _cache!;

    final jsonStr = await rootBundle.loadString(
        'assets/data/constellation_lines.json');
    final json = jsonDecode(jsonStr) as Map<String, dynamic>;
    final list = json['constellations'] as List;

    _cache = list.map((c) {
      final lines = (c['lines'] as List).map((seg) {
        final s = seg as List;
        return ConstellationLine(
          (s[0] as num).toDouble(),
          (s[1] as num).toDouble(),
          (s[2] as num).toDouble(),
          (s[3] as num).toDouble(),
        );
      }).toList();
      return Constellation(abbr: c['abbr'] as String, lines: lines);
    }).toList();

    return _cache!;
  }
}
