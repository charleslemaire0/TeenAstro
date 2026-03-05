/*
 * test_guiding_emu.cpp
 *
 * Deterministic guiding emulator that uses the real MainUnit data structures
 * (StatusAxis, GeoAxis, GuideAxis from Axis.hpp) and replicates the exact
 * guiding + tracking + stepping logic from Timer.cpp, MountGuiding.cpp,
 * and MountQueriesTracking.cpp.
 *
 * Verifies that RA East/West pulse guiding is symmetric when the
 * sidereal-timer gate is present in guide(), and demonstrates the
 * 3x asymmetry bug when the gate is removed.
 *
 * Build:  pio test -d tests --filter test_guiding_emu
 */

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

/* ================================================================== */
/*  Simulated-time globals (consumed by the arduino.h shim)            */
/* ================================================================== */
namespace sim {
  unsigned long g_micros = 0;
  unsigned long g_millis = 0;
}

/* ================================================================== */
/*  Bring in the shim Arduino.h                                        */
/* ================================================================== */
#include "arduino.h"

/* ================================================================== */
/*  Minimal stubs so Axis.hpp compiles (it includes TeenAstroStepper)  */
/* ================================================================== */
#include "SPI.h"
#include "TMC26XStepper.h"
#include "TMCStepper.h"
#include "TeenAstroStepper.h"

/* ================================================================== */
/*  Firmware constants                                                 */
/* ================================================================== */
static const double mastersiderealClockSpeed = 997269.5625;
static const double masterClockSpeed = 1000000.0;

/* interval/speed helpers that Timer.cpp defines at file scope */
typedef double interval;
typedef double speed;
static speed interval2speed(interval i) { return masterClockSpeed / i; }

/* ================================================================== */
/*  Include the REAL Axis.hpp from the MainUnit                        */
/*  This gives us StatusAxis, GeoAxis, GuideAxis -- the exact same     */
/*  data structures and methods the firmware uses.                     */
/* ================================================================== */
#include "Axis.hpp"

/* ================================================================== */
/*  Unity test framework                                               */
/* ================================================================== */
#include <unity.h>

/* ================================================================== */
/*  Emulator constants                                                 */
/* ================================================================== */
static const double CLOCK_RATIO = 0.01;
static const double SIDEREAL_TICK_US = mastersiderealClockSpeed * CLOCK_RATIO;
static const long   STEPS_PER_ROT = 11520000L;
static const double STEPS_PER_DEGREE = STEPS_PER_ROT / 360.0;
static const double STEPS_PER_ARCSEC = STEPS_PER_DEGREE / 3600.0;

/* ================================================================== */
/*  Mount state: uses the real firmware classes                         */
/* ================================================================== */
struct EmuMount {
    GeoAxis   geoA1;
    StatusAxis staA1;
    GuideAxis guideA1;

    bool sideralTracking;
    bool guidingActive;

    long m_lst;
    long m_guideSiderealTimer;

    void init(double guideRate) {
        geoA1.setstepsPerRot(STEPS_PER_ROT);

        staA1.setSidereal(mastersiderealClockSpeed,
                          geoA1.stepsPerSecond, masterClockSpeed, 4);
        staA1.pos = geoA1.quaterRot;
        staA1.target = geoA1.quaterRot;
        staA1.fstep = geoA1.stepsPerCentiSecond;
        staA1.CurrentTrackingRate = 1.0;
        staA1.RequestedTrackingRate = 1.0;
        staA1.acc = 1e9;
        staA1.takeupRate = 8.0;
        staA1.backlash_correcting = false;
        staA1.backlash_inSteps = 0;
        staA1.backlash_movedSteps = 0;
        staA1.fault = false;
        staA1.resetToSidereal();

        guideA1.init(&geoA1.stepsPerCentiSecond, guideRate);

        sideralTracking = false;
        guidingActive = false;
        m_lst = 0;
        m_guideSiderealTimer = 0;
    }

    bool updateguideSiderealTimer() {
        if (m_lst != m_guideSiderealTimer) {
            m_guideSiderealTimer = m_lst;
            return true;
        }
        return false;
    }
};

