import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:shared_preferences/shared_preferences.dart';

const _keyNightView = 'night_view_enabled';

class NightViewNotifier extends Notifier<bool> {
  bool _didLoad = false;

  @override
  bool build() => false;

  Future<void> loadSaved() async {
    if (_didLoad) return;
    _didLoad = true;
    final prefs = await SharedPreferences.getInstance();
    state = prefs.getBool(_keyNightView) ?? false;
  }

  Future<void> setEnabled(bool value) async {
    state = value;
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool(_keyNightView, value);
  }

  Future<void> load() async {
    final prefs = await SharedPreferences.getInstance();
    state = prefs.getBool(_keyNightView) ?? false;
  }
}

final nightViewProvider =
    NotifierProvider<NightViewNotifier, bool>(NightViewNotifier.new);
