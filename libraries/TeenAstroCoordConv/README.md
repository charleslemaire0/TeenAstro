# TeenAstroCoordConv

Mount alignment library for the TeenAstro telescope controller. Computes a 3x3 transformation matrix that maps between sky (reference) coordinates and instrument (axis) coordinates, using 2-star alignment based on the Taki method.

## Algorithm

1. **Reference stars** — Two observed stars provide direction cosine vectors in both the reference frame (sky) and the axis frame (instrument)
2. **Third reference** — Derived mathematically as the normalized cross product of the first two vectors in each frame
3. **Transformation matrix** — `T = dcAxis^T x inv(dcRef^T)` maps reference to instrument coordinates
4. **SVD refinement** — Singular Value Decomposition enforces a proper rotation matrix (det = +1), filtering out noise
5. **Optional minimization** — `minimizeAxis1()` and `minimizeAxis2()` iteratively reduce alignment error

## Main class: `CoordConv`

```cpp
CoordConv conv;
conv.addReference(angle1, angle2, axis1, axis2);  // star 1
conv.addReference(angle1, angle2, axis1, axis2);  // star 2
conv.calculateThirdReference();                    // derive 3rd

// T maps sky -> instrument, Tinv maps instrument -> sky
double T[3][3], Tinv[3][3];
// Access via conv.T and conv.Tinv after calculation
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

## Dependencies

- **TeenAstroLA3** — base class; provides direction cosines, matrix ops, SVD, Euler angles
- **svd3** — 3x3 SVD implementation