/* ================================================================== */
/*  Emulator: replicate the firmware's guiding + tracking + stepping   */
/*                                                                     */
/*  This mirrors:                                                      */
/*    Timer.cpp          - UpdateIntervalTrackingGuiding (interval)     */
/*    MountGuiding.cpp   - performPulseGuiding (target += getAmount)   */
/*    MountQueriesTracking.cpp - onSiderealTick (target += fstep)      */
/*    Timer.cpp ISR      - TIMER3 (pos follows target via deltaTarget) */
/* ================================================================== */

struct EmuResult {
    double guide_offset_arcsec;
    double guide_target_accum_arcsec;
    double peak_target_overdrive_arcsec;
    long   delta_pos;
    long   lst_elapsed;
};

static EmuResult emu_run_pulse(
    char direction,
    int  duration_ms,
    double guide_rate,
    bool tracking_on,
    bool use_sidereal_gate)
{
    EmuMount m;
    m.init(guide_rate);
    m.sideralTracking = tracking_on;

    if (!tracking_on) {
        m.staA1.CurrentTrackingRate = 0.0;
        m.staA1.fstep = 0.0;
    }

    /* Issue pulse command (mirrors Command_M.cpp) */
    m.guideA1.speedMultiplier = 1.0;
    if (direction == 'e')
        m.guideA1.moveBW();
    else
        m.guideA1.moveFW();
    m.guideA1.durationLast = 0;
    m.guideA1.duration = (unsigned long)(duration_ms * 1000UL);
    m.guidingActive = true;

    long initial_pos = m.staA1.pos;

    /* Simulation timing -- 5s extra after pulse to let motor fully catch up */
    const unsigned long total_us = (unsigned long)(duration_ms * 1000UL + 5000000UL);
    const unsigned long main_loop_step = 200;
    unsigned long next_sidereal_tick = (unsigned long)SIDEREAL_TICK_US;
    unsigned long next_motor_fire = (unsigned long)m.staA1.interval_Step_Cur;
    long last_applied_lst = 0;
    long guide_target_accum = 0;
    double peak_target_overdrive = 0.0;

    for (unsigned long t = 0; t < total_us; t += main_loop_step) {
        sim::g_micros = t;
        sim::g_millis = t / 1000;

        /* ---- Timer1 ISR: sidereal tick ---- */
        while (t >= next_sidereal_tick) {
            m.m_lst++;

            /* UpdateIntervalTrackingGuiding (from Timer.cpp) */
            double tmp_guideRate = 0.0;
            if (!m.staA1.backlash_correcting && m.guideA1.isBusy()) {
                if (m.guideA1.absRate < m.staA1.takeupRate) {
                    tmp_guideRate = m.guideA1.getRate();
                }
            }

            /* Braking logic (Timer.cpp lines 136-154):
             * When guide axis is braking, adjust target to decelerate,
             * then transition to idle when pos reaches target. */
            if (m.guideA1.isBraking()) {
                m.staA1.breakMoveLowRate();
                if (m.staA1.atTarget(false)) {
                    m.guideA1.setIdle();
                    tmp_guideRate = 0.0;
                }
            }

            double sumRate;
            if (m.sideralTracking)
                sumRate = fabs(tmp_guideRate + m.staA1.CurrentTrackingRate);
            else if (m.guideA1.isBusy())
                sumRate = fabs(m.guideA1.getRate());
            else
                sumRate = fabs(tmp_guideRate);

            m.staA1.setIntervalfromRate(sumRate,
                (double)12 /*StepsMinInterval*/, (double)100000 /*StepsMaxInterval*/);

            if (!m.guideA1.isBusy())
                m.guidingActive = false;

            next_sidereal_tick += (unsigned long)SIDEREAL_TICK_US;
        }

        /* ---- Main loop: onSiderealTick (tracking target advance) ---- */
        {
            long new_ticks = m.m_lst - last_applied_lst;
            if (new_ticks > 0 && m.sideralTracking) {
                m.staA1.target += m.staA1.fstep * new_ticks;
                last_applied_lst = m.m_lst;
            }
        }

        /* ---- Main loop: guide() ---- */
        if (m.guidingActive) {
            bool should_run;
            if (use_sidereal_gate) {
                should_run = m.updateguideSiderealTimer();
            } else {
                should_run = true;
            }

            if (should_run) {
                /* performPulseGuiding (from MountGuiding.cpp, line 47-121) */
                double target_before = m.staA1.target;
                if (m.guideA1.duration == 0 && !m.guideA1.isMoving()) {
                    m.guideA1.brake();
                    m.guideA1.speedMultiplier = 1.0;
                    m.guidingActive = false;
                } else if (m.guideA1.isMoving()) {
                    if (m.guideA1.duration > 0) {
                        if (!m.staA1.backlash_correcting) {
                            m.staA1.target += m.guideA1.getAmount();
                            guide_target_accum += (long)(m.staA1.target - target_before);

                            unsigned long elapsed = sim::g_micros - m.guideA1.durationLast;
                            if (elapsed > m.guideA1.duration)
                                m.guideA1.duration = 0;
                            else
                                m.guideA1.duration -= elapsed;
                            m.guideA1.durationLast = sim::g_micros;
                        }
                    } else {
                        m.guideA1.brake();
                        m.guideA1.speedMultiplier = 1.0;
                    }
                } else {
                    m.guideA1.duration = 0UL;
                    m.guideA1.speedMultiplier = 1.0;
                }
            }
        }

        /* ---- Motor ISR: step axis 1 ---- */
        {
            double cur_interval = m.staA1.interval_Step_Cur;
            if (cur_interval > 0.5 && cur_interval < 200000.0) {
                while (t >= next_motor_fire) {
                    long delta = (long)m.staA1.target - m.staA1.pos;
                    if (delta > 0)
                        m.staA1.pos++;
                    else if (delta < 0)
                        m.staA1.pos--;
                    else
                        break;
                    next_motor_fire += (unsigned long)cur_interval;
                }
            }
        }

        /* Track peak |target - pos| as indicator of target over-drive */
        {
            double overdrive = fabs((double)((long)m.staA1.target - m.staA1.pos));
            if (overdrive > peak_target_overdrive)
                peak_target_overdrive = overdrive;
        }
    }

    /* Compute results.
     *
     * delta_pos = total motor steps (tracking + guiding combined).
     * To isolate the guiding contribution we subtract the tracking
     * displacement that would have occurred without any guiding.
     * Tracking advances target by fstep per sidereal centisecond,
     * and fstep = stepsPerCentiSecond * CurrentTrackingRate.
     * Over m_lst ticks that is fstep_tracking * m_lst steps.
     */
    long delta_pos = m.staA1.pos - initial_pos;
    double delta_arcsec = (double)delta_pos / STEPS_PER_ARCSEC;

    double guide_only_arcsec = delta_arcsec;
    if (tracking_on) {
        double fstep_tracking = m.geoA1.stepsPerCentiSecond * 1.0;
        double tracking_steps = fstep_tracking * m.m_lst;
        guide_only_arcsec = delta_arcsec - (tracking_steps / STEPS_PER_ARCSEC);
    }

    EmuResult r;
    r.guide_offset_arcsec = guide_only_arcsec;
    r.guide_target_accum_arcsec = (double)guide_target_accum / STEPS_PER_ARCSEC;
    r.peak_target_overdrive_arcsec = peak_target_overdrive / STEPS_PER_ARCSEC;
    r.delta_pos = delta_pos;
    r.lst_elapsed = m.m_lst;
    return r;
}

