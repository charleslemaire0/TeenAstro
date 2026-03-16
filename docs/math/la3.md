# Linear algebra (LA3)

`TeenAstroLA3` provides 3-vector and 3×3 matrix operations, rotation matrices, Euler angle extraction, atmospheric refraction, and SVD. Single static class `LA3` with 28 public methods.

**Source:** `libraries/TeenAstroLA3/`  
**Authors:** Markus Noga, Charles Lemaire

---

## Types

**RotAxis:** `ROTAXISX`, `ROTAXISY`, `ROTAXISZ`

**SingleRotation:**
```c
struct SingleRotation { RotAxis axis; double angle; };  // angle in radians
```

**RefrOpt:**
```c
struct RefrOpt { bool use; double Temperature; double Pressure; };  // °C, mbar
```

---

## Vector operations

| Method | Formula |
|--------|---------|
| `dotProduct(a, b)` | a·b = a₀b₀ + a₁b₁ + a₂b₂ |
| `crossProduct(out, a, b)` | out = (a₁b₂−a₂b₁, a₂b₀−a₀b₂, a₀b₁−a₁b₀) |
| `norm(in)` | ‖v‖ = √(v₀² + v₁² + v₂²) |
| `normalize(out, in)` | v̂ = v / ‖v‖ |
| `angle2Vectors(a, b)` | θ = acos((a·b) / (‖a‖‖b‖)) |
| `toDirCos(dc, ang1, ang2)` | dc₀ = cos(θ₂)cos(−θ₁), dc₁ = cos(θ₂)sin(−θ₁), dc₂ = sin(θ₂) |

`toDirCos` maps polar angles (e.g. Az, Alt) to a unit direction vector.

---

## Matrix operations

| Method | Formula |
|--------|---------|
| `transpose(out, in)` | Mᵀ[i][j] = M[j][i] |
| `determinant(m)` | det(M) = M₀₀(M₁₁M₂₂−M₂₁M₁₂) − M₀₁(M₁₀M₂₂−M₁₂M₂₀) + M₀₂(M₁₀M₂₁−M₁₁M₂₀) |
| `invert(out, m)` | M⁻¹ = (1/det(M)) · adj(M), cofactor method |
| `multiply(out, a, b)` | out[i][j] = Σₖ a[i][k]b[k][j] |
| `multiply(out, m, v)` | out[i] = Σⱼ m[i][j]v[j] |
| `getIdentityMatrix(out)` | I |

---

## Rotation matrices (right-hand rule)

**Rx(θ):** rotation about X  
**Ry(θ):** rotation about Y  
**Rz(θ):** rotation about Z  

`getSingleRotationMatrix(out, sr)` — one axis.  
`getMultipleRotationMatrix(out, sr_array, n)` — product R₁·R₂·…·Rₙ.

---

## Euler angle extraction

Four conventions; all handle gimbal lock.

- **getEulerRxRyRz** — R = Rx(θX)·Ry(θY)·Rz(θZ). Primary for coordinate conversions.
- **getEulerRzRyRx**, **getEulerRzRxRy**, **getEulerRx0RyRx1** — other intrinsic/proper orders.

---

## Atmospheric refraction

**Topocentric → Apparent (Saemundsson, Meeus 16.4):**  
TPC = (P/1010)·(283/(273+T)), R = (1.02/tan(h°+10.3/(h°+5.11)) + 0.0019279)·TPC, Alt_app = Alt_true + R·π/10800. Not applied if Alt < −0.06 rad or `Opt.use` false.

**Apparent → Topocentric (Bennett, Meeus 16.3):**  
R = (1.0/tan(h°+7.31/(h°+4.4)) + 0.0013515)·TPC, Alt_true = Alt_app − R·π/10800.

---

## SVD

`getsvd(m, u, v)` — 3×3 SVD (McAdams et al., UW-Madison TR1690). Returns U and V; used in CoordConv to project the Taki matrix onto the nearest proper rotation (det = +1).

---

**See also:** [Coordinate systems](coord.md) · [Alignment](alignment.md)
