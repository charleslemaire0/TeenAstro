import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';

/// The phone alone as a finder: only the sky chart and the camera.
class FinderShell extends StatelessWidget {
  final String location;
  final Widget child;
  const FinderShell({super.key, required this.location, required this.child});

  static const sky = '/finder-mode/sky';
  static const camera = '/finder-mode/camera';

  @override
  Widget build(BuildContext context) {
    final index = location.startsWith(sky) ? 0 : 1;
    void go(int i) => context.go(i == 0 ? sky : camera);
    if (MediaQuery.orientationOf(context) == Orientation.landscape) {
      return Scaffold(
        body: SafeArea(
          child: Row(
            children: [
              NavigationRail(
                selectedIndex: index,
                onDestinationSelected: go,
                labelType: NavigationRailLabelType.all,
                minWidth: 56,
                destinations: const [
                  NavigationRailDestination(
                      icon: Icon(Icons.public), label: Text('Sky')),
                  NavigationRailDestination(
                      icon: Icon(Icons.photo_camera), label: Text('Camera')),
                ],
              ),
              const VerticalDivider(width: 1),
              Expanded(child: child),
            ],
          ),
        ),
      );
    }
    return Scaffold(
      body: SafeArea(child: child),
      bottomNavigationBar: NavigationBar(
        selectedIndex: index,
        onDestinationSelected: go,
        destinations: const [
          NavigationDestination(icon: Icon(Icons.public), label: 'Sky'),
          NavigationDestination(icon: Icon(Icons.photo_camera), label: 'Camera'),
        ],
      ),
    );
  }
}
