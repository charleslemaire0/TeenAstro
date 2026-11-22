import 'package:flutter_test/flutter_test.dart';
import 'package:teenastro_app/services/phone_finder.dart' show sphereSeparationDeg;
import 'package:teenastro_app/services/push_to.dart';

void main() {
  const lat = 45.0;
  const lst = 6.0;

  test('azimuth wraps across north', () {
    // Near the pole: west of north (az ~359) to east of north (az ~1).
    final p = pushTo(
      fromRa: lst - 6, fromDec: 89, toRa: lst + 6, toDec: 89, latDeg: lat, lstH: lst);
    expect(p.dAzDeg, inInclusiveRange(0.5, 5));
    expect(p.separationDeg, closeTo(2, 1e-6));
  });

  test('up along the meridian', () {
    final p = pushTo(
      fromRa: lst, fromDec: 0, toRa: lst, toDec: 10, latDeg: lat, lstH: lst);
    expect(p.dAltDeg, closeTo(10, 1e-9));
    expect(p.dAzDeg!.abs(), lessThan(1e-9));
    expect(formatPushTo(p), startsWith('10.0°'));
    expect(formatPushTo(p), contains('up 10.0°'));
  });

  test('down along the meridian', () {
    final p = pushTo(
      fromRa: lst, fromDec: 10, toRa: lst, toDec: 0, latDeg: lat, lstH: lst);
    expect(p.dAltDeg, closeTo(-10, 1e-9));
    expect(formatPushTo(p), contains('down 10.0°'));
  });

  test('west of south is to the right, east is to the left', () {
    final west = pushTo(
      fromRa: lst, fromDec: 0, toRa: lst - 1, toDec: 0, latDeg: lat, lstH: lst);
    expect(west.dAzDeg, greaterThan(0));
    expect(formatPushTo(west), contains('right'));
    final east = pushTo(
      fromRa: lst, fromDec: 0, toRa: lst + 1, toDec: 0, latDeg: lat, lstH: lst);
    expect(east.dAzDeg, lessThan(0));
    expect(formatPushTo(east), contains('left'));
  });

  test('separation matches the great circle', () {
    final p = pushTo(fromRa: 3.2, fromDec: 21, toRa: 4.9, toDec: -7, latDeg: lat, lstH: lst);
    expect(p.separationDeg, closeTo(sphereSeparationDeg(3.2, 21, 4.9, -7), 1e-12));
  });

  test('without a site, RA and Dec change, RA wrapped', () {
    final p = pushTo(fromRa: 23.5, fromDec: 10, toRa: 0.5, toDec: 12);
    expect(p.altAz, isFalse);
    expect(p.dRaH, closeTo(1, 1e-9));
    expect(p.dDecDeg, closeTo(2, 1e-9));
    expect(formatPushTo(p), contains('RA +60.0m'));
    expect(formatPushTo(p), contains('Dec +2.0°'));
  });
}
