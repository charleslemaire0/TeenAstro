import 'dart:async';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/mount_state.dart';
import '../models/lx200_commands.dart';
import 'lx200_tcp_client.dart';

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
        final ok = await _client.reconnect();
        if (!ok) {
          await Future.delayed(const Duration(seconds: 5));
          continue;
        }
        if (state.productName.isEmpty) _versionFetched = false;
        if (state.latitude == 0.0 && state.longitude == 0.0) _siteFetched = false;
      }

      try {
        await _poll();
      } catch (e) {}
      await Future.delayed(_pollInterval);
    }
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
  }

  Future<void> _fetchAllState() async {
    final raw = await _client.sendCommand(LX200.getAllState);
    if (raw != null) {
      final base64 = raw.endsWith('#') ? raw.substring(0, raw.length - 1) : raw;
      if (base64.length == 88) {
        state = state.parseBinaryState(base64);
      }
    }
  }

  /// True if [s] looks like a :GXAS# base64 payload (88 chars) rather than version/driver text.
  static bool _looksLikeGxasBase64(String? s) {
    if (s == null || s.length != 88) return false;
    final base64 = RegExp(r'^[A-Za-z0-9+/]+$');
    return base64.hasMatch(s);
  }

  /// True if [s] looks like a version number (e.g. 1.5.9) — wrong reply for :GVP#.
  static bool _looksLikeVersion(String? s) {
    if (s == null || s.isEmpty) return false;
    return RegExp(r'^\d+\.\d+(\.\d+)?$').hasMatch(s.trim());
  }

  /// True if [s] is the known product name — wrong reply when we asked :GVN# (misattributed from delayed :GVP#).
  static bool _isProductNameMisattributedToGVN(String? s) {
    if (s == null || s.isEmpty) return false;
    return s.trim() == 'TeenAstro';
  }

  Future<void> _fetchVersion() async {
    var name = await _client.sendCommand(LX200.getProductName);
    if (_looksLikeGxasBase64(name) || _looksLikeVersion(name)) {
      await Future.delayed(_cmdDelay);
      final retry = await _client.sendCommand(LX200.getProductName);
      name = (_looksLikeGxasBase64(retry) || _looksLikeVersion(retry)) ? null : retry;
    }
    await Future.delayed(_cmdDelay);
    var ver = await _client.sendCommand(LX200.getVersionNum);
    if (_looksLikeGxasBase64(ver) || _isProductNameMisattributedToGVN(ver)) {
      await Future.delayed(_cmdDelay);
      final retry = await _client.sendCommand(LX200.getVersionNum);
      ver = (_looksLikeGxasBase64(retry) || _isProductNameMisattributedToGVN(retry)) ? null : retry;
    }
    await Future.delayed(_cmdDelay);
    var board = await _client.sendCommand(LX200.getBoardVersion);
    if (_looksLikeGxasBase64(board)) {
      await Future.delayed(_cmdDelay);
      final retry = await _client.sendCommand(LX200.getBoardVersion);
      board = _looksLikeGxasBase64(retry) ? null : retry;
    }
    await Future.delayed(_cmdDelay);
    var driver = await _client.sendCommand(LX200.getDriverType);
    if (_looksLikeGxasBase64(driver)) {
      await Future.delayed(_cmdDelay);
      final retry = await _client.sendCommand(LX200.getDriverType);
      driver = _looksLikeGxasBase64(retry) ? null : retry;
    }

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

  /// Refresh site information (lat/lon) from the mount
  Future<void> refreshSite() async {
    if (!_client.isConnected) return;
    _siteFetched = false;
    await _fetchSite();
    _siteFetched = true;
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
