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
    return Scaffold(
      body: SafeArea(child: child),
      bottomNavigationBar: NavigationBar(
        selectedIndex: location.startsWith(sky) ? 0 : 1,
        onDestinationSelected: (i) => context.go(i == 0 ? sky : camera),
        destinations: const [
          NavigationDestination(icon: Icon(Icons.public), label: 'Sky'),
          NavigationDestination(icon: Icon(Icons.photo_camera), label: 'Camera'),
        ],
      ),
    );
  }
}
