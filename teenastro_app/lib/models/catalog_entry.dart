/// Catalog types
enum CatalogType { dso, star, doubleStar, variableStar }

/// A single catalog
class Catalog {
  final String title;
  final String prefix;
  final int epoch;
  final CatalogType type;
  final List<CatalogEntry> objects;

  const Catalog({
    required this.title,
    required this.prefix,
    this.epoch = 2000,
    required this.type,
    required this.objects,
  });

  factory Catalog.fromJson(Map<String, dynamic> json) {
    final typeStr = json['type'] as String? ?? 'dso';
    final type = switch (typeStr) {
      'star' => CatalogType.star,
      'double' => CatalogType.doubleStar,
      'variable' => CatalogType.variableStar,
      _ => CatalogType.dso,
    };
    final objects = (json['objects'] as List)
        .map((o) => CatalogEntry.fromJson(o as Map<String, dynamic>, type))
        .toList();
    return Catalog(
      title: json['catalog'] as String,
      prefix: json['prefix'] as String,
      epoch: json['epoch'] as int? ?? 2000,
      type: type,
      objects: objects,
    );
  }
}

/// Object types for DSOs
const List<String> dsoTypeNames = [
  'Galaxy', 'Open Cluster', 'Star', 'Double Star', 'Other',
  'Galaxy Pair', 'Galaxy Triplet', 'Galaxy Group', 'Globular Cluster',
  'Planetary Nebula', 'Nebula', 'HII Region', 'Cluster+Nebula', 'Asterism',
  'Reflection Nebula', 'Supernova Remnant', 'Emission Nebula', 'Non Existent',
  'Nova', 'Duplicate', 'Dark Nebula',
];

/// Constellation abbreviations (alphabetical, matching TeenAstroCatalog)
const List<String> constellationAbbr = [
  'And', 'Ant', 'Aps', 'Aql', 'Aqr', 'Ara', 'Ari', 'Aur', 'Boo', 'CMa', 'CMi',
  'CVn', 'Cae', 'Cam', 'Cap', 'Car', 'Cas', 'Cen', 'Cep', 'Cet', 'Cha', 'Cir',
  'Cnc', 'Col', 'Com', 'CrA', 'CrB', 'Crt', 'Cru', 'Crv', 'Cyg', 'Del', 'Dor',
  'Dra', 'Equ', 'Eri', 'For', 'Gem', 'Gru', 'Her', 'Hor', 'Hya', 'Hyi', 'Ind',
  'LMi', 'Lac', 'Leo', 'Lep', 'Lib', 'Lup', 'Lyn', 'Lyr', 'Men', 'Mic', 'Mon',
  'Mus', 'Nor', 'Oct', 'Oph', 'Ori', 'Pav', 'Peg', 'Per', 'Phe', 'Pic', 'PsA',
  'Psc', 'Pup', 'Pyx', 'Ret', 'Scl', 'Sco', 'Sct', 'Ser', 'Sex', 'Sge', 'Sgr',
  'Tau', 'Tel', 'TrA', 'Tri', 'Tuc', 'UMa', 'UMi', 'Vel', 'Vir', 'Vol', 'Vul',
  '---',
];

/// A single object in a catalog
class CatalogEntry {
  final int id;
  final String name;
  final String? subId;
  final double ra; // hours (J2000)
  final double dec; // degrees (J2000)
  final double? mag;
  final int? objType; // index into dsoTypeNames
  final int constellation; // index into constellationAbbr
  // Double star extras
  final double? separation; // arcsec
  final int? positionAngle; // degrees
  final double? mag2;
  // Variable star extras
  final double? period; // days
  final double? magMin;

  const CatalogEntry({
    required this.id,
    this.name = '',
    this.subId,
    required this.ra,
    required this.dec,
    this.mag,
    this.objType,
    this.constellation = 88,
    this.separation,
    this.positionAngle,
    this.mag2,
    this.period,
    this.magMin,
  });

  String get constellationStr =>
      constellation < constellationAbbr.length
          ? constellationAbbr[constellation]
          : '---';

  String get typeStr =>
      objType != null && objType! < dsoTypeNames.length
          ? dsoTypeNames[objType!]
          : '';

  /// Format RA as HH:MM:SS
  String get raStr {
    final h = ra.floor();
    final m = ((ra - h) * 60).floor();
    final s = (((ra - h) * 60 - m) * 60).round();
    return '${h.toString().padLeft(2, '0')}:${m.toString().padLeft(2, '0')}:${s.toString().padLeft(2, '0')}';
  }

  /// Format Dec as +DD:MM:SS
  String get decStr {
    final sign = dec >= 0 ? '+' : '-';
    final absD = dec.abs();
    final d = absD.floor();
    final m = ((absD - d) * 60).floor();
    final s = (((absD - d) * 60 - m) * 60).round();
    return '$sign${d.toString().padLeft(2, '0')}:${m.toString().padLeft(2, '0')}:${s.toString().padLeft(2, '0')}';
  }

  factory CatalogEntry.fromJson(Map<String, dynamic> j, CatalogType type) {
    return CatalogEntry(
      id: j['id'] as int,
      name: j['name'] as String? ?? '',
      subId: j['subId'] as String?,
      ra: (j['ra'] as num).toDouble(),
      dec: (j['dec'] as num).toDouble(),
      mag: (j['mag'] as num?)?.toDouble(),
      objType: j['type_idx'] as int?,
      constellation: j['cons'] as int? ?? 88,
      separation: (j['sep'] as num?)?.toDouble(),
      positionAngle: j['pa'] as int?,
      mag2: (j['mag2'] as num?)?.toDouble(),
      period: (j['period'] as num?)?.toDouble(),
      magMin: (j['magMin'] as num?)?.toDouble(),
    );
  }
}
