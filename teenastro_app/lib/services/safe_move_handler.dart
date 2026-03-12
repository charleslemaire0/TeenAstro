import 'dart:async';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/lx200_commands.dart';
import 'lx200_tcp_client.dart';

/// Number of times to send :Q# on stop to improve chance the mount receives at least one.
const int kRedundantStopCount = 3;

/// Delay in ms between redundant stop sends.
const int kRedundantStopDelayMs = 60;

/// Max move duration in seconds; after this the handler auto-sends stop.
const int kMaxMoveDurationSeconds = 5;

/// Safe move layer: redundant stop on release and optional max move duration
/// so that if a stop command is lost, damage risk is reduced.
class SafeMoveHandler {
  SafeMoveHandler(this._client, {void Function()? onMaxDurationReached})
      : _onMaxDurationReached = onMaxDurationReached;

  final LX200TcpClient _client;
  final void Function()? _onMaxDurationReached;

  Timer? _maxMoveTimer;

  /// Start moving in the given direction. Sends the move command once and
  /// starts a max-duration timer. Call [stopMove] on release or use the STOP button.
  void startMove(String moveCmd) {
    _maxMoveTimer?.cancel();
    _maxMoveTimer = null;
    _client.sendImmediate(moveCmd);
    _maxMoveTimer = Timer(Duration(seconds: kMaxMoveDurationSeconds), () {
      _maxMoveTimer = null;
      _onMaxDurationReached?.call();
      _sendRedundantStop();
    });
  }

  /// Stop movement: cancel the max-duration timer and send stop multiple times.
  void stopMove() {
    _maxMoveTimer?.cancel();
    _maxMoveTimer = null;
    _sendRedundantStop();
  }

  /// Send :Q# multiple times with short delay to improve chance the mount receives stop.
  void _sendRedundantStop() {
    for (int i = 0; i < kRedundantStopCount; i++) {
      if (_client.isConnected) {
        _client.sendImmediate(LX200.stopAll);
      }
      if (i < kRedundantStopCount - 1) {
        Future.delayed(const Duration(milliseconds: kRedundantStopDelayMs))
            .then((_) => _sendNextStop(i + 1));
        return;
      }
    }
  }

  void _sendNextStop(int i) {
    if (i >= kRedundantStopCount) return;
    if (_client.isConnected) _client.sendImmediate(LX200.stopAll);
    if (i < kRedundantStopCount - 1) {
      Future.delayed(const Duration(milliseconds: kRedundantStopDelayMs))
          .then((_) => _sendNextStop(i + 1));
    }
  }

  /// Cancel any active timer. Call when disconnecting or disposing.
  void dispose() {
    _maxMoveTimer?.cancel();
    _maxMoveTimer = null;
  }
}

/// Provider for the safe move handler; uses the LX200 client and disposes the handler when no longer used.
final safeMoveHandlerProvider = Provider<SafeMoveHandler>((ref) {
  final client = ref.watch(lx200ClientProvider);
  final handler = SafeMoveHandler(client);
  ref.onDispose(() => handler.dispose());
  return handler;
});
