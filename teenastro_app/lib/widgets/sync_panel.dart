import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/lx200_commands.dart';
import '../models/mount_state.dart';
import '../services/lx200_tcp_client.dart';
import '../services/location_service.dart';
import '../services/mount_state_provider.dart';
import '../theme.dart';

/// Card showing mount GNSS/time status and sync buttons.
/// Compares mount time with phone time and provides one-tap sync.
class TimeSyncCard extends ConsumerStatefulWidget {
  const TimeSyncCard({super.key});

  @override
  ConsumerState<TimeSyncCard> createState() => _TimeSyncCardState();
}

class _TimeSyncCardState extends ConsumerState<TimeSyncCard> {
  String _syncResult = '';
  bool _syncing = false;
  Timer? _clockTimer;

  @override
  void initState() {
    super.initState();
    _clockTimer = Timer.periodic(
        const Duration(seconds: 1), (_) => setState(() {}));
  }

  @override
  void dispose() {
    _clockTimer?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final state = ref.watch(mountStateProvider);
    final now = DateTime.now().toUtc();

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Time and Location Sync',
                style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),

            // GNSS status indicators
            _GnssStatusRow(state: state),
            const SizedBox(height: 12),

            // Time comparison (UTC and local from :GXAS# cache; no extra commands)
            _ComparisonRow(
              icon: Icons.access_time,
              label: 'UTC Time',
              mountValue: '${state.utcTime}  ${state.utcDate}',
              phoneValue: _formatUtc(now),
              delta: _timeDelta(state, now),
            ),
            if (state.localTime != '?' && state.localDate != '?') ...[
              const SizedBox(height: 4),
              _ComparisonRow(
                icon: Icons.schedule,
                label: 'Local Time',
                mountValue: '${state.localTime}  ${state.localDate}',
                phoneValue: _formatUtc(now),
                delta: null,
              ),
            ],
            const SizedBox(height: 8),

            // Location comparison
            _ComparisonRow(
              icon: Icons.location_on,
              label: 'Location',
              mountValue:
                  '${state.latitude.toStringAsFixed(4)}N  ${state.longitude.toStringAsFixed(4)}E',
              phoneValue: 'GPS (mobile) or Internet (Windows)',
              delta: null,
            ),
            const SizedBox(height: 16),

