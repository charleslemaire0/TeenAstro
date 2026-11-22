import 'package:shared_preferences/shared_preferences.dart';
import 'phone_finder.dart';

/// Plate solver used on the camera frame.
enum PhoneSolver { tetra, stellar }

/// Cones and frame size kept on the phone. No efinder is involved.
class PhoneFinderStore {
  static const _hz = 'phone_finder_hz_rad';
  static const _ve = 'phone_finder_ve_rad';
  static const _fov = 'phone_finder_fov_deg';
  static const _auto = 'phone_finder_auto_sync';
  static const _limit = 'phone_finder_recenter_arcmin';
  static const _solver = 'phone_finder_solver';
  static const _liveSeconds = 'phone_finder_live_seconds';

  static const liveMinSeconds = 30;
  static const liveMaxSeconds = 180;
  static const liveDefaultSeconds = 60;

  static const _gamma = 'phone_finder_view_gamma';
  static const gammaMin = 1.0;
  static const gammaMax = 5.0;
  static const gammaDefault = 2.5;

  /// Display gamma for the preview and photo. Solving always uses the raw frame.
  Future<double> loadGamma() async {
    final p = await SharedPreferences.getInstance();
    return (p.getDouble(_gamma) ?? gammaDefault).clamp(gammaMin, gammaMax);
  }

  static const _exposureUs = 'phone_finder_exposure_us';
  static const _iso = 'phone_finder_iso';

  /// Shutter steps offered to the user, in microseconds.
  static const exposureSteps = <int>[
    50000, 100000, 200000, 500000, 1000000, 2000000, 4000000, 8000000,
    15000000, 30000000,
  ];
  static const isoSteps = <int>[100, 200, 400, 800, 1600, 3200, 6400];
  static const exposureDefaultUs = 1000000;
  static const isoDefault = 1600;

  Future<int> loadExposureUs() async {
    final p = await SharedPreferences.getInstance();
    return p.getInt(_exposureUs) ?? exposureDefaultUs;
  }

  Future<void> saveExposureUs(int us) async {
    final p = await SharedPreferences.getInstance();
    await p.setInt(_exposureUs, us);
  }

  Future<int> loadIso() async {
    final p = await SharedPreferences.getInstance();
    return p.getInt(_iso) ?? isoDefault;
  }

  Future<void> saveIso(int iso) async {
    final p = await SharedPreferences.getInstance();
    await p.setInt(_iso, iso);
  }

  Future<void> saveGamma(double gamma) async {
    final p = await SharedPreferences.getInstance();
    await p.setDouble(_gamma, gamma.clamp(gammaMin, gammaMax));
  }

  /// How long Live keeps solving, in seconds.
  Future<int> loadLiveSeconds() async {
    final p = await SharedPreferences.getInstance();
    return (p.getInt(_liveSeconds) ?? liveDefaultSeconds)
        .clamp(liveMinSeconds, liveMaxSeconds);
  }

  Future<void> saveLiveSeconds(int seconds) async {
    final p = await SharedPreferences.getInstance();
    await p.setInt(
        _liveSeconds, seconds.clamp(liveMinSeconds, liveMaxSeconds));
  }

  Future<PhoneSolver> loadSolver() async {
    final p = await SharedPreferences.getInstance();
    return p.getString(_solver) == PhoneSolver.stellar.name
        ? PhoneSolver.stellar
        : PhoneSolver.tetra;
  }

  Future<void> saveSolver(PhoneSolver solver) async {
    final p = await SharedPreferences.getInstance();
    await p.setString(_solver, solver.name);
  }

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
