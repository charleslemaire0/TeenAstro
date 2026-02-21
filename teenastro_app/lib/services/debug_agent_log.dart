import 'dart:convert';
import 'dart:io';

/// Debug session logging: NDJSON to file.
/// Session ID: 723a44. Log path: debug-723a44.log (in workspace root)
const _kLogPath = r'c:\Users\charl\source\repos\charleslemaire0\TeenAstro\debug-723a44.log';

void agentLog(
  String location,
  String message,
  Map<String, dynamic> data,
  String hypothesisId,
) {
  final payload = {
    'sessionId': '723a44',
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
