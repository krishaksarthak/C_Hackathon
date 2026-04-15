/*
 * File   : state.c
 * Purpose: System state evaluation — NORMAL / DEGRADED / SAFE.
 *
 * ── State escalation rules ───────────────────────────────────────────────
 *
 *   STATE_NORMAL
 *     No active faults and no ECU lock pending.
 *
 *   STATE_DEGRADED
 *     One or more MAJOR faults active this cycle (1st occurrence).
 *     Vehicle is operable but operator is warned.
 *
 *   STATE_SAFE  (latched — only clears on manual reset)
 *     Triggered by ANY of the following:
 *       a) current_mode == MODE_FAULT
 *          A CRITICAL fault (OVERTEMP, INVALID_GEAR, INVALID_MODE) forced
 *          the ECU into FAULT mode this cycle.
 *       b) Any MAJOR fault's consecutive/cumulative counter reaches
 *          MAJOR_FAULT_SAFE_THRESHOLD (default 2).
 *          The same MAJOR fault has fired enough times to be deemed unsafe.
 *
 *   Once STATE_SAFE is set, the evaluator returns early on every subsequent
 *   call — the system stays locked until init_system() is called (operator
 *   enters -1 in main).  Recovery is intentionally manual.
 *
 * ── Why this order matters ───────────────────────────────────────────────
 *   If evaluate_system_state() ran before run_control_checks(), critical
 *   faults detected in control.c (which force MODE_FAULT) would not yet be
 *   visible, and the SAFE escalation would be missed for an entire cycle.
 *   The scheduler in main.c guarantees: update_fault_status → evaluate_system_state.
 *
 * MISRA C:2012 compliance notes
 *   • Loop variable typed as uint32_t (Rule 10.1).
 *   • Single exit point per helper (Rule 15.5).
 *   • No recursion; bounded loop on FAULT_BIT_COUNT (Rule 17.2).
 *   • Explicit cast on enum-to-index (Rule 10.5).
 */

#include "state.h"
#include "fault.h"
#include "log.h"

/* ── Internal helper ────────────────────────────────────────────────────── */
/*
 * Scans every MAJOR fault and returns 1 if any has accumulated
 * consecutive/cumulative occurrences >= MAJOR_FAULT_SAFE_THRESHOLD.
 * Returns 0 if no MAJOR fault has reached the threshold.
 */
static int any_major_fault_at_threshold(const FaultStatus *faults)
{
    uint32_t i;
    int      found = 0;

    for (i = 0U; (i < (uint32_t)FAULT_BIT_COUNT) && (found == 0); i++)
    {
        FaultBit bit = (FaultBit)i;

        if (get_fault_severity(bit) == FAULT_SEVERITY_MAJOR)
        {
            if (faults->consecutive[i] >= MAJOR_FAULT_SAFE_THRESHOLD)
            {
                found = 1;
            }
        }
    }

    return found;
}

/* ── evaluate_system_state ──────────────────────────────────────────────── */
void evaluate_system_state(VehicleStatus *status, const FaultStatus *faults)
{
    SystemState  new_state;
    const char  *reason;
    int          active;

    /*
     * STATE_SAFE is latched.  Once reached, this function returns
     * immediately on every call — the ECU stays locked until init_system()
     * (manual operator reset) clears system_state and reset_required.
     */
    if (status->system_state == STATE_SAFE)
    {
        return;
    }

    active    = count_active_faults(faults);
    new_state = status->system_state; /* default: no change                  */
    reason    = "no change";

    if (status->current_mode == MODE_FAULT)
    {
        /*
         * Path A — CRITICAL fault:
         *   MODE_FAULT was forced this cycle by a CRITICAL condition
         *   (OVERTEMP in control.c, INVALID_GEAR in control.c, or
         *    INVALID_MODE in mode.c).  Escalate directly to STATE_SAFE.
         */
        new_state              = STATE_SAFE;
        reason                 = "critical fault — MODE_FAULT active";
        status->reset_required = 1U;
    }
    else if (any_major_fault_at_threshold(faults) != 0)
    {
        /*
         * Path B — MAJOR fault threshold reached:
         *   A MAJOR fault (OVERSPEED or HIGH_TEMP) has fired
         *   MAJOR_FAULT_SAFE_THRESHOLD or more times.
         *   Escalate to STATE_SAFE and lock the ECU.
         */
        new_state              = STATE_SAFE;
        reason                 = "major fault reached safe threshold — ECU locked";
        status->reset_required = 1U;
    }
    else if (active > 0)
    {
        /*
         * Path C — first/single MAJOR fault occurrence:
         *   One or more faults active but below the SAFE threshold.
         *   DEGRADED: vehicle is operable, operator is warned.
         */
        new_state = STATE_DEGRADED;
        reason    = "active fault(s) present — degraded operation";
    }
    else
    {
        /*
         * Path D — all clear:
         *   No active faults and not in FAULT mode.
         *   Return to NORMAL.  (Cannot reach here from STATE_SAFE
         *   since that path returns early above.)
         */
        new_state = STATE_NORMAL;
        reason    = "all faults cleared";
    }

    if (new_state != status->system_state)
    {
        log_state_transition(status->system_state, new_state, reason);
        status->previous_state = status->system_state;
        status->system_state   = new_state;
    }
}

/* ── state_to_string ────────────────────────────────────────────────────── */
const char *state_to_string(SystemState state)
{
    const char *name;

    switch (state)
    {
        case STATE_NORMAL:   name = "NORMAL";   break;
        case STATE_DEGRADED: name = "DEGRADED"; break;
        case STATE_SAFE:     name = "SAFE";     break;
        default:             name = "UNKNOWN";  break;
    }

    return name;
}