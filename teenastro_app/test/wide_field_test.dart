import 'dart:convert';
import 'dart:io';
import 'dart:math' as math;

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
        (
          raH: byName['Vega']!.ra,
          decD: byName['Vega']!.dec,
          x: 2271.2,
          y: 1541.3
        ),
        (
          raH: byName['Deneb']!.ra,
          decD: byName['Deneb']!.dec,
          x: 2040.0,
          y: 31.7
        ),
        (
          raH: byName['Altair']!.ra,
          decD: byName['Altair']!.dec,
          x: 378.7,
          y: 1652.4
        ),
      ],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
    );
    final back = camera!.pixel(camera.direction(2271.2, 1541.3));
    expect(back!.x, closeTo(2271.2, 1.5));
    expect(back.y, closeTo(1541.3, 1.5));
    final shifted = WideCamera(
      solution: camera.solution,
      fovDeg: camera.fovDeg,
      focalPx: camera.focalPx,
      lens: const LensTerms.none(),
      originX: 1400,
      originY: 2200,
      width: 3060,
      height: 4080,
      toSky: const [
        (x: 1, y: 0, z: 0),
        (x: 0, y: 1, z: 0),
        (x: 0, y: 0, z: 1),
      ],
    );
    final shiftedBack = shifted.pixel(shifted.direction(900, 1800));
    expect(shiftedBack!.x, closeTo(900, 1.5));
    expect(shiftedBack.y, closeTo(1800, 1.5));
    final fixed = widePoseFromNamedStars(
      stars: [
        (
          raH: byName['Vega']!.ra,
          decD: byName['Vega']!.dec,
          x: 2271.2,
          y: 1541.3
        ),
        (
          raH: byName['Deneb']!.ra,
          decD: byName['Deneb']!.dec,
          x: 2040.0,
          y: 31.7
        ),
        (
          raH: byName['Altair']!.ra,
          decD: byName['Altair']!.dec,
          x: 378.7,
          y: 1652.4
        ),
      ],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
      distortion: const LensTerms(-0.06, 0, 0),
    );
    expect(fixed!.lens.k1, -0.06);
    expect(fixed.lens.k2, 0);
    expect(fixed.lens.k3, 0);
    final higher = widePoseFromNamedStars(
      stars: [
        (
          raH: byName['Vega']!.ra,
          decD: byName['Vega']!.dec,
          x: 2271.2,
          y: 1541.3
        ),
        (
          raH: byName['Deneb']!.ra,
          decD: byName['Deneb']!.dec,
          x: 2040.0,
          y: 31.7
        ),
        (
          raH: byName['Altair']!.ra,
          decD: byName['Altair']!.dec,
          x: 378.7,
          y: 1652.4
        ),
      ],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
      distortion: const LensTerms(-0.12, 0.08, -0.03),
    );
    expect(higher, isNotNull);
    expect(higher!.lens.k1, -0.12);
    expect(higher.lens.k2, 0.08);
    expect(higher.lens.k3, -0.03);
    final edge = higher.pixel(higher.direction(200, 200));
    expect(edge, isNotNull);
    expect(edge!.x, closeTo(200, 1.5));
    expect(edge.y, closeTo(200, 1.5));
    final corner = fixed.pixel(fixed.direction(200, 200));
    expect(corner, isNotNull);
    expect(corner!.x, closeTo(200, 1.5));
    expect(corner.y, closeTo(200, 1.5));
    ({double x, double y, double z}) sky(double raH, double decD) {
      final ra = raH * math.pi / 12;
      final dec = decD * math.pi / 180;
      return (
        x: math.cos(dec) * math.cos(ra),
        y: math.cos(dec) * math.sin(ra),
        z: math.sin(dec),
      );
    }

    final cor = byName['Cor Caroli']!;
    final corPix = fixed.pixel(sky(cor.ra, cor.dec));
    if (corPix != null) {
      final inside = corPix.x >= 0 &&
          corPix.y >= 0 &&
          corPix.x <= 3060 &&
          corPix.y <= 4080;
      expect(inside, isFalse);
    }
  });

  test('unnamed clicks solve around a close hint', () {
    final roof = _roofCatalog();
    final trace = <String>[];
    final result = solveWideFromHint(
      marks: _roofClicks(),
      catalog: roof.catalog,
      catalogNames: roof.names,
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
      hintRa: 18.75,
      hintDec: 23.2,
      trace: trace,
    );
    final solution = result.solution;
    expect(solution, isNotNull, reason: trace.join('\n'));
    expect(solution!.raH, closeTo(18.68, 0.08));
    expect(solution.decD, closeTo(22.4, 1.0));
    expect(solution.matches, greaterThanOrEqualTo(3));
    final text = trace.join('\n');
    expect(text, contains('Pinhole camera'));
    expect(text, contains('No lens calibration'));
    expect(text, contains('roll '));
    expect(result.matched.map((s) => s.name), contains('Vega'));
  });

  test('one named star anchors a hinted solve', () {
    final roof = _roofCatalog();
    final marks = _roofClicks();
    final vega = roof.byName['Vega']!;
    marks[0] = (
      name: 'Vega',
      named: true,
      raH: vega.ra,
      decD: vega.dec,
      x: marks[0].x,
      y: marks[0].y,
    );
    final trace = <String>[];
    final result = solveWideFromHint(
      marks: marks,
      catalog: roof.catalog,
      catalogNames: roof.names,
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
      hintRa: 18.75,
      hintDec: 23.2,
      trace: trace,
    );
    final solution = result.solution;
    expect(solution, isNotNull, reason: trace.join('\n'));
    expect(solution!.raH, closeTo(18.68, 0.08));
    expect(solution.decD, closeTo(22.4, 1.0));
    expect(solution.matches, greaterThanOrEqualTo(3));
    expect(trace.join('\n'), contains('named mark'));
    expect(result.matched.map((s) => s.name), contains('Vega'));
  });

  test('a stored camera solves without named stars', () {
    final roof = _roofCatalog();
    final by = roof.byName;
    final camera = widePoseFromNamedStars(
      stars: [
        (raH: by['Vega']!.ra, decD: by['Vega']!.dec, x: 2271.2, y: 1541.3),
        (raH: by['Deneb']!.ra, decD: by['Deneb']!.dec, x: 2040.0, y: 31.7),
        (raH: by['Altair']!.ra, decD: by['Altair']!.dec, x: 378.7, y: 1652.4),
      ],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
      fitDistortion: true,
    );
    expect(camera, isNotNull);
    final trace = <String>[];
    final result = solveWideFromHint(
      marks: _roofClicks(),
      catalog: roof.catalog,
      catalogNames: roof.names,
      width: 3060,
      height: 4080,
      fovDeg: camera!.fovDeg,
      distortion: camera.lens,
      calibrated: true,
      hintRa: 18.75,
      hintDec: 23.2,
      trace: trace,
    );
    final solution = result.solution;
    expect(solution, isNotNull, reason: trace.join('\n'));
    expect(solution!.raH, closeTo(18.68, 0.08));
    expect(solution.decD, closeTo(22.4, 1.0));
    expect(solution.matches, greaterThanOrEqualTo(3));
    final text = trace.join('\n');
    expect(text, contains('Stored field'));
    expect(text, isNot(contains('90% to 110%')));
    expect(result.matched.map((s) => s.name), contains('Vega'));
  });

  test('a click on a faint star is not named as the bright neighbor', () {
    final roof = _roofCatalog();
    final by = roof.byName;
    final camera = widePoseFromNamedStars(
      stars: [
        (raH: by['Vega']!.ra, decD: by['Vega']!.dec, x: 2271.2, y: 1541.3),
        (raH: by['Deneb']!.ra, decD: by['Deneb']!.dec, x: 2040.0, y: 31.7),
        (raH: by['Altair']!.ra, decD: by['Altair']!.dec, x: 378.7, y: 1652.4),
      ],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
    );
    expect(camera, isNotNull);
    (double, double) raDec(({double x, double y, double z}) v) {
      final dec = math.asin(v.z.clamp(-1.0, 1.0)) * 180 / math.pi;
      var ra = math.atan2(v.y, v.x);
      if (ra < 0) ra += 2 * math.pi;
      return (ra * 12 / math.pi, dec);
    }

    double ang(
      ({double x, double y, double z}) a,
      ({double x, double y, double z}) b,
    ) {
      final d = (a.x * b.x + a.y * b.y + a.z * b.z).clamp(-1.0, 1.0);
      return math.acos(d) * 180 / math.pi;
    }

    ({double x, double y, double z}) skyOf(FinderStar s) {
      final ra = s.raH * math.pi / 12;
      final dec = s.decD * math.pi / 180;
      return (
        x: math.cos(dec) * math.cos(ra),
        y: math.cos(dec) * math.sin(ra),
        z: math.sin(dec),
      );
    }

    ({double x, double y})? quiet;
    for (var y = 300.0; y < 3800 && quiet == null; y += 100) {
      for (var x = 300.0; x < 2800 && quiet == null; x += 100) {
        final dir = camera!.direction(x, y);
        var crowded = false;
        for (final s in roof.catalog) {
          if (ang(dir, skyOf(s)) < 1.3) {
            crowded = true;
            break;
          }
        }
        if (!crowded) quiet = (x: x, y: y);
      }
    }
    expect(quiet, isNotNull);
    final click = quiet!;
    ({String name, bool named, double raH, double decD, double x, double y})
        star(String name, double raH, double decD, double x, double y) => (
              name: name,
              named: true,
              raH: raH,
              decD: decD,
              x: x,
              y: y,
            );
    final anchors = [
      star('Vega', by['Vega']!.ra, by['Vega']!.dec, 2271.2, 1541.3),
      star('Deneb', by['Deneb']!.ra, by['Deneb']!.dec, 2040.0, 31.7),
      star('Altair', by['Altair']!.ra, by['Altair']!.dec, 378.7, 1652.4),
    ];
    final there = raDec(camera!.direction(click.x, click.y));
    final trace = <String>[];
    final apart = solveWideField(
      marks: [
        ...anchors,
        (
          name: '14',
          named: false,
          raH: 0.0,
          decD: 0.0,
          x: click.x,
          y: click.y,
        ),
      ],
      catalog: roof.catalog,
      catalogNames: roof.names,
      identifyCatalog: [
        FinderStar(by['Vega']!.ra, by['Vega']!.dec),
        FinderStar(by['Deneb']!.ra, by['Deneb']!.dec),
        FinderStar(by['Altair']!.ra, by['Altair']!.dec),
        FinderStar(there.$1, there.$2 + 0.4),
        FinderStar(there.$1, there.$2),
      ],
      identifyNames: const ['Vega', 'Deneb', 'Altair', 'Bright', 'Faint'],
      identifyMags: const [0.0, 1.2, 0.8, 2.2, 7.5],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
      trace: trace,
    );
    expect(apart.solution, isNotNull, reason: trace.join('\n'));
    expect(
      apart.matched.where((s) => (s.x - click.x).abs() < 1).map((s) => s.name),
      ['Faint'],
    );

    final tie = solveWideField(
      marks: [
        ...anchors,
        (
          name: '14',
          named: false,
          raH: 0.0,
          decD: 0.0,
          x: click.x,
          y: click.y,
        ),
      ],
      catalog: roof.catalog,
      catalogNames: roof.names,
      identifyCatalog: [
        FinderStar(by['Vega']!.ra, by['Vega']!.dec),
        FinderStar(by['Deneb']!.ra, by['Deneb']!.dec),
        FinderStar(by['Altair']!.ra, by['Altair']!.dec),
        FinderStar(there.$1, there.$2 + 0.04),
        FinderStar(there.$1, there.$2 + 0.02),
      ],
      identifyNames: const ['Vega', 'Deneb', 'Altair', 'Bright', 'Faint'],
      identifyMags: const [0.0, 1.2, 0.8, 2.2, 7.5],
      width: 3060,
      height: 4080,
      fovDeg: 66.8,
    );
    expect(
      tie.matched.where((s) => (s.x - click.x).abs() < 1).map((s) => s.name),
      ['Bright'],
    );
  });

  test('a gap in the recognized magnitudes sets the limit', () {
    expect(
      recognizedMagnitudeLimit(
        [2.2, 3.1, 3.4, 3.8, 4.1, 4.4, 4.9, 5.2, 5.47, 6.25, 7.9],
      ),
      closeTo(5.47, 0.001),
    );
    expect(recognizedMagnitudeLimit([0, 1.2, 2.2, 7.5]), isNull);
    expect(
      recognizedMagnitudeLimit(
        [3.0, 3.2, 3.4, 3.6, 3.8, 4.0, 4.2, 4.4, 4.6, 4.8],
      ),
      isNull,
    );
  });
}

({
  List<FinderStar> catalog,
  List<String> names,
  Map<String, ({double ra, double dec})> byName,
}) _roofCatalog() {
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
  return (catalog: catalog, names: names, byName: byName);
}

List<({String name, bool named, double raH, double decD, double x, double y})>
    _roofClicks() {
  ({String name, bool named, double raH, double decD, double x, double y})
      click(String name, double x, double y) => (
            name: name,
            named: false,
            raH: 0,
            decD: 0,
            x: x,
            y: y,
          );
  return [
    click('Vega', 2271.2, 1541.3),
    click('Deneb', 2040.0, 31.7),
    click('1', 1439.4, 1362.5),
    click('2', 1339.2, 267.9),
    click('3', 1800.2, 412.4),
    click('4', 2248.4, 682.2),
    click('8', 1937.3, 1610.6),
    click('14', 1612.7, 2228.2),
    click('21', 1625.8, 3046.0),
    click('25', 883.5, 1433.4),
    click('26', 848.9, 1268.0),
    click('Altair', 378.7, 1652.4),
  ];
}
