import 'package:shared_preferences/shared_preferences.dart';
import 'phone_finder.dart';

/// Cones and frame size kept on the phone. No efinder is involved.
class PhoneFinderStore {
  static const _hz = 'phone_finder_hz_rad';
  static const _ve = 'phone_finder_ve_rad';
  static const _fov = 'phone_finder_fov_deg';
  static const _auto = 'phone_finder_auto_sync';
  static const _limit = 'phone_finder_recenter_arcmin';

  Future<FinderCones> loadCones() async {
    final p = await SharedPreferences.getInstance();
    return FinderCones(
      hzRad: p.getDouble(_hz) ?? 0,
      veRad: p.getDouble(_ve) ?? 0,
    );
  }

  Future<void> saveCones(FinderCones cones) async {
    final p = await SharedPreferences.getInstance();
    await p.setDouble(_hz, cones.hzRad);
    await p.setDouble(_ve, cones.veRad);
  }

  Future<void> clearCones() => saveCones(const FinderCones());

  Future<double> loadFov() async {
    final p = await SharedPreferences.getInstance();
    return p.getDouble(_fov) ?? 60;
  }

  Future<void> saveFov(double deg) async {
    final p = await SharedPreferences.getInstance();
    await p.setDouble(_fov, deg);
  }

  Future<bool> loadAutoSync() async {
    final p = await SharedPreferences.getInstance();
    return p.getBool(_auto) ?? false;
  }

  Future<void> saveAutoSync(bool on) async {
    final p = await SharedPreferences.getInstance();
    await p.setBool(_auto, on);
  }

  /// Recenter when the telescope axis is farther than this from the goto target.
  Future<int> loadRecenterArcmin() async {
    final p = await SharedPreferences.getInstance();
    final v = p.getInt(_limit) ?? 2;
    return v.clamp(2, 60);
  }

  Future<void> saveRecenterArcmin(int arcmin) async {
    final p = await SharedPreferences.getInstance();
    await p.setInt(_limit, arcmin.clamp(2, 60));
  }
}
