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

  group('AlignStep enum (app 2-star flow)', () {
    test('step order matches 2-star alignment from home', () {
      expect(AlignStep.idle.index, lessThan(AlignStep.selectStar1.index));
      expect(AlignStep.selectStar1.index, lessThan(AlignStep.slewingStar1.index));
      expect(AlignStep.slewingStar1.index, lessThan(AlignStep.recenterStar1.index));
      expect(AlignStep.recenterStar1.index, lessThan(AlignStep.selectStar2.index));
      expect(AlignStep.selectStar2.index, lessThan(AlignStep.slewingStar2.index));
      expect(AlignStep.slewingStar2.index, lessThan(AlignStep.recenterStar2.index));
      expect(AlignStep.recenterStar2.index, lessThan(AlignStep.done.index));
    });

    test('accept star 1–2 sends :An#; star 3 finalize sends :AP#', () {
      expect(LX200.alignAddStar(1), equals(':A1#'));
      expect(LX200.alignAddStar(2), equals(':A2#'));
      expect(LX200.alignAddStar(3), equals(':AP#'));
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
