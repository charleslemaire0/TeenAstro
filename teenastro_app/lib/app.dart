import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:go_router/go_router.dart';
import 'theme.dart';
import 'screens/connection_screen.dart';
import 'screens/dashboard_screen.dart';
import 'screens/control_screen.dart';
import 'screens/goto_screen.dart';
import 'screens/catalog_browser_screen.dart';
import 'screens/tracking_screen.dart';
import 'screens/alignment_screen.dart';
import 'screens/settings_screen.dart';

final _router = GoRouter(
  initialLocation: '/connect',
  routes: [
    GoRoute(path: '/connect', builder: (_, __) => const ConnectionScreen()),
    ShellRoute(
      builder: (context, state, child) => AppShell(location: state.uri.toString(), child: child),
      routes: [
        GoRoute(path: '/dashboard', builder: (_, __) => const DashboardScreen()),
        GoRoute(path: '/control', builder: (_, __) => const ControlScreen()),
        GoRoute(path: '/goto', builder: (_, __) => const GotoScreen()),
        GoRoute(path: '/catalogs', builder: (_, __) => const CatalogBrowserScreen()),
        GoRoute(path: '/tracking', builder: (_, __) => const TrackingScreen()),
        GoRoute(path: '/alignment', builder: (_, __) => const AlignmentScreen()),
        GoRoute(path: '/settings', builder: (_, __) => const SettingsScreen()),
      ],
    ),
  ],
);

class TeenAstroApp extends ConsumerWidget {
  const TeenAstroApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return MaterialApp.router(
      title: 'TeenAstro Controller',
      theme: teenAstroTheme,
      routerConfig: _router,
      debugShowCheckedModeBanner: false,
    );
  }
}

class AppShell extends StatelessWidget {
  final String location;
  final Widget child;
  const AppShell({super.key, required this.location, required this.child});

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: SafeArea(child: child),
      bottomNavigationBar: NavigationBar(
        selectedIndex: _currentIndex(location),
        onDestinationSelected: (i) => _navigate(context, i),
        destinations: const [
          NavigationDestination(icon: Icon(Icons.dashboard), label: 'Status'),
          NavigationDestination(icon: Icon(Icons.gamepad), label: 'Control'),
          NavigationDestination(icon: Icon(Icons.my_location), label: 'Goto'),
          NavigationDestination(icon: Icon(Icons.track_changes), label: 'Tracking'),
          NavigationDestination(icon: Icon(Icons.architecture), label: 'Align'),
          NavigationDestination(icon: Icon(Icons.settings), label: 'Settings'),
        ],
      ),
    );
  }

  int _currentIndex(String location) {
    if (location.startsWith('/control')) return 1;
    if (location.startsWith('/goto') || location.startsWith('/catalogs')) return 2;
    if (location.startsWith('/tracking')) return 3;
    if (location.startsWith('/alignment')) return 4;
    if (location.startsWith('/settings')) return 5;
    return 0;
  }

  void _navigate(BuildContext context, int index) {
    switch (index) {
      case 0: context.go('/dashboard');
      case 1: context.go('/control');
      case 2: context.go('/goto');
      case 3: context.go('/tracking');
      case 4: context.go('/alignment');
      case 5: context.go('/settings');
    }
  }
}