            // Sync buttons
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: [
                ElevatedButton.icon(
                  onPressed: _syncing ? null : () => _syncTimeFromPhone(state),
                  icon: const Icon(Icons.schedule, size: 18),
                  label: const Text('Sync Time'),
                ),
                ElevatedButton.icon(
                  onPressed: (_syncing || (!state.atHome && !state.isParked))
                      ? null
                      : () => _syncLocationFromPhone(state),
                  icon: const Icon(Icons.my_location, size: 18),
                  label: const Text('Sync Location'),
                ),
                ElevatedButton.icon(
                  onPressed: _syncing ? null : () => _syncAllFromPhone(state),
                  icon: const Icon(Icons.sync, size: 18),
                  label: const Text('Sync All'),
                ),
                if (state.hasGNSS)
                  ElevatedButton.icon(
                    style: ElevatedButton.styleFrom(
                        backgroundColor: TAColors.surfaceVariant),
                    onPressed:
                        _syncing || !state.gnssValid ? null : _syncFromGnss,
                    icon: const Icon(Icons.satellite_alt, size: 18),
                    label: const Text('From GNSS'),
                  ),
              ],
            ),

            // Result message
            if (_syncResult.isNotEmpty) ...[
              const SizedBox(height: 8),
              Text(_syncResult,
                  style: TextStyle(
                      color: _syncResult.startsWith('Error')
                          ? TAColors.error
                          : TAColors.success,
                      fontSize: 12)),
            ],
          ],
        ),
      ),
    );
  }

  String _formatUtc(DateTime dt) =>
      '${dt.hour.toString().padLeft(2, '0')}:'
      '${dt.minute.toString().padLeft(2, '0')}:'
      '${dt.second.toString().padLeft(2, '0')}  '
      '${dt.month.toString().padLeft(2, '0')}/'
      '${dt.day.toString().padLeft(2, '0')}/'
      '${(dt.year % 100).toString().padLeft(2, '0')}';

  String? _timeDelta(MountState state, DateTime now) {
    if (state.utcTime == '?' || state.utcDate == '?') return null;
    try {
      final parts = state.utcTime.split(':');
      if (parts.length < 3) return null;
      final mH = int.parse(parts[0]);
      final mM = int.parse(parts[1]);
      final mS = int.parse(parts[2]);
      final mountSec = mH * 3600 + mM * 60 + mS;
      final phoneSec = now.hour * 3600 + now.minute * 60 + now.second;
      final diff = (mountSec - phoneSec).abs();
      if (diff > 43200) return '${86400 - diff}s';
      return '${diff}s';
    } catch (_) {
      return null;
    }
  }

  Future<void> _syncTimeFromPhone(MountState state) async {
    setState(() {
      _syncing = true;
      _syncResult = '';
    });

    final client = ref.read(lx200ClientProvider);
    final now = DateTime.now().toUtc();

    try {
      // Set UTC offset to 0 (we're sending UTC directly)
      await client.sendBool(LX200.setTimeZone('+00.0'));
      await Future.delayed(const Duration(milliseconds: 80));

      // Set date: MM/DD/YY
      final dateStr =
          '${now.month.toString().padLeft(2, '0')}/'
          '${now.day.toString().padLeft(2, '0')}/'
          '${(now.year % 100).toString().padLeft(2, '0')}';
      await client.sendBool(LX200.setDate(dateStr));
      await Future.delayed(const Duration(milliseconds: 80));

      // Set time: HH:MM:SS
      final timeStr =
          '${now.hour.toString().padLeft(2, '0')}:'
          '${now.minute.toString().padLeft(2, '0')}:'
          '${now.second.toString().padLeft(2, '0')}';
      final ok = await client.sendBool(LX200.setLocalTime(timeStr));

      setState(() {
        _syncResult = ok ? 'Time synced to $timeStr UTC' : 'Error: time sync failed';
      });
    } catch (e) {
      setState(() => _syncResult = 'Error: $e');
    } finally {
      setState(() => _syncing = false);
    }
  }

  Future<void> _syncLocationFromPhone(MountState state) async {
    setState(() {
      _syncing = true;
      _syncResult = '';
    });

    final client = ref.read(lx200ClientProvider);

    try {
      if (!mounted) return;

      // TeenAstro only accepts :St#/:Sg# when mount is at home or parked
      if (!state.atHome && !state.isParked) {
        setState(() {
          _syncing = false;
          _syncResult = 'Mount must be at home or parked to set location';
        });
        return;
      }

      // Try auto location: GPS on mobile, Internet (IP) on Windows
      LocationResult? loc = await LocationService.getCurrentLocation();

      _LatLon? result;
      if (loc != null) {
        result = _LatLon(loc.latitude, loc.longitude);
      } else {
        // Fallback: manual entry
        result = await showDialog<_LatLon>(
          context: context,
          builder: (_) => _LocationInputDialog(
            initialLat: state.latitude,
            initialLon: state.longitude,
          ),
        );
      }

      if (result == null) {
        setState(() {
          _syncing = false;
          _syncResult = loc != null ? '' : 'Location unavailable';
        });
        return;
      }

      // Format latitude: sDD:MM:SS
      final latSign = result.lat >= 0 ? '+' : '-';
      final latAbs = result.lat.abs();
      final latD = latAbs.floor();
      final latM = ((latAbs - latD) * 60).floor();
      final latS = (((latAbs - latD) * 60 - latM) * 60).round();
      final latStr = '$latSign${latD.toString().padLeft(2, '0')}:'
          '${latM.toString().padLeft(2, '0')}:${latS.toString().padLeft(2, '0')}';

      // Format longitude: sDDD:MM:SS
      final lonSign = result.lon >= 0 ? '+' : '-';
      final lonAbs = result.lon.abs();
      final lonD = lonAbs.floor();
      final lonM = ((lonAbs - lonD) * 60).floor();
      final lonS = (((lonAbs - lonD) * 60 - lonM) * 60).round();
      final lonStr = '$lonSign${lonD.toString().padLeft(3, '0')}:'
          '${lonM.toString().padLeft(2, '0')}:${lonS.toString().padLeft(2, '0')}';

      final latOk = await client.sendBool(LX200.setLatitude(latStr));
      await Future.delayed(const Duration(milliseconds: 80));
      final lonOk = await client.sendBool(LX200.setLongitude(lonStr));

      final src = loc != null
          ? LocationService.sourceLabel(loc.source)
          : 'Manual';
      setState(() {
        _syncResult = (latOk && lonOk)
            ? 'Location synced ($src): $latStr / $lonStr'
            : 'Error: lat=$latOk lon=$lonOk';
      });
    } catch (e) {
      setState(() => _syncResult = 'Error: $e');
    } finally {
      setState(() => _syncing = false);
    }
  }

  Future<void> _syncAllFromPhone(MountState state) async {
    await _syncTimeFromPhone(state);
    if (_syncResult.startsWith('Error')) return;
    final timeMsg = _syncResult;
    await _syncLocationFromPhone(state);
    if (mounted) {
      setState(() => _syncResult = '$timeMsg\n$_syncResult');
    }
  }

  Future<void> _syncFromGnss() async {
    setState(() {
      _syncing = true;
      _syncResult = '';
    });

    final client = ref.read(lx200ClientProvider);
    try {
      final ok = await client.sendBool(LX200.gnssSyncFull);
      setState(() {
        _syncResult = ok ? 'GNSS sync OK' : 'Error: GNSS sync failed';
      });
    } catch (e) {
      setState(() => _syncResult = 'Error: $e');
    } finally {
      setState(() => _syncing = false);
    }
  }
}

