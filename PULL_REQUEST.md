# Pull request: Fix RA East/West guiding asymmetry when tracking is ON

Use this as the PR title and description after you push the branch (e.g. to your fork or with credentials that have push access).

---

## Title

**Fix RA East/West guiding asymmetry when tracking is ON**

---

## Description

### Problem

With tracking ON, a 0.5×, 5 s RA guide pulse gave:
- **East**: RA change ≈ +38″ (correct)
- **West**: RA change ≈ +114″ (~3× expected, wrong sign)

With tracking OFF, East/West were symmetric.

### Root cause

In Release 1.6 the `rtk.updateguideSiderealTimer()` gate was removed from `MountGuiding::guide()`. Without it, guiding ran on every main-loop iteration (~200 µs) instead of once per sidereal centisecond (~10 ms), so `target += getAmount()` fired ~50× per tick and over-drove the axis target. West+tracking showed the bug because the motor runs at 1.5× sidereal and chases the over-driven target; East+tracking is rate-limited at 0.5× and braking corrects the target each tick.

### Fix

Restore the sidereal-timer gate in `MountGuiding::guide()` (as in Release 1.5):

```cpp
if (rtk.updateguideSiderealTimer()) {
  if (GuidingState == GuidingPulse)
    performPulseGuiding();
  // ... other modes
}
```

### Verification

- **C++ emulator** (`pio test -d tests --filter test_guiding_emu`): 7 tests using real `Axis.hpp` and replicated Timer/MountGuiding logic; all pass (East/West symmetry, ungated bug demo, tracking-off symmetry).
- **Docs**: `tests/conform_reproduce/RA_EW_remaining_issue.md` updated with root cause, fix, and verification.

### Branch

`fix/ra-ew-guiding-sidereal-gate` (commit `f043dead`)

---

**To open the PR:** Push this branch to your fork (or to origin if you have access), then on GitHub create a Pull Request from `fix/ra-ew-guiding-sidereal-gate` into `Release_1.6`.
