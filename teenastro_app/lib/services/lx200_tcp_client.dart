import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/lx200_commands.dart';
import 'debug_agent_log.dart';

enum TcpState { disconnected, connecting, connected, error }

// ---------------------------------------------------------------------------
// In-app debug log (last 50 entries, visible on dashboard)
// ---------------------------------------------------------------------------
class DebugLog {
  static final List<String> entries = [];
  static void add(String msg) {
    final ts = DateTime.now().toString().substring(11, 19);
    final line = '[$ts] $msg';
    entries.add(line);
    if (entries.length > 50) entries.removeAt(0);
    print('[DBG] $msg');
  }
  static void clear() => entries.clear();
}

// ---------------------------------------------------------------------------
// Timestamped connection trace (last 200 events, ms precision)
// Exported as plain text for copy-paste into a bug report.
// ---------------------------------------------------------------------------
class ConnectTrace {
  static final List<String> _log = [];

  static void record(String event, [Map<String, dynamic>? data]) {
    final ts = DateTime.now().toString().substring(11, 23); // HH:mm:ss.mmm
    final d = data == null
        ? ''
        : ' ' + data.entries.map((e) => '${e.key}=${e.value}').join(' ');
    _log.add('[$ts] $event$d');
    if (_log.length > 200) _log.removeAt(0);
    print('[TRACE] $event$d');
  }

  static List<String> get entries => List.unmodifiable(_log);

  static String export() => _log.join('\n');

  static void clear() => _log.clear();
}

// ---------------------------------------------------------------------------
// LX200 TCP client
// ---------------------------------------------------------------------------
class LX200TcpClient {
  Socket? _socket;
  TcpState _state = TcpState.disconnected;
  String _lastError = '';
  bool _busy = false;
  Future<void> _immediateLast = Future.value();

  // Last known connection parameters for auto-reconnect
  String _lastIp = '';
  int _lastPort = 9999;

  // Single-listener: one subscription buffers all incoming bytes
  StreamSubscription<Uint8List>? _subscription;
  final List<int> _rxBuffer = [];

  // Heartbeat: fires every 1.5 s; sends :GXAS# if idle for >1 s (same as poll, keeps connection alive)
  Timer? _heartbeatTimer;
  DateTime? _lastCmdTime;

  // How long to wait without activity before sending a heartbeat command
  static const _heartbeatInterval = Duration(milliseconds: 1500);
  static const _heartbeatIdleThreshold = Duration(milliseconds: 1000);

  // ---------------------------------------------------------------------------
  // Getters
  // ---------------------------------------------------------------------------
  TcpState get state => _state;
  String get lastError => _lastError;
  bool get isConnected => _state == TcpState.connected;
  String get lastIp => _lastIp;
  int get lastPort => _lastPort;

  // ---------------------------------------------------------------------------
  // Connect
  // ---------------------------------------------------------------------------
  Future<bool> connect(String ip, int port,
      {Duration timeout = const Duration(seconds: 5)}) async {
    if (_state == TcpState.connected || _subscription != null) {
      await disconnect();
    }
    _state = TcpState.connecting;
    _rxBuffer.clear();
    _lastIp = ip;
    _lastPort = port;
    _lastCmdTime = null;

    ConnectTrace.record('connect.start', {'ip': ip, 'port': port});
    DebugLog.add('connect($ip:$port) ...');

    try {
      _socket = await Socket.connect(ip, port, timeout: timeout);
      _state = TcpState.connected;
      _lastError = '';

      _socket!.setOption(SocketOption.tcpNoDelay, true);

      // #region agent log
      agentLog('lx200_tcp_client.dart:connect', 'socket connected',
          {'ip': ip, 'port': port}, 'H1');
      // #endregion
      ConnectTrace.record('connect.ok', {'ip': ip, 'port': port});
      DebugLog.add('connected OK');

      // Single listener: accumulate all incoming bytes into _rxBuffer
      _subscription = _socket!.listen(
        (data) {
          _rxBuffer.addAll(data);
          DebugLog.add(
              'RX ${data.length}B: ${String.fromCharCodes(data).replaceAll('\n', '\\n')}');
        },
        onError: (e) {
          ConnectTrace.record('socket.error', {
            'error': e.toString(),
            'msSinceCmd': _msSinceLastCmd(),
          });
          DebugLog.add('socket error: $e');
          _lastError = e.toString();
          _state = TcpState.error;
          _subscription = null;
          _stopHeartbeat();
          // #region agent log
          agentLog('lx200_tcp_client.dart:onError', 'socket error',
              {'error': e.toString(), 'state': _state.name}, 'H2');
          // #endregion
        },
        onDone: () {
          ConnectTrace.record('socket.done', {
            'prevState': _state.name,
            'msSinceCmd': _msSinceLastCmd(),
          });
          DebugLog.add('socket closed (onDone)');
          _state = TcpState.disconnected;
          _socket = null;
          _subscription = null; // prevent dangling reference
          _stopHeartbeat();
          // #region agent log
          agentLog('lx200_tcp_client.dart:onDone', 'socket closed',
              {'state': 'disconnected'}, 'H1');
          // #endregion
        },
        cancelOnError: true,
      );

      _startHeartbeat();
      return true;
    } catch (e) {
      ConnectTrace.record('connect.failed', {'error': e.toString()});
      DebugLog.add('connect FAILED: $e');
      _lastError = e.toString();
      _state = TcpState.error;
      return false;
    }
  }

