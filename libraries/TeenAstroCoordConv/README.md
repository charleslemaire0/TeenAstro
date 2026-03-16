# TeenAstroCoordConv

Mount alignment library for the TeenAstro telescope controller. Computes a 3x3 transformation matrix that maps between sky (reference) coordinates and instrument (axis) coordinates, using 2-star alignment based on the Taki method.

## Algorithm

1. **Reference stars** -- Two observed stars provide direction cosine vectors in both the reference frame (sky) and the axis frame (instrument)
2. **Third reference** -- Derived mathematically as the normalized cross product of the first two vectors in each frame
3. **Transformation matrix** -- `T = dcAxis^T x inv(dcRef^T)` maps reference to instrument coordinates
4. **SVD refinement** -- Singular Value Decomposition enforces a proper rotation matrix (det = +1), filtering out noise
5. **Optional minimization** -- `minimizeAxis1()` and `minimizeAxis2()` iteratively reduce alignment error

## Usage

```cpp
#include <TeenAstroCoordConv.hpp>

CoordConv conv;
conv.addReference(az1, alt1, instAz1, instAlt1);  // star 1 (radians)
conv.addReference(az2, alt2, instAz2, instAlt2);  // star 2 (radians)
// calculateThirdReference() is called automatically after the 2nd star

if (conv.isReady()) {
    // conv.T    maps sky -> instrument
    // conv.Tinv maps instrument -> sky
    Coord_IN in = ho.To_Coord_IN(conv.T);
    Coord_HO ho = in.To_Coord_HO(conv.Tinv, refrOpt);
}
```

## Key API

| Method | Description |
|--------|-------------|
| `addReference(a1, a2, ax1, ax2)` | Add a reference star (radians). Max 2. |
| `calculateThirdReference()` | Compute third reference and build T/Tinv |
| `isReady()` | True when transformation is computed |
| `getError()` | Alignment angular error (radians) |
| `getRefs()` | Number of reference stars added |
| `getT() / setT()` | Save/restore T matrix (for EEPROM) |
| `setTinvFromT()` | Recompute Tinv from stored T |
| `minimizeAxis1(offset)` | Reduce axis1 alignment error via Euler angles |
| `minimizeAxis2()` | Iterative axis2 error minimization |
| `reset()` | Clear reference stars |
| `clean()` | Reset all matrices and state |

## Example

See `libraries/TeenAstroCoord/examples/CoordDemo/` for a full integrated C++ example that exercises alignment with a simulated polar misalignment. Build with: `pio run -d libraries/TeenAstroCoord/examples/CoordDemo`

## Dependencies

- **TeenAstroLA3** -- base class; provides direction cosines, matrix ops, SVD, Euler angles
- **svd3** -- 3x3 SVD implementation

## Testing

Unit tests and integration tests are in `tests/test/test_coordconv/` (PlatformIO Unity, native platform). They cover construction, identity alignment, matrix properties (det = +1, T * Tinv = I), getT/setT serialization, and full-chain round-trips through all three libraries.

```
pio test -d tests --filter test_coordconv
```