class _GnssStatusRow extends StatelessWidget {
  final MountState state;
  const _GnssStatusRow({required this.state});

  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        _indicator('GNSS', state.hasGNSS),
        const SizedBox(width: 12),
        _indicator('Valid', state.gnssValid),
        const SizedBox(width: 12),
        _indicator('Time', state.gnssTimeSync),
        const SizedBox(width: 12),
        _indicator('Site', state.gnssLocationSync),
      ],
    );
  }

  Widget _indicator(String label, bool ok) => Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 8,
            height: 8,
            decoration: BoxDecoration(
              shape: BoxShape.circle,
              color: ok ? TAColors.success : TAColors.error,
            ),
          ),
          const SizedBox(width: 4),
          Text(label,
              style: TextStyle(color: TAColors.textSecondary, fontSize: 11)),
        ],
      );
}

class _ComparisonRow extends StatelessWidget {
  final IconData icon;
  final String label;
  final String mountValue;
  final String phoneValue;
  final String? delta;

  const _ComparisonRow({
    required this.icon,
    required this.label,
    required this.mountValue,
    required this.phoneValue,
    this.delta,
  });

  @override
  Widget build(BuildContext context) {
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Icon(icon, size: 16, color: TAColors.textSecondary),
        const SizedBox(width: 8),
        Expanded(
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text(label,
                  style: TextStyle(
                      color: TAColors.textSecondary,
                      fontSize: 11,
                      fontWeight: FontWeight.w600)),
              const SizedBox(height: 2),
              Text('Mount: $mountValue',
                  style: TextStyle(
                      color: TAColors.textHigh,
                      fontFamily: 'monospace',
                      fontSize: 11)),
              Text('Phone: $phoneValue',
                  style: TextStyle(
                      color: TAColors.text,
                      fontFamily: 'monospace',
                      fontSize: 11)),
            ],
          ),
        ),
        if (delta != null)
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2),
            decoration: BoxDecoration(
              color: TAColors.warning.withValues(alpha: 0.15),
              borderRadius: BorderRadius.circular(4),
            ),
            child: Text('$delta',
                style: TextStyle(
                    color: TAColors.warning,
                    fontSize: 10,
                    fontWeight: FontWeight.w600)),
          ),
      ],
    );
  }
}

/// Dialog for entering lat/lon manually (fallback when GPS unavailable).
class _LocationInputDialog extends StatefulWidget {
  final double initialLat;
  final double initialLon;
  const _LocationInputDialog(
      {required this.initialLat, required this.initialLon});

  @override
  State<_LocationInputDialog> createState() => _LocationInputDialogState();
}

class _LatLon {
  final double lat, lon;
  const _LatLon(this.lat, this.lon);
}

class _LocationInputDialogState extends State<_LocationInputDialog> {
  late final TextEditingController _latCtl;
  late final TextEditingController _lonCtl;

  @override
  void initState() {
    super.initState();
    _latCtl = TextEditingController(
        text: widget.initialLat.toStringAsFixed(4));
    _lonCtl = TextEditingController(
        text: widget.initialLon.toStringAsFixed(4));
  }

  @override
  void dispose() {
    _latCtl.dispose();
    _lonCtl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return AlertDialog(
      title: const Text('Enter Location'),
      content: Column(
        mainAxisSize: MainAxisSize.min,
        children: [
          TextField(
            controller: _latCtl,
            decoration: const InputDecoration(
                labelText: 'Latitude (degrees, + = North)'),
            keyboardType:
                const TextInputType.numberWithOptions(decimal: true, signed: true),
          ),
          const SizedBox(height: 8),
          TextField(
            controller: _lonCtl,
            decoration: const InputDecoration(
                labelText: 'Longitude (degrees, + = East)'),
            keyboardType:
                const TextInputType.numberWithOptions(decimal: true, signed: true),
          ),
        ],
      ),
      actions: [
        TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel')),
        ElevatedButton(
          onPressed: () {
            final lat = double.tryParse(_latCtl.text);
            final lon = double.tryParse(_lonCtl.text);
            if (lat != null && lon != null) {
              Navigator.pop(context, _LatLon(lat, lon));
            }
          },
          child: const Text('Sync'),
        ),
      ],
    );
  }
}
