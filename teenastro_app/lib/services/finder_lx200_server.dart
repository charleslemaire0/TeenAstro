import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'finder_lx200.dart';

/// TCP port SkySafari connects to.
const finderLx200Port = 4030;

/// LX200 server on the phone's network. Each client gets its own precision.
class FinderLx200Server {
  final FinderSnapshot Function() snapshot;
  final void Function(double raH, double decD) onTarget;

  ServerSocket? _server;
  final _clients = <Socket>{};

  FinderLx200Server({required this.snapshot, required this.onTarget});

  bool get running => _server != null;

  Future<void> start({int port = finderLx200Port}) async {
    if (_server != null) return;
    final server = await ServerSocket.bind(InternetAddress.anyIPv4, port, shared: true);
    _server = server;
    server.listen(_serve, onError: (_) {});
  }

  void _serve(Socket client) {
    _clients.add(client);
    client.setOption(SocketOption.tcpNoDelay, true);
    final lx200 = FinderLx200(onTarget: onTarget);
    client.listen(
      (bytes) {
        for (final reply in lx200.feed(latin1.decode(bytes), snapshot())) {
          client.add(latin1.encode(reply));
        }
      },
      onDone: () => _drop(client),
      onError: (_) => _drop(client),
      cancelOnError: true,
    );
  }

  void _drop(Socket client) {
    _clients.remove(client);
    client.destroy();
  }

  Future<void> stop() async {
    for (final c in _clients.toList()) {
      c.destroy();
    }
    _clients.clear();
    await _server?.close();
    _server = null;
  }
}

/// The phone's Wi-Fi IPv4 address, or the first non-loopback one.
Future<String?> phoneLanAddress() async {
  try {
    final interfaces = await NetworkInterface.list(
      type: InternetAddressType.IPv4,
      includeLoopback: false,
    );
    String? first;
    for (final i in interfaces) {
      for (final a in i.addresses) {
        first ??= a.address;
        final name = i.name.toLowerCase();
        if (name.contains('wlan') || name.contains('wi-fi') || name.contains('wifi')) {
          return a.address;
        }
      }
    }
    return first;
  } catch (_) {
    return null;
  }
}
