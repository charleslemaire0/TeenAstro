import 'package:flutter_riverpod/flutter_riverpod.dart';

/// Remembers the last selected tab in the Goto section (Goto, Catalogs, Planetarium).
final lastGotoTabRouteProvider = StateProvider<String>((ref) => '/goto');
