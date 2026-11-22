import 'dart:math' as math;

import 'package:flutter_test/flutter_test.dart';

import '../tool/field_size.dart';

void main() {
  test('the longer side is recovered from stars on a wide plate', () {
    const width = 4000.0;
    const height = 3000.0;
    const longSide = 64.0;
    final focal = (width / 2) / math.tan(longSide * math.pi / 360);
    const ra0 = 80.0 * math.pi / 180;
    const dec0 = -5.0 * math.pi / 180;
    final center = [
      math.cos(dec0) * math.cos(ra0),
      math.cos(dec0) * math.sin(ra0),
      math.sin(dec0),
    ];
    final east = [-math.sin(ra0), math.cos(ra0), 0.0];
    final north = [
      -math.sin(dec0) * math.cos(ra0),
      -math.sin(dec0) * math.sin(ra0),
      math.cos(dec0),
    ];
    final stars = <SkyPixel>[];
    for (final spot in const [
      [400.0, 300.0],
      [3600.0, 400.0],
      [500.0, 2600.0],
      [3400.0, 2500.0],
      [2000.0, 1500.0],
      [1200.0, 900.0],
      [2800.0, 1800.0],
      [800.0, 1600.0],
    ]) {
      final x = spot[0];
      final y = spot[1];
      final camX = (x - (width - 1) / 2) / focal;
      final camY = ((height - 1) / 2 - y) / focal;
      final sky = [
        camX * east[0] + camY * north[0] + center[0],
        camX * east[1] + camY * north[1] + center[1],
        camX * east[2] + camY * north[2] + center[2],
      ];
      final norm = math.sqrt(sky[0] * sky[0] + sky[1] * sky[1] + sky[2] * sky[2]);
      final ra = math.atan2(sky[1] / norm, sky[0] / norm);
      final dec = math.asin((sky[2] / norm).clamp(-1.0, 1.0));
      stars.add(SkyPixel(
        x,
        y,
        ra * 180 / math.pi,
        dec * 180 / math.pi,
      ));
    }

    final field = longSideFieldDeg(
      centerRaDeg: 80,
      centerDecDeg: -5,
      width: width,
      height: height,
      stars: stars,
    );
    expect(field, isNotNull);
    expect(field!, closeTo(longSide, 0.05));
    expect(longSideFromWidthDeg(40, 2000, 1000), closeTo(40, 0.001));
    expect(longSideFromWidthDeg(40, 1000, 2000), greaterThan(40));
  });
}
