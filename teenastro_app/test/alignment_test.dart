// Unit tests for 2-star alignment: main unit protocol, SHC state flow, and app commands.

import 'package:flutter_test/flutter_test.dart';
import 'package:teenastro_app/models/lx200_commands.dart';
import 'package:teenastro_app/screens/alignment_screen.dart';
import 'package:teenastro_app/models/catalog_entry.dart';

void main() {
  group('LX200 alignment commands (main unit protocol)', () {
    test('alignStart is :A0#', () {
      expect(LX200.alignStart, equals(':A0#'));
    });

    test('alignStartMechanicalPole is :A0,m#', () {
      expect(LX200.alignStartMechanicalPole, equals(':A0,m#'));
    });

    test('alignStartThreeStars aliases :A0,m#', () {
      expect(LX200.alignStartThreeStars, equals(':A0,m#'));
    });

    test('gotoPolarAlignCurrent is :MP#', () {
      expect(LX200.gotoPolarAlignCurrent, equals(':MP#'));
    });

    test('gotoPolarAlignMalign is :Malign#', () {
      expect(LX200.gotoPolarAlignMalign, equals(':Malign#'));
    });

    test('alignAddStar(1) is :A1# (first star, OnStepX-style)', () {
      expect(LX200.alignAddStar(1), equals(':A1#'));
    });

    test('alignAddStar(2) is :A2# (second star)', () {
      expect(LX200.alignAddStar(2), equals(':A2#'));
    });

    test('alignAddStar(3) is :AP# (polar / mechanical finalize)', () {
      expect(LX200.alignAddStar(3), equals(':AP#'));
    });

    test('alignStartRigid is :A0,r4# and :A0,r6#', () {
      expect(LX200.alignStartRigid(4), equals(':A0,r4#'));
      expect(LX200.alignStartRigid(6), equals(':A0,r6#'));
    });

    test('alignSelectStar sends :An# through star 6', () {
      expect(LX200.alignSelectStar(1), equals(':A1#'));
      expect(LX200.alignSelectStar(3), equals(':A3#'));
      expect(LX200.alignSelectStar(4), equals(':A4#'));
      expect(LX200.alignSelectStar(6), equals(':A6#'));
    });

    test('mount error commands use Wallace degrees on the wire as arcseconds', () {
      expect(LX200.getKnownGeomUse, equals(':GXKk#'));
      expect(LX200.setKnownGeomUse(true), equals(':SXKk,1#'));
      expect(LX200.setKnownGeomUse(false), equals(':SXKk,0#'));
      expect(LX200.getKnownCone, equals(':GXKc#'));
      expect(LX200.getKnownPerp, equals(':GXKp#'));
      expect(LX200.setKnownConeDeg(1.5), equals(':SXKc,5400.000#'));
      expect(LX200.setKnownPerpDeg(-0.25), equals(':SXKp,-900.000#'));
    });

    test('getAlignError is :AE#', () {
      expect(LX200.getAlignError, equals(':AE#'));
    });

    test('alignSave is :AW#, alignClear is :AC#', () {
      expect(LX200.alignSave, equals(':AW#'));
      expect(LX200.alignClear, equals(':AC#'));
    });

    test('goto uses :Sr# :Sd# :MS# pattern', () {
      expect(LX200.setTargetRa('12:30:00'), equals(':Sr12:30:00#'));
      expect(LX200.setTargetDec('+45*30:00'), equals(':Sd+45*30:00#'));
      expect(LX200.gotoTarget, equals(':MS#'));
    });
  });

  group('Alignment session kinds', () {
    test('2, 4 and 3+3 start on the rigid or two-star command', () {
      expect(alignStartCommand(AlignKind.two), equals(':A0#'));
      expect(alignStartCommand(AlignKind.four), equals(':A0,r4#'));
      expect(alignStartCommand(AlignKind.six), equals(':A0,r6#'));
      expect(alignStartCommand(AlignKind.mechanical), equals(':A0,m#'));
      expect(alignStarCount(AlignKind.four), 4);
      expect(alignStarCount(AlignKind.six), 6);
    });

    test('alt-az keeps 2 Stars and 4 Stars', () {
      expect(alignKindOnAltAz(AlignKind.two), isTrue);
      expect(alignKindOnAltAz(AlignKind.four), isTrue);
      expect(alignKindOnAltAz(AlignKind.six), isFalse);
      expect(alignKindOnAltAz(AlignKind.mechanical), isFalse);
    });

    test('step order is select, slew, recenter, then done', () {
      expect(AlignUi.idle.index, lessThan(AlignUi.select.index));
      expect(AlignUi.select.index, lessThan(AlignUi.slewing.index));
      expect(AlignUi.slewing.index, lessThan(AlignUi.recenter.index));
      expect(AlignUi.recenter.index, lessThan(AlignUi.done.index));
    });

    test('rigid head prints Wallace arcseconds and hides an unsolved CH', () {
      expect(
        formatRigidHead(mask: '-PI', ch: 0, np: 120, id: -30, sideIn: 4, sideOut: 0),
        equals('CH n/a 4+0\nNP +120"\nID -30"'),
      );
      expect(
        formatRigidHead(mask: 'CPI', ch: 15.4, np: -8, id: 3, sideIn: 3, sideOut: 3),
        equals('CH +15"\nNP -8"\nID +3"'),
      );
      expect(formatRigidHead(mask: '---'), isEmpty);
      expect(formatRigidRms(90), equals("1.5'"));
      expect(formatRigidRms(12), equals('12"'));
    });

    test('accept star 1–2 sends :An#; mechanical finalize stays :AP#', () {
      expect(LX200.alignAddStar(1), equals(':A1#'));
      expect(LX200.alignAddStar(2), equals(':A2#'));
      expect(LX200.alignAddStar(3), equals(':AP#'));
      expect(LX200.alignSelectStar(3), equals(':A3#'));
    });
  });

  group('CatalogEntry RA/Dec format for goto', () {
    test('decStr first colon replaced with * for LX200', () {
      // Dec format from catalog is +DD:MM:SS; LX200 expects +DD*MM:SS
      final entry = CatalogEntry(
        id: 1,
        name: 'Vega',
        ra: 18.61564,
        dec: 38.78369,
        mag: 0.03,
        constellation: 76,
      );
      final decStr = entry.decStr;
      expect(decStr, contains('+'));
      final lx200Dec = decStr.replaceFirst(':', '*');
      expect(lx200Dec, contains('*'));
      expect(entry.raStr, isNotEmpty);
    });
  });
}
