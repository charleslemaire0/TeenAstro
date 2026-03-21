import 'dart:convert';
import 'package:flutter/services.dart';

/// Milky Way band defined by two RA/Dec polylines (center and width edge).
/// The planetarium renderer fills the area between them as a semi-transparent band.
class MilkyWayBand {
  /// Center line: list of [RA hours, Dec degrees]
  final List<(double, double)> center;

  /// Width line (outer edge): list of [RA hours, Dec degrees]
  final List<(double, double)> width;

  const MilkyWayBand({required this.center, required this.width});

  static MilkyWayBand? _cache;

  static Future<MilkyWayBand> load() async {
    if (_cache != null) return _cache!;
    final jsonStr =
        await rootBundle.loadString('assets/data/milky_way.json');
    final json = jsonDecode(jsonStr) as Map<String, dynamic>;

    List<(double, double)> parse(List<dynamic> arr) {
      return arr.map((p) {
        final pair = p as List<dynamic>;
        return ((pair[0] as num).toDouble(), (pair[1] as num).toDouble());
      }).toList();
    }

    _cache = MilkyWayBand(
      center: parse(json['band'] as List),
      width: parse(json['width'] as List),
    );
    return _cache!;
  }
}
