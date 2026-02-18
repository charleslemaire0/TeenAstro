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
        _versionFetched = false;
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
      return; // Don't overload: version fetch is enough for the first cycle
    }

    // Always fetch status
    await _fetchStatus();
    await Future.delayed(_cmdDelay);

    // Alternate between position and time/target to spread the load
    if (_pollCount % 2 == 0) {
      await _fetchPosition();
    } else {
      await _fetchTime();
      await Future.delayed(_cmdDelay);
      await _fetchTarget();
    }

    _pollCount++;
  }

  Future<void> _fetchStatus() async {
    final raw = await _client.sendCommand(LX200.getStatus);
    if (raw != null && raw.isNotEmpty) {
      state = state.parseStatus(raw);
    }
  }

  Future<void> _fetchPosition() async {
    final ra = await _client.sendCommand(LX200.getRa);
    await Future.delayed(_cmdDelay);
    final dec = await _client.sendCommand(LX200.getDec);
    await Future.delayed(_cmdDelay);
    final az = await _client.sendCommand(LX200.getAz);
    await Future.delayed(_cmdDelay);
    final alt = await _client.sendCommand(LX200.getAlt);

    state = state.copyWith(
      ra: ra ?? state.ra,
      dec: dec ?? state.dec,
      az: az ?? state.az,
      alt: alt ?? state.alt,
    );
  }

  Future<void> _fetchTime() async {
    final utc = await _client.sendCommand(LX200.getUtcTime);
    await Future.delayed(_cmdDelay);
    final date = await _client.sendCommand(LX200.getUtcDate);
    await Future.delayed(_cmdDelay);
    final sid = await _client.sendCommand(LX200.getSidereal);

    state = state.copyWith(
      utcTime: utc ?? state.utcTime,
      utcDate: date ?? state.utcDate,
      siderealTime: sid ?? state.siderealTime,
    );
  }

  Future<void> _fetchTarget() async {
    final ra = await _client.sendCommand(LX200.getTargetRa);
    await Future.delayed(_cmdDelay);
    final dec = await _client.sendCommand(LX200.getTargetDec);

    state = state.copyWith(
      targetRa: ra ?? state.targetRa,
      targetDec: dec ?? state.targetDec,
    );
  }

  Future<void> _fetchVersion() async {
    final name = await _client.sendCommand(LX200.getProductName);
    await Future.delayed(_cmdDelay);
    final ver = await _client.sendCommand(LX200.getVersionNum);
    await Future.delayed(_cmdDelay);
    final board = await _client.sendCommand(LX200.getBoardVersion);
    await Future.delayed(_cmdDelay);
    final driver = await _client.sendCommand(LX200.getDriverType);

    state = state.copyWith(
      productName: name ?? '',
      versionNum: ver ?? '',
      boardVersion: board ?? '',
      driverType: driver ?? '',
    );
  }

  /// Force a full refresh
  Future<void> refresh() async {
    _versionFetched = false;
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
