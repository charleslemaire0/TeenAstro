import 'dart:convert';
import 'dart:io';

/// Debug session logging: NDJSON to file or HTTP ingest (for connection-loss debugging).
/// Session ID: d3761d. Log path: workspace debug-d3761d.log
void agentLog(
  String location,
  String message,
  Map<String, dynamic> data,
  String hypothesisId,
) {
  final payload = {
    'sessionId': 'd3761d',
    'location': location,
    'message': message,
    'data': data,
    'timestamp': DateTime.now().millisecondsSinceEpoch,
    'hypothesisId': hypothesisId,
  };
  final line = '${jsonEncode(payload)}\n';

  // #region agent log
  final filePath = String.fromEnvironment('DEBUG_LOG_FILE', defaultValue: '');
  if (filePath.isNotEmpty) {
    try {
      File(filePath).writeAsStringSync(line, mode: FileMode.append);
    } catch (_) {}
  }
  // Optional HTTP ingest (skip if firewall blocks; use DEBUG_LOG_FILE instead)
  final useIngest = String.fromEnvironment('DEBUG_LOG_INGEST', defaultValue: 'false');
  if (useIngest == 'true') {
    final host = String.fromEnvironment('DEBUG_LOG_HOST', defaultValue: '127.0.0.1');
    HttpClient()
        .postUrl(Uri.parse('http://$host:7554/ingest/c13b50aa-99ce-4f99-85c8-f3d5e0e22d30'))
        .then((req) {
          req.headers.set('Content-Type', 'application/json');
          req.headers.set('X-Debug-Session-Id', 'd3761d');
          req.write(line.trim());
          return req.close();
        })
        .then((response) => response.drain())
        .catchError((_) {});
  }
  // #endregion
}