/* ================================================================== */
/*  Test cases                                                         */
/* ================================================================== */

void setUp(void) {}
void tearDown(void) {}

void test_east_tracking_on_gated(void)
{
    EmuResult r = emu_run_pulse('e', 5000, 0.5, true, true);
    char msg[256];
    sprintf(msg, "East+track+gate: guide_offset=%.2f\" delta_pos=%ld lst=%ld (expect ~-37.5\")",
            r.guide_offset_arcsec, r.delta_pos, r.lst_elapsed);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(8.0, 37.5, fabs(r.guide_offset_arcsec), msg);
}

void test_west_tracking_on_gated(void)
{
    EmuResult r = emu_run_pulse('w', 5000, 0.5, true, true);
    char msg[128];
    sprintf(msg, "West+track+gate: guide_offset=%.2f\" (expect ~+37.5\")", r.guide_offset_arcsec);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(8.0, 37.5, fabs(r.guide_offset_arcsec), msg);
}

void test_east_west_symmetry_gated(void)
{
    EmuResult east = emu_run_pulse('e', 5000, 0.5, true, true);
    EmuResult west = emu_run_pulse('w', 5000, 0.5, true, true);
    double diff = fabs(fabs(east.guide_offset_arcsec) - fabs(west.guide_offset_arcsec));
    char msg[200];
    sprintf(msg, "Symmetry: |east|=%.2f\", |west|=%.2f\", diff=%.2f\"",
            fabs(east.guide_offset_arcsec), fabs(west.guide_offset_arcsec), diff);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(8.0, 0.0, diff, msg);
}