  // ---------------------------------------------------------------------------
  // Reconnect
  // ---------------------------------------------------------------------------
  Future<bool> reconnect() async {
    if (_lastIp.isEmpty) return false;
    ConnectTrace.record('reconnect.attempt', {'ip': _lastIp, 'port': _lastPort});
    DebugLog.add('reconnecting to $_lastIp:$_lastPort ...');
    return connect(_lastIp, _lastPort);
  }

  // ---------------------------------------------------------------------------
  // Disconnect
  // ---------------------------------------------------------------------------
  Future<void> disconnect() async {
    ConnectTrace.record('disconnect.called',
        {'wasConnected': _state == TcpState.connected});
    // #region agent log
    agentLog('lx200_tcp_client.dart:disconnect', 'disconnect() called',
        {'wasConnected': _state == TcpState.connected}, 'H4');
    // #endregion
    _stopHeartbeat();
    await _subscription?.cancel();
    _subscription = null;
    try {
      await _socket?.close();
    } catch (_) {}
    _socket = null;
    _state = TcpState.disconnected;
    _rxBuffer.clear();
  }

  // ---------------------------------------------------------------------------
  // Send command and return response
  // ---------------------------------------------------------------------------
  Future<String?> sendCommand(String cmd, {Duration? timeout}) async {
    if (!isConnected || _socket == null) {
      ConnectTrace.record('sendCommand.notConnected', {'cmd': cmd});
      DebugLog.add('sendCommand($cmd) NOT CONNECTED');
      return null;
    }

    // Sequential mutex: one command at a time
    while (_busy) {
      await Future.delayed(const Duration(milliseconds: 5));
    }
    _busy = true;

    final sendTime = DateTime.now();
    _lastCmdTime = sendTime;

    try {
      final replyType = getReplyType(cmd);
      final effectiveTimeout = timeout ?? const Duration(seconds: 2);

      // Drain stale bytes before sending
      _rxBuffer.clear();

      _socket!.add(Uint8List.fromList(cmd.codeUnits));
      await _socket!.flush();

      // #region agent log
      agentLog('lx200_tcp_client.dart:sendCommand', 'cmd sent', {
        'cmd': cmd,
        'replyType': replyType.name,
        'connected': isConnected,
      }, 'H6');
      // #endregion

      if (replyType == CmdReply.none) {
        ConnectTrace.record('cmd.noReply', {'cmd': cmd});
        return '';
      }

      // Wait for response by polling _rxBuffer, checking connection state each iteration
      final deadline = DateTime.now().add(effectiveTimeout);
      final buffer = StringBuffer();
      bool gotData = false;

      while (DateTime.now().isBefore(deadline) &&
          _state == TcpState.connected) {
        if (_rxBuffer.isNotEmpty) {
          final chunk = String.fromCharCodes(_rxBuffer);
          buffer.write(chunk);
          _rxBuffer.clear();

          // #region agent log
          if (!gotData) {
            final latencyMs =
                DateTime.now().difference(sendTime).inMilliseconds;
            agentLog('lx200_tcp_client.dart:sendCommand', 'first data received',
                {
                  'cmd': cmd,
                  'chunk': chunk,
                  'latencyMs': latencyMs,
                  'connected': isConnected,
                }, 'H6');
            gotData = true;
          }
          // #endregion
        }

        if (replyType == CmdReply.short_ || replyType == CmdReply.shortBool) {
          if (buffer.length >= 1) {
            final result = buffer.toString().substring(0, 1);
            final ms = DateTime.now().difference(sendTime).inMilliseconds;
            ConnectTrace.record('cmd.ok',
                {'cmd': cmd, 'result': result, 'ms': ms});
            DebugLog.add('$cmd -> "$result"');
            return result;
          }
        } else if (replyType == CmdReply.long_) {
          final s = buffer.toString();
          final hashIdx = s.indexOf('#');
          if (hashIdx >= 0) {
            final result = s.substring(0, hashIdx);
            final ms = DateTime.now().difference(sendTime).inMilliseconds;
            ConnectTrace.record('cmd.ok', {
              'cmd': cmd,
              'result': result.length > 20
                  ? '${result.substring(0, 20)}…'
                  : result,
              'ms': ms,
            });
            DebugLog.add(
                '$cmd -> "${result.length > 30 ? '${result.substring(0, 30)}...' : result}"');
            return result;
          }
        }

        await Future.delayed(const Duration(milliseconds: 20));
      }

      // Timeout or connection dropped during wait
      final reason = _state != TcpState.connected ? 'disconnected' : 'timeout';
      final ms = DateTime.now().difference(sendTime).inMilliseconds;
      ConnectTrace.record('cmd.failed', {
        'cmd': cmd,
        'reason': reason,
        'ms': ms,
        'bufLen': buffer.length,
      });

      // #region agent log
      agentLog('lx200_tcp_client.dart:sendCommand', 'TIMEOUT/DISCONNECTED', {
        'cmd': cmd,
        'bufContent': buffer.toString(),
        'gotData': gotData,
        'connected': isConnected,
        'reason': reason,
      }, 'H6');
      // #endregion
      DebugLog.add('$cmd -> $reason (buf="${buffer.toString()}")');
      return '';
    } catch (e) {
      final ms = DateTime.now().difference(sendTime).inMilliseconds;
      ConnectTrace.record('cmd.exception',
          {'cmd': cmd, 'error': e.toString(), 'ms': ms});
      DebugLog.add('$cmd -> EXCEPTION: $e');
      _lastError = e.toString();
      // Immediately mark as disconnected so poll loop reconnects
      if (e is SocketException || e is IOException || e is OSError) {
        _state = TcpState.error;
        _subscription = null;
        _socket = null;
        _stopHeartbeat();
      }
      // #region agent log
      agentLog('lx200_tcp_client.dart:sendCommand', 'sendCommand exception',
          {'cmd': cmd, 'error': e.toString(), 'state': _state.name}, 'H3');
      // #endregion
      return null;
    } finally {
      _busy = false;
    }
  }

