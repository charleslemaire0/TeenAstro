import 'package:flutter_riverpod/flutter_riverpod.dart';

/// Last route under the Goto tab (/goto or /planetarium).
/// When user taps the Goto nav item, we navigate here so they return to the same view.
final lastGotoTabRouteProvider = StateProvider<String>((ref) => '/goto');
