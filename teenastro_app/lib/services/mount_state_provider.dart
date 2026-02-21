import 'dart:async';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/mount_state.dart';
import '../models/lx200_commands.dart';
import 'lx200_tcp_client.dart';
import 'debug_agent_log.dart';

/// Notifier that polls the mount and updates state.
/// Paces commands to avoid overwhelming the embedded system.
class MountStateNotifier extends StateNotifier<MountState> {
  final LX200TcpClient _client;
  bool _polling = false;
  int _pollCount = 0;
  bool _versionFetched = false;
  bool _siteFetched = false;

  /// Delay between individual commands to give the firmware breathing room
  static const _cmdDelay = Duration(milliseconds: 50);
  /// Delay between poll cycles
  static const _pollInterval = Duration(seconds: 2);

  MountStateNotifier(this._client) : super(const MountState());

  /// Start polling the mount
  void startPolling() {
    if (_polling) return;
    _polling = true;
    _versionFetched = false;
    _pollCount = 0;
    // #region agent log
    DebugLog.add('startPolling() client.isConnected=${_client.isConnected}');
    // #endregion
    _pollLoop();
  }

  void stopPolling() {
    _polling = false;
    _pollCount = 0;
  }

  /// Non-overlapping poll loop with auto-reconnect
  Future<void> _pollLoop() async {
    while (_polling) {
      if (!_client.isConnected) {
        // #region agent log
        DebugLog.add('connection lost, attempting reconnect...');
        agentLog('mount_state_provider.dart:_pollLoop', 'connection lost, attempting reconnect', {'clientState': _client.state.name}, 'H5');
        // #endregion
        final ok = await _client.reconnect();
        // #region agent log
        agentLog('mount_state_provider.dart:_pollLoop', 'reconnect result', {'ok': ok, 'clientState': _client.state.name}, 'H5');
        // #endregion
        if (!ok) {
          // #region agent log
          DebugLog.add('reconnect failed, retrying in 5s');
          // #endregion
          await Future.delayed(const Duration(seconds: 5));
          continue;
        }
        if (state.productName.isEmpty) _versionFetched = false;
        if (state.latitude == 0.0 && state.longitude == 0.0) _siteFetched = false;
      }

      try {
        await _poll();
      } catch (e) {
        // #region agent log
        DebugLog.add('poll error: $e');
        // #endregion
      }
      // Wait between polls
      await Future.delayed(_pollInterval);
    }
    // #region agent log
    DebugLog.add('pollLoop ended (polling stopped)');
    // #endregion
  }

  Future<void> _poll() async {
    if (!_client.isConnected) return;

    // Fetch version info once
    if (!_versionFetched) {
      await _fetchVersion();
      _versionFetched = true;
      return;
    }

    // Fetch site location once (needed for sky map projection)
    if (!_siteFetched) {
      await _fetchSite();
      _siteFetched = true;
      return;
    }

    // Single :GXAS# command replaces all per-field queries.
    // The WiFi bridge serves from its own 500 ms cache when fresh, so this
    // costs at most one serial round-trip to the MainUnit per 2 s poll cycle.
    await _fetchAllState();

    _pollCount++;

    // #region agent log
    if (_pollCount <= 6) {
      agentLog('mount_state_provider.dart:_poll', 'poll cycle done', {
        'pollCount': _pollCount,
        'ra': state.ra,
        'dec': state.dec,
        'alt': state.alt,
        'az': state.az,
        'utcTime': state.utcTime,
        'siderealTime': state.siderealTime,
        'lat': state.latitude,
        'lon': state.longitude,
        'tracking': state.tracking.name,
        'valid': state.valid,
      }, 'H2A');
    }
    // #endregion
  }

  Future<void> _fetchAllState() async {
    final raw = await _client.sendCommand(LX200.getAllState);
    if (raw != null && raw.length == 64) {
      state = state.parseBinaryState(raw);
    }
  }

  Future<void> _fetchVersion() async {
    // #region agent log
    agentLog('mount_state_provider.dart:_fetchVersion', 'start', {
      'connected': _client.isConnected,
    }, 'H7');
    // #endregion
    final name = await _client.sendCommand(LX200.getProductName);
    await Future.delayed(_cmdDelay);
    final ver = await _client.sendCommand(LX200.getVersionNum);
    await Future.delayed(_cmdDelay);
    final board = await _client.sendCommand(LX200.getBoardVersion);
    await Future.delayed(_cmdDelay);
    final driver = await _client.sendCommand(LX200.getDriverType);

    // #region agent log
    agentLog('mount_state_provider.dart:_fetchVersion', 'done', {
      'name': name ?? 'null', 'ver': ver ?? 'null',
      'board': board ?? 'null', 'driver': driver ?? 'null',
      'connected': _client.isConnected,
    }, 'H7');
    // #endregion

    state = state.copyWith(
      productName: name ?? '',
      versionNum: ver ?? '',
      boardVersion: board ?? '',
      driverType: driver ?? '',
    );
  }

  Future<void> _fetchSite() async {
    final latStr = await _client.sendCommand(LX200.getLatitude);
    await Future.delayed(_cmdDelay);
    final lonStr = await _client.sendCommand(LX200.getLongitude);

    // #region agent log
    agentLog('mount_state_provider.dart:_fetchSite', 'site fetched', {
      'latStr': latStr ?? 'null',
      'lonStr': lonStr ?? 'null',
    }, 'H2B');
    // #endregion

    if (latStr != null && latStr.isNotEmpty) {
      final lat = _parseDms(latStr);
      if (lat != null) state = state.copyWith(latitude: lat);
    }
    if (lonStr != null && lonStr.isNotEmpty) {
      final lon = _parseDms(lonStr);
      if (lon != null) state = state.copyWith(longitude: lon);
    }
  }

  /// Parse a DMS string like "+47*37:54" or "+047*37:54" into decimal degrees.
  static double? _parseDms(String s) {
    if (s.isEmpty) return null;
    final sign = s.startsWith('-') ? -1.0 : 1.0;
    final withColons = s.replaceAll('*', ':');
    final cleaned = withColons.replaceAll(RegExp(r'[^0-9.:]'), '');
    final parts = cleaned.split(':');
    if (parts.isEmpty) return null;
    final deg = double.tryParse(parts[0]) ?? 0;
    final min = parts.length > 1 ? (double.tryParse(parts[1]) ?? 0) : 0;
    final sec = parts.length > 2 ? (double.tryParse(parts[2]) ?? 0) : 0;
    return sign * (deg + min / 60 + sec / 3600);
  }

  /// Force a full refresh
  Future<void> refresh() async {
    _versionFetched = false;
    _siteFetched = false;
    await _poll();
  }

  @override
  void dispose() {
    stopPolling();
    super.dispose();
  }
}

/// Provider for mount state
final mountStateProvider =
    StateNotifierProvider<MountStateNotifier, MountState>((ref) {
  final client = ref.watch(lx200ClientProvider);
  return MountStateNotifier(client);
});
