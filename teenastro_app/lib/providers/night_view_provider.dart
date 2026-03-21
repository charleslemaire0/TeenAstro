import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';

const _keyNightView = 'night_view';

final nightViewProvider = StateNotifierProvider<NightViewNotifier, bool>((ref) {
  return NightViewNotifier();
});

class NightViewNotifier extends StateNotifier<bool> {
  NightViewNotifier() : super(false) {
    _load();
  }

  Future<void> _load() async {
    final prefs = await SharedPreferences.getInstance();
    state = prefs.getBool(_keyNightView) ?? false;
  }

  Future<void> setEnabled(bool enabled) async {
    if (state == enabled) return;
    state = enabled;
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool(_keyNightView, enabled);
  }

  Future<void> toggle() => setEnabled(!state);
}
