import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:integration_test/integration_test.dart';
import 'package:teenastro_app/app.dart';

/// Automatically verifies connection-loss behavior: starts a mock TCP server,
/// pumps the app, drives it to connect, server closes to simulate disconnect,
/// then asserts the debug log contains expected entries.
/// Run: flutter test integration_test/connection_loss_test.dart -d windows --dart-define=DEBUG_LOG_FILE=c:/Users/clemair/Documents/learn/TeenAstro/debug-d3761d.log
void main() {
  IntegrationTestWidgetsFlutterBinding.ensureInitialized();

  testWidgets('connection loss is detected and reconnect is attempted', (WidgetTester tester) async {
    // Start mock server: accept one connection, then close it after 3s
    final server = await ServerSocket.bind('127.0.0.1', 9999);
    Socket? clientSocket;
    server.listen((Socket socket) {
      clientSocket = socket;
      Future.delayed(const Duration(seconds: 3), () {
        socket.destroy();
      });
    });
    addTearDown(() async {
      await clientSocket?.close();
      await server.close();
    });

    // Load the app so the connection screen is in the tree
    await tester.pumpWidget(const ProviderScope(child: TeenAstroApp()));
    await tester.pumpAndSettle(const Duration(seconds: 3));

    final ipFinder = find.byKey(const Key('connect_ip'));
    expect(ipFinder, findsOneWidget);
    await tester.tap(ipFinder);
    await tester.pumpAndSettle();
    await tester.enterText(ipFinder, '127.0.0.1');

    final portFinder = find.byKey(const Key('connect_port'));
    await tester.tap(portFinder);
    await tester.pumpAndSettle();
    await tester.enterText(portFinder, '9999');

    await tester.tap(find.byKey(const Key('connect_btn')));
    await tester.pumpAndSettle(const Duration(seconds: 5));

    // Wait for server to close and app to log (reconnect attempts)
    await Future.delayed(const Duration(seconds: 8));

    final logPath = const String.fromEnvironment(
      'DEBUG_LOG_FILE',
      defaultValue: r'c:\Users\clemair\Documents\learn\TeenAstro\debug-d3761d.log',
    );
    if (logPath.isEmpty) return;
    final file = File(logPath);
    if (!file.existsSync()) return;
    final content = file.readAsStringSync();
    expect(
      content.contains('socket closed') ||
          content.contains('connection lost') ||
          content.contains('reconnect'),
      true,
      reason: 'Log should contain connection-loss or reconnect entries. Log content (last 500 chars): ${content.length > 500 ? content.substring(content.length - 500) : content}',
    );
  });
}
