// Basic Flutter widget smoke test for TeenAstro app.
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:teenastro_app/app.dart';

void main() {
  testWidgets('TeenAstroApp renders', (WidgetTester tester) async {
    await tester.pumpWidget(const ProviderScope(child: TeenAstroApp()));
    await tester.pumpAndSettle();
    expect(find.byType(TeenAstroApp), findsOneWidget);
  });
}
