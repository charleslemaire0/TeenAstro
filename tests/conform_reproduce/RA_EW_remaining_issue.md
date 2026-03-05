## Remaining RA East/West guiding issue (Release_1.6)

### Scope

This note documents the **remaining RA East/West asymmetry when tracking is ON**, after fixing timer/interval and guiding geometry issues in the TeenAstro main unit and ASCOM driver.

### Current behaviour (from `pulseguide_probe.py --all-tests`)

- **Mount geometry**
  - RA and Dec share identical geometry: `stepsPerRot = 11 520 000`, `stepsPerCentiSecond = 1.3333`.
- **Intervals (axis 1 / RA) during a 0.5×, 5 s pulse**
  - `interval_Step_Cur` idle: ~7479.52 µs.
  - `interval_Step_Cur` during East pulse: ~14959.04 µs.
  - `interval_Step_Cur` during West pulse: ~14959.04 µs.
  - Axis‑2 (Dec) interval remains at 100000 µs during RA E/W tests (no Dec guiding).
- **Axis‑1 steps (tracking OFF, pure guiding)**
  - East: `∆pos ≈ +340` steps.
  - West: `∆pos ≈ −300` steps.
  - `deltaTarget` sign is correct: positive during East, negative during West.
- **Sky motion with tracking OFF → HA domain**
  - HA(East) ≈ **+40″**.
  - HA(West) ≈ **−35″**.
  - Conclusion: **guiding geometry in HA is symmetric and correct when tracking is disabled.**
- **Sky motion with tracking ON → RA domain**
  - RA(East) ≈ **+38″** (correct for 0.5×, 5 s → ~37.5″).
  - RA(West) ≈ **+114″** (≈3× expected, wrong sign in the Conform sense).

### Fixes already applied

1. **Timer / interval logic (`Timer.cpp`)**
   - `UpdateIntervalTrackingGuiding` was previously producing different `interval_Step_Cur` for East vs West, causing a **3:1 step‑rate asymmetry**.
   - Now:
     - With **tracking OFF** and **GuidingPulse/GuidingST4**, we use `|guideA->getRate()|` to set the interval (HA domain).
     - With **tracking ON**, we use `fabs(tmp_guideRateA + staA->CurrentTrackingRate)` (signed sum, then magnitude) to set the interval (RA domain).
   - Runtime evidence: `interval_Step_Cur` is identical for East and West during pulses; the previous East/West speed asymmetry is gone.

2. **Guiding geometry and Dec tests**
   - For Dec N/S pulses, axis‑2 `pos/target/deltaTarget` and Dec sky motion now match the theoretical 15″/s × guideRate within ~0.5″.
   - This confirms that guiding geometry and the Dec mapping are working as expected.

3. **ASCOM driver and GXAS handling**
   - Previous issues with `IsPulseGuiding` staying TRUE or going FALSE too early were fixed by forcing GXAS refreshes and correctly mapping pulse commands in the ASCOM driver.

### Remaining problem

With all low‑level guiding and timer issues fixed, the only remaining discrepancy is:

- **When tracking is ON**, for a 0.5×, 5 s RA guide pulse:
  - East: RA change ≈ **+38″** (correct).
  - West: RA change ≈ **+114″** (≈3× expected and same sign as East).

Key points:

- Axis‑1 step counts and `deltaTarget` are **symmetric and signed correctly** for East and West.
- `interval_Step_Cur` is **identical** for East and West during the pulse.
- HA behaviour with tracking OFF is symmetric and close to theoretical expectations.
- Dec behaviour (N/S) with tracking ON is also correct.

Therefore, the remaining 3× asymmetry and sign issue for RA‑West exists **only in the RA coordinate domain when tracking is enabled**, not in:

- Step generation or backlash (`Timer.cpp`, `Axis.hpp`),
+- Guiding amount (`GuideAxis`, `MountGuiding`),
+- Interval setting for pure guiding (HA) or Dec guiding.

This strongly suggests that the bug lies in the **RA mapping with tracking ON** (how axis‑1 motion + tracking are converted into RA in the coordinate / tracking layer), rather than in the guiding timers or geometry.

### How to reproduce

1. Build and flash the main unit (Release_1.6 with the current fixes).
2. Connect via serial and run, from the TeenAstro repo root:

   ```bash
   python tests/conform_reproduce/pulseguide_probe.py \
     --port COM3 \
     --baud 57600 \
     --duration 5000 \
     --guide-rate-multiple 0.5 \
     --all-tests
   ```

   (Replace `COM3` with the actual Teensy port.)

3. Observe in the output:
   - **Tracking OFF (HA)**: HA(E) ≈ +40″, HA(W) ≈ −35″.
   - **Tracking ON (RA)**: RA(E) ≈ +38″, RA(W) ≈ +114″.
   - **Dec N test**: Dec change ≈ +38″ for a 0.5×, 5 s N pulse.

These results match the behaviour logged in `debug-97e435.log` for this debug session.