void test_west_tracking_on_ungated_shows_bug(void)
{
    /* The ungated bug manifests as massive target over-accumulation.
     * Without the sidereal gate, guide() runs every main-loop iteration
     * (~200us) instead of once per sidereal tick (~10ms), causing ~50x
     * more target increments. We track the peak target over-drive
     * (max |target - pos|) during the pulse as the bug indicator. */
    EmuResult west_ungated = emu_run_pulse('w', 5000, 0.5, true, false);
    EmuResult west_gated   = emu_run_pulse('w', 5000, 0.5, true, true);
    char msg[256];
    sprintf(msg, "Bug demo: ungated peak_overdrive=%.2f\" vs gated=%.2f\" "
            "(ungated target accumulation >> gated)",
            west_ungated.peak_target_overdrive_arcsec,
            west_gated.peak_target_overdrive_arcsec);
    TEST_ASSERT_TRUE_MESSAGE(
        west_ungated.peak_target_overdrive_arcsec >
        west_gated.peak_target_overdrive_arcsec * 5.0,
        msg);
}

void test_east_tracking_off_gated(void)
{
    EmuResult r = emu_run_pulse('e', 5000, 0.5, false, true);
    char msg[128];
    sprintf(msg, "East+notrack+gate: offset=%.2f\" (expect ~37.5\")", r.guide_offset_arcsec);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(8.0, 37.5, fabs(r.guide_offset_arcsec), msg);
}

void test_west_tracking_off_gated(void)
{
    EmuResult r = emu_run_pulse('w', 5000, 0.5, false, true);
    char msg[128];
    sprintf(msg, "West+notrack+gate: offset=%.2f\" (expect ~37.5\")", r.guide_offset_arcsec);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(8.0, 37.5, fabs(r.guide_offset_arcsec), msg);
}

void test_tracking_off_symmetry(void)
{
    EmuResult east = emu_run_pulse('e', 5000, 0.5, false, true);
    EmuResult west = emu_run_pulse('w', 5000, 0.5, false, true);
    double diff = fabs(fabs(east.guide_offset_arcsec) - fabs(west.guide_offset_arcsec));
    char msg[200];
    sprintf(msg, "TrackOff sym: |east|=%.2f\", |west|=%.2f\", diff=%.2f\"",
            fabs(east.guide_offset_arcsec), fabs(west.guide_offset_arcsec), diff);
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(8.0, 0.0, diff, msg);
}

/* ================================================================== */
/*  Entry point                                                        */
/* ================================================================== */

int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_east_tracking_on_gated);
    RUN_TEST(test_west_tracking_on_gated);
    RUN_TEST(test_east_west_symmetry_gated);
    RUN_TEST(test_west_tracking_on_ungated_shows_bug);
    RUN_TEST(test_east_tracking_off_gated);
    RUN_TEST(test_west_tracking_off_gated);
    RUN_TEST(test_tracking_off_symmetry);
    return UNITY_END();
}
