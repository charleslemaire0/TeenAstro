## RA East/West guiding asymmetry (Release_1.6) -- RESOLVED

### Scope

This note documents the **RA East/West asymmetry when tracking is ON** that was present in Release 1.6, its root cause, and the fix.

### Symptom

With a 0.5x, 5 s RA guide pulse and tracking ON:
- East: RA change = **+38"** (correct for 0.5x * 5 s -> ~37.5").
- West: RA change = **+114"** (~3x expected, wrong sign in the Conform sense).

With tracking OFF, both directions were symmetric and correct.

### Root cause

In Release 1.6, the `rtk.updateguideSiderealTimer()` gate was removed from `MountGuiding::guide()`. This gate ensures that guiding functions (`performPulseGuiding`, etc.) are called **once per sidereal centisecond** (~10 ms), matching the rate at which `getAmount()` is calibrated.

Without the gate, `guide()` ran on every main-loop iteration (~200 us), causing `target += getAmount()` to fire ~50x per sidereal tick instead of once. This massively over-drove the axis-1 target.

**Why the asymmetry (East OK, West 3x):**

- **East + tracking**: guiding opposes tracking. The combined motor rate is 0.5x sidereal (slow). The over-driven target creates a large `target - pos` deficit, but the motor is rate-limited by the slow 0.5x interval. The Timer ISR's braking logic (`breakMoveLowRate` -> `setIdle`) corrects the target each sidereal tick, so the motor ends up near the correct position.

- **West + tracking**: guiding reinforces tracking. The combined motor rate is 1.5x sidereal (fast). The over-driven target creates a large `target - pos` surplus, and the motor chases it at the faster 1.5x interval. The braking logic corrects per tick, but the motor has already moved ~3x the expected distance because it runs 3x faster than in the East case.

### Fix

Restore the sidereal-timer gate in `MountGuiding::guide()`, as it was in Release 1.5:

```cpp
void MountGuiding::guide()
{
  if (GuidingState == GuidingOFF)
    return;
  if (rtk.updateguideSiderealTimer())   // restored gate
  {
    if (GuidingState == GuidingPulse)
      performPulseGuiding();
    // ... other guiding modes ...
  }
}
```

This ensures `performPulseGuiding()` (and all guiding modes) execute at the correct sidereal-centisecond cadence, matching the per-centisecond calibration of `GuideAxis::getAmount()`.

### Verification (C++ emulator)

A deterministic C++ emulator (`tests/test/test_guiding_emu/test_guiding_emu.cpp`) was created to verify the fix without flashing hardware. It uses the **real firmware data structures** (`StatusAxis`, `GeoAxis`, `GuideAxis` from `Axis.hpp`) and replicates the guiding + tracking + stepping logic from `Timer.cpp`, `MountGuiding.cpp`, and `MountQueriesTracking.cpp`.

Run with: `pio test -d tests --filter test_guiding_emu`

**Test results (all 7 pass):**

| Test | Result |
|------|--------|
| East + tracking ON + gated | ~37.5" |
| West + tracking ON + gated | ~37.5" |
| East/West symmetry (gated) | diff < 8" |
| Ungated bug demo (peak target over-drive) | ungated >> gated |
| East + tracking OFF + gated | ~37.5" |
| West + tracking OFF + gated | ~37.5" |
| Tracking-OFF symmetry | diff < 8" |

### Previous fixes (still applied)

1. **Timer / interval logic (`Timer.cpp`)**: `UpdateIntervalTrackingGuiding` now uses `|guideRate + trackingRate|` for tracking-ON and `|guideRate|` for tracking-OFF, eliminating the previous 3:1 interval asymmetry.

2. **ASCOM driver and GXAS handling**: `IsPulseGuiding` timing and GXAS refresh issues were fixed in the ASCOM driver.
