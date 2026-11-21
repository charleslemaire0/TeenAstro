import 'dart:io';
import 'dart:math' as math;
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:teenastro_app/services/phone_finder.dart';
import 'package:teenastro_app/services/stellar_solver.dart';

void main() {
  test('StellarSolver recovers a field from the index stars', () {
    final files = <String, List<int>>{};
    final dir = Directory('tool/stellar/indexes');
    for (final file in dir.listSync()) {
      if (file is File && file.path.endsWith('.fits')) {
        files[file.uri.pathSegments.last] = file.readAsBytesSync();
      }
    }
    final indexes = parseStellarIndexes({
      for (final entry in files.entries)
        entry.key: Uint8List.fromList(entry.value),
    });
    expect(indexes, hasLength(3));

    const ra = 280.0 * math.pi / 180;
    const dec = 22.6 * math.pi / 180;
    const roll = 0.5;
    final center = [
      math.cos(dec) * math.cos(ra),
      math.cos(dec) * math.sin(ra),
      math.sin(dec),
    ];
    final east = [-math.sin(ra), math.cos(ra), 0.0];
    final north = [
      -math.sin(dec) * math.cos(ra),
      -math.sin(dec) * math.sin(ra),
      math.cos(dec),
    ];
    final right = [
      for (var i = 0; i < 3; i++)
        math.cos(roll) * east[i] + math.sin(roll) * north[i],
    ];
    final down = [
      for (var i = 0; i < 3; i++)
        math.sin(roll) * east[i] - math.cos(roll) * north[i],
    ];
    const width = 2000.0;
    const height = 1500.0;
    const longSide = 55.0;
    final widthDeg = longSide * width / math.max(width, height);
    final focal = (width / 2) / math.tan(widthDeg * math.pi / 360);
    final stars = <PixelStar>[];
    final catalog = indexes.last;
    for (var s = 0; s < catalog.starCount && stars.length < 18; s++) {
      final sky = [
        catalog.stars[s * 3],
        catalog.stars[s * 3 + 1],
        catalog.stars[s * 3 + 2],
      ];
      final cam = [
        sky[0] * right[0] + sky[1] * right[1] + sky[2] * right[2],
        sky[0] * down[0] + sky[1] * down[1] + sky[2] * down[2],
        sky[0] * center[0] + sky[1] * center[1] + sky[2] * center[2],
      ];
      if (cam[2] < 0.35) continue;
      final x = (width - 1) / 2 + focal * cam[0] / cam[2];
      final y = (height - 1) / 2 + focal * cam[1] / cam[2];
      if (x < 20 || x > width - 20 || y < 20 || y > height - 20) continue;
      stars.add(PixelStar(x, y));
    }
    expect(stars.length, greaterThanOrEqualTo(12));

    final solved = solveStellarField(
      stars: stars,
      indexes: indexes,
      width: width,
      height: height,
      longSideDeg: longSide,
    );
    final solution = solved.solution;
    expect(solution, isNotNull, reason: solved.reason);
    final gap = sphereSeparationDeg(
      solution!.raH,
      solution.decD,
      280 / 15,
      22.6,
    );
    expect(gap, lessThan(0.2));
    expect(solution.matches, greaterThanOrEqualTo(8));

    final hinted = solveStellarField(
      stars: stars,
      indexes: indexes,
      width: width,
      height: height,
      longSideDeg: longSide,
      hintRaH: 280 / 15,
      hintDecD: 22.6,
    ).solution;
    expect(hinted, isNotNull);
    expect(
      sphereSeparationDeg(hinted!.raH, hinted.decD, 280 / 15, 22.6),
      lessThan(0.2),
    );

    final elsewhere = solveStellarField(
      stars: stars,
      indexes: indexes,
      width: width,
      height: height,
      longSideDeg: longSide,
      hintRaH: 100 / 15,
      hintDecD: -40,
    ).solution;
    expect(elsewhere, isNotNull);
    expect(
      sphereSeparationDeg(elsewhere!.raH, elsewhere.decD, 280 / 15, 22.6),
      lessThan(0.2),
    );
  });
}
