import 'dart:convert';
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:teenastro_app/models/equinox_precession.dart';
import 'package:teenastro_app/models/planet_positions.dart';
import 'package:teenastro_app/services/phone_finder.dart';

void main() {
  test('picked Vega Deneb and Altair solve the roof photo', () {
    final jd = julianDate(DateTime.utc(2026, 11, 20));
    final raw = jsonDecode(File(
      'assets/catalogs/stars.json',
    ).readAsStringSync()) as Map<String, dynamic>;
    final catalog = <FinderStar>[];
    final names = <String>[];
    final byName = <String, ({double ra, double dec})>{};
    for (final item in raw['objects'] as List<dynamic>) {
      final m = item as Map<String, dynamic>;
      final mag = (m['mag'] as num?)?.toDouble();
      if (mag == null || mag > 4.5) continue;
      final now = equatorialEquinoxToJNow(
        (m['ra'] as num).toDouble(),
        (m['dec'] as num).toDouble(),
        2000,
        jd,
      );
      final name = ((m['name'] as String?) ?? '').trim();
      catalog.add(FinderStar(now.$1, now.$2));
      names.add(name.isEmpty ? 'Star ${m['id']}' : name);
      if (name.isNotEmpty) byName[name] = (ra: now.$1, dec: now.$2);
    }
    ({String name, bool named, double raH, double decD, double x, double y})
        named(String name, double x, double y) {
      final sky = byName[name]!;
      return (
        name: name,
        named: true,
        raH: sky.ra,
        decD: sky.dec,
        x: x,
        y: y,
      );
    }

    ({String name, bool named, double raH, double decD, double x, double y})
        click(String name, double x, double y) => (
              name: name,
              named: false,
              raH: 0,
              decD: 0,
              x: x,
              y: y,
            );
    final trace = <String>[];
    final result = solveWideField(
      marks: [
        named('Vega', 2271.2, 1541.3),
        named('Deneb', 2040.0, 31.7),
        click('1', 1439.4, 1362.5),
        click('2', 1339.2, 267.9),
        click('3', 1800.2, 412.4),
        click('4', 2248.4, 682.2),
        click('8', 1937.3, 1610.6),
        click('14', 1612.7, 2228.2),
        click('21', 1625.8, 3046.0),
        click('25', 883.5, 1433.4),
        click('26', 848.9, 1268.0),
        named('Altair', 378.7, 1652.4),
      ],
      catalog: catalog,
      catalogNames: names,
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
      trace: trace,
    );
    final solution = result.solution;
    expect(solution, isNotNull, reason: trace.join('\n'));
    expect(solution!.raH, closeTo(18.68, 0.08));
    expect(solution.decD, closeTo(22.4, 1.0));
    expect(solution.matches, greaterThanOrEqualTo(8));
    expect(solution.rmsDeg, lessThan(0.5));
    final text = trace.join('\n');
    expect(text, contains('Match radius 1.0°'));
    expect(text, contains('Vega'));
    expect(text, contains('Deneb'));
    expect(text, contains('Altair'));
    expect(text, contains('Unnamed 14  no catalog star'));
    expect(result.matched.map((s) => s.name), contains('Vega'));
    expect(result.missing, isNotEmpty);
    expect(result.missing.map((s) => s.name), isNot(contains('Vega')));
    expect(result.missing.map((s) => s.name), isNot(contains('Altair')));
    for (final star in result.missing) {
      expect(star.x, inInclusiveRange(0, 3060));
      expect(star.y, inInclusiveRange(0, 4080));
    }
    final camera = widePoseFromNamedStars(
      stars: [
        (raH: byName['Vega']!.ra, decD: byName['Vega']!.dec, x: 2271.2, y: 1541.3),
        (raH: byName['Deneb']!.ra, decD: byName['Deneb']!.dec, x: 2040.0, y: 31.7),
        (raH: byName['Altair']!.ra, decD: byName['Altair']!.dec, x: 378.7, y: 1652.4),
      ],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
    );
    final back = camera!.pixel(camera.direction(2271.2, 1541.3));
    expect(back!.x, closeTo(2271.2, 1.5));
    expect(back.y, closeTo(1541.3, 1.5));
    final fixed = widePoseFromNamedStars(
      stars: [
        (raH: byName['Vega']!.ra, decD: byName['Vega']!.dec, x: 2271.2, y: 1541.3),
        (raH: byName['Deneb']!.ra, decD: byName['Deneb']!.dec, x: 2040.0, y: 31.7),
        (raH: byName['Altair']!.ra, decD: byName['Altair']!.dec, x: 378.7, y: 1652.4),
      ],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
      distortion: -0.06,
    );
    expect(fixed!.distortion, -0.06);
  });
}