  // ---------------------------------------------------------------------------
  // Convenience helpers
  // ---------------------------------------------------------------------------
  Future<void> send(String cmd) async => await sendCommand(cmd);

  Future<bool> sendBool(String cmd) async {
    final r = await sendCommand(cmd);
    return r == '1';
  }

  /// Send move/stop commands without waiting on the poll mutex.
  /// Only accepts no-reply commands to avoid blocking the socket writer.
  void sendImmediate(String cmd) {
    if (!isConnected || _socket == null) return;
    if (getReplyType(cmd) != CmdReply.none) return;
    final prev = _immediateLast;
    final done = Completer<void>();
    _immediateLast = done.future;
    prev.then((_) async {
      try {
        _socket?.add(Uint8List.fromList(cmd.codeUnits));
        await _socket?.flush();
      } finally {
        done.complete();
      }
    });
  }

  // ---------------------------------------------------------------------------
  // Heartbeat
  // ---------------------------------------------------------------------------

  /// Starts a periodic timer that sends a lightweight status command whenever
  /// the connection has been idle for longer than [_heartbeatIdleThreshold].
  /// This prevents the firmware's idle-timeout from firing during poll gaps.
  void _startHeartbeat() {
    _heartbeatTimer?.cancel();
    _heartbeatTimer =
        Timer.periodic(_heartbeatInterval, (_) => _onHeartbeatTick());
  }

  void _stopHeartbeat() {
    _heartbeatTimer?.cancel();
    _heartbeatTimer = null;
  }

  Future<void> _onHeartbeatTick() async {
    if (!isConnected || _busy) return;
    final now = DateTime.now();
    final last = _lastCmdTime;
    if (last == null || now.difference(last) >= _heartbeatIdleThreshold) {
      ConnectTrace.record('heartbeat.ping',
          {'idleMs': last == null ? -1 : now.difference(last).inMilliseconds});
      // :GXAS# is the single bulk state command used by the poll loop
      await sendCommand(LX200.getAllState,
          timeout: const Duration(seconds: 3));
    }
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------
  int _msSinceLastCmd() {
    final last = _lastCmdTime;
    if (last == null) return -1;
    return DateTime.now().difference(last).inMilliseconds;
  }
}

/// Global provider for the LX200 TCP client
final lx200ClientProvider =
    Provider<LX200TcpClient>((ref) => LX200TcpClient());
