import 'dart:math' as math;
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:image/image.dart' as img;
import 'package:teenastro_app/services/phone_finder.dart';

List<PixelStar> _project({
  required List<FinderStar> stars,
  required double centerRa,
  required double centerDec,
  required double roll,
  required double fovDeg,
  required double width,
  required double height,
}) {
  final scale = fovDeg / math.max(width, height);
  final out = <PixelStar>[];
  for (final s in stars) {
    final (e, n) = skyOffset(s.raH, s.decD, centerRa, centerDec);
    final (right, up) = skyToTelescope(e, n, roll);
    final x = width / 2 + right / scale;
    final y = height / 2 - up / scale;
    if (x > 1 && x < width - 1 && y > 1 && y < height - 1) {
      out.add(PixelStar(x, y));
    }
  }
  return out;
}

void main() {
  test('solve recovers the camera axis and the frame roll', () {
    const centerRa = 5.25;
    const centerDec = 22.0;
    const roll = 28 * math.pi / 180;
    const width = 800.0;
    const height = 600.0;
    const fov = 36.0;
    final stars = <FinderStar>[
      const FinderStar(5.05, 18.0),
      const FinderStar(5.55, 19.5),
      const FinderStar(4.90, 24.0),
      const FinderStar(5.70, 26.5),
      const FinderStar(5.10, 28.0),
      const FinderStar(5.80, 21.0),
      const FinderStar(4.70, 16.5),
      const FinderStar(6.05, 23.5),
    ];
    final pixels = _project(
      stars: stars,
      centerRa: centerRa,
      centerDec: centerDec,
      roll: roll,
      fovDeg: fov,
      width: width,
      height: height,
    );
    expect(pixels.length, greaterThanOrEqualTo(6));

    final solved = solveField(
      stars: pixels,
      catalog: stars,
      hintRa: centerRa + 0.12,
      hintDec: centerDec - 1.5,
      width: width,
      height: height,
      fovDeg: fov,
    );
    expect(solved, isNotNull);
    final s = solved!;
    var dRa = s.raH - centerRa;
    if (dRa > 12) dRa -= 24;
    if (dRa < -12) dRa += 24;
    expect(dRa.abs() * 15, lessThan(0.4));
    expect((s.decD - centerDec).abs(), lessThan(0.4));
    var dRoll = s.rollRad - roll;
    while (dRoll > math.pi) dRoll -= 2 * math.pi;
    while (dRoll < -math.pi) dRoll += 2 * math.pi;
    expect(dRoll.abs(), lessThan(2 * math.pi / 180));
  });

  test('cones map the camera axis back to the telescope axis', () {
    const telescopeRa = 8.0;
    const telescopeDec = 15.0;
    const camera = FinderSolution(
      raH: 8.04,
      decD: 15.25,
      rollRad: 0.4,
      matches: 6,
    );
    final cones = measureCones(
      telescopeRa: telescopeRa,
      telescopeDec: telescopeDec,
      camera: camera,
    );
    expect(cones.isZero, isFalse);
    final back = telescopeAxis(camera, cones);
    var dRa = back.$1 - telescopeRa;
    if (dRa > 12) dRa -= 24;
    if (dRa < -12) dRa += 24;
    expect(dRa.abs(), lessThan(1e-6));
    expect((back.$2 - telescopeDec).abs(), lessThan(1e-6));

    const later = FinderSolution(
      raH: 10.2,
      decD: -8.0,
      rollRad: -1.1,
      matches: 5,
    );
    final shifted = telescopeAxis(later, cones);
    final again = telescopeAxis(later, const FinderCones());
    expect(shifted.$1, isNot(closeTo(again.$1, 1e-6)));
    final undone = measureCones(
      telescopeRa: shifted.$1,
      telescopeDec: shifted.$2,
      camera: later,
    );
    expect(undone.hzRad, closeTo(cones.hzRad, 1e-6));
    expect(undone.veRad, closeTo(cones.veRad, 1e-6));
  });

  test('detectStars finds isolated peaks', () {
    final frame = img.Image(width: 64, height: 64);
    img.fill(frame, color: img.ColorRgb8(8, 8, 8));
    for (final p in const [(12, 16), (40, 18), (20, 44), (48, 46)]) {
      frame.setPixelRgb(p.$1, p.$2, 255, 255, 255);
    }
    final found = detectStars(frame);
    expect(found.length, 4);
    final bytes = Uint8List.fromList(img.encodeJpg(frame));
    final frameRead = readStarFrame(bytes);
    expect(frameRead, isNotNull);
    expect(frameRead!.stars.length, 4);
    expect(frameRead.width, 64);
    expect(frameRead.height, 64);
  });

  test('separation is the tangent-plane distance in arcminutes', () {
    expect(separationArcmin(5, 10, 5, 10.1), closeTo(6, 0.01));
    expect(separationArcmin(5.01, 0, 5, 0), closeTo(9, 0.05));
  });

  test('mount coordinate text parses', () {
    expect(parseMountHours('05:15:00'), closeTo(5.25, 1e-9));
    expect(parseMountDeg('+22*30:00'), closeTo(22.5, 1e-9));
    expect(parseMountDeg('-03*00:00'), closeTo(-3, 1e-9));
    expect(parseMountHours('?'), isNull);
  });

  double haHours(double lst, double ra) {
    var d = lst - ra;
    if (d > 12) d -= 24;
    if (d < -12) d += 24;
    return d;
  }

  test('from-home plans match the efinder places', () {
    const lat = 48.85;
    const lst = 12.0;
    final two = phoneAlignPlans(stars: 2, lstH: lst, latDeg: lat, altAz: false);
    expect(two, isNotEmpty);
    expect(two.first.places, hasLength(2));
    expect(two.first.flipAfterThird, isFalse);
    expect(haHours(lst, two.first.places[0].raH), closeTo(2.2, 0.02));
    expect(haHours(lst, two.first.places[1].raH), closeTo(5.0, 0.02));

    final four = phoneAlignPlans(stars: 4, lstH: lst, latDeg: lat, altAz: false);
    expect(four.first.places, hasLength(4));
    expect(four.first.flipAfterThird, isFalse);
    expect(haHours(lst, four.first.places[0].raH), greaterThan(0));

    final six = phoneAlignPlans(stars: 6, lstH: lst, latDeg: lat, altAz: false);
    expect(six.first.places, hasLength(6));
    expect(six.first.flipAfterThird, isTrue);
    final ha0 = haHours(lst, six.first.places[0].raH);
    final ha3 = haHours(lst, six.first.places[3].raH);
    expect(ha0.sign, isNot(ha3.sign));

    final spread =
        phoneAlignPlans(stars: 4, lstH: lst, latDeg: lat, altAz: true);
    expect(spread.single.flipAfterThird, isFalse);
    expect(haHours(lst, spread.single.places.first.raH), lessThan(0));
    expect(haHours(lst, spread.single.places.last.raH), greaterThan(0));

    expect(
      phoneAlignPlans(
          stars: 2, lstH: lst, latDeg: lat, altAz: false, minAlt: 70),
      isEmpty,
    );
    expect(phoneAlignPlans(stars: 5, lstH: lst, latDeg: lat, altAz: false),
        isEmpty);
    final high =
        phoneAlignPlans(stars: 2, lstH: lst, latDeg: 80, altAz: false);
    expect(high.first.places.first.decD, closeTo(38, 0.01));
  });
}
