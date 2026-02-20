import 'dart:convert';
import 'dart:io';

/// Debug session logging: NDJSON to file.
/// Session ID: d3761d. Log path: debug-d3761d.log (in workspace root)
const _kLogPath = r'c:\Users\clemair\Documents\learn\TeenAstro\debug-d3761d.log';

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
  try {
    File(_kLogPath).writeAsStringSync(line, mode: FileMode.append, flush: true);
  } catch (_) {}
  // Also print to console for immediate visibility
  print('[AGENT:$hypothesisId] $message ${jsonEncode(data)}');
  // #endregion
}
