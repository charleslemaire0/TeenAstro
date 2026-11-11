// Renders phone-sized screens with real fonts and writes PNGs.
//   flutter run -d windows -t tool/wiki_shooter.dart
// The Windows window must be at least 480 by 1100 or the phone is clipped.
// Restore windows/runner/main.cpp to 1280 by 720 when the capture is done.
// The status shot includes the temporary debug panel; crop that card out
// before publishing.
import 'dart:io';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';
import 'package:flutter/rendering.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:teenastro_app/models/mount_state.dart';
import 'package:teenastro_app/screens/alignment_screen.dart';
import 'package:teenastro_app/screens/catalog_browser_screen.dart';
import 'package:teenastro_app/screens/connection_screen.dart';
import 'package:teenastro_app/screens/control_screen.dart';
import 'package:teenastro_app/screens/dashboard_screen.dart';
import 'package:teenastro_app/screens/goto_screen.dart';
import 'package:teenastro_app/screens/phone_finder_screen.dart';
import 'package:teenastro_app/screens/planetarium_screen.dart';
import 'package:teenastro_app/screens/settings_screen.dart';
import 'package:teenastro_app/screens/tracking_screen.dart';
import 'package:teenastro_app/services/lx200_tcp_client.dart';
import 'package:teenastro_app/services/mount_state_provider.dart';
import 'package:teenastro_app/theme.dart';

class _ShotMount extends MountStateNotifier {
  _ShotMount(LX200TcpClient client, MountState initial) : super(client) {
    state = initial;
  }

  @override
  void startPolling() {}
}

MountState _demo({bool aligned = true}) {
  return MountState(
    tracking: TrackState.on,
    sidereal: SiderealMode.star,
    parkState: ParkState.unparked,
    speed: SpeedLevel.medium,
    aligned: aligned,
    mountType: MountType.gem,
    pierSide: PierSide.east,
    valid: true,
    ra: '18:36:56',
    dec: '+38*47:01',
    ha: '-00:12:04',
    alt: '+62*10:22',
    az: '275*18:40',
    targetRa: '16:41:42',
    targetDec: '+36*27:37',
    utcTime: '21:14:08',
    utcDate: '11/11/26',
    localTime: '22:14:08',
    localDate: '11/11/26',
    siderealTime: '18:49:00',
    timezoneOffset: -1,
    productName: 'TeenAstro',
    versionNum: '1.6.9',
    boardVersion: '2.5',
    latitude: 48.8566,
    longitude: 2.3522,
    focuserConnected: true,
    focuserPos: 12500,
    trackRateRa: 15.041,
  );
}

const _shots = <(String, Widget, int, bool)>[
  ('app_connect', ConnectionScreen(), -1, true),
  ('app_status', DashboardScreen(), 0, true),
  ('app_control', ControlScreen(), 1, true),
  ('app_goto', GotoScreen(), 2, true),
  ('app_catalogs', CatalogBrowserScreen(), 2, true),
  ('app_planetarium', PlanetariumScreen(), 2, true),
  ('app_tracking', TrackingScreen(), 3, true),
  ('app_align', AlignmentScreen(), 4, false),
  ('app_finder', PhoneFinderScreen(), 4, true),
  ('app_settings', SettingsScreen(), 5, true),
];

void main() {
  TA.setNight(false);
  runApp(const _Shooter());
}

class _Shooter extends StatefulWidget {
  const _Shooter();

  @override
  State<_Shooter> createState() => _ShooterState();
}

class _ShooterState extends State<_Shooter> {
  final _key = GlobalKey();
  var _index = 0;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) => _capture());
  }

  Future<void> _capture() async {
    final out = Directory('../docs/wiki/screenshots');
    out.createSync(recursive: true);
    for (var i = 0; i < _shots.length; i++) {
      setState(() => _index = i);
      final wait = _shots[i].$1 == 'app_planetarium' || _shots[i].$1 == 'app_catalogs'
          ? const Duration(milliseconds: 900)
          : const Duration(milliseconds: 400);
      await Future<void>.delayed(wait);
      await WidgetsBinding.instance.endOfFrame;
      final boundary = _key.currentContext!.findRenderObject()! as RenderRepaintBoundary;
      final image = await boundary.toImage(pixelRatio: 2);
      final data = await image.toByteData(format: ui.ImageByteFormat.png);
      final file = File('${out.path}/${_shots[i].$1}.png');
      await file.writeAsBytes(data!.buffer.asUint8List());
      stderr.writeln('wrote ${file.path}');
    }
    exit(0);
  }

  @override
  Widget build(BuildContext context) {
    final shot = _shots[_index];
    return ProviderScope(
      key: ValueKey(shot.$1),
      overrides: [
        mountStateProvider.overrideWith(
          (ref) => _ShotMount(ref.watch(lx200ClientProvider), _demo(aligned: shot.$4)),
        ),
      ],
      child: MaterialApp(
        theme: teenAstroTheme,
        debugShowCheckedModeBanner: false,
        home: Align(
          alignment: Alignment.topCenter,
          child: RepaintBoundary(
            key: _key,
            child: SizedBox(
              width: 390,
              height: 844,
              child: Scaffold(
                body: SafeArea(child: shot.$2),
                bottomNavigationBar: shot.$3 < 0
                    ? null
                    : NavigationBar(
                        selectedIndex: shot.$3,
                        destinations: const [
                          NavigationDestination(icon: Icon(Icons.dashboard), label: 'Status'),
                          NavigationDestination(icon: Icon(Icons.gamepad), label: 'Control'),
                          NavigationDestination(icon: Icon(Icons.my_location), label: 'Goto'),
                          NavigationDestination(icon: Icon(Icons.track_changes), label: 'Tracking'),
                          NavigationDestination(icon: Icon(Icons.architecture), label: 'Align'),
                          NavigationDestination(icon: Icon(Icons.settings), label: 'Settings'),
                        ],
                      ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}
