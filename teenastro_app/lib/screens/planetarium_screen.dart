import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../models/catalog_entry.dart';
import '../models/lx200_commands.dart';
import '../providers/last_goto_route_provider.dart';
import '../services/lx200_tcp_client.dart';
import '../widgets/sky_map.dart';

/// Planetarium (interactive sky map) with Goto on star tap.
class PlanetariumScreen extends ConsumerStatefulWidget {
  const PlanetariumScreen({super.key});

  @override
  ConsumerState<PlanetariumScreen> createState() => _PlanetariumScreenState();
}

class _PlanetariumScreenState extends ConsumerState<PlanetariumScreen> {
  String? _message;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) {
      ref.read(lastGotoTabRouteProvider.notifier).state = '/planetarium';
    });
  }

  Future<void> _gotoStar(CatalogEntry star) async {
    final client = ref.read(lx200ClientProvider);
    final setRa = await client.sendBool(LX200.setTargetRa(star.raStr));
    final setDec = await client.sendBool(LX200.setTargetDec(star.decStr));
    if (!setRa || !setDec) {
      setState(() => _message = 'Failed to set target');
      return;
    }
    final reply = await client.sendCommand(LX200.gotoTarget);
    setState(() {
      _message = reply == '0' ? 'Slewing to ${star.name.isNotEmpty ? star.name : "target"}...' : 'Goto failed';
    });
  }

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        if (_message != null)
          Padding(
            padding: const EdgeInsets.only(bottom: 8),
            child: Text(_message!, style: TextStyle(color: Theme.of(context).colorScheme.primary)),
          ),
        SkyMapWidget(
          onStarSelected: _gotoStar,
          overlayMessage: _message,
        ),
      ],
    );
  }
}
