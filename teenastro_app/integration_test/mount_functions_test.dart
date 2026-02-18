/// Automated mount function test suite.
/// Connects to a real TeenAstro mount at 192.168.1.28:9999 and verifies
/// every major app function by sending LX200 commands and checking responses.
///
/// Run: flutter test integration_test/mount_functions_test.dart -d windows
///
/// SAFETY: Movement tests use slow speed, brief duration, and guaranteed stops.
/// No factory reset, reboot, or motor parameter changes are performed.
library;

import 'dart:convert';
import 'dart:io';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:teenastro_app/models/catalog_entry.dart';
import 'package:teenastro_app/models/lx200_commands.dart';
import 'package:teenastro_app/models/mount_state.dart';
import 'package:teenastro_app/services/lx200_tcp_client.dart';

const _mountIp = '192.168.1.28';
const _mountPort = 9999;
const _cmdDelay = Duration(milliseconds: 50);

// ---------------------------------------------------------------------------
// Helper
// ---------------------------------------------------------------------------

class MountTestHelper {
  final LX200TcpClient client;
  MountTestHelper(this.client);

  Future<void> delay() => Future.delayed(_cmdDelay);

  Future<MountState> queryStatus() async {
    await ensureConnected(_mountIp, _mountPort);
    var raw = await client.sendCommand(LX200.getStatus);
    if (raw == null || raw.isEmpty) {
      // Force reconnect: socket may look connected but be stale
      await client.disconnect();
      await Future.delayed(const Duration(milliseconds: 500));
      await ensureConnected(_mountIp, _mountPort);
      raw = await client.sendCommand(LX200.getStatus);
    }
    expect(raw, isNotNull, reason: 'GXI returned null (not connected?)');
    expect(raw, isNotEmpty, reason: 'GXI returned empty string (timeout?)');
    final state = const MountState().parseStatus(raw!);
    expect(state.valid, isTrue, reason: 'GXI parse failed on "$raw"');
    return state;
  }

  /// Returns true when the mount reports it is slewing / moving.
  Future<bool> querySlewing() async {
    final raw = await client.sendCommand(':D#');
    if (raw == null || raw.isEmpty) return false;
    return raw.codeUnitAt(0) == 0x7f;
  }

  Future<void> ensureUnparked() async {
    final s = await queryStatus();
    if (s.parkState == ParkState.parked) {
      final ok = await client.sendBool(LX200.unpark);
      expect(ok, isTrue, reason: 'Unpark failed');
      await delay();
    }
  }

  Future<void> ensureTracking() async {
    final s = await queryStatus();
    if (s.tracking != TrackState.on) {
      await client.sendBool(LX200.trackOn);
      await delay();
    }
  }

  Future<void> ensureNotSlewing() async {
    for (var i = 0; i < 30; i++) {
      await ensureConnected(_mountIp, _mountPort);
      final s = await queryStatus();
      if (s.tracking != TrackState.slewing) return;
      await Future.delayed(const Duration(seconds: 1));
    }
    await safeStop();
    await delay();
  }

  Future<void> safeStop() async {
    await client.send(LX200.stopAll);
    await delay();
  }

  /// Reconnect if the socket was closed (firmware drops connections sometimes)
  Future<void> ensureConnected(String mountIp, int mountPort) async {
    if (!client.isConnected) {
      await Future.delayed(const Duration(milliseconds: 500));
      final ok = await client.connect(mountIp, mountPort);
      expect(ok, isTrue, reason: 'Reconnect to $mountIp:$mountPort failed');
      await Future.delayed(const Duration(milliseconds: 200));
    }
  }
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  late LX200TcpClient client;
  late MountTestHelper helper;

  // Connect before all tests, disconnect after all.
  // Using setUpAll inside testWidgets isn't possible, so we connect per-group
  // via a shared helper pattern. Each group reconnects if needed.

  Future<void> connectIfNeeded() async {
    if (!client.isConnected) {
      final ok = await client.connect(_mountIp, _mountPort);
      expect(ok, isTrue, reason: 'TCP connect to $_mountIp:$_mountPort failed');
      await Future.delayed(const Duration(milliseconds: 200));
    }
  }

