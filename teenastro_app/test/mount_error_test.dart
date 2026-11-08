// Mount errors: :GXAS# byte 4 and the one-character goto reply.

import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter_test/flutter_test.dart';
import 'package:teenastro_app/models/lx200_commands.dart';
import 'package:teenastro_app/models/mount_state.dart';

String _gxasWithError(int error) {
  final pkt = Uint8List(102);
  pkt[4] = error;
  var chk = 0;
  for (var i = 0; i < 101; i++) {
    chk ^= pkt[i];
  }
  pkt[101] = chk;
  return base64.encode(pkt);
}

void main() {
  group('Mount error from :GXAS#', () {
    test('byte 4 = 8 is the unsupported pier side', () {
      final s = const MountState().parseBinaryState(_gxasWithError(8));
      expect(s.valid, isTrue);
      expect(s.error, MountError.pierSide);
      expect(s.errorLabel, 'Pier side');
    });

    test('meridian stays 7', () {
      final s = const MountState().parseBinaryState(_gxasWithError(7));
      expect(s.error, MountError.meridian);
    });
  });

  group('Goto reply', () {
    test("'0' + 18 = 'B' is the unsupported pier side", () {
      expect(gotoErrorFromReply('B'), GotoError.pierSide);
      expect(gotoErrorCause(GotoError.pierSide),
          'Mount type does not support this pier side');
    });

    test('codes from 10 up decode from their character', () {
      expect(gotoErrorFromReply('A'), GotoError.meridian);
      expect(gotoErrorFromReply('@'), GotoError.underPole);
      expect(gotoErrorFromReply('6'), GotoError.limits);
    });

    test('anything else is not a code', () {
      expect(gotoErrorFromReply(null), isNull);
      expect(gotoErrorFromReply(''), isNull);
      expect(gotoErrorFromReply('Z'), isNull);
      expect(gotoErrorFromReply('12'), isNull);
    });
  });
}
