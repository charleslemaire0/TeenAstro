import 'dart:convert';
import 'dart:ffi';
import 'dart:io';
import 'dart:typed_data';

/// One star from [CedarPhoto.extract], in crop pixels.
class CedarStar {
  const CedarStar(this.x, this.y, this.flux, this.brightness);

  final double x;
  final double y;
  final double flux;
  final double brightness;
}

class CedarCrop {
  const CedarCrop(this.stars, this.found, this.noise, this.ms);

  /// Brightest aperture first, at most the requested count.
  final List<CedarStar> stars;
  final int found;
  final double noise;

  /// Time inside the detector, crop copy included.
  final double ms;
}

typedef _OpenC = Pointer<Void> Function(Pointer<Uint8>, Pointer<Uint32>, Pointer<Uint32>);
typedef _OpenDart = Pointer<Void> Function(Pointer<Uint8>, Pointer<Uint32>, Pointer<Uint32>);
typedef _OpenMemoryC = Pointer<Void> Function(
    Pointer<Uint8>, IntPtr, Pointer<Uint32>, Pointer<Uint32>);
typedef _OpenMemoryDart = Pointer<Void> Function(
    Pointer<Uint8>, int, Pointer<Uint32>, Pointer<Uint32>);
typedef _CloseC = Void Function(Pointer<Void>);
typedef _CloseDart = void Function(Pointer<Void>);
typedef _ExtractC = Int32 Function(
  Pointer<Void>,
  Uint32,
  Uint32,
  Uint32,
  Uint32,
  Double,
  Uint32,
  Uint32,
  Pointer<Double>,
  Pointer<Uint32>,
  Pointer<Double>,
  Pointer<Double>,
);
typedef _ExtractDart = int Function(
  Pointer<Void>,
  int,
  int,
  int,
  int,
  double,
  int,
  int,
  Pointer<Double>,
  Pointer<Uint32>,
  Pointer<Double>,
  Pointer<Double>,
);
typedef _AllocC = Pointer<Uint8> Function(IntPtr);
typedef _AllocDart = Pointer<Uint8> Function(int);
typedef _FreeC = Void Function(Pointer<Uint8>, IntPtr);
typedef _FreeDart = void Function(Pointer<Uint8>, int);

class _Cedar {
  _Cedar(DynamicLibrary lib)
      : open = lib.lookupFunction<_OpenC, _OpenDart>('cedar_open'),
        openMemory =
            lib.lookupFunction<_OpenMemoryC, _OpenMemoryDart>('cedar_open_memory'),
        close = lib.lookupFunction<_CloseC, _CloseDart>('cedar_close'),
        extract = lib.lookupFunction<_ExtractC, _ExtractDart>('cedar_extract'),
        alloc = lib.lookupFunction<_AllocC, _AllocDart>('cedar_alloc'),
        free = lib.lookupFunction<_FreeC, _FreeDart>('cedar_free');

  final _OpenDart open;
  final _OpenMemoryDart openMemory;
  final _CloseDart close;
  final _ExtractDart extract;
  final _AllocDart alloc;
  final _FreeDart free;

  static _Cedar? _loaded;

  static _Cedar load() {
    final cached = _loaded;
    if (cached != null) return cached;
    return _loaded = _Cedar(_openLibrary());
  }

  static DynamicLibrary _openLibrary() {
    if (Platform.isAndroid) return DynamicLibrary.open('libcedar_native.so');
    if (Platform.isWindows) {
      final local = File(_windowsPath);
      if (local.existsSync()) return DynamicLibrary.open(local.absolute.path);
      return DynamicLibrary.open('cedar_native.dll');
    }
    return DynamicLibrary.open('libcedar_native.so');
  }
}

const _windowsPath = 'tool/cedar/cedar_native.dll';

/// Cedar's detector in this process. The photo is decoded once.
class CedarPhoto {
  CedarPhoto._(this.address, this.width, this.height);

  /// The native handle, so another isolate can [attach] to the same photo.
  final int address;
  final int width;
  final int height;

  static const _starWords = 4;

  static bool get available =>
      Platform.isAndroid || File(_windowsPath).existsSync();

  /// From an encoded photo in memory, such as a camera JPEG.
  static CedarPhoto? openBytes(Uint8List encoded) {
    final cedar = _Cedar.load();
    final size = 8 + encoded.length;
    final scratch = cedar.alloc(size);
    if (scratch == nullptr) return null;
    try {
      final dims = scratch.cast<Uint32>();
      final data = scratch + 8;
      data.asTypedList(encoded.length).setAll(0, encoded);
      final handle = cedar.openMemory(data, encoded.length, dims, dims + 1);
      if (handle == nullptr) return null;
      return CedarPhoto._(handle.address, dims[0], dims[1]);
    } finally {
      cedar.free(scratch, size);
    }
  }

  /// Null when the photo cannot be read.
  static CedarPhoto? open(String path) {
    final cedar = _Cedar.load();
    final bytes = utf8.encode(path);
    final size = 8 + bytes.length + 1;
    final scratch = cedar.alloc(size);
    if (scratch == nullptr) return null;
    try {
      final dims = scratch.cast<Uint32>();
      final text = scratch + 8;
      text.asTypedList(bytes.length + 1)
        ..setAll(0, bytes)
        ..[bytes.length] = 0;
      final handle = cedar.open(text, dims, dims + 1);
      if (handle == nullptr) return null;
      return CedarPhoto._(handle.address, dims[0], dims[1]);
    } finally {
      cedar.free(scratch, size);
    }
  }

  static CedarPhoto attach(int address, int width, int height) =>
      CedarPhoto._(address, width, height);

  void close() => _Cedar.load().close(Pointer<Void>.fromAddress(address));

  /// Stars in the crop at ([x0], [y0]), brightest aperture first.
  CedarCrop? extract({
    required int x0,
    required int y0,
    required int cropWidth,
    required int cropHeight,
    required double sigma,
    required int binning,
    required int maxStars,
  }) {
    final cedar = _Cedar.load();
    final starBytes = maxStars * _starWords * 8;
    final size = starBytes + 24;
    final scratch = cedar.alloc(size);
    if (scratch == nullptr) return null;
    try {
      final stars = scratch.cast<Double>();
      final noise = (scratch + starBytes).cast<Double>();
      final ms = (scratch + starBytes + 8).cast<Double>();
      final found = (scratch + starBytes + 16).cast<Uint32>();
      final count = cedar.extract(
        Pointer<Void>.fromAddress(address),
        x0,
        y0,
        cropWidth,
        cropHeight,
        sigma,
        binning,
        maxStars,
        stars,
        found,
        noise,
        ms,
      );
      if (count < 0) return null;
      final words = stars.asTypedList(count * _starWords);
      return CedarCrop(
        [
          for (var i = 0; i < count; i++)
            CedarStar(
              words[i * _starWords],
              words[i * _starWords + 1],
              words[i * _starWords + 2],
              words[i * _starWords + 3],
            ),
        ],
        found.value,
        noise.value,
        ms.value,
      );
    } finally {
      cedar.free(scratch, size);
    }
  }
}
