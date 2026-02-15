# TeenAstroLA3

Linear algebra library for 3-vectors and 3x3 matrices, designed for telescope coordinate transformations. Provides rotation matrices, Euler angle decomposition, atmospheric refraction, and SVD.

## API overview

### Vectors

| Function | Description |
|----------|-------------|
| `dotProduct(a, b)` | Dot product of two 3-vectors |
| `crossProduct(out, a, b)` | Cross product |
| `angle2Vectors(a, b)` | Angle between two vectors (radians) |
| `norm(v)` | Euclidean length |
| `normalize(out, in)` | Normalize to unit length |
| `toDirCos(dc, ang1, ang2)` | Direction cosines from two polar angles |

### Matrices

| Function | Description |
|----------|-------------|
| `getIdentityMatrix(m)` | 3x3 identity |
| `copy(out, in)` | Copy matrix |
| `transpose(out, in)` | Transpose |
| `invert(out, m)` | Inverse (3x3) |
| `multiply(out, a, b)` | Matrix x matrix or matrix x vector |
| `determinant(m)` | Determinant |

### Rotations

| Function | Description |
|----------|-------------|
| `getSingleRotationMatrix(out, sr)` | Rotation matrix for one axis/angle |
| `getMultipleRotationMatrix(out, sr[], n)` | Combined rotation from a sequence |

### Euler angle extraction

| Function | Convention |
|----------|-----------|
| `getEulerRzRxRy(m, z, x, y)` | Rz-Rx-Ry |
| `getEulerRzRyRx(m, z, y, x)` | Rz-Ry-Rx |
| `getEulerRxRyRz(m, x, y, z)` | Rx-Ry-Rz |
| `getEulerRx0RyRx1(m, x0, y, x1)` | Rx-Ry-Rx (proper Euler) |

### Atmospheric refraction

| Function | Formula | Direction |
|----------|---------|-----------|
| `Topocentric2Apparent(Alt, Opt)` | Saemundsson (Meeus Eq. 16.4) | True -> apparent (add refraction) |
| `Apparent2Topocentric(Alt, Opt)` | Bennett (Meeus Eq. 16.3) | Apparent -> true (subtract refraction) |

Altitude in radians; `RefrOpt` carries `use`, `Temperature` (C), `Pressure` (mbar). The pair is consistent to ~0.4 arcsec at moderate altitudes.

### SVD

| Function | Description |
|----------|-------------|
| `getsvd(m, u, v)` | SVD of 3x3 matrix (U and V; singular values implicit) |

### Utilities

| Function | Description |
|----------|-------------|
| `modRad(angle)` | Wrap angle to [-PI, PI] |
| `toRad(deg)` | Degrees to radians |
| `toDeg(rad)` | Radians to degrees |
| `normalizeRads(rad)` | Normalize to [0, 2*PI) |

## Types

| Type | Description |
|------|-------------|
| `RotAxis` | Enum: `ROTAXISX`, `ROTAXISY`, `ROTAXISZ` |
| `SingleRotation` | Struct: `{ RotAxis axis, double angle }` |
| `RefrOpt` | Struct: `{ bool use, double Temperature, double Pressure }` |

## Dependencies

- **svd3** — 3x3 SVD implementation
- **math.h** — standard math functions
