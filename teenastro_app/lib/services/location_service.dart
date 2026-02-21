import 'dart:convert';
import 'dart:io';
import 'package:geolocator/geolocator.dart';

/// Result of a location lookup (lat/lon in degrees).
class LocationResult {
  final double latitude;
  final double longitude;
  final String source; // 'gps' | 'ip' | 'manual'

  const LocationResult({
    required this.latitude,
    required this.longitude,
    required this.source,
  });
}

/// Fetches current location: GPS on mobile, IP-based on Windows/desktop.
class LocationService {
  /// Returns current position or null if unavailable.
  /// - Android/iOS: uses device GPS.
  /// - Windows/macOS/Linux: uses ip-api.com (internet-based).
  static Future<LocationResult?> getCurrentLocation() async {
    if (Platform.isAndroid || Platform.isIOS) {
      return _getFromGps();
    }
    if (Platform.isWindows || Platform.isMacOS || Platform.isLinux) {
      return _getFromIp();
    }
    return null;
  }

  /// GPS-based location (mobile).
  static Future<LocationResult?> _getFromGps() async {
    try {
      final enabled = await Geolocator.isLocationServiceEnabled();
      if (!enabled) return null;

      LocationPermission permission = await Geolocator.checkPermission();
      if (permission == LocationPermission.denied) {
        permission = await Geolocator.requestPermission();
        if (permission == LocationPermission.denied ||
            permission == LocationPermission.deniedForever) {
          return null;
        }
      }

      final pos = await Geolocator.getCurrentPosition(
        locationSettings: const LocationSettings(
          accuracy: LocationAccuracy.high,
          timeLimit: Duration(seconds: 15),
        ),
      );
      return LocationResult(
        latitude: pos.latitude,
        longitude: pos.longitude,
        source: 'gps',
      );
    } catch (_) {
      return null;
    }
  }

  /// IP-based location (Windows/desktop via internet).
  static Future<LocationResult?> _getFromIp() async {
    try {
      final client = HttpClient();
      client.connectionTimeout = const Duration(seconds: 10);
      final req = await client.getUrl(
        Uri.parse('http://ip-api.com/json/?fields=lat,lon,status'),
      );
      final resp = await req.close();
      if (resp.statusCode != 200) return null;
      final body = await resp.transform(utf8.decoder).join();
      client.close();

      final map = jsonDecode(body) as Map<String, dynamic>?;
      if (map == null || map['status'] != 'success') return null;

      final lat = (map['lat'] as num?)?.toDouble();
      final lon = (map['lon'] as num?)?.toDouble();
      if (lat == null || lon == null) return null;

      return LocationResult(
        latitude: lat,
        longitude: lon,
        source: 'ip',
      );
    } catch (_) {
      return null;
    }
  }

  /// Human-readable source for UI.
  static String sourceLabel(String source) {
    switch (source) {
      case 'gps':
        return 'Phone GPS';
      case 'ip':
        return 'Internet (IP)';
      case 'manual':
        return 'Manual';
      default:
        return source;
    }
  }
}
