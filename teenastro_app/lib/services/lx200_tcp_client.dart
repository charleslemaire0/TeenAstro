import 'dart:async';
import 'dart:io';
import 'dart:typed_data';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/lx200_commands.dart';
import 'debug_agent_log.dart';

enum TcpState { disconnected, connecting, connected, error }

/// In-app debug log visible on the dashboard
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

class LX200TcpClient {
  Socket? _socket;
  TcpState _state = TcpState.disconnected;
  String _lastError = '';
  bool _busy = false;

  // Store last connection info for auto-reconnect
  String _lastIp = '';
  int _lastPort = 9999;

  // Single-listener architecture: one listener buffers all incoming data
  StreamSubscription<Uint8List>? _subscription;
  // Accumulation buffer for incoming bytes
  final List<int> _rxBuffer = [];

  TcpState get state => _state;
  String get lastError => _lastError;
  bool get isConnected => _state == TcpState.connected;
  String get lastIp => _lastIp;
  int get lastPort => _lastPort;

  Future<bool> connect(String ip, int port, {Duration timeout = const Duration(seconds: 5)}) async {
    if (_state == TcpState.connected) await disconnect();
    _state = TcpState.connecting;
    _rxBuffer.clear();
    _lastIp = ip;
    _lastPort = port;
    DebugLog.add('connect($ip:$port) ...');
    try {
      _socket = await Socket.connect(ip, port, timeout: timeout);
      _state = TcpState.connected;
      _lastError = '';
      // #region agent log
      agentLog('lx200_tcp_client.dart:connect', 'socket connected', {'ip': ip, 'port': port}, 'H1');
      // #endregion
      // Single listener: accumulate all incoming data into _rxBuffer
      _subscription = _socket!.listen(
        (data) {
          _rxBuffer.addAll(data);
          DebugLog.add('RX ${data.length}B: ${String.fromCharCodes(data).replaceAll('\n', '\\n')}');
        },
        onError: (e) {
          DebugLog.add('socket error: $e');
          _lastError = e.toString();
          _state = TcpState.error;
          // #region agent log
          agentLog('lx200_tcp_client.dart:onError', 'socket error', {'error': e.toString(), 'state': _state.name}, 'H2');
          // #endregion
        },
        onDone: () {
          DebugLog.add('socket closed');
          _state = TcpState.disconnected;
          _socket = null;
          // #region agent log
          agentLog('lx200_tcp_client.dart:onDone', 'socket closed', {'state': 'disconnected'}, 'H1');
          // #endregion
        },
      );
      DebugLog.add('connected OK');
      return true;
    } catch (e) {
      DebugLog.add('connect FAILED: $e');
      _lastError = e.toString();
      _state = TcpState.error;
      return false;
    }
  }

  /// Reconnect using last known IP/port
  Future<bool> reconnect() async {
    if (_lastIp.isEmpty) return false;
    DebugLog.add('reconnecting to $_lastIp:$_lastPort ...');
    return connect(_lastIp, _lastPort);
  }

  Future<void> disconnect() async {
    // #region agent log
    agentLog('lx200_tcp_client.dart:disconnect', 'disconnect() called', {'wasConnected': _state == TcpState.connected}, 'H4');
    // #endregion
    await _subscription?.cancel();
    _subscription = null;
    try {
      await _socket?.close();
    } catch (_) {}
    _socket = null;
    _state = TcpState.disconnected;
    _rxBuffer.clear();
  }

  /// Send an LX200 command and return the response.
  Future<String?> sendCommand(String cmd, {Duration? timeout}) async {
    if (!isConnected || _socket == null) {
      DebugLog.add('sendCommand($cmd) NOT CONNECTED');
      return null;
    }

    // Simple sequential mutex
    while (_busy) {
      await Future.delayed(const Duration(milliseconds: 5));
    }
    _busy = true;

    try {
      final replyType = getReplyType(cmd);
      final effectiveTimeout = timeout ?? const Duration(seconds: 2);

      // Drain any stale data in the buffer before sending
      _rxBuffer.clear();

      // Send the command
      _socket!.add(Uint8List.fromList(cmd.codeUnits));
      await _socket!.flush();

      if (replyType == CmdReply.none) {
        return '';
      }

      // Wait for response by polling _rxBuffer with a timeout
      final deadline = DateTime.now().add(effectiveTimeout);
      final buffer = StringBuffer();

      while (DateTime.now().isBefore(deadline)) {
        // Consume any bytes already in the buffer
        if (_rxBuffer.isNotEmpty) {
          buffer.write(String.fromCharCodes(_rxBuffer));
          _rxBuffer.clear();
        }

        // Check if we have a complete response
        if (replyType == CmdReply.short_ || replyType == CmdReply.shortBool) {
          if (buffer.length >= 1) {
            final result = buffer.toString().substring(0, 1);
            DebugLog.add('$cmd -> "$result"');
            return result;
          }
        } else if (replyType == CmdReply.long_) {
          final s = buffer.toString();
          final hashIdx = s.indexOf('#');
          if (hashIdx >= 0) {
            final result = s.substring(0, hashIdx);
            DebugLog.add('$cmd -> "${result.length > 30 ? '${result.substring(0, 30)}...' : result}"');
            return result;
          }
        }

        // Wait a bit then recheck
        await Future.delayed(const Duration(milliseconds: 20));
      }

      DebugLog.add('$cmd -> TIMEOUT (buf="${buffer.toString()}")');
      return '';
    } catch (e) {
      DebugLog.add('$cmd -> EXCEPTION: $e');
      _lastError = e.toString();
      // #region agent log
      agentLog('lx200_tcp_client.dart:sendCommand', 'sendCommand exception', {'cmd': cmd, 'error': e.toString(), 'state': _state.name}, 'H3');
      // #endregion
      return null;
    } finally {
      _busy = false;
    }
  }

  /// Convenience: send command with no reply expected
  Future<void> send(String cmd) async {
    await sendCommand(cmd);
  }

  /// Convenience: send and get boolean result
  Future<bool> sendBool(String cmd) async {
    final r = await sendCommand(cmd);
    return r == '1';
  }
}

/// Global provider for the LX200 TCP client
final lx200ClientProvider = Provider<LX200TcpClient>((ref) => LX200TcpClient());
