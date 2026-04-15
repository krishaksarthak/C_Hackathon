/*
 * File   : control.c
 * Purpose: Per-cycle sensor checks — temperature, gear, overspeed.
 *
 * ── Fault severity and ECU response ─────────────────────────────────────
 *
 *  Fault                  Severity   ECU Response
 *  ─────────────────────  ─────────  ──────────────────────────────────────────
 *  CRITICAL OVERHEAT      CRITICAL   Force MODE_FAULT this cycle → STATE_SAFE
 *  INVALID_GEAR           CRITICAL   Force MODE_FAULT this cycle → STATE_SAFE
 *  INVALID_MODE†          CRITICAL   Handled in mode.c / update_mode()
 *  OVERSPEED              MAJOR      Stay in current mode; DEGRADED 1st cycle,
 *                                   STATE_SAFE + lock after threshold cycles
 *  HIGH TEMPERATURE       MAJOR      Same as OVERSPEED
 *
 *  † INVALID_MODE is set in mode.c, not here, but shares the same critical path.
 *
 * ── Priority order for log reporting (highest → lowest) ─────────────────
 *   P1  Critical Overheat  (CRITICAL)
 *   P2  Invalid Mode / Gear (CRITICAL)
 *   P3  Overspeed           (MAJOR)
 *   P4  High Temperature    (MAJOR)
 *
 * ALL faults that apply in a cycle are SET.  log_faults_prioritised()
 * (called from main after this function) emits them in priority order
 * and marks the highest-priority one as [PRIMARY FAULT].
 *
 * MISRA C:2012 compliance notes
 *   • No dynamic allocation (Rule 21.3).
 *   • Unused parameters suppressed with (void) cast (Rule 2.7).
 *   • Explicit casts on integer comparisons (Rule 10.4).
 *   • snprintf return value cast to void (Rule 17.7).
 *   • Single exit point per function (Rule 15.5).
 */

#include "control.h"
#include "fault.h"
#include "log.h"
#include <stdio.h>

/* ── Internal helper: force ECU into FAULT mode ─────────────────────────── */
/*
 * Called only by CRITICAL fault handlers below.
 * Guards against re-entering FAULT mode if already there.
 * Does NOT set reset_required — that is the responsibility of
 * evaluate_system_state() (single place, single responsibility).
 */
static void force_fault_mode(VehicleStatus *status)
{
    if (status->current_mode != MODE_FAULT)
    {
        log_mode_transition(status->current_mode, MODE_FAULT, 1);
        status->previous_mode = status->current_mode;
        status->current_mode  = MODE_FAULT;
    }
}

/* ── Temperature check ──────────────────────────────────────────────────── */
/*
 * CRITICAL OVERHEAT (temp > TEMP_CRITICAL_THRESHOLD):
 *   Sets FAULT_BIT_OVERTEMP, clears FAULT_BIT_HIGH_TEMP (mutually exclusive),
 *   then forces MODE_FAULT.  evaluate_system_state() will escalate to
 *   STATE_SAFE on the same cycle and set reset_required.
 *
 * HIGH TEMPERATURE (temp > TEMP_HIGH_THRESHOLD):
 *   Sets FAULT_BIT_HIGH_TEMP (MAJOR).  Current mode is intentionally
 *   preserved — the vehicle is still operable but degraded.
 *   After MAJOR_FAULT_SAFE_THRESHOLD consecutive/cumulative occurrences
 *   the state evaluator escalates to STATE_SAFE automatically.
 */
static void check_temperature(const VehicleInput *input,
                               VehicleStatus      *status,
                               FaultStatus        *faults)
{
    if (input->temperature > (int16_t)TEMP_CRITICAL_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_OVERTEMP);
        clear_fault(faults, FAULT_BIT_HIGH_TEMP);       /* mutually exclusive  */
        log_fault_event(FAULT_BIT_OVERTEMP,
                        "CRITICAL OVERHEAT detected — forcing FAULT mode");
        force_fault_mode(status);                        /* CRITICAL response   */
    }
    else if (input->temperature > (int16_t)TEMP_HIGH_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_HIGH_TEMP);
        clear_fault(faults, FAULT_BIT_OVERTEMP);        /* mutually exclusive  */
        log_fault_event(FAULT_BIT_HIGH_TEMP,
                        "HIGH TEMPERATURE — major fault, mode preserved");
        /* MAJOR: no mode change; state evaluator handles DEGRADED / SAFE      */
    }
    else
    {
        clear_fault(faults, FAULT_BIT_OVERTEMP);
        clear_fault(faults, FAULT_BIT_HIGH_TEMP);
    }
}

/* ── Gear check ─────────────────────────────────────────────────────────── */
/*
 * Invalid gear (outside 0-5) is a CRITICAL fault.
 * validate_inputs() intentionally does NOT restore last_valid_gear for an
 * out-of-range reading, so gear_valid == 0 reaches this layer correctly.
 * Response: set FAULT_BIT_INVALID_GEAR and force MODE_FAULT.
 */
static void check_gear(const VehicleInput *input,
                       VehicleStatus      *status,
                       FaultStatus        *faults)
{
    if (input->gear_valid == 0U)
    {
        set_fault(faults, FAULT_BIT_INVALID_GEAR);
        log_fault_event(FAULT_BIT_INVALID_GEAR,
                        "Gear out of range [0-5] — critical fault, forcing FAULT mode");
        force_fault_mode(status);                        /* CRITICAL response   */
    }
    else
    {
        clear_fault(faults, FAULT_BIT_INVALID_GEAR);
    }
}

/* ── Overspeed check ────────────────────────────────────────────────────── */
/*
 * Overspeed (speed > OVERSPEED_THRESHOLD) is a MAJOR fault.
 * The vehicle remains in its current operating mode — the driver is
 * warned but the ECU does not cut power immediately.
 *
 * First occurrence  → state evaluator sets STATE_DEGRADED.
 * After threshold   → state evaluator sets STATE_SAFE + ECU lock.
 * (Threshold = MAJOR_FAULT_SAFE_THRESHOLD consecutive/cumulative cycles.)
 */
static void check_overspeed(const VehicleInput *input, FaultStatus *faults)
{
    char detail[72]; /* fixed-size buffer, no VLA — MISRA Rule 18.8 */

    if (input->speed > (int16_t)OVERSPEED_THRESHOLD)
    {
        set_fault(faults, FAULT_BIT_OVERSPEED);
        (void)snprintf(detail, sizeof(detail),
                       "Speed %d km/h exceeds %d km/h limit [MAJOR — mode preserved]",
                       (int)input->speed, OVERSPEED_THRESHOLD);
        log_fault_event(FAULT_BIT_OVERSPEED, detail);
    }
    else
    {
        clear_fault(faults, FAULT_BIT_OVERSPEED);
    }
}

/* ── Public entry point ─────────────────────────────────────────────────── */
void run_control_checks(const VehicleInput *input,
                        VehicleStatus      *status,
                        FaultStatus        *faults)
{
    check_temperature(input, status, faults);  /* P1 CRITICAL + P4 MAJOR */
    check_gear(input, status, faults);         /* P2 CRITICAL (gear half) */
    check_overspeed(input, faults);            /* P3 MAJOR               */
    /*
     * P2 CRITICAL (mode half) is handled in mode.c / update_mode().
     * It runs before this function in the scheduler, so MODE_FAULT may
     * already be set when we arrive here.
     */
}