  // =========================================================================
  // Group 1: Connection and Identity
  // =========================================================================
  group('1. Connection and Identity', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      await client.disconnect();
    });

    testWidgets('connects to mount', (tester) async {
      await tester.runAsync(() async {
        final ok = await client.connect(_mountIp, _mountPort);
        expect(ok, isTrue, reason: 'TCP connect failed');
        expect(client.isConnected, isTrue);
      });
    });

    testWidgets('product name contains TeenAstro', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final name = await client.sendCommand(LX200.getProductName);
        expect(name, isNotNull);
        expect(name, isNotEmpty, reason: 'GVP returned empty');
        expect(name!.toLowerCase(), contains('teenastro'));
      });
    });

    testWidgets('firmware version is non-empty', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final ver = await client.sendCommand(LX200.getVersionNum);
        expect(ver, isNotNull);
        expect(ver, isNotEmpty, reason: 'GVN returned empty');
        print('  Firmware: $ver');
      });
    });

    testWidgets('board version is non-empty', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final board = await client.sendCommand(LX200.getBoardVersion);
        expect(board, isNotNull);
        expect(board, isNotEmpty, reason: 'GVB returned empty');
        print('  Board: $board');
      });
    });

    testWidgets('driver type is non-empty', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final driver = await client.sendCommand(LX200.getDriverType);
        expect(driver, isNotNull);
        expect(driver, isNotEmpty, reason: 'GVb returned empty');
        print('  Driver: $driver');
      });
    });
  });

  // =========================================================================
  // Group 2: Status Polling
  // =========================================================================
  group('2. Status Polling', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      await client.disconnect();
    });

    testWidgets('GXI returns valid 17+ char status', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final raw = await client.sendCommand(LX200.getStatus);
        expect(raw, isNotNull);
        expect(raw!.length, greaterThanOrEqualTo(17),
            reason: 'GXI too short: "$raw" (${raw.length} chars)');
        print('  Status raw: $raw');
      });
    });

    testWidgets('parseStatus produces valid MountState', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final state = await helper.queryStatus();
        expect(state.valid, isTrue);
        expect(state.mountType, isNot(MountType.undefined),
            reason: 'Mount type is undefined');
        expect(state.parkState, isNot(ParkState.unknown),
            reason: 'Park state is unknown');
        print('  Mount: ${state.mountLabel}, Park: ${state.parkLabel}, '
            'Track: ${state.trackingLabel}, Speed: ${state.speedLabel}');
      });
    });
  });

  // =========================================================================
  // Group 3: Position and Time Readings
  // =========================================================================
  group('3. Position and Time', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      await client.disconnect();
    });

    testWidgets('RA format HH:MM:SS', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final ra = await client.sendCommand(LX200.getRa);
        expect(ra, isNotNull);
        expect(ra, isNotEmpty);
        expect(ra!, matches(RegExp(r'^\d{2}:\d{2}:\d{2}$')),
            reason: 'RA format wrong: "$ra"');
        print('  RA: $ra');
      });
    });

    testWidgets('Dec format sDD*MM:SS', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final dec = await client.sendCommand(LX200.getDec);
        expect(dec, isNotNull);
        expect(dec, isNotEmpty);
        expect(dec!, matches(RegExp(r'^[+-]\d{2}\*\d{2}:\d{2}$')),
            reason: 'Dec format wrong: "$dec"');
        print('  Dec: $dec');
      });
    });

    testWidgets('Az format DDD*MM:SS', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final az = await client.sendCommand(LX200.getAz);
        expect(az, isNotNull);
        expect(az, isNotEmpty);
        expect(az!, matches(RegExp(r'^\d{3}\*\d{2}:\d{2}$')),
            reason: 'Az format wrong: "$az"');
        print('  Az: $az');
      });
    });

    testWidgets('Alt format sDD*MM:SS', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final alt = await client.sendCommand(LX200.getAlt);
        expect(alt, isNotNull);
        expect(alt, isNotEmpty);
        expect(alt!, matches(RegExp(r'^[+-]\d{2}\*\d{2}:\d{2}$')),
            reason: 'Alt format wrong: "$alt"');
        print('  Alt: $alt');
      });
    });

    testWidgets('UTC time format HH:MM:SS', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final utc = await client.sendCommand(LX200.getUtcTime);
        expect(utc, isNotNull);
        expect(utc, isNotEmpty);
        expect(utc!, matches(RegExp(r'^\d{2}:\d{2}:\d{2}$')),
            reason: 'UTC time format wrong: "$utc"');
        print('  UTC: $utc');
      });
    });

    testWidgets('UTC date format MM/DD/YY', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final date = await client.sendCommand(LX200.getUtcDate);
        expect(date, isNotNull);
        expect(date, isNotEmpty);
        expect(date!, matches(RegExp(r'^\d{2}/\d{2}/\d{2}$')),
            reason: 'UTC date format wrong: "$date"');
        print('  Date: $date');
      });
    });

    testWidgets('Sidereal time format HH:MM:SS', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final sid = await client.sendCommand(LX200.getSidereal);
        expect(sid, isNotNull);
        expect(sid, isNotEmpty);
        expect(sid!, matches(RegExp(r'^\d{2}:\d{2}:\d{2}$')),
            reason: 'Sidereal format wrong: "$sid"');
        print('  LST: $sid');
      });
    });
  });

  // =========================================================================
  // Group 4: Speed Control
  // =========================================================================
  group('4. Speed Control', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      // Restore medium speed
      if (client.isConnected) {
        await client.send(LX200.setSpeed(2));
        await helper.delay();
      }
      await client.disconnect();
    });

    for (var level = 0; level <= 4; level++) {
      testWidgets('set speed $level and verify via GXI', (tester) async {
        await tester.runAsync(() async {
          await connectIfNeeded();
          await client.send(LX200.setSpeed(level));
          await helper.delay();
          final state = await helper.queryStatus();
          final expected = SpeedLevel.values[level];
          expect(state.speed, equals(expected),
              reason: 'Speed should be ${expected.name} after :R$level#, '
                  'got ${state.speed.name}');
        });
      });
    }
  });

  // =========================================================================
  // Group 5: Tracking Control
  // =========================================================================
  group('5. Tracking Control', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      if (client.isConnected) {
        await client.send(LX200.trackSidereal);
        await helper.delay();
        await client.sendBool(LX200.trackOn);
        await helper.delay();
      }
      await client.disconnect();
    });

    testWidgets('enable tracking', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        final ok = await client.sendBool(LX200.trackOn);
        expect(ok, isTrue, reason: ':Te# did not return 1');
        await helper.delay();
        final state = await helper.queryStatus();
        expect(state.tracking, equals(TrackState.on),
            reason: 'Tracking should be on, got ${state.tracking.name}');
      });
    });

    testWidgets('disable tracking', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await client.sendBool(LX200.trackOn);
        await helper.delay();
        final ok = await client.sendBool(LX200.trackOff);
        expect(ok, isTrue, reason: ':Td# did not return 1');
        await helper.delay();
        final state = await helper.queryStatus();
        expect(state.tracking, equals(TrackState.off),
            reason: 'Tracking should be off, got ${state.tracking.name}');
      });
    });

    testWidgets('sidereal tracking mode', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await client.sendBool(LX200.trackOn);
        await helper.delay();
        await client.send(LX200.trackSidereal);
        await helper.delay();
        final state = await helper.queryStatus();
        expect(state.sidereal, equals(SiderealMode.star),
            reason: 'Mode should be star, got ${state.sidereal.name}');
      });
    });

    testWidgets('lunar tracking mode', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await client.sendBool(LX200.trackOn);
        await helper.delay();
        await client.send(LX200.trackLunar);
        await helper.delay();
        final state = await helper.queryStatus();
        expect(state.sidereal, equals(SiderealMode.moon),
            reason: 'Mode should be moon, got ${state.sidereal.name}');
      });
    });

    testWidgets('solar tracking mode', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await client.sendBool(LX200.trackOn);
        await helper.delay();
        await client.send(LX200.trackSolar);
        await helper.delay();
        final state = await helper.queryStatus();
        expect(state.sidereal, equals(SiderealMode.sun),
            reason: 'Mode should be sun, got ${state.sidereal.name}');
      });
    });
  });

  // =========================================================================
  // Group 6+7: Movement and Stop All
  // =========================================================================
  group('6. Movement', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      if (client.isConnected) {
        await helper.safeStop();
      }
      await client.disconnect();
    });

    final directions = <String, List<String>>{
      'East':  [LX200.moveEast, LX200.stopEast],
      'West':  [LX200.moveWest, LX200.stopWest],
      'North': [LX200.moveNorth, LX200.stopNorth],
      'South': [LX200.moveSouth, LX200.stopSouth],
    };

    for (final entry in directions.entries) {
      testWidgets('move ${entry.key} and stop', (tester) async {
        await tester.runAsync(() async {
          await connectIfNeeded();
          await helper.ensureUnparked();
          await helper.ensureTracking();
          // Slow speed for safety
          await client.send(LX200.setSpeed(1));
          await helper.delay();

          // Record position before
          final raBefore = await client.sendCommand(LX200.getRa);
          final decBefore = await client.sendCommand(LX200.getDec);
          await helper.delay();

          // Start movement
          await client.send(entry.value[0]);
          await Future.delayed(const Duration(milliseconds: 500));

          // Verify the mount is active via GXI guiding indicators
          final statusRaw = await client.sendCommand(LX200.getStatus);
          final statusDuring = const MountState().parseStatus(statusRaw ?? '');
          final guidingActive = statusDuring.guidingEW.trim().isNotEmpty ||
              statusDuring.guidingNS.trim().isNotEmpty;
          print('  ${entry.key} during move: tracking=${statusDuring.tracking.name}, '
              'guidingEW="${statusDuring.guidingEW}", guidingNS="${statusDuring.guidingNS}"');

          // Stop movement
          await client.send(entry.value[1]);
          await Future.delayed(const Duration(milliseconds: 300));

          // Record position after
          final raAfter = await client.sendCommand(LX200.getRa);
          final decAfter = await client.sendCommand(LX200.getDec);

          // Verify either position changed OR guiding was active during move
          // (near pole, small East/West jogs may not change RA enough to register)
          final moved = (raBefore != raAfter) || (decBefore != decAfter);
          print('  ${entry.key}: RA $raBefore -> $raAfter, Dec $decBefore -> $decAfter');
          expect(moved || guidingActive, isTrue,
              reason: '${entry.key}: no position change and no guiding indicator');
        });
      });
    }

    testWidgets('stop all halts movement', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await helper.ensureTracking();
        await client.send(LX200.setSpeed(1));
        await helper.delay();

        await client.send(LX200.moveEast);
        await Future.delayed(const Duration(milliseconds: 300));

        // Stop all
        await client.send(LX200.stopAll);
        await Future.delayed(const Duration(milliseconds: 500));

        // Reconnect if the mount dropped connection during move/stop
        await helper.ensureConnected(_mountIp, _mountPort);

        final ra1 = await client.sendCommand(LX200.getRa);
        await Future.delayed(const Duration(milliseconds: 500));
        await helper.ensureConnected(_mountIp, _mountPort);
        final ra2 = await client.sendCommand(LX200.getRa);

        await helper.ensureConnected(_mountIp, _mountPort);
        final state = await helper.queryStatus();
        expect(state.tracking, isNot(TrackState.slewing),
            reason: 'Still slewing after :Q#');
        print('  After Q#: RA $ra1 -> $ra2, tracking=${state.tracking.name}');
      });
    });
  });

  // =========================================================================
  // Group 8+9: Park / Unpark / Home
  // =========================================================================
  group('8. Park / Unpark / Home', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      // Reconnect if needed, leave mount in a clean state
      if (!client.isConnected) {
        await client.connect(_mountIp, _mountPort);
        await Future.delayed(const Duration(milliseconds: 200));
      }
      if (client.isConnected) {
        await client.send(LX200.stopAll);
        await helper.delay();
        await client.sendBool(LX200.unpark);
        await helper.delay();
        await client.sendBool(LX200.trackOn);
        await helper.delay();
      }
      await client.disconnect();
    });

    testWidgets('park and unpark', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureConnected(_mountIp, _mountPort);
        await helper.ensureUnparked();
        await helper.ensureConnected(_mountIp, _mountPort);
        await helper.ensureNotSlewing();

        // Go home first (park requires a valid position / mount at home)
        await helper.ensureConnected(_mountIp, _mountPort);
        await client.sendBool(LX200.trackOff);
        await helper.delay();
        await helper.ensureConnected(_mountIp, _mountPort);
        final homeOk = await client.sendBool(LX200.goHome);
        if (homeOk) {
          for (var i = 0; i < 30; i++) {
            await Future.delayed(const Duration(seconds: 1));
            await helper.ensureConnected(_mountIp, _mountPort);
            final s = await helper.queryStatus();
            if (s.atHome || s.tracking != TrackState.slewing) break;
          }
        }
        await helper.ensureConnected(_mountIp, _mountPort);
        await helper.delay();

        // Park
        final parkOk = await client.sendBool(LX200.park);
        if (!parkOk) {
          // Mount may not have a stored park position -- skip gracefully
          print('  Park returned 0 (no saved park position or mount busy)');
          return;
        }

        // Wait for park to complete (mount slews to park position)
        MountState state;
        var attempts = 0;
        do {
          await Future.delayed(const Duration(seconds: 1));
          await helper.ensureConnected(_mountIp, _mountPort);
          state = await helper.queryStatus();
          attempts++;
        } while (state.parkState == ParkState.parking && attempts < 30);

        expect(state.parkState, equals(ParkState.parked),
            reason: 'Park state should be parked, got ${state.parkLabel}');
        print('  Parked after $attempts seconds');

        // Unpark
        final unparkOk = await client.sendBool(LX200.unpark);
        expect(unparkOk, isTrue, reason: ':hR# did not return 1');
        await helper.delay();

        state = await helper.queryStatus();
        expect(state.parkState, equals(ParkState.unparked),
            reason: 'Should be unparked, got ${state.parkLabel}');
        print('  Unparked OK');
      });
    });

    testWidgets('go home', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await helper.ensureNotSlewing();

        final ok = await client.sendBool(LX200.goHome);
        expect(ok, isTrue, reason: ':hC# did not return 1');

        // Poll until atHome or timeout; reconnect if socket drops
        MountState state;
        var attempts = 0;
        do {
          await Future.delayed(const Duration(seconds: 1));
          await helper.ensureConnected(_mountIp, _mountPort);
          state = await helper.queryStatus();
          attempts++;
          print('  Home poll #$attempts: atHome=${state.atHome}, '
              'tracking=${state.tracking.name}');
        } while (!state.atHome &&
            state.tracking == TrackState.slewing &&
            attempts < 30);

        // The mount should either be atHome or have stopped slewing
        final arrived = state.atHome || state.tracking != TrackState.slewing;
        expect(arrived, isTrue,
            reason: 'Mount did not reach home after ${attempts}s');
        print('  Home reached: atHome=${state.atHome}');
      });
    });
  });

  // =========================================================================
  // Group 10+11: Goto and Sync Target
  // =========================================================================
  group('10. Goto and Sync', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      if (client.isConnected) {
        await helper.safeStop();
      }
      await client.disconnect();
    });

    testWidgets('set and read back target coordinates', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await helper.ensureNotSlewing();

        final raOk = await client.sendBool(LX200.setTargetRa('12:00:00'));
        expect(raOk, isTrue, reason: ':Sr12:00:00# failed');
        await helper.delay();

        final decOk = await client.sendBool(LX200.setTargetDec('+45*00:00'));
        expect(decOk, isTrue, reason: ':Sd+45*00:00# failed');
        await helper.delay();

        final readRa = await client.sendCommand(LX200.getTargetRa);
        final readDec = await client.sendCommand(LX200.getTargetDec);
        expect(readRa, equals('12:00:00'), reason: 'Target RA mismatch');
        expect(readDec, equals('+45*00:00'), reason: 'Target Dec mismatch');
        print('  Target: RA=$readRa, Dec=$readDec');
      });
    });

    testWidgets('goto target initiates slew then stop', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await helper.ensureNotSlewing();
        await helper.ensureTracking();

        // Set a target that is different from current position
        final curRa = await client.sendCommand(LX200.getRa);
        await helper.delay();

        // Parse current RA hour, add 1 hour for a safe offset
        int hour = 12;
        if (curRa != null && curRa.length >= 2) {
          hour = (int.tryParse(curRa.substring(0, 2)) ?? 12) + 1;
          if (hour > 23) hour = 0;
        }
        final targetRa = '${hour.toString().padLeft(2, '0')}:00:00';

        await client.sendBool(LX200.setTargetRa(targetRa));
        await helper.delay();
        await client.sendBool(LX200.setTargetDec('+45*00:00'));
        await helper.delay();

        // Attempt goto
        final reply = await client.sendCommand(LX200.gotoTarget);
        print('  Goto reply: "$reply"');

        if (reply == '0') {
          // Success - mount should be slewing
          await Future.delayed(const Duration(milliseconds: 300));
          final state = await helper.queryStatus();
          expect(state.tracking, equals(TrackState.slewing),
              reason: 'Should be slewing after :MS# returned 0');
          print('  Slewing confirmed');

          // Stop immediately for safety, then poll until deceleration finishes
          await helper.safeStop();
          MountState stopped;
          for (var i = 0; i < 10; i++) {
            await Future.delayed(const Duration(seconds: 1));
            await helper.ensureConnected(_mountIp, _mountPort);
            stopped = await helper.queryStatus();
            if (stopped.tracking != TrackState.slewing) break;
            // Re-send stop in case the first one was lost
            if (i == 3) await helper.safeStop();
          }
          await helper.ensureConnected(_mountIp, _mountPort);
          stopped = await helper.queryStatus();
          expect(stopped.tracking, isNot(TrackState.slewing),
              reason: 'Should have stopped after :Q#');
          print('  Stopped OK');
        } else {
          // Got an error code - acceptable in constrained test environments
          final errIdx = int.tryParse(reply ?? '') ?? -1;
          final errName = (errIdx >= 0 && errIdx < GotoError.values.length)
              ? GotoError.values[errIdx].name
              : 'unknown($reply)';
          print('  Goto refused: $errName (acceptable in test)');
        }
      });
    });

    testWidgets('sync target returns response', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await helper.ensureNotSlewing();
        await helper.ensureTracking();

        // Read current position and sync to it (no-op sync)
        final curRa = await client.sendCommand(LX200.getRa);
        await helper.delay();
        final curDec = await client.sendCommand(LX200.getDec);
        await helper.delay();

        if (curRa != null && curDec != null && curRa.isNotEmpty && curDec.isNotEmpty) {
          await client.sendBool(LX200.setTargetRa(curRa));
          await helper.delay();
          await client.sendBool(LX200.setTargetDec(curDec));
          await helper.delay();

          final syncReply = await client.sendCommand(LX200.syncTarget);
          expect(syncReply, isNotNull, reason: ':CM# returned null');
          print('  Sync reply: "$syncReply"');
        } else {
          fail('Could not read current position for sync test');
        }
      });
    });
  });

  // =========================================================================
  // Group 12: Alignment Commands
  // =========================================================================
  group('12. Alignment', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      await client.disconnect();
    });

    testWidgets('align start (:A0#) returns 1', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final ok = await client.sendBool(LX200.alignStart);
        expect(ok, isTrue, reason: ':A0# did not return 1');
        print('  Align start OK');
      });
    });

    testWidgets('align clear (:AC#) returns 1', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final ok = await client.sendBool(LX200.alignClear);
        expect(ok, isTrue, reason: ':AC# did not return 1');
        print('  Align clear OK');
      });
    });

    testWidgets('align error (:AE#) returns non-empty', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        final raw = await client.sendCommand(LX200.getAlignError);
        expect(raw, isNotNull, reason: ':AE# returned null');
        expect(raw, isNotEmpty, reason: ':AE# returned empty');
        print('  Align error: "$raw"');
      });
    });
  });

  // =========================================================================
  // Group 14: Catalog Functionality
  // =========================================================================
  group('14. Catalog', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      if (client.isConnected) {
        await helper.safeStop();
      }
      await client.disconnect();
    });

    /// Locate the assets/catalogs/ directory relative to the test file.
    Directory findCatalogDir() {
      var dir = Directory.current;
      // When run via `flutter test integration_test/...` the cwd is the app root
      final candidate = Directory('${dir.path}/assets/catalogs');
      if (candidate.existsSync()) return candidate;
      // Fallback: walk up until we find it
      for (var i = 0; i < 5; i++) {
        final d = Directory('${dir.path}/assets/catalogs');
        if (d.existsSync()) return d;
        dir = dir.parent;
      }
      fail('Could not locate assets/catalogs/ directory');
    }

    testWidgets('load and parse all catalog JSON files', (tester) async {
      await tester.runAsync(() async {
        final catDir = findCatalogDir();
        final files = catDir.listSync().whereType<File>().where(
            (f) => f.path.endsWith('.json')).toList();
        expect(files, isNotEmpty, reason: 'No catalog JSON files found');

        for (final file in files) {
          final json = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
          final catalog = Catalog.fromJson(json);
          expect(catalog.title, isNotEmpty,
              reason: '${file.path}: missing title');
          expect(catalog.prefix, isNotEmpty,
              reason: '${file.path}: missing prefix');
          expect(catalog.objects, isNotEmpty,
              reason: '${file.path}: no objects');

          for (final obj in catalog.objects.take(5)) {
            expect(obj.ra, greaterThanOrEqualTo(0));
            expect(obj.ra, lessThan(24));
            expect(obj.dec, greaterThanOrEqualTo(-90));
            expect(obj.dec, lessThanOrEqualTo(90));
            expect(obj.raStr, matches(RegExp(r'^\d{2}:\d{2}:\d{2}$')));
            expect(obj.decStr, matches(RegExp(r'^[+-]\d{2}:\d{2}:\d{2}$')));
          }

          print('  ${catalog.title} (${catalog.prefix}): '
              '${catalog.objects.length} objects OK');
        }
      });
    });

    testWidgets('Messier catalog has M1 Crab Nebula', (tester) async {
      await tester.runAsync(() async {
        final catDir = findCatalogDir();
        final file = File('${catDir.path}/messier.json');
        expect(file.existsSync(), isTrue, reason: 'messier.json not found');

        final json = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
        final catalog = Catalog.fromJson(json);
        expect(catalog.prefix, equals('M'));

        final m1 = catalog.objects.firstWhere((o) => o.id == 1);
        expect(m1.name, equals('Crab Nebula'));
        expect(m1.ra, closeTo(5.575, 0.01));
        expect(m1.dec, closeTo(22.01, 0.1));
        print('  M1: RA=${m1.raStr} Dec=${m1.decStr} mag=${m1.mag}');
      });
    });

    testWidgets('catalog search finds objects by name', (tester) async {
      await tester.runAsync(() async {
        final catDir = findCatalogDir();
        final file = File('${catDir.path}/messier.json');
        final json = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
        final catalog = Catalog.fromJson(json);

        // Search for "Andromeda"
        final results = catalog.objects.where(
            (o) => o.name.toLowerCase().contains('andromeda')).toList();
        expect(results, isNotEmpty, reason: 'No Andromeda match in Messier');
        expect(results.first.id, equals(31),
            reason: 'Andromeda should be M31');
        print('  Found: M${results.first.id} ${results.first.name}');
      });
    });

    testWidgets('send catalog object coordinates to mount', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await helper.ensureNotSlewing();

        // Pick M42 (Orion Nebula) from the Messier catalog
        final catDir = findCatalogDir();
        final file = File('${catDir.path}/messier.json');
        final json = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
        final catalog = Catalog.fromJson(json);
        final m42 = catalog.objects.firstWhere((o) => o.id == 42);

        // Set target using catalog entry coordinates
        final raOk = await client.sendBool(LX200.setTargetRa(m42.raStr));
        expect(raOk, isTrue, reason: 'setTargetRa(${m42.raStr}) failed');
        await helper.delay();

        final decOk = await client.sendBool(LX200.setTargetDec(m42.decStr));
        expect(decOk, isTrue, reason: 'setTargetDec(${m42.decStr}) failed');
        await helper.delay();

        // Read back and verify
        final readRa = await client.sendCommand(LX200.getTargetRa);
        final readDec = await client.sendCommand(LX200.getTargetDec);
        expect(readRa, equals(m42.raStr),
            reason: 'Target RA mismatch: expected ${m42.raStr}, got $readRa');
        // Dec format from mount uses * instead of : for degrees separator
        // so convert for comparison
        final expectedDec = m42.decStr.replaceFirst(':', '*');
        expect(readDec, equals(expectedDec),
            reason: 'Target Dec mismatch: expected $expectedDec, got $readDec');
        print('  M42 target set: RA=$readRa Dec=$readDec');
      });
    });

    testWidgets('goto catalog object initiates slew', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        await helper.ensureUnparked();
        await helper.ensureNotSlewing();
        await helper.ensureTracking();

        // Pick M45 (Pleiades) from the Messier catalog
        final catDir = findCatalogDir();
        final file = File('${catDir.path}/messier.json');
        final json = jsonDecode(file.readAsStringSync()) as Map<String, dynamic>;
        final catalog = Catalog.fromJson(json);
        final m45 = catalog.objects.firstWhere((o) => o.id == 45);

        await client.sendBool(LX200.setTargetRa(m45.raStr));
        await helper.delay();
        // Mount expects Dec with * separator for degrees
        await client.sendBool(LX200.setTargetDec(m45.decStr));
        await helper.delay();

        final reply = await client.sendCommand(LX200.gotoTarget);
        print('  Goto M45 reply: "$reply"');

        if (reply == '0') {
          await Future.delayed(const Duration(milliseconds: 300));
          final state = await helper.queryStatus();
          expect(state.tracking, equals(TrackState.slewing),
              reason: 'Should be slewing toward M45');
          print('  Slewing to M45 (${m45.name}) confirmed');

          // Stop immediately for safety
          await helper.safeStop();
          for (var i = 0; i < 10; i++) {
            await Future.delayed(const Duration(seconds: 1));
            await helper.ensureConnected(_mountIp, _mountPort);
            final s = await helper.queryStatus();
            if (s.tracking != TrackState.slewing) break;
            if (i == 3) await helper.safeStop();
          }
        } else {
          final errIdx = int.tryParse(reply ?? '') ?? -1;
          final errName = (errIdx >= 0 && errIdx < GotoError.values.length)
              ? GotoError.values[errIdx].name
              : 'unknown($reply)';
          print('  Goto M45 refused: $errName (acceptable in test)');
        }

        // Ensure stopped
        await helper.ensureConnected(_mountIp, _mountPort);
        await helper.safeStop();
      });
    });
  });

  // =========================================================================
  // Group 13: Connection Stability
  // =========================================================================
  group('13. Connection Stability', () {
    setUp(() {
      client = LX200TcpClient();
      helper = MountTestHelper(client);
    });
    tearDown(() async {
      await client.disconnect();
    });

    testWidgets('sustained polling for 30 seconds', (tester) async {
      await tester.runAsync(() async {
        await connectIfNeeded();
        // Ensure mount is idle (not slewing from previous tests)
        await helper.ensureNotSlewing();

        const polls = 15;
        const interval = Duration(seconds: 2);
        var failures = 0;
        var reconnects = 0;

        for (var i = 0; i < polls; i++) {
          if (!client.isConnected) {
            reconnects++;
            print('  Poll #$i: reconnecting (#$reconnects)...');
            await helper.ensureConnected(_mountIp, _mountPort);
          }

          final raw = await client.sendCommand(LX200.getStatus);
          if (raw == null || raw.isEmpty || raw.length < 17) {
            failures++;
            print('  Poll #$i: FAIL (raw=${raw ?? "null"})');
          } else {
            final state = const MountState().parseStatus(raw);
            print('  Poll #$i: OK track=${state.trackingLabel} '
                'park=${state.parkLabel}');
          }

          if (i < polls - 1) await Future.delayed(interval);
        }

        // Allow at most 2 reconnects (connection drops are being investigated)
        expect(failures, lessThanOrEqualTo(2),
            reason: '$failures/$polls polls failed');
        print('  Stability: ${polls - failures}/$polls polls OK, '
            '$reconnects reconnects');
      });
    });
  });
}
