import 'dart:async';

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../models/equinox_precession.dart';
import '../models/mount_state.dart';
import 'finder_lx200.dart';
import 'finder_lx200_server.dart';
import 'location_service.dart';
import 'mount_state_provider.dart';

const _keep = Object();

/// The phone used as a finder, with no TeenAstro. Coordinates are JNow.
class FinderSky {
  final bool active;
  final double? latDeg;
  final double? lonEastDeg;

  /// 'gps', 'ip' or 'saved'. Empty while the site is unknown.
  final String siteSource;
  final bool locating;
  final double? raH;
  final double? decD;
  final DateTime? solvedAt;
  final double? targetRa;
  final double? targetDec;
  final String targetName;
  final DateTime clock;

  /// `address:port` SkySafari connects to.
  final String? serverAddress;
  final String? serverError;

  const FinderSky({
    required this.clock,
    this.active = false,
    this.latDeg,
    this.lonEastDeg,
    this.siteSource = '',
    this.locating = false,
    this.raH,
    this.decD,
    this.solvedAt,
    this.targetRa,
    this.targetDec,
    this.targetName = '',
    this.serverAddress,
    this.serverError,
  });

  bool get hasSite => latDeg != null && lonEastDeg != null;
  bool get hasSolve => raH != null && decD != null;
  bool get hasTarget => targetRa != null && targetDec != null;

  FinderSky copyWith({
    bool? active,
    Object? latDeg = _keep,
    Object? lonEastDeg = _keep,
    String? siteSource,
    bool? locating,
    Object? raH = _keep,
    Object? decD = _keep,
    Object? solvedAt = _keep,
    Object? targetRa = _keep,
    Object? targetDec = _keep,
    String? targetName,
    DateTime? clock,
    Object? serverAddress = _keep,
    Object? serverError = _keep,
  }) =>
      FinderSky(
        active: active ?? this.active,
        latDeg: identical(latDeg, _keep) ? this.latDeg : latDeg as double?,
        lonEastDeg: identical(lonEastDeg, _keep) ? this.lonEastDeg : lonEastDeg as double?,
        siteSource: siteSource ?? this.siteSource,
        locating: locating ?? this.locating,
        raH: identical(raH, _keep) ? this.raH : raH as double?,
        decD: identical(decD, _keep) ? this.decD : decD as double?,
        solvedAt: identical(solvedAt, _keep) ? this.solvedAt : solvedAt as DateTime?,
        targetRa: identical(targetRa, _keep) ? this.targetRa : targetRa as double?,
        targetDec: identical(targetDec, _keep) ? this.targetDec : targetDec as double?,
        targetName: targetName ?? this.targetName,
        clock: clock ?? this.clock,
        serverAddress:
            identical(serverAddress, _keep) ? this.serverAddress : serverAddress as String?,
        serverError: identical(serverError, _keep) ? this.serverError : serverError as String?,
      );

  FinderSnapshot snapshot([DateTime? now]) => FinderSnapshot(
        local: now ?? DateTime.now(),
        latDeg: latDeg,
        lonEastDeg: lonEastDeg,
        raH: raH,
        decD: decD,
      );

  /// The sky in the shape the planetarium reads from a mount.
  /// Without a site, sidereal time is unknown.
  MountState toMountState() {
    final pos = hasSolve ? formatRaDecLx200(raH!, decD!) : null;
    final target = targetRa != null && targetDec != null
        ? formatRaDecLx200(targetRa!, targetDec!)
        : null;
    return MountState(
      ra: pos?.$1 ?? '?',
      dec: pos?.$2 ?? '?',
      targetRa: target?.$1 ?? '',
      targetDec: target?.$2 ?? '',
      siderealTime:
          hasSite ? formatRaDecLx200(snapshot(clock).lstH, 0).$1 : '?',
      latitude: latDeg ?? 0,
      longitude: lonEastDeg ?? 0,
    );
  }
}

class FinderSkyNotifier extends StateNotifier<FinderSky> {
  static const _latKey = 'finder_site_lat';
  static const _lonKey = 'finder_site_lon';

  Timer? _tick;
  FinderLx200Server? _server;

  FinderSkyNotifier() : super(FinderSky(clock: DateTime.now()));

  /// Start the finder session: clock, site and the SkySafari server.
  Future<void> start() async {
    if (state.active) return;
    state = FinderSky(active: true, clock: DateTime.now(), locating: true);
    _tick = Timer.periodic(const Duration(seconds: 2), (_) {
      if (mounted) state = state.copyWith(clock: DateTime.now());
    });
    await _loadSavedSite();
    await _startServer();
    unawaited(locate());
  }

  Future<void> stop() async {
    _tick?.cancel();
    _tick = null;
    await _server?.stop();
    _server = null;
    if (mounted) state = FinderSky(clock: DateTime.now());
  }

  /// Read the site from GPS (Android) or the network (desktop).
  Future<void> locate() async {
    if (!state.active) return;
    state = state.copyWith(locating: true);
    final loc = await LocationService.getCurrentLocation();
    if (!mounted || !state.active) return;
    if (loc == null) {
      state = state.copyWith(locating: false);
      return;
    }
    state = state.copyWith(
      latDeg: loc.latitude,
      lonEastDeg: loc.longitude,
      siteSource: loc.source,
      locating: false,
    );
    final prefs = await SharedPreferences.getInstance();
    await prefs.setDouble(_latKey, loc.latitude);
    await prefs.setDouble(_lonKey, loc.longitude);
  }

  /// Where the last plate solve pointed, in JNow.
  void publishSolve(double raH, double decD) {
    state = state.copyWith(raH: raH, decD: decD, solvedAt: DateTime.now());
  }

  /// The object to push the telescope to, in JNow.
  void setTarget(double raH, double decD, String name) {
    if (!state.active) return;
    state = state.copyWith(targetRa: raH, targetDec: decD, targetName: name);
  }

  void clearTarget() {
    state = state.copyWith(targetRa: null, targetDec: null, targetName: '');
  }

  Future<void> _loadSavedSite() async {
    final prefs = await SharedPreferences.getInstance();
    final lat = prefs.getDouble(_latKey);
    final lon = prefs.getDouble(_lonKey);
    if (lat == null || lon == null || !mounted) return;
    state = state.copyWith(latDeg: lat, lonEastDeg: lon, siteSource: 'saved');
  }

  Future<void> _startServer() async {
    final server = FinderLx200Server(
      snapshot: () => state.snapshot(),
      onTarget: (ra, dec) {
        if (mounted) setTarget(ra, dec, 'SkySafari target');
      },
    );
    try {
      await server.start();
      _server = server;
      final ip = await phoneLanAddress();
      if (!mounted) return;
      state = state.copyWith(
        serverAddress: ip == null ? null : '$ip:$finderLx200Port',
        serverError: ip == null ? 'The phone is not on a network' : null,
      );
    } catch (_) {
      if (mounted) {
        state = state.copyWith(serverError: 'Port $finderLx200Port is already in use');
      }
    }
  }

  @override
  void dispose() {
    _tick?.cancel();
    _server?.stop();
    super.dispose();
  }
}

final finderSkyProvider =
    StateNotifierProvider<FinderSkyNotifier, FinderSky>((ref) => FinderSkyNotifier());

/// The mount state, or the finder sky while the finder session is on.
final skyStateProvider = Provider<MountState>((ref) {
  final sky = ref.watch(finderSkyProvider);
  if (sky.active) return sky.toMountState();
  return ref.watch(mountStateProvider);
});
