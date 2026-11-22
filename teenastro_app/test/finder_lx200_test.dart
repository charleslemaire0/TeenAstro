import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:teenastro_app/services/finder_lx200.dart';
import 'package:teenastro_app/services/finder_lx200_server.dart';
import 'package:teenastro_app/services/finder_sky.dart';

void main() {
  final local = DateTime(2026, 11, 22, 21, 30, 15);
  FinderSnapshot sky({double? ra = 5.5, double? dec = -12.5}) => FinderSnapshot(
        local: local,
        latDeg: 47.5,
        lonEastDeg: 7.5,
        raH: ra,
        decD: dec,
      );

  group('sidereal time', () {
    test('Greenwich, Meeus example 12.a', () {
      final lst = localSiderealHours(DateTime.utc(1987, 4, 10), 0);
      expect(lst, closeTo(13 + 10 / 60 + 46.3668 / 3600, 1 / 3600));
    });

    test('15° east adds one hour', () {
      final t = DateTime.utc(1987, 4, 10);
      expect(localSiderealHours(t, 15), closeTo(localSiderealHours(t, 0) + 1, 1e-9));
    });
  });

  group('formats', () {
    test('Meade longitude is degrees west, 0 to 360', () {
      expect(formatMeadeLongitude(7.5), '352*30');
      expect(formatMeadeLongitude(-71.25), '071*15');
      expect(formatMeadeLongitude(0), '000*00');
    });

    test('UTC offset is hours added to local time', () {
      expect(formatUtcOffset(const Duration(hours: 1)), '-01.0');
      expect(formatUtcOffset(const Duration(hours: -5, minutes: -30)), '+05.5');
      expect(formatUtcOffset(Duration.zero), '+00.0');
    });

    test('low precision RA and Dec', () {
      expect(formatRa(23.999, false), '23:59.9');
      expect(formatRa(23.9999, false), '00:00.0');
      expect(formatRa(5.5, false), '05:30.0');
      expect(formatDec(-12.5, false), '-12*30');
    });
  });

  group('replies', () {
    test('position in high and low precision', () {
      final lx = FinderLx200();
      expect(lx.command(':GR#', sky()), '05:30:00#');
      expect(lx.command(':GD#', sky()), '-12*30:00#');
      expect(lx.command(':U#', sky()), isNull);
      expect(lx.command(':GR#', sky()), '05:30.0#');
      expect(lx.command(':GD#', sky()), '-12*30#');
    });

    test('before a solve, the zenith', () {
      final lx = FinderLx200();
      final s = sky(ra: null, dec: null);
      expect(lx.command(':GD#', s), '+47*30:00#');
      expect(lx.command(':GA#', s), '+90*00:00#');
    });

    test('status, slews and syncs', () {
      final lx = FinderLx200();
      expect(lx.command(':GW#', sky()), 'AT2#');
      expect(lx.command(':GVP#', sky()), 'TeenAstro Finder#');
      expect(lx.command(':D#', sky()), '#');
      expect(lx.command(':MS#', sky()), '1');
      expect(lx.command(':CM#', sky()), 'N/A#');
      expect(lx.command(':Q#', sky()), isNull);
      expect(lx.command(':Me#', sky()), isNull);
      expect(lx.command(':XYZ#', sky()), '#');
    });

    test('site and time', () {
      final lx = FinderLx200();
      expect(lx.command(':Gt#', sky()), '+47*30#');
      expect(lx.command(':Gg#', sky()), '352*30#');
      expect(lx.command(':GL#', sky()), '21:30:15#');
      expect(lx.command(':GC#', sky()), '11/22/26#');
      expect(lx.command(':GS#', sky()), matches(RegExp(r'^\d\d:\d\d:\d\d#$')));
    });

    test('target is reported once RA and Dec are both set', () {
      final got = <(double, double)>[];
      final lx = FinderLx200(onTarget: (ra, dec) => got.add((ra, dec)));
      expect(lx.command(':Sr 10:30:00#', sky()), '1');
      expect(got, isEmpty);
      expect(lx.command(':Sd -05*15:00#', sky()), '1');
      expect(got.single.$1, closeTo(10.5, 1e-9));
      expect(got.single.$2, closeTo(-5.25, 1e-9));
      expect(lx.command(':Sr 25:00:00#', sky()), '0');
      expect(lx.command(':SC 11/22/26#', sky()), '1');
    });

    test('ACK and split commands', () {
      final lx = FinderLx200();
      expect(lx.feed('\x06', sky()), ['A']);
      expect(lx.feed(':G', sky()), isEmpty);
      expect(lx.feed('R#:GD#:Q#', sky()), ['05:30:00#', '-12*30:00#']);
      expect(lx.feed('junk\x06', sky()), ['A']);
    });
  });

  test('finder sky without a solve has no telescope position', () {
    final state = FinderSky(clock: local, active: true, latDeg: 47.5, lonEastDeg: 7.5)
        .toMountState();
    expect(state.ra, '?');
    expect(state.latitude, 47.5);
    expect(state.siderealTime, matches(RegExp(r'^\d\d:\d\d:\d\d$')));
    final solved = FinderSky(clock: local, active: true, raH: 5.5, decD: -12.5).toMountState();
    expect(solved.ra, '05:30:00');
    expect(solved.siderealTime, '?');
  });

  test('server answers over TCP', () async {
    final server = FinderLx200Server(snapshot: () => sky(), onTarget: (_, __) {});
    await server.start(port: 40300);
    final socket = await Socket.connect(InternetAddress.loopbackIPv4, 40300);
    socket.write(':GR#');
    final reply = await socket.first;
    expect(String.fromCharCodes(reply), '05:30:00#');
    socket.destroy();
    await server.stop();
  });
}